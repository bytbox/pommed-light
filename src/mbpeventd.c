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

#include <errno.h>

#include <linux/input.h>

#include <smbios/SystemInfo.h>

#include "mbpeventd.h"
#include "kbd_backlight.h"
#include "lcd_backlight.h"
#include "cd_eject.h"

/* Machine type */
int machine;

/* Used by signal handlers */
static int running;


void
process_evdev_events(int fd)
{
  int ret;

  struct input_event ev;

  ret = read(fd, &ev, sizeof(struct input_event));

  if (ret != sizeof(struct input_event))
    return;

  if (ev.type == EV_KEY)
    {
      /* key released - we don't care */
      if (ev.value == 0)
	return;

      switch (ev.code)
	{
	  case K_LCD_BCK_DOWN:
	    debug("\nKEY: LCD backlight down\n");

	    lcd_backlight_step(STEP_DOWN);
	    break;

	  case K_LCD_BCK_UP:
	    debug("\nKEY: LCD backlight up\n");

	    lcd_backlight_step(STEP_UP);
	    break;

	  case K_AUDIO_MUTE:
	    debug("\nKEY: audio mute\n");
	    break;

	  case K_AUDIO_DOWN:
	    debug("\nKEY: audio down\n");
	    break;

	  case K_AUDIO_UP:
	    debug("\nKEY: audio up\n");
	    break;

	  case K_VIDEO_TOGGLE:
	    debug("\nKEY: video toggle\n");
	    break;

	  case K_KBD_BCK_OFF:
	    debug("\nKEY: keyboard backlight off\n");

	    kbd_backlight_set(KBD_BACKLIGHT_OFF);
	    break;

	  case K_KBD_BCK_DOWN:
	    debug("\nKEY: keyboard backlight down\n");

	    kbd_backlight_step(STEP_DOWN);
	    break;

	  case K_KBD_BCK_UP:
	    debug("\nKEY: keyboard backlight up\n");

	    kbd_backlight_step(STEP_UP);
	    break;

	  case K_CD_EJECT:
	    debug("\nKEY: CD eject\n");

	    cd_eject();
	    break;

	  default:
	    break;
	}
    }
}


int
check_machine_smbios(void)
{
  int ret = -1;

  const char *prop;

  if (geteuid() != 0)
    {
      debug("Error: SMBIOS machine detection only works as root\n");

      return ret;
    }

  /* Check vendor name */
  prop = SMBIOSGetVendorName();

  debug("SMBIOS vendor name: [%s]\n", prop);
  if (strcmp(prop, "Apple Computer, Inc.") == 0)
    ret = MACHINE_VENDOR_APPLE;

  SMBIOSFreeMemory(prop);

  if (ret != MACHINE_VENDOR_APPLE)
    return ret;

  ret = MACHINE_MAC_UNKNOWN;

  /* Check system name */
  prop = SMBIOSGetSystemName();

  debug("SMBIOS system name: [%s]\n", prop);
  if (strcmp(prop, "MacBookPro2,2") == 0)
    ret = MACHINE_MACBOOKPRO_22;

  SMBIOSFreeMemory(prop);

  return ret;
}


int
evdev_is_geyser4(unsigned short vendor, unsigned short product)
{
  if (vendor != USB_VENDOR_ID_APPLE)
    return 0;

  return (product == USB_PRODUCT_ID_GEYSER4_ISO);
}

