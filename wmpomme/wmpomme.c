/*
 * wmpomme -- WindowMaker dockapp for use with pommed
 *
 * Copyright (C) 2006-2008 Julien BLACHE <jb@jblache.org>
 *
 * Based on wmwave by Carsten Schuermann <carsten@schuermann.org>
 * wmwave derived from:
 *   Dan Piponi dan@tanelorn.demon.co.uk
 *   http://www.tanelorn.demon.co.uk
 *     who derived it from code originally contained
 *     in wmsysmon by Dave Clark (clarkd@skynet.ca)
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
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <time.h>
#include <poll.h>

#include <signal.h>

#include <errno.h>

#include <X11/Xlib.h>
#include <X11/xpm.h>

#include <dbus/dbus.h>

#ifndef NO_SYS_TIMERFD_H
# include <sys/timerfd.h>
#else
# include "timerfd-syscalls.h"
#endif

#include "wmgeneral.h"
#include "wmpomme-master.xpm"

#include "../client-common/dbus-client.h"
#include "../client-common/video-client.h"


struct {
  int lcd_lvl;
  int lcd_max;
  int kbd_lvl;
  int kbd_max;
  int snd_lvl;
  int snd_max;
  int snd_mute;

  int ambient_l;
  int ambient_r;
  int ambient_max;
} mbp;


#define DISPLAY_TYPE_DBUS_NOK  (1 << 0)
#define DISPLAY_TYPE_NO_DATA   (1 << 1)
#define DISPLAY_TYPE_MACBOOK   (1 << 2)
#define DISPLAY_TYPE_AMBIENT   (1 << 3)

#define DISPLAY_MASK_TYPE      (0xffff)
#define DISPLAY_TYPE(d)        (d & DISPLAY_MASK_TYPE)

#define DISPLAY_FLAG_UPDATE    (1 << 16)

#define DISPLAY_MASK_FLAGS     (0xffff0000)
#define DISPLAY_FLAGS(d)       (d & DISPLAY_MASK_FLAGS)

unsigned int mbpdisplay = DISPLAY_TYPE_DBUS_NOK;


char wmmbp_mask_bits[64*64];
int wmmbp_mask_width = 64;
int wmmbp_mask_height = 64;

#define WMPOMME_VERSION "0.2"

char *ProgName;


DBusError dbus_err;
DBusConnection *conn;

void
wmmbp_get_values(void);

int
wmmbp_dbus_init(void)
{
  unsigned int signals;

  signals = MBP_DBUS_SIG_LCD | MBP_DBUS_SIG_KBD
            | MBP_DBUS_SIG_VOL | MBP_DBUS_SIG_MUTE
            | MBP_DBUS_SIG_LIGHT | MBP_DBUS_SIG_VIDEO;

  conn = mbp_dbus_init(&dbus_err, signals);

  if (conn == NULL)
    {
      mbpdisplay = DISPLAY_FLAG_UPDATE | DISPLAY_TYPE_DBUS_NOK;

      return -1;
    }
  else
    wmmbp_get_values();

  return 0;
}

void
mbp_dbus_listen(void)
{
  DBusMessage *msg;

  int scratch;

  if (conn == NULL)
    return;

  while (1)
    {
      dbus_connection_read_write(conn, 0);

      msg = dbus_connection_pop_message(conn);

      if (msg == NULL)
	return;

      if (dbus_message_is_signal(msg, "org.pommed.signal.ambientLight", "ambientLight"))
	{
	  dbus_message_get_args(msg, &dbus_err,
				DBUS_TYPE_UINT32, &mbp.ambient_l,
				DBUS_TYPE_UINT32, &scratch, /* previous left */
				DBUS_TYPE_UINT32, &mbp.ambient_r,
				DBUS_TYPE_UINT32, &scratch, /* previous right */
				DBUS_TYPE_UINT32, &mbp.ambient_max,
				DBUS_TYPE_INVALID);

	  if (mbpdisplay & DISPLAY_TYPE_AMBIENT)
	    mbpdisplay |= DISPLAY_FLAG_UPDATE;
	}
      else if (dbus_message_is_signal(msg, "org.pommed.signal.lcdBacklight", "lcdBacklight"))
	{
	  dbus_message_get_args(msg, &dbus_err,
				DBUS_TYPE_UINT32, &mbp.lcd_lvl,
				DBUS_TYPE_UINT32, &scratch, /* previous */
				DBUS_TYPE_UINT32, &mbp.lcd_max,
				DBUS_TYPE_UINT32, &scratch, /* who */
				DBUS_TYPE_INVALID);

	  if (mbpdisplay & DISPLAY_TYPE_MACBOOK)
	    mbpdisplay |= DISPLAY_FLAG_UPDATE;
	}
      else if (dbus_message_is_signal(msg, "org.pommed.signal.kbdBacklight", "kbdBacklight"))
	{
	  dbus_message_get_args(msg, &dbus_err,
				DBUS_TYPE_UINT32, &mbp.kbd_lvl,
				DBUS_TYPE_UINT32, &scratch, /* previous */
				DBUS_TYPE_UINT32, &mbp.kbd_max,
				DBUS_TYPE_UINT32, &scratch, /* who */
				DBUS_TYPE_INVALID);

	  if (mbpdisplay & (DISPLAY_TYPE_MACBOOK | DISPLAY_TYPE_AMBIENT))
	    mbpdisplay |= DISPLAY_FLAG_UPDATE;
	}
      else if (dbus_message_is_signal(msg, "org.pommed.signal.audioVolume", "audioVolume"))
	{
	  dbus_message_get_args(msg, &dbus_err,
				DBUS_TYPE_UINT32, &mbp.snd_lvl,
				DBUS_TYPE_UINT32, &scratch, /* previous */
				DBUS_TYPE_UINT32, &mbp.snd_max,
				DBUS_TYPE_INVALID);

	  if (mbpdisplay & DISPLAY_TYPE_MACBOOK)
	    mbpdisplay |= DISPLAY_FLAG_UPDATE;
	}
      else if (dbus_message_is_signal(msg, "org.pommed.signal.audioMute", "audioMute"))
	{
	  dbus_message_get_args(msg, &dbus_err,
				DBUS_TYPE_BOOLEAN, &mbp.snd_mute,
				DBUS_TYPE_INVALID);

	  if (mbpdisplay & DISPLAY_TYPE_MACBOOK)
	    mbpdisplay |= DISPLAY_FLAG_UPDATE;
	}
      else if (dbus_message_is_signal(msg, "org.pommed.signal.videoSwitch", "videoSwitch"))
	{
	  mbp_video_switch(display);
	}
      else if (dbus_message_is_signal(msg, DBUS_INTERFACE_LOCAL, "Disconnected"))
	{
	  fprintf(stderr, "DBus disconnected\n");

	  mbpdisplay = DISPLAY_FLAG_UPDATE | DISPLAY_TYPE_DBUS_NOK;

	  mbp_dbus_cleanup();
	  conn = NULL;

	  dbus_message_unref(msg);

	  break;
	}

      dbus_message_unref(msg);
    }
}


