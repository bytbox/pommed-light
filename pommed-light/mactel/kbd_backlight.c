/*
 * pommed - Apple laptops hotkeys handler daemon
 *
 * Copyright (C) 2006-2008 Julien BLACHE <jb@jblache.org>
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
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include <syslog.h>

#include <errno.h>

#include "../pommed-light.h"
#include "../evloop.h"
#include "../conffile.h"
#include "../kbd_backlight.h"

struct _kbd_bck_info kbd_bck_info;


static int
kbd_backlight_open(int flags)
{
  char *kbdbck_node[] =
    {
      "/sys/class/leds/smc::kbd_backlight/brightness", /* 2.6.25 & up */
      "/sys/class/leds/smc:kbd_backlight/brightness"
    };
  int fd;
  int i;

  for (i = 0; i < sizeof(kbdbck_node) / sizeof(*kbdbck_node); i++)
    {
      logdebug("Trying %s\n", kbdbck_node[i]);

      fd = open(kbdbck_node[i], flags);
      if (fd >= 0)
	return fd;

      if (errno == ENOENT)
	continue;

      logmsg(LOG_WARNING, "Could not open %s: %s", kbdbck_node[i], strerror(errno));
      return -1;
    }

  return -1;
}


static int
kbd_backlight_get(void)
{
  int fd;
  int ret;
  char buf[8];

  fd = kbd_backlight_open(O_RDONLY);
  if (fd < 0)
    return -1;

  memset(buf, 0, 8);

  ret = read(fd, buf, 8);

  close(fd);

  if ((ret < 1) || (ret > 7))
    return -1;

  ret = atoi(buf);

  logdebug("KBD backlight value is %d\n", ret);

  if ((ret < KBD_BACKLIGHT_OFF) || (ret > KBD_BACKLIGHT_MAX))
    ret = -1;

  return ret;
}

static void
kbd_backlight_set(int val, int who)
{
  int curval;

  int i;
  float fadeval;
  float step;
  struct timespec fade_step;

  int fd;
  FILE *fp;

  if (kbd_bck_info.inhibit & ~KBD_INHIBIT_CFG)
    return;

  curval = kbd_backlight_get();

  if (val == curval)
    return;

  if ((val < KBD_BACKLIGHT_OFF) || (val > KBD_BACKLIGHT_MAX))
    return;

  if (who == KBD_AUTO)
    {
      fade_step.tv_sec = 0;
      fade_step.tv_nsec = (KBD_BACKLIGHT_FADE_LENGTH / KBD_BACKLIGHT_FADE_STEPS) * 1000000;

      fadeval = (float)curval;
      step = (float)(val - curval) / (float)KBD_BACKLIGHT_FADE_STEPS;

      for (i = 0; i < KBD_BACKLIGHT_FADE_STEPS; i++)
	{
	  fadeval += step;

	  fd = kbd_backlight_open(O_WRONLY);
	  if (fd < 0)
	    continue;

	  fp = fdopen(fd, "a");
	  if (fp == NULL)
	    {
	      logmsg(LOG_WARNING, "Could not fdopen backlight fd: %s", strerror(errno));
	      close(fd);
	      continue;
	    }

	  fprintf(fp, "%d", (int)fadeval);

	  fclose(fp);

	  logdebug("KBD backlight value faded to %d\n", (int)fadeval);

	  nanosleep(&fade_step, NULL);
	}
    }

  fd = kbd_backlight_open(O_WRONLY);
  if (fd < 0)
    return;

  fp = fdopen(fd, "a");
  if (fp == NULL)
    {
      logmsg(LOG_WARNING, "Could not fdopen backlight fd %d: %s", fd, strerror(errno));
      close(fd);
      return;
    }

  fprintf(fp, "%d", val);

  fclose(fp);

  logdebug("KBD backlight value set to %d\n", val);

  kbd_bck_info.level = val;
}

void
kbd_backlight_step(int dir)
{
  int val;
  int newval;

  if (kbd_bck_info.inhibit & ~KBD_INHIBIT_CFG)
    return;

  val = kbd_backlight_get();

  if (val < 0)
    return;

  if (dir == STEP_UP)
    {
      newval = val + kbd_cfg.step;

      if (newval > KBD_BACKLIGHT_MAX)
	newval = KBD_BACKLIGHT_MAX;

      logdebug("KBD stepping +%d -> %d\n", kbd_cfg.step, newval);
    }
  else if (dir == STEP_DOWN)
    {
      newval = val - kbd_cfg.step;

      if (newval < KBD_BACKLIGHT_OFF)
	newval = KBD_BACKLIGHT_OFF;

      logdebug("KBD stepping -%d -> %d\n", kbd_cfg.step, newval);
    }
  else
    return;

  kbd_backlight_set(newval, KBD_USER);
}


/* Include automatic backlight routines */
#include "../kbd_auto.c"


void
kbd_backlight_init(void)
{
  if (kbd_cfg.auto_on)
    kbd_bck_info.inhibit = 0;
  else
    kbd_bck_info.inhibit = KBD_INHIBIT_CFG;

  kbd_bck_info.toggle_lvl = kbd_cfg.auto_lvl;

  kbd_bck_info.inhibit_lvl = 0;

  kbd_bck_info.auto_on = 0;

  if (!has_kbd_backlight())
    {
      kbd_bck_info.r_sens = 0;
      kbd_bck_info.l_sens = 0;

      kbd_bck_info.level = 0;

      return;
    }

  kbd_bck_info.level = kbd_backlight_get();
  if (kbd_bck_info.level < 0)
    kbd_bck_info.level = 0;

  kbd_bck_info.max = KBD_BACKLIGHT_MAX;

  kbd_auto_init();
}

void
kbd_backlight_cleanup(void)
{
  if (has_kbd_backlight())
    kbd_auto_cleanup();
}


void
kbd_backlight_fix_config(void)
{
  if (kbd_cfg.auto_lvl > KBD_BACKLIGHT_MAX)
    kbd_cfg.auto_lvl = KBD_BACKLIGHT_MAX;

  if (kbd_cfg.step < 1)
    kbd_cfg.step = 1;

  if (kbd_cfg.step > (KBD_BACKLIGHT_MAX / 2))
    kbd_cfg.step = KBD_BACKLIGHT_MAX / 2;
}
