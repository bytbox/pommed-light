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

#include <syslog.h>

#include <errno.h>

#include "mbpeventd.h"
#include "conffile.h"
#include "kbd_backlight.h"
#include "ambient.h"


static struct
{
  int auto_on;  /* automatic */
  int off;      /* turned off ? */
  int value;    /* previous value */
  int r_sens;   /* right sensor */
  int l_sens;   /* left sensor */
} kbd_bck_status;


int
has_kbd_backlight(void)
{
  return ((mops->type == MACHINE_MACBOOKPRO_1)
	  || (mops->type == MACHINE_MACBOOKPRO_2));
}


static int
kbd_backlight_get(void)
{
  int fd;
  int ret;
  char buf[8];

  fd = open(KBD_BACKLIGHT, O_RDONLY);
  if (fd < 0)
    {
      logmsg(LOG_WARNING, "Could not open %s: %s", KBD_BACKLIGHT, strerror(errno));
      return -1;
    }

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
kbd_backlight_set(int val)
{
  int curval;

  FILE *fp;

  curval = kbd_backlight_get();

  /* automatic backlight toggle by user */
  if ((val == KBD_BACKLIGHT_OFF) && (kbd_bck_status.auto_on))
    {
      if (!kbd_bck_status.off)
	{
	  kbd_bck_status.off = 1;
	  kbd_bck_status.value = curval;
	}
      else
	{
	  kbd_bck_status.off = 0;
	  val = kbd_bck_status.value;
	}
    }

  /* backlight turned on again by user */
  if ((val > KBD_BACKLIGHT_OFF)
      && (kbd_bck_status.auto_on) && (kbd_bck_status.off))
    kbd_bck_status.off = 0;

  if (val == curval)
    return;

  if ((val < KBD_BACKLIGHT_OFF) || (val > KBD_BACKLIGHT_MAX))
    return;

  fp = fopen(KBD_BACKLIGHT, "a");
  if (fp == NULL)
    {
      logmsg(LOG_WARNING, "Could not open %s: %s", KBD_BACKLIGHT, strerror(errno));
      return;
    }

  fprintf(fp, "%d", val);

  fclose(fp);

  logdebug("KBD backlight value set to %d\n", val);
}

void
kbd_backlight_off(void)
{
  kbd_backlight_set(KBD_BACKLIGHT_OFF);
}

void
kbd_backlight_step(int dir)
{
  int val;
  int newval;

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

  kbd_backlight_set(newval);
}

void
kbd_backlight_init(void)
{
  kbd_bck_status.auto_on = 0;
  kbd_bck_status.off = 0;

  if (!has_kbd_backlight())
    {
      kbd_bck_status.value = 0;
      kbd_bck_status.r_sens = 0;
      kbd_bck_status.l_sens = 0;

      return;
    }

  kbd_bck_status.value = kbd_backlight_get();
  if (kbd_bck_status.value < 0)
    kbd_bck_status.value = 0;

  ambient_get(&kbd_bck_status.r_sens, &kbd_bck_status.l_sens);
}

void
kbd_backlight_ambient_check(void)
{
  int amb_r, amb_l;

  ambient_get(&amb_r, &amb_l);

  if ((amb_r < 0) || (amb_l < 0))
    return;

  if ((amb_r < kbd_cfg.on_thresh) && (amb_l < kbd_cfg.on_thresh))
    {
      logdebug("Ambient light lower threshold reached\n");

      /* backlight turned on automatically, then disabled by user */
      if (kbd_bck_status.auto_on && kbd_bck_status.off)
	return;

      /* backlight already on */
      if (kbd_backlight_get() > KBD_BACKLIGHT_OFF)
	return;

      /* turn on backlight */
      kbd_bck_status.auto_on = 1;
      kbd_bck_status.off = 0;

      kbd_backlight_set(kbd_cfg.auto_lvl);
    }
  else if (kbd_bck_status.auto_on)
    {
      if ((amb_r > kbd_cfg.off_thresh) || (amb_l > kbd_cfg.off_thresh))
	{
	  logdebug("Ambient light upper threshold reached\n");

	  kbd_bck_status.auto_on = 0;
	  kbd_bck_status.off = 0;

	  kbd_backlight_set(KBD_BACKLIGHT_OFF);
	}
    }

  kbd_bck_status.r_sens = amb_r;
  kbd_bck_status.l_sens = amb_l;
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