/* DBus method call callbacks */
void
wmmbp_lcd_getlevel_cb(DBusPendingCall *pending, void *status)
{
  DBusMessage *msg;

  msg = dbus_pending_call_steal_reply(pending);

  if (msg == NULL)
    {
      fprintf(stderr, "Could not steal reply\n");

      dbus_pending_call_unref(pending);

      return;
    }

  dbus_pending_call_unref(pending);

  if (!mbp_dbus_check_error(msg))
    {
      dbus_message_get_args(msg, &dbus_err,
			    DBUS_TYPE_UINT32, &mbp.lcd_lvl,
			    DBUS_TYPE_UINT32, &mbp.lcd_max,
			    DBUS_TYPE_INVALID);
    }
  else
    *(int *)status = -1;

  dbus_message_unref(msg);
}

void
wmmbp_kbd_getlevel_cb(DBusPendingCall *pending, void *status)
{
  DBusMessage *msg;

  msg = dbus_pending_call_steal_reply(pending);

  if (msg == NULL)
    {
      fprintf(stderr, "Could not steal reply\n");

      dbus_pending_call_unref(pending);

      return;
    }

  dbus_pending_call_unref(pending);

  if (!mbp_dbus_check_error(msg))
    {
      dbus_message_get_args(msg, &dbus_err,
			    DBUS_TYPE_UINT32, &mbp.kbd_lvl,
			    DBUS_TYPE_UINT32, &mbp.kbd_max,
			    DBUS_TYPE_INVALID);
    }
  else
    *(int *)status = -1;

  dbus_message_unref(msg);
}

