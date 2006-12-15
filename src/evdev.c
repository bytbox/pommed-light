/*
 * mbpeventd - MacBook Pro hotkey handler daemon
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
#include <string.h>
#include <poll.h>

#include <syslog.h>

#include <errno.h>

#include <linux/input.h>

#include "mbpeventd.h"
#include "conffile.h"
#include "evdev.h"
#include "kbd_backlight.h"
#include "lcd_backlight.h"
#include "cd_eject.h"
#include "audio.h"


#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)    ((array[LONG(bit)] >> OFF(bit)) & 1)


void
evdev_process_events(int fd)
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

	    audio_toggle_mute();
	    break;

	  case K_AUDIO_DOWN:
	    logdebug("\nKEY: audio down\n");

	    audio_step(STEP_DOWN);
	    break;

	  case K_AUDIO_UP:
	    logdebug("\nKEY: audio up\n");

	    audio_step(STEP_UP);
	    break;

	  case K_VIDEO_TOGGLE:
	    logdebug("\nKEY: video toggle\n");
	    break;

	  case K_KBD_BCK_OFF:
	    logdebug("\nKEY: keyboard backlight off\n");

	    if (!has_kbd_backlight())
	      break;

	    kbd_backlight_off();
	    break;

	  case K_KBD_BCK_DOWN:
	    logdebug("\nKEY: keyboard backlight down\n");

	    if (!has_kbd_backlight())
	      break;

	    kbd_backlight_step(STEP_DOWN);
	    break;

	  case K_KBD_BCK_UP:
	    logdebug("\nKEY: keyboard backlight up\n");

	    if (!has_kbd_backlight())
	      break;

	    kbd_backlight_step(STEP_UP);
	    break;

	  case K_CD_EJECT:
	    logdebug("\nKEY: CD eject\n");

	    cd_eject();
	    break;

	  case K_IR_FFWD:
	    logdebug("\nKEY: IR fast forward\n");
	    break;

	  case K_IR_REWD:
	    logdebug("\nKEY: IR rewind\n");
	    break;

	  case K_IR_PLAY:
	    logdebug("\nKEY: IR play/pause\n");
	    break;

	  case K_IR_MENU:
	    logdebug("\nKEY: IR menu\n");
	    break;

	  default:
#if 0
	    logdebug("\nKEY: %x\n", ev.code);
#endif /* 0 */
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

/* Apple Remote IR Receiver */
static int
evdev_is_appleir(unsigned short vendor, unsigned short product)
{
  if (vendor != USB_VENDOR_ID_APPLE)
    return 0;

  return (product == USB_PRODUCT_ID_APPLEIR);
}


int
evdev_open(struct pollfd **fds)
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

      if ((!mops->evdev_identify(id[ID_VENDOR], id[ID_PRODUCT]))
	  && !(appleir_cfg.enabled && evdev_is_appleir(id[ID_VENDOR], id[ID_PRODUCT])))
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
evdev_close(struct pollfd **fds, int nfds)
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
evdev_reopen(struct pollfd **fds, int nfds)
{
  int i;

  evdev_close(fds, nfds);

  /* When resuming, we need to reopen event devices which
   * disappear at suspend time. We need to wait for udev to
   * recreate the device nodes.
   * Wait for up to 3 seconds, 10 * 0.3 seconds
   */
  for (i = 0; i < 10; i++)
    {
      usleep(300000);

      nfds = evdev_open(fds);

      if (nfds > 0)
	break;
    }

  return nfds;
}