int
open_evdev(struct pollfd **fds)
{
  int ret;
  int i, j;

  int found = 0;
  int fd[32];

  unsigned short id[4];
  unsigned long bit[EV_MAX][NBITS(KEY_MAX)];
  char evdev[32];

  for (i = 0; i < EVDEV_MAX; i++)
    {
      ret = snprintf(evdev, 32, "%s%d", EVDEV_BASE, i);

      if ((ret <= 0) || (ret > 31))
	return -1;

      fd[i] = open(evdev, O_RDWR);
      if (fd[i] < 0)
	{
	  if (errno != ENOENT)
	    debug("Could not open %s: %s\n", evdev, strerror(errno));

	  continue;
	}

      ioctl(fd[i], EVIOCGID, id);

      if (!evdev_is_geyser4(id[ID_VENDOR], id[ID_PRODUCT]))
	{
	  debug("Discarding evdev %d vid 0x%04x, pid 0x%04x\n", i, id[ID_VENDOR], id[ID_PRODUCT]);

	  close(fd[i]);
	  fd[i] = -1;

	  continue;
	}

      memset(bit, 0, sizeof(bit));

      ioctl(fd[i], EVIOCGBIT(0, EV_MAX), bit[0]);

      if (!test_bit(1, bit[0]))
	{
	  debug("Discarding evdev %d with no key event type (not a keyboard)\n", i);

	  close(fd[i]);
	  fd[i] = -1;

	  continue;
	}
      else if (test_bit(2, bit[0]))
	{
	  debug("Discarding evdev %d with event type >= 2 (not a keyboard)\n", i);

	  close(fd[i]);
	  fd[i] = -1;

	  continue;
	}

      found++;
    }

  debug("Found %d devices\n", found);

  *fds = (struct pollfd *) malloc(found * sizeof(struct pollfd));

  if (*fds == NULL)
    {
      for (i = 0; i < EVDEV_MAX; i++)
	{
	  if (fd[i] > 0)
	    close(fd[i]);
	}

      debug("Out of memory for %d pollfd structs\n", found);

      return -1;
    }

  j = 0;
  for (i = 0; i < EVDEV_MAX && j < found; i++)
    {
      if (fd[i] < 0)
	continue;

      (*fds)[j].fd = fd[i];
      (*fds)[j].events = POLLIN;
      j++;
    }

  return found;
}

void
close_evdev(struct pollfd **fds, int nfds)
{
  int i;

  if (*fds != NULL)
    {
      for (i = 0; i < nfds; i++)
	close((*fds)[i].fd);

      free(*fds);
    }

  *fds = NULL;
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
  int i;

  FILE *pidfile;

  struct pollfd *fds;
  int nfds;

  struct timeval tv_now;
  struct timeval tv_als;
  struct timeval tv_diff;

  machine = check_machine_smbios();
  switch (machine)
    {
      case MACHINE_MACBOOKPRO_22:
	debug("Detected (SMBIOS) a MacBookPro2,2\n");
	break;
      case MACHINE_MAC_UNKNOWN:
	fprintf(stderr, "Error: unknown Apple machine\n");

	exit(1);
	break;
      default:
	fprintf(stderr, "Error: unknown non-Apple machine\n");

	exit(1);
	break;
    }


  ret = lcd_backlight_probe_X1600();
  if (ret < 0)
    {
      fprintf(stderr, "Error: no Radeon Mobility X1600 found\n");

      exit(1);
    }

  nfds = open_evdev(&fds);
  if (nfds < 1)
    {
      fprintf(stderr, "Error: no devices found\n");

      exit(1);
    }

  kbd_backlight_status_init();

  /*
   * Detach from the console
   */
  if (daemon(1, 1) != 0)
    {
      fprintf(stderr, "Error: daemon() failed: %s\n", strerror(errno));

      close_evdev(&fds, nfds);
      exit(-1);
    }

  pidfile = fopen(PIDFILE, "w");
  if (pidfile == NULL)
    {
      debug("Could not open pidfile: %s\n", strerror(errno));

      close_evdev(&fds, nfds);
      exit(-1);
    }
  fprintf(pidfile, "%d\n", getpid());
  fclose(pidfile);

  gettimeofday(&tv_als, NULL);

  running = 1;
  signal(SIGINT, sig_int_term_handler);
  signal(SIGTERM, sig_int_term_handler);

  while (1 && running)
    {
      ret = poll(fds, nfds, LOOP_TIMEOUT);

      if (ret < 0) /* error */
	{
	  debug("Poll error: %s\n", strerror(errno));

	  break;
	}
      else
	{
	  if (ret != 0)
	    {
	      for (i = 0; i < nfds; i++)
		{
		  if (fds[i].revents & POLLIN)
		    process_evdev_events(fds[i].fd);
		}
	    }

	  /* only check ambient light sensors when poll() times out
	   * or when we haven't checked since LOOP_TIMEOUT
	   */
	  gettimeofday(&tv_now, NULL);
	  if (ret == 0)
	    {
	      kbd_backlight_ambient_check();
	      tv_als = tv_now;
	    }
	  else
	    {
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
    }

  close_evdev(&fds, nfds);
  unlink(PIDFILE);

  return 0;
}