void
wmmbp_ambient_getlevel_cb(DBusPendingCall *pending, void *status)
{
  DBusMessage *msg;

  msg = dbus_pending_call_steal_reply(pending);

  if (msg == NULL)
    {
      fprintf(stderr, "Could not steal reply\n");

      dbus_pending_call_unref(pending);

      return;
    }

  dbus_pending_call_unref(pending);

  if (!mbp_dbus_check_error(msg))
    {
      dbus_message_get_args(msg, &dbus_err,
			    DBUS_TYPE_UINT32, &mbp.ambient_l,
			    DBUS_TYPE_UINT32, &mbp.ambient_r,
			    DBUS_TYPE_UINT32, &mbp.ambient_max,
			    DBUS_TYPE_INVALID);
    }
  else
    *(int *)status = -1;

  dbus_message_unref(msg);
}

void
wmmbp_audio_getvolume_cb(DBusPendingCall *pending, void *status)
{
  DBusMessage *msg;

  msg = dbus_pending_call_steal_reply(pending);

  if (msg == NULL)
    {
      fprintf(stderr, "Could not steal reply\n");

      dbus_pending_call_unref(pending);

      return;
    }

  dbus_pending_call_unref(pending);

  if (!mbp_dbus_check_error(msg))
    {
      dbus_message_get_args(msg, &dbus_err,
			    DBUS_TYPE_UINT32, &mbp.snd_lvl,
			    DBUS_TYPE_UINT32, &mbp.snd_max,
			    DBUS_TYPE_INVALID);
    }
  else
    *(int *)status = -1;

  dbus_message_unref(msg);
}

void
wmmbp_audio_getmute_cb(DBusPendingCall *pending, void *status)
{
  DBusMessage *msg;

  msg = dbus_pending_call_steal_reply(pending);

  if (msg == NULL)
    {
      fprintf(stderr, "Could not steal reply\n");

      dbus_pending_call_unref(pending);

      return;
    }

  dbus_pending_call_unref(pending);

  if (!mbp_dbus_check_error(msg))
    {
      dbus_message_get_args(msg, &dbus_err,
			    DBUS_TYPE_BOOLEAN, &mbp.snd_mute,
			    DBUS_TYPE_INVALID);
    }
  else
    *(int *)status = -1;

  dbus_message_unref(msg);
}


void
wmmbp_get_values(void)
{
  int ret;
  int cbret;

  ret = mbp_call_lcd_getlevel(wmmbp_lcd_getlevel_cb, &cbret);
  if ((ret < 0) || (cbret < 0))
    {
      fprintf(stderr, "lcdBacklight getLevel call failed !\n");
      goto mcall_error;
    }

  ret = mbp_call_kbd_getlevel(wmmbp_kbd_getlevel_cb, &cbret);
  if ((ret < 0) || (cbret < 0))
    {
      fprintf(stderr, "kbdBacklight getLevel call failed !\n");
      goto mcall_error;
    }

  ret = mbp_call_ambient_getlevel(wmmbp_ambient_getlevel_cb, &cbret);
  if ((ret < 0) || (cbret < 0))
    {
      fprintf(stderr, "ambient getLevel call failed !\n");
      goto mcall_error;
    }

  ret = mbp_call_audio_getvolume(wmmbp_audio_getvolume_cb, &cbret);
  if ((ret < 0) || (cbret < 0))
    {
      fprintf(stderr, "audio getVolume call failed !\n");
      goto mcall_error;
    }

  ret = mbp_call_audio_getmute(wmmbp_audio_getmute_cb, &cbret);
  if ((ret < 0) || (cbret < 0))
    {
      fprintf(stderr, "audio getMute call failed !\n");
      goto mcall_error;
    }

  if (DISPLAY_TYPE(mbpdisplay) <= DISPLAY_TYPE_NO_DATA)
    mbpdisplay = DISPLAY_TYPE_MACBOOK;

  mbpdisplay |= DISPLAY_FLAG_UPDATE;

  return;

 mcall_error:
  mbpdisplay = DISPLAY_FLAG_UPDATE | DISPLAY_TYPE_NO_DATA;
}


