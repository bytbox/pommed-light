/*
 * pommed - Apple laptops hotkeys handler daemon
 *
 * $Id$
 *
 * Copyright (C) 2006 Julien BLACHE <jb@jblache.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <poll.h>
#include <signal.h>

#include <syslog.h>
#include <stdarg.h>

#include <errno.h>

#ifndef __powerpc__
# include <smbios/SystemInfo.h>
# define check_machine() check_machine_smbios()
#else
# define check_machine() check_machine_pmu()
#endif /* __powerpc__ */

#include <getopt.h>

#include "pommed.h"
#include "kbd_backlight.h"
#include "lcd_backlight.h"
#include "cd_eject.h"
#include "evdev.h"
#include "conffile.h"
#include "audio.h"
#include "dbus.h"


/* Machine-specific operations */
struct machine_ops *mops;

#ifdef __powerpc__
/* PowerBook machines */

/* PowerBook3,2 */
struct machine_ops pb32_ops = {
  .type = MACHINE_POWERBOOK_32,
  .lcd_backlight_probe = r128_backlight_probe,
  .lcd_backlight_step = r128_backlight_step,
  .evdev_identify = evdev_is_adb,
};

/* PowerBook5,5 */
struct machine_ops pb55_ops = {
  .type = MACHINE_POWERBOOK_55,
  .lcd_backlight_probe = r9600_sysfs_backlight_probe,
  .lcd_backlight_step = sysfs_backlight_step,
  .evdev_identify = evdev_is_adb,
};

/* PowerBook5,6 / PowerBook G4 15" (Feb 2005) */
struct machine_ops pb56_ops = {
  .type = MACHINE_POWERBOOK_56,
  .lcd_backlight_probe = r9600_sysfs_backlight_probe,
  .lcd_backlight_step = sysfs_backlight_step,
  .evdev_identify = evdev_is_fountain,
};

/* PowerBook5,7 */
struct machine_ops pb57_ops = {
  .type = MACHINE_POWERBOOK_57,
  .lcd_backlight_probe = r9600_sysfs_backlight_probe,
  .lcd_backlight_step = sysfs_backlight_step,
  .evdev_identify = evdev_is_fountain,
};

/* PowerBook6,3 */
struct machine_ops pb63_ops = {
  .type = MACHINE_POWERBOOK_63,
  .lcd_backlight_probe = r9600_sysfs_backlight_probe,
  .lcd_backlight_step = sysfs_backlight_step,
  .evdev_identify = evdev_is_adb,
};

#else

/* MacBook Pro machines */

/* MacBookPro1,1 / MacBookPro1,2 (Core Duo) */
struct machine_ops mbp1_ops = {
  .type = MACHINE_MACBOOKPRO_1,
  .lcd_backlight_probe = x1600_backlight_probe,
  .lcd_backlight_step = x1600_backlight_step,
  .evdev_identify = evdev_is_geyser3,
};

/* MacBookPro2,1 / MacBookPro2,2 (Core2 Duo) */
struct machine_ops mbp2_ops = {
  .type = MACHINE_MACBOOKPRO_2,
  .lcd_backlight_probe = x1600_backlight_probe,
  .lcd_backlight_step = x1600_backlight_step,
  .evdev_identify = evdev_is_geyser4,
};


/* MacBook machines */

/* MacBook1,1 (Core Duo) */
struct machine_ops mb1_ops = {
  .type = MACHINE_MACBOOK_1,
  .lcd_backlight_probe = gma950_backlight_probe,
  .lcd_backlight_step = gma950_backlight_step,
  .evdev_identify = evdev_is_geyser3,
};

/* MacBook2,1 (Core2 Duo) */
struct machine_ops mb2_ops = {
  .type = MACHINE_MACBOOK_2,
  .lcd_backlight_probe = gma950_backlight_probe,
  .lcd_backlight_step = gma950_backlight_step,
  .evdev_identify = evdev_is_geyser4,
};
#endif /* __powerpc__ */


/* debug mode */
int debug = 0;
int console = 0;


/* Used by signal handlers */
static int running;


void
logmsg(int level, char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);

  if (console)
    {
      switch (level)
	{
	  case LOG_INFO:
	    fprintf(stdout, "I: ");
	    break;

	  case LOG_WARNING:
	    fprintf(stdout, "W: ");
	    break;

	  case LOG_ERR:
	    fprintf(stdout, "E: ");
	    break;

	  default:
	    break;
	}
      vfprintf(stdout, fmt, ap);
      fprintf(stdout, "\n");
    }
  else
    {
      vsyslog(level | LOG_DAEMON, fmt, ap);
    }

  va_end(ap);
}

