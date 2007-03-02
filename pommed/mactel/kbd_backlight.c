/*
 * pommed - Apple laptops hotkeys handler daemon
 *
 * $Id$
 *
 * Copyright (C) 2006-2007 Julien BLACHE <jb@jblache.org>
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

#include "../pommed.h"
#include "../conffile.h"
#include "../kbd_backlight.h"
#include "../ambient.h"
#include "../dbus.h"


struct _kbd_bck_info kbd_bck_info;


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
kbd_backlight_set(int val, int who)
{
  int curval;

  FILE *fp;

  if (kbd_bck_info.inhibit)
    return;

  curval = kbd_backlight_get();

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

  mbpdbus_send_kbd_backlight(val, curval, who);

  kbd_bck_info.level = val;
}

void
kbd_backlight_step(int dir)
{
  int val;
  int newval;

  if (kbd_bck_info.inhibit)
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


void
kbd_backlight_inhibit_set(int mask)
{
  if (!kbd_bck_info.inhibit)
    kbd_bck_info.inhibit_lvl = kbd_bck_info.level;

  kbd_backlight_set(KBD_BACKLIGHT_OFF,
		    (mask == KBD_INHIBIT_LID) ? (KBD_AUTO) : (KBD_USER));

  kbd_bck_info.inhibit |= mask;

  logdebug("KBD: inhibit set 0x%02x -> 0x%02x\n", mask, kbd_bck_info.inhibit);
}

void
kbd_backlight_inhibit_clear(int mask)
{
  kbd_bck_info.inhibit &= ~mask;

  logdebug("KBD: inhibit clear 0x%02x -> 0x%02x\n", mask, kbd_bck_info.inhibit);

  if (kbd_bck_info.inhibit)
    return;

  if (kbd_bck_info.auto_on)
    {
      kbd_bck_info.auto_on = 0;
      kbd_bck_info.inhibit_lvl = 0;
    }

  kbd_backlight_set(kbd_bck_info.inhibit_lvl,
		    (mask == KBD_INHIBIT_LID) ? (KBD_AUTO) : (KBD_USER));
}

void
kbd_backlight_inhibit_toggle(int mask)
{
  if (kbd_bck_info.inhibit & mask)
    kbd_backlight_inhibit_clear(mask);
  else
    kbd_backlight_inhibit_set(mask);
}


void
kbd_backlight_init(void)
{
  if (kbd_cfg.auto_on)
    kbd_bck_info.inhibit = 0;
  else
    kbd_bck_info.inhibit = KBD_INHIBIT_USER;

  kbd_bck_info.inhibit_lvl = 0;

  kbd_bck_info.auto_on = 0;

  if (!has_kbd_backlight())
    {
      kbd_bck_info.r_sens = 0;
      kbd_bck_info.l_sens = 0;

      kbd_bck_info.level = 0;

      ambient_info.left = 0;
      ambient_info.right = 0;
      ambient_info.max = 0;

      return;
    }

  kbd_bck_info.level = kbd_backlight_get();
  if (kbd_bck_info.level < 0)
    kbd_bck_info.level = 0;

  kbd_bck_info.max = KBD_BACKLIGHT_MAX;

  ambient_init(&kbd_bck_info.r_sens, &kbd_bck_info.l_sens);
}

void
kbd_backlight_ambient_check(void)
{
  int amb_r, amb_l;

  ambient_get(&amb_r, &amb_l);

  if ((amb_r < 0) || (amb_l < 0))
    return;

  mbpdbus_send_ambient_light(amb_l, kbd_bck_info.l_sens, amb_r, kbd_bck_info.r_sens);

  kbd_bck_info.r_sens = amb_r;
  kbd_bck_info.l_sens = amb_l;

  /* Inhibited */
  if (kbd_bck_info.inhibit)
    return;

  if ((amb_r < kbd_cfg.on_thresh) && (amb_l < kbd_cfg.on_thresh))
    {
      logdebug("Ambient light lower threshold reached\n");

      /* backlight already on */
      if (kbd_backlight_get() > KBD_BACKLIGHT_OFF)
	return;

      /* turn on backlight */
      kbd_bck_info.auto_on = 1;

      kbd_backlight_set(kbd_cfg.auto_lvl, KBD_AUTO);
    }
  else if (kbd_bck_info.auto_on)
    {
      if ((amb_r > kbd_cfg.off_thresh) || (amb_l > kbd_cfg.off_thresh))
	{
	  logdebug("Ambient light upper threshold reached\n");

	  kbd_bck_info.auto_on = 0;

	  kbd_backlight_set(KBD_BACKLIGHT_OFF, KBD_AUTO);
	}
    }
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