void
usage(void);

void
printversion(void);

void
BlitString(char *name, int x, int y);

void
BlitNum(int num, int x, int y);

void
wmmbp_routine(int argc, char **argv);


void
DrawBar(float percent, int dx, int dy)
{
  int tx;

  tx = (int)(54.0 * (percent * 0.01));
  copyXPMArea(67, 36, tx, 4, dx, dy);
  copyXPMArea(67, 43, 54-tx, 4, dx+tx, dy);
}

void
DrawGreenBar(float percent, int dx, int dy)
{
  int tx;

  tx = (int)(54.0 * (percent * 0.01));
  copyXPMArea(67, 58, tx, 4, dx, dy);
  copyXPMArea(67, 43, 54-tx, 4, dx+tx, dy);
}

void
DrawRedDot(void)
{
  copyXPMArea(80, 65, 6, 6, 52, 5);
}

void
DrawYellowDot(void)
{
  copyXPMArea(86, 65, 6, 6, 52, 5);
}

void
DrawGreenDot(void)
{
  copyXPMArea(92, 65, 6, 6, 52, 5);
}

void
DrawEmptyDot(void)
{
  copyXPMArea(98, 65, 6, 6, 52, 5);
}


void
DisplayMBPStatus(void)
{
  switch (DISPLAY_TYPE(mbpdisplay))
    {
      case DISPLAY_TYPE_MACBOOK:
	BlitString("MacBook", 4, 4);
	DrawGreenDot();

	BlitString("LCD level", 4, 18);
	DrawBar(((float)mbp.lcd_lvl / (float)mbp.lcd_max) * 100.0, 4, 27);

	BlitString("KBD level", 4, 32);
	DrawGreenBar(((float)mbp.kbd_lvl / (float)mbp.kbd_max) * 100.0, 4, 41);

	if (mbp.snd_mute)
	  BlitString("Audio OFF", 4, 46);
	else
	  BlitString("Audio    ", 4, 46);
	DrawGreenBar(((float)mbp.snd_lvl / (float)mbp.snd_max) * 100.0, 4, 55);
	break;

      case DISPLAY_TYPE_AMBIENT:
	BlitString("Ambient", 4, 4);
	DrawYellowDot();

	BlitString("Left     ", 4, 18);
	DrawBar(((float)mbp.ambient_l / (float)mbp.ambient_max) * 100.0, 4, 27);

	BlitString("Right    ", 4, 32);
	DrawBar(((float)mbp.ambient_r / (float)mbp.ambient_max) * 100.0, 4, 41);

	BlitString("KBD level", 4, 46);
	DrawGreenBar(((float)mbp.kbd_lvl / (float)mbp.kbd_max) * 100.0, 4, 55);
	break;

      case DISPLAY_TYPE_DBUS_NOK:
	BlitString(" Error ", 4, 4);
	DrawRedDot();

	BlitString("DBus     ", 4, 18);
	DrawBar(0.0, 4, 27);

	BlitString("Connect  ", 4, 32);
	DrawGreenBar(0.0, 4, 41);

	BlitString("Failed   ", 4, 46);
	DrawGreenBar(0.0, 4, 55);
	break;

      case DISPLAY_TYPE_NO_DATA:
	BlitString("No Data", 4, 4);
	DrawRedDot();

	BlitString("Server   ", 4, 18);
	DrawBar(0.0, 4, 27);

	BlitString("Not      ", 4, 32);
	DrawGreenBar(0.0, 4, 41);

	BlitString("Running ?", 4, 46);
	DrawGreenBar(0.0, 4, 55);
	break;
    }

  mbpdisplay = DISPLAY_TYPE(mbpdisplay);
}


