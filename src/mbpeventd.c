/*
 * $Id$
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

#include <smbios/SystemInfo.h>

#include <getopt.h>

#include "mbpeventd.h"
#include "kbd_backlight.h"
#include "lcd_backlight.h"
#include "cd_eject.h"
#include "evdev.h"


/* Machine-specific operations */
struct machine_ops *mops;

/* MacBookPro2,2 machine operations */
struct machine_ops mbp22_ops = {
  .type = MACHINE_MACBOOKPRO_22,
  .lcd_backlight_probe = x1600_backlight_probe,
  .lcd_backlight_step = x1600_backlight_step,
};


/* debug mode */
#ifndef DEBUG
int debug = 0;
#endif

/* Used by signal handlers */
static int running;

void
logmsg(int level, char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);

  if (debug)
    {
      switch (level)
	{
	  case LOG_INFO:
	    fprintf(stderr, "I: ");
	    break;

	  case LOG_WARNING:
	    fprintf(stderr, "W: ");
	    break;

	  case LOG_ERR:
	    fprintf(stderr, "E: ");
	    break;

	  default:
	    break;
	}
      vfprintf(stderr, fmt, ap);
      fprintf(stderr, "\n");
    }
  else
    {
      vsyslog(level | LOG_DAEMON, fmt, ap);
    }

  va_end(ap);
}


int
check_machine_smbios(void)
{
  int ret = -1;

  const char *prop;

  if (geteuid() != 0)
    {
      logdebug("Error: SMBIOS machine detection only works as root\n");

      return ret;
    }

  /* Check vendor name */
  prop = SMBIOSGetVendorName();

  logdebug("SMBIOS vendor name: [%s]\n", prop);
  if (strcmp(prop, "Apple Computer, Inc.") == 0)
    ret = MACHINE_VENDOR_APPLE;

  SMBIOSFreeMemory(prop);

  if (ret != MACHINE_VENDOR_APPLE)
    return ret;

  ret = MACHINE_MAC_UNKNOWN;

  /* Check system name */
  prop = SMBIOSGetSystemName();

  logdebug("SMBIOS system name: [%s]\n", prop);
  if (strcmp(prop, "MacBookPro2,2") == 0)
    ret = MACHINE_MACBOOKPRO_22;

  SMBIOSFreeMemory(prop);

  return ret;
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
  int machine;

  struct timeval tv_now;
  struct timeval tv_als;
  struct timeval tv_diff;

  while ((c = getopt(argc, argv, "f")) != -1)
    {
      switch (c)
	{
	  case 'f':
#ifndef DEBUG
	    debug = 1;
#endif
	    break;
	  default:
	    fprintf(stderr, "Unknown option -%c\n", c);

	    exit(-1);
	    break;
	}
    }

  if (!debug)
    {
      openlog("mbpeventd", LOG_PID, LOG_DAEMON);
    }

  logmsg(LOG_INFO, "mbpeventd $Rev$, Copyright (C) 2006 Julien BLACHE <jb@jblache.org>");

  machine = check_machine_smbios();
  switch (machine)
    {
      case MACHINE_MACBOOKPRO_22:
	logmsg(LOG_INFO, "Running on a MacBookPro2,2 (detected via SMBIOS)");

	mops = &mbp22_ops;
	break;

      case MACHINE_MAC_UNKNOWN:
	logmsg(LOG_ERR, "Unknown Apple machine");

	exit(1);
	break;

      default:
	logmsg(LOG_ERR, "Unknown non-Apple machine");

	exit(1);
	break;
    }

  ret = mops->lcd_backlight_probe();
  if (ret < 0)
    {
      logmsg(LOG_ERR, "No LCD backlight found");

      exit(1);
    }

  nfds = open_evdev(&fds);
  if (nfds < 1)
    {
      logmsg(LOG_ERR, "No suitable event devices found");

      exit(1);
    }

  kbd_backlight_status_init();

  if (!debug)
    {
      /*
       * Detach from the console
       */
      if (daemon(0, 0) != 0)
	{
	  logmsg(LOG_ERR, "daemon() failed: %s", strerror(errno));

	  close_evdev(&fds, nfds);
	  exit(-1);
	}
    }

  pidfile = fopen(PIDFILE, "w");
  if (pidfile == NULL)
    {
      logmsg(LOG_WARNING, "Could not open pidfile %s: %s", PIDFILE, strerror(errno));

      close_evdev(&fds, nfds);
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
	  nfds = reopen_evdev(&fds, nfds);

	  if (nfds < 1)
	    {
	      logmsg(LOG_ERR, "No suitable event devices found (reopen)");

	      break;
	    }

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
	      if ((fds[i].revents & POLLERR)
		  || (fds[i].revents & POLLHUP)
		  || (fds[i].revents & POLLNVAL))
		{
		  logmsg(LOG_WARNING, "Error condition signaled on evdev, reopening");
		  reopen = 1;
		  break;
		}

	      if (fds[i].revents & POLLIN)
		process_evdev_events(fds[i].fd);
	    }

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
      else
	{
	  /* poll() timed out, check ambient light sensors */
	  kbd_backlight_ambient_check();
	  gettimeofday(&tv_als, NULL);
	}
    }

  close_evdev(&fds, nfds);
  unlink(PIDFILE);

  logmsg(LOG_INFO, "Exiting");

  if (!debug)
    closelog();

  return 0;
}