void
logdebug(char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);

  if (debug)
    vfprintf(stderr, fmt, ap);

  va_end(ap);
}


void
kbd_set_fnmode(void)
{
  FILE *fp;

  if ((general_cfg.fnmode < 1) || (general_cfg.fnmode > 2))
    general_cfg.fnmode = 1;

  fp = fopen(KBD_FNMODE_FILE, "a");
  if (fp == NULL)
    {
      logmsg(LOG_INFO, "Could not open %s: %s", KBD_FNMODE_FILE, strerror(errno));
      fp = fopen(KBD_FNMODE_FILE2620, "a");
      if (fp == NULL)
        {
          logmsg(LOG_INFO, "Could not open %s: %s", KBD_FNMODE_FILE2620, strerror(errno));
          return;
        }
    }

  fprintf(fp, "%d", general_cfg.fnmode);

  fclose(fp);
}

#ifdef __powerpc__
static machine_type
check_machine_pmu(void)
{
  int fd;
  int n;
  int ret = MACHINE_UNKNOWN;

  char buffer[128];

  /* Check copyright node, look for "Apple Computer, Inc." */
  fd = open("/proc/device-tree/copyright", O_RDONLY);
  if (fd < 0)
    {
      logmsg(LOG_ERR, "Could not open /proc/device-tree/copyright");

      return ret;
    }

  n = read(fd, buffer, sizeof(buffer) - 1);
  if (n < 1)
    {
      logmsg(LOG_ERR, "Error reading /proc/device-tree/copyright");

      close(fd);
      return ret;
    }
  close(fd);
  buffer[n] = '\0';

  logdebug("device-tree copyright node: [%s]\n", buffer);

  if (strstr(buffer, "Apple Computer, Inc.") == NULL)
    return ret;


  ret = MACHINE_MAC_UNKNOWN;

  /* Grab machine identifier string */
  fd = open("/proc/device-tree/model", O_RDONLY);
  if (fd < 0)
    {
      logmsg(LOG_ERR, "Could not open /proc/device-tree/model");

      return ret;
    }

  n = read(fd, buffer, sizeof(buffer) - 1);
  if (n < 1)
    {
      logmsg(LOG_ERR, "Error reading /proc/device-tree/model");

      close(fd);
      return ret;
    }
  close(fd);
  buffer[n] = '\0';

  logdebug("device-tree model node: [%s]\n", buffer);

  /* PowerBook G4 Titanium 15" (December 2000) */
  if (strncmp(buffer, "PowerBook3,2", 12) == 0)
    ret = MACHINE_POWERBOOK_32;
  /* PowerBook G4 Aluminium 17" (April 2004) */
  else if (strncmp(buffer, "PowerBook5,5", 12) == 0)
    ret = MACHINE_POWERBOOK_55;
  /* PowerBook G4 Aluminium 15" (February 2005) */
  else if (strncmp(buffer, "PowerBook5,6", 12) == 0)
    ret = MACHINE_POWERBOOK_56;
  /* PowerBook G4 Aluminium 17" (February 2005) */
  else if (strncmp(buffer, "PowerBook5,7", 12) == 0)
    ret = MACHINE_POWERBOOK_57;
  /* iBook G4 (October 2003) */
  else if (strncmp(buffer, "PowerBook6,3", 12) == 0)
    ret = MACHINE_POWERBOOK_63;
  else
    logmsg(LOG_ERR, "Unknown Apple machine: %s", buffer);
  
  if (ret != MACHINE_MAC_UNKNOWN)
    logmsg(LOG_INFO, "PMU machine check: running on a %s", buffer);

  return ret;
}

#else