static int
mbp_create_timer(int timeout)
{
  int fd;
  int ret;

  struct itimerspec timing;

  fd = timerfd_create(CLOCK_MONOTONIC, 0);
  if (fd < 0)
    {
      fprintf(stderr, "Could not create timer: %s", strerror(errno));

      return -1;
    }

  timing.it_interval.tv_sec = (timeout >= 1000) ? timeout / 1000 : 0;
  timing.it_interval.tv_nsec = (timeout - (timing.it_interval.tv_sec * 1000)) * 1000000;

  ret = clock_gettime(CLOCK_MONOTONIC, &timing.it_value);
  if (ret < 0)
    {
      fprintf(stderr, "Could not get current time: %s", strerror(errno));

      close(fd);
      return -1;
    }

  timing.it_value.tv_sec += timing.it_interval.tv_sec;
  timing.it_value.tv_nsec += timing.it_interval.tv_nsec;
  if (timing.it_value.tv_nsec > 1000000000)
    {
      timing.it_value.tv_sec++;
      timing.it_value.tv_nsec -= 1000000000;
    }

  ret = timerfd_settime(fd, TFD_TIMER_ABSTIME, &timing, NULL);
  if (ret < 0)
    {
      fprintf(stderr, "Could not setup timer: %s", strerror(errno));

      close(fd);
      return -1;
    }

  return fd;
}


int running;

void
sig_int_term_handler(int signo)
{
  running = 0;
}

void
sig_chld_handler(int signo)
{
  int ret;

  do
    {
      ret = waitpid(-1, NULL, WNOHANG);
    }
  while (ret > 0);
}

int
main(int argc, char **argv)
{
  int i;

  signal(SIGINT, sig_int_term_handler);
  signal(SIGTERM, sig_int_term_handler);
  signal(SIGCHLD, sig_chld_handler);

  ProgName = argv[0];
  if (strlen(ProgName) >= 5)
    ProgName += (strlen(ProgName) - 5);

  /* Parse Command Line */
  for (i = 1; i < argc; i++)
    {
      char *arg = argv[i];

      if (*arg == '-')
	{
	  switch (arg[1])
	    {
	      case 'd':
		if (strcmp(arg+1, "display"))
		  {
		    usage();
		    exit(1);
		  }
		break;

	      case 'g':
		if (strcmp(arg+1, "geometry"))
		  {
		    usage();
		    exit(1);
		  }
		break;

	      case 'v':
		printversion();
		exit(0);
		break;

	      default:
		usage();
		exit(0);
		break;
	    }
	}
    }

  wmmbp_dbus_init();

  createXBMfromXPM(wmmbp_mask_bits, wmmbp_master_xpm, wmmbp_mask_width, wmmbp_mask_height);

  openXwindow(argc, argv, wmmbp_master_xpm, wmmbp_mask_bits, wmmbp_mask_width, wmmbp_mask_height);

  wmmbp_routine(argc, argv);

  mbp_dbus_cleanup();

  return 0;
}

/*
 * Main loop
 */
