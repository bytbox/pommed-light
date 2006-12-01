/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>

#include <syslog.h>

#include <errno.h>

#include <linux/input.h>

#include "mbpeventd.h"
#include "kbd_backlight.h"
#include "lcd_backlight.h"
#include "cd_eject.h"


#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)    ((array[LONG(bit)] >> OFF(bit)) & 1)


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
	    logdebug("\nKEY: LCD backlight down\n");

	    mops->lcd_backlight_step(STEP_DOWN);
	    break;

	  case K_LCD_BCK_UP:
	    logdebug("\nKEY: LCD backlight up\n");

	    mops->lcd_backlight_step(STEP_UP);
	    break;

	  case K_AUDIO_MUTE:
	    logdebug("\nKEY: audio mute\n");
	    break;

	  case K_AUDIO_DOWN:
	    logdebug("\nKEY: audio down\n");
	    break;

	  case K_AUDIO_UP:
	    logdebug("\nKEY: audio up\n");
	    break;

	  case K_VIDEO_TOGGLE:
	    logdebug("\nKEY: video toggle\n");
	    break;

	  case K_KBD_BCK_OFF:
	    logdebug("\nKEY: keyboard backlight off\n");

	    kbd_backlight_off();
	    break;

	  case K_KBD_BCK_DOWN:
	    logdebug("\nKEY: keyboard backlight down\n");

	    kbd_backlight_step(STEP_DOWN);
	    break;

	  case K_KBD_BCK_UP:
	    logdebug("\nKEY: keyboard backlight up\n");

	    kbd_backlight_step(STEP_UP);
	    break;

	  case K_CD_EJECT:
	    logdebug("\nKEY: CD eject\n");

	    cd_eject();
	    break;

	  default:
	    break;
	}
    }
}

/* Core Duo MacBook & MacBook Pro */
int
evdev_is_geyser3(unsigned short vendor, unsigned short product)
{
  if (vendor != USB_VENDOR_ID_APPLE)
    return 0;

  return ((product == USB_PRODUCT_ID_GEYSER3_ANSI)
	  || (product == USB_PRODUCT_ID_GEYSER3_ISO)
	  || (product == USB_PRODUCT_ID_GEYSER3_JIS));
}

/* Core2 Duo MacBook & MacBook Pro */
int
evdev_is_geyser4(unsigned short vendor, unsigned short product)
{
  if (vendor != USB_VENDOR_ID_APPLE)
    return 0;

  return ((product == USB_PRODUCT_ID_GEYSER4_ANSI)
	  || (product == USB_PRODUCT_ID_GEYSER4_ISO)
	  || (product == USB_PRODUCT_ID_GEYSER4_JIS));
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
	    logmsg(LOG_WARNING, "Could not open %s: %s", evdev, strerror(errno));

	  continue;
	}

      ioctl(fd[i], EVIOCGID, id);

      if (!mops->evdev_identify(id[ID_VENDOR], id[ID_PRODUCT]))
	{
	  logdebug("Discarding evdev %d vid 0x%04x, pid 0x%04x\n", i, id[ID_VENDOR], id[ID_PRODUCT]);

	  close(fd[i]);
	  fd[i] = -1;

	  continue;
	}

      memset(bit, 0, sizeof(bit));

      ioctl(fd[i], EVIOCGBIT(0, EV_MAX), bit[0]);

      if (!test_bit(1, bit[0]))
	{
	  logdebug("Discarding evdev %d with no key event type (not a keyboard)\n", i);

	  close(fd[i]);
	  fd[i] = -1;

	  continue;
	}
      else if (test_bit(2, bit[0]))
	{
	  logdebug("Discarding evdev %d with event type >= 2 (not a keyboard)\n", i);

	  close(fd[i]);
	  fd[i] = -1;

	  continue;
	}

      found++;
    }

  logdebug("Found %d devices\n", found);

  *fds = (struct pollfd *) malloc(found * sizeof(struct pollfd));

  if (*fds == NULL)
    {
      for (i = 0; i < EVDEV_MAX; i++)
	{
	  if (fd[i] > 0)
	    close(fd[i]);
	}

      logmsg(LOG_ERR, "Out of memory for %d pollfd structs", found);

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


int
reopen_evdev(struct pollfd **fds, int nfds)
{
  int i;

  close_evdev(fds, nfds);

  /* When resuming, we need to reopen event devices which
   * disappear at suspend time. We need to wait for udev to
   * recreate the device nodes.
   * Wait for up to 1.5 seconds, 5 * 0.3 seconds
   */
  for (i = 0; i < 5; i++)
    {
      usleep(300000);

      nfds = open_evdev(fds);

      if (nfds > 0)
	break;
    }

  return nfds;
}