static machine_type
check_machine_smbios(void)
{
  int ret = MACHINE_UNKNOWN;

  const char *prop;

  if (geteuid() != 0)
    {
      logmsg(LOG_ERR, "root privileges needed for SMBIOS machine detection");

      return MACHINE_ERROR;
    }

  /* Check vendor name */
  prop = SMBIOSGetVendorName();
  logdebug("SMBIOS vendor name: [%s]\n", prop);

  if (strcmp(prop, "Apple Computer, Inc.") == 0)
    ret = MACHINE_MAC_UNKNOWN;

  SMBIOSFreeMemory(prop);

  if (ret != MACHINE_MAC_UNKNOWN)
    return ret;

  /* Check system name */
  prop = SMBIOSGetSystemName();
  logdebug("SMBIOS system name: [%s]\n", prop);

  /* Core Duo MacBook Pro 15" (January 2006) & 17" (April 2006) */
  if ((strcmp(prop, "MacBookPro1,1") == 0) || (strcmp(prop, "MacBookPro1,2") == 0))
    ret = MACHINE_MACBOOKPRO_1;
  /* Core2 Duo MacBook Pro 17" & 15" (October 2006) */
  else if ((strcmp(prop, "MacBookPro2,1") == 0) || (strcmp(prop, "MacBookPro2,2") == 0))
    ret = MACHINE_MACBOOKPRO_2;
  /* Core Duo MacBook (May 2006) */
  else if (strcmp(prop, "MacBook1,1") == 0)
    ret = MACHINE_MACBOOK_1;
  /* Core2 Duo MacBook (November 2006) */
  else if (strcmp(prop, "MacBook2,1") == 0)
    ret = MACHINE_MACBOOK_2;
  else
    logmsg(LOG_ERR, "Unknown Apple machine: %s", prop);

  if (ret != MACHINE_MAC_UNKNOWN)
    logmsg(LOG_INFO, "SMBIOS machine check: running on a %s", prop);

  SMBIOSFreeMemory(prop);

  return ret;
}
#endif /* __powerpc__ */


static void
usage(void)
{
  printf("pommed v" M_VERSION " ($Rev$) Apple laptops hotkeys handler\n");
  printf("Copyright (C) 2006 Julien BLACHE <jb@jblache.org>\n");

  printf("Usage:\n");
  printf("\tpommed\t-- start pommed as a daemon\n");
  printf("\tpommed -v\t-- print version and exit\n");
  printf("\tpommed -f\t-- run in the foreground with log messages\n");
  printf("\tpommed -d\t-- run in the foreground with debug messages\n");
}


void
sig_int_term_handler(int signal)
{
  running = 0;
}