void
wmmbp_routine(int argc, char **argv)
{
  int nfds;
  struct pollfd fds[2];

  int t_fd;
  uint64_t ticks;

  int ret;

  XEvent Event;

  /* X */
  fds[0].fd = x_fd;
  fds[0].events = POLLIN;
  nfds = 1;

  /* DBus */
  if (conn != NULL)
    {
      if (dbus_connection_get_unix_fd(conn, &fds[1].fd))
	nfds = 2;
    }
  fds[1].events = POLLIN;

  t_fd = -1;

  wmmbp_get_values();

  RedrawWindow();

  running = 1;
  while (running)
    {
      if ((conn == NULL) && (t_fd == -1))
	{
	  /* setup reconnect timer */
	  t_fd = mbp_create_timer(200);

	  if (t_fd != -1)
	    {
	      fds[1].fd = t_fd;
	      nfds = 2;
	    }
	  else
	    nfds = 1;

	  fds[1].revents = 0;
	}

      ret = poll(fds, nfds, -1);

      if (ret < 0)
	continue;

      /* DBus */
      if ((nfds == 2) && (fds[1].revents != 0))
	{
	  /* reconnection */
	  if (conn == NULL)
	    {
	      /* handle timer & reconnect, fd */
	      read(fds[1].fd, &ticks, sizeof(ticks));

	      if (wmmbp_dbus_init() == 0)
		{
		  close(t_fd);
		  t_fd = -1;

		  if (!dbus_connection_get_unix_fd(conn, &fds[1].fd))
		    {
		      fds[1].fd = -1;
		      nfds = 1;
		    }
		}
	    }
	  else /* events */
	    {
	      mbp_dbus_listen();
	    }
	}

      if ((mbpdisplay & DISPLAY_TYPE_NO_DATA) && (conn != NULL))
	wmmbp_get_values();

      /* X Events */
      if (fds[0].revents != 0)
	{
	  while (XPending(display))
	    {
	      XNextEvent(display, &Event);
	      switch (Event.type)
		{
		  case Expose:
		    mbpdisplay |= DISPLAY_FLAG_UPDATE;
		    break;

		  case DestroyNotify:
		    XCloseDisplay(display);
		    return;

		  case ButtonPress:
		    if (DISPLAY_TYPE(mbpdisplay) > DISPLAY_TYPE_NO_DATA)
		      {
			switch (DISPLAY_TYPE(mbpdisplay))
			  {
			    case DISPLAY_TYPE_MACBOOK:
			      mbpdisplay = DISPLAY_FLAG_UPDATE | DISPLAY_TYPE_AMBIENT;
			      break;

			    case DISPLAY_TYPE_AMBIENT:
			      mbpdisplay = DISPLAY_FLAG_UPDATE | DISPLAY_TYPE_MACBOOK;
			      break;
			  }
		      }
		    break;
		}
	    }
	}

      /* Update display */
      if (mbpdisplay & DISPLAY_FLAG_UPDATE)
	{
	  DisplayMBPStatus();
	  RedrawWindow();
	}
    }
}

/*
 * Blits a string at given co-ordinates
 */
void
BlitString(char *name, int x, int y)
{
  int i;
  int c;
  int k;

  k = x;
  for (i=0; name[i]; i++)
    {

      c = toupper(name[i]);
      /* A letter */
      if (c >= 'A' && c <= 'Z')
        {
	  c -= 'A';
	  copyXPMArea(c * 6, 74, 6, 8, k, y);
	  k += 6;
	}
      /* A number or symbol */
      else if (c>='0' && c<='9')
	{
	  c -= '0';
	  copyXPMArea(c * 6, 64, 6, 8, k, y);
	  k += 6;
	}
      else
	{
	  copyXPMArea(5, 84, 6, 8, k, y);
	  k += 6;
	}
    }
}

void
BlitNum(int num, int x, int y)
{
  char buf[1024];

  sprintf(buf, "%03i", num);

  BlitString(buf, x, y);
}

void
usage(void)
{
  fprintf(stderr, "wmpomme v" WMPOMME_VERSION "\n");
  fprintf(stderr, "Copyright (C) 2006-2008 Julien BLACHE <jb@jblache.org>\n");
  fprintf(stderr, "Based on wmwave by Carsten Schuermann <carsten@schuermann.org>\n\n");
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "\t-display <display name>\n");
}

void
printversion(void)
{
  fprintf(stderr, "wmpomme v%s\n", WMPOMME_VERSION);
  fprintf(stderr, "Copyright (C) 2006-2008 Julien BLACHE <jb@jblache.org>\n");
}
