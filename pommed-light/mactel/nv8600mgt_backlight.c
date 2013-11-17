/*
 * pommed - Apple laptops hotkeys handler daemon
 *
 * Apple Macbook Pro LCD backlight control, nVidia 8600M GT
 *
 * Copyright (C) 2006 Nicolas Boichat <nicolas @boichat.ch>
 * Copyright (C) 2006 Felipe Alfaro Solana <felipe_alfaro @linuxmail.org>
 * Copyright (C) 2007-2008 Julien BLACHE <jb@jblache.org>
 *  + Adapted for pommed
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *
 * This driver triggers SMIs which cause the firmware to change the
 * backlight brightness. This is icky in many ways, but it's impractical to
 * get at the firmware code in order to figure out what it's actually doing.
 */

#include <stdio.h>
#include <sys/io.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <syslog.h>

#include <errno.h>

#include "../pommed-light.h"
#include "../conffile.h"
#include "../lcd_backlight.h"


struct _lcd_bck_info lcd_bck_info;


static int nv8600mgt_inited = 0;
static unsigned int bl_port;


static unsigned char
nv8600mgt_backlight_get()
{
  unsigned char value;

  if (nv8600mgt_inited == 0)
    return 0;

  outb(0x03, bl_port + 1);
  outb(0xbf, bl_port);

  value = inb(bl_port + 1) >> 4;

  return value;
}

static void
nv8600mgt_backlight_set(unsigned char value)
{
  if (nv8600mgt_inited == 0)
    return;

  outb(0x04 | (value << 4), bl_port + 1);
  outb(0xbf, bl_port);
}


void
nv8600mgt_backlight_step(int dir)
{
  int val;
  int newval;

  if (nv8600mgt_inited == 0)
    return;

  val = nv8600mgt_backlight_get();

  if (dir == STEP_UP)
    {
      newval = val + lcd_nv8600mgt_cfg.step;

      if (newval > NV8600MGT_BACKLIGHT_MAX)
	newval = NV8600MGT_BACKLIGHT_MAX;

      logdebug("LCD stepping +%d -> %d\n", lcd_nv8600mgt_cfg.step, newval);
    }
  else if (dir == STEP_DOWN)
    {
      newval = val - lcd_nv8600mgt_cfg.step;

      if (newval < NV8600MGT_BACKLIGHT_OFF)
	newval = NV8600MGT_BACKLIGHT_OFF;

      logdebug("LCD stepping -%d -> %d\n", lcd_nv8600mgt_cfg.step, newval);
    }
  else
    return;

  nv8600mgt_backlight_set((unsigned char)newval);

  lcd_bck_info.level = newval;
}

void
nv8600mgt_backlight_toggle(int lvl)
{
  int val;

  if (lcd_nv8600mgt_cfg.on_batt == 0)
    return;

  if (nv8600mgt_inited == 0)
    return;

  val = nv8600mgt_backlight_get();
  if (val != lcd_bck_info.level)
    {
      lcd_bck_info.level = val;
    }

  if (lcd_bck_info.level == 0)
    return;

  switch (lvl)
    {
      case LCD_ON_AC_LEVEL:
	if (lcd_bck_info.level >= lcd_bck_info.ac_lvl)
	  break;

	logdebug("LCD switching to AC level\n");

	nv8600mgt_backlight_set(lcd_bck_info.ac_lvl);

	lcd_bck_info.level = lcd_bck_info.ac_lvl;
	break;

      case LCD_ON_BATT_LEVEL:
	if (lcd_bck_info.level <= lcd_nv8600mgt_cfg.on_batt)
	  break;

	logdebug("LCD switching to battery level\n");

	lcd_bck_info.ac_lvl = lcd_bck_info.level;

	nv8600mgt_backlight_set(lcd_nv8600mgt_cfg.on_batt);

	lcd_bck_info.level = lcd_nv8600mgt_cfg.on_batt;
	break;
    }
}


int
nv8600mgt_backlight_probe(void)
{
  int ret;

  /* Determine backlight I/O port */
  switch (mops->type)
    {
      case MACHINE_MACBOOKPRO_3:
      case MACHINE_MACBOOKPRO_4:
	bl_port = 0xb2; /* 0xb2 - 0xb3 */
	break;

      case MACHINE_MACBOOKPRO_5:
      case MACHINE_MACBOOKPRO_6:
      case MACHINE_MACBOOK_5:
      case MACHINE_MACBOOK_6:
      case MACHINE_MACBOOKAIR_2:
      case MACHINE_MACBOOKAIR_3:
	bl_port = 0x52e; /* 0x52e - 0x52f */
	break;

      default:
	logmsg(LOG_ERR, "nv8600mgt LCD backlight support not supported on this hardware");
	return -1;
    }

  lcd_bck_info.max = NV8600MGT_BACKLIGHT_MAX;

  ret = iopl(3);
  if (ret < 0)
    {
      logmsg(LOG_ERR, "iopl() failed: %s", strerror(errno));

      lcd_bck_info.level = 0;

      return -1;
    }

  nv8600mgt_inited = 1;

  /*
   * Set the initial backlight level
   * The value has been sanity checked already
   */
  if (lcd_nv8600mgt_cfg.init > -1)
    {
      nv8600mgt_backlight_set((unsigned char)lcd_nv8600mgt_cfg.init);
    }

  lcd_bck_info.level = nv8600mgt_backlight_get();
  lcd_bck_info.ac_lvl = lcd_bck_info.level;

  return 0;
}


void
nv8600mgt_backlight_fix_config(void)
{
  if (lcd_nv8600mgt_cfg.init < 0)
    lcd_nv8600mgt_cfg.init = -1;

  if (lcd_nv8600mgt_cfg.init > NV8600MGT_BACKLIGHT_MAX)
    lcd_nv8600mgt_cfg.init = NV8600MGT_BACKLIGHT_MAX;

  if (lcd_nv8600mgt_cfg.step < 1)
    lcd_nv8600mgt_cfg.step = 1;

  if (lcd_nv8600mgt_cfg.step > (NV8600MGT_BACKLIGHT_MAX / 4))
    lcd_nv8600mgt_cfg.step = NV8600MGT_BACKLIGHT_MAX / 4;

  if ((lcd_nv8600mgt_cfg.on_batt > NV8600MGT_BACKLIGHT_MAX)
      || (lcd_nv8600mgt_cfg.on_batt < NV8600MGT_BACKLIGHT_OFF))
    lcd_nv8600mgt_cfg.on_batt = 0;
}