int
main (int argc, char **argv)
{
  int ret;
  int c;
  int i;

  FILE *pidfile;

  struct pollfd *fds;
  int nfds;

  int reopen;
  machine_type machine;

  struct timeval tv_now;
  struct timeval tv_als;
  struct timeval tv_diff;

  while ((c = getopt(argc, argv, "fdv")) != -1)
    {
      switch (c)
	{
	  case 'f':
	    console = 1;
	    break;

	  case 'd':
	    debug = 1;
	    console = 1;
	    break;

	  case 'v':
	    printf("pommed v" M_VERSION " ($Rev$) Apple laptops hotkeys handler\n");
	    printf("Copyright (C) 2006 Julien BLACHE <jb@jblache.org>\n");

	    exit(0);
	    break;

	  default:
	    usage();

	    exit(-1);
	    break;
	}
    }

  if (!console)
    {
      openlog("pommed", LOG_PID, LOG_DAEMON);
    }

  logmsg(LOG_INFO, "pommed v" M_VERSION " ($Rev$) Apple laptops hotkeys handler");
  logmsg(LOG_INFO, "Copyright (C) 2006 Julien BLACHE <jb@jblache.org>");

  /* Load our configuration */
  ret = config_load();
  if (ret < 0)
    {
      exit(-1);
    }

  /* Identify the machine we're running on */
  machine = check_machine();
  switch (machine)
    {
#ifndef __powerpc__
      case MACHINE_MACBOOKPRO_1:
	mops = &mbp1_ops;
	break;

      case MACHINE_MACBOOKPRO_2:
	mops = &mbp2_ops;
	break;

      case MACHINE_MACBOOK_1:
	mops = &mb1_ops;
	break;

      case MACHINE_MACBOOK_2:
	mops = &mb2_ops;
	break;
#else
      case MACHINE_POWERBOOK_32:
	mops = &pb32_ops;
	break;

      case MACHINE_POWERBOOK_55:
	mops = &pb55_ops;
	break;

      case MACHINE_POWERBOOK_56:
	mops = &pb56_ops;
	break;

      case MACHINE_POWERBOOK_57:
	mops = &pb57_ops;
	break;
      
      case MACHINE_POWERBOOK_63:
	mops = &pb63_ops;
	break;

#endif /* !__powerpc__ */

      case MACHINE_MAC_UNKNOWN:
	logmsg(LOG_ERR, "Unknown Apple machine");

	exit(1);
	break;

      case MACHINE_UNKNOWN:
	logmsg(LOG_ERR, "Unknown non-Apple machine");

	exit(1);
	break;

      case MACHINE_ERROR:
	exit(1);
	break;
    }

  ret = mops->lcd_backlight_probe();
  if (ret < 0)
    {
      logmsg(LOG_ERR, "No LCD backlight found");

      exit(1);
    }

  nfds = evdev_open(&fds);
  if (nfds < 1)
    {
      logmsg(LOG_ERR, "No suitable event devices found");

      exit(1);
    }

  kbd_set_fnmode();
  kbd_backlight_init();

  ret = audio_init();
  if (ret < 0)
    {
      logmsg(LOG_WARNING, "Audio initialization failed, audio support disabled");
    }

  ret = mbpdbus_init();
  if (ret < 0)
    {
      logmsg(LOG_WARNING, "Could not connect to DBus system bus");
    }

  if (!console)
    {
      /*
       * Detach from the console
       */
      if (daemon(0, 0) != 0)
	{
	  logmsg(LOG_ERR, "daemon() failed: %s", strerror(errno));

	  evdev_close(&fds, nfds);

	  exit(-1);
	}
    }

  pidfile = fopen(PIDFILE, "w");
  if (pidfile == NULL)
    {
      logmsg(LOG_WARNING, "Could not open pidfile %s: %s", PIDFILE, strerror(errno));

      evdev_close(&fds, nfds);

      exit(-1);
    }
  fprintf(pidfile, "%d\n", getpid());
  fclose(pidfile);

  gettimeofday(&tv_als, NULL);

  running = 1;
  signal(SIGINT, sig_int_term_handler);
  signal(SIGTERM, sig_int_term_handler);

  reopen = 0;

  while (running)
    {
      /* Attempt to reopen event devices, typically after resuming */
      if (reopen)
	{
	  nfds = evdev_reopen(&fds, nfds);

	  if (nfds < 1)
	    {
	      logmsg(LOG_ERR, "No suitable event devices found (reopen)");

	      break;
	    }

	  /* Re-set the keyboard mode
	   * When we need to reopen the event devices, it means we've
	   * just resumed from sleep
	   */
	  kbd_set_fnmode();

	  reopen = 0;
	}

      ret = poll(fds, nfds, LOOP_TIMEOUT);

      if (ret < 0) /* error */
	{
	  if (errno != EINTR)
	    {
	      logmsg(LOG_ERR, "poll() error: %s", strerror(errno));

	      break;
	    }
	}
      else if (ret != 0)
	{
	  for (i = 0; i < nfds; i++)
	    {
	      /* the event devices cease to exist when suspending */
	      if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
		{
		  logmsg(LOG_WARNING, "Error condition signaled on evdev, reopening");
		  reopen = 1;
		  break;
		}

	      if (fds[i].revents & POLLIN)
		evdev_process_events(fds[i].fd);
	    }

	  if (kbd_cfg.auto_on && has_kbd_backlight())
	    {
	      /* is it time to chek the ambient light sensors ? */
	      gettimeofday(&tv_now, NULL);
	      tv_diff.tv_sec = tv_now.tv_sec - tv_als.tv_sec;
	      if (tv_diff.tv_sec < 0)
		tv_diff.tv_sec = 0;

	      if (tv_diff.tv_sec == 0)
		{
		  tv_diff.tv_usec = tv_now.tv_usec - tv_als.tv_usec;
		}
	      else
		{
		  tv_diff.tv_sec--;
		  tv_diff.tv_usec = 1000000 - tv_als.tv_usec + tv_now.tv_usec;
		  tv_diff.tv_usec += tv_diff.tv_sec * 1000000;
		}

	      if (tv_diff.tv_usec >= (1000 * LOOP_TIMEOUT))
		{
		  kbd_backlight_ambient_check();
		  tv_als = tv_now;
		}
	    }
	}
      else if (kbd_cfg.auto_on && has_kbd_backlight())
	{
	  /* poll() timed out, check ambient light sensors */
	  kbd_backlight_ambient_check();
	  gettimeofday(&tv_als, NULL);
	}

      /* Process DBus requests */
      mbpdbus_process_requests();
    }

  evdev_close(&fds, nfds);

  mbpdbus_cleanup();

  config_cleanup();

  logmsg(LOG_INFO, "Exiting");

  if (!console)
    closelog();

  unlink(PIDFILE);

  return 0;
}
