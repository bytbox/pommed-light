/*
 * pommed - Apple laptops hotkeys handler daemon
 *
 * Apple Macbook Pro LCD backlight control, nVidia 8600M GT
 *
 * $Id$
 *
 * Copyright (C) 2006 Nicolas Boichat <nicolas @boichat.ch>
 * Copyright (C) 2006 Felipe Alfaro Solana <felipe_alfaro @linuxmail.org>
 * Copyright (C) 2007 Julien BLACHE <jb@jblache.org>
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

#include <pci/pci.h>

#include "../pommed.h"
#include "../conffile.h"
#include "../lcd_backlight.h"
#include "../dbus.h"


struct _lcd_bck_info lcd_bck_info;


static int nv8600mgt_inited = 0;


static unsigned char
nv8600mgt_backlight_get()
{
  unsigned char value;

  if (nv8600mgt_inited == 0)
    return 0;

  outb(0x03, 0xB3);
  outb(0xBF, 0xB2);

  value = inb(0xB3) >> 4;

  return value;
}

static void
nv8600mgt_backlight_set(unsigned char value)
{
  if (nv8600mgt_inited == 0)
    return;

  outb(0x04 | (value << 4), 0xB3);
  outb(0xBF, 0xB2);
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

  mbpdbus_send_lcd_backlight(newval, val, LCD_USER);

  lcd_bck_info.level = newval;
}

void
nv8600mgt_backlight_toggle(int lvl)
{
  if (lcd_nv8600mgt_cfg.on_batt == 0)
    return;

  if (nv8600mgt_inited == 0)
    return;

  switch (lvl)
    {
      case LCD_ON_AC_LEVEL:
	logdebug("LCD switching to AC level\n");

	nv8600mgt_backlight_set(lcd_bck_info.ac_lvl);

	mbpdbus_send_lcd_backlight(lcd_bck_info.ac_lvl, lcd_bck_info.level, LCD_AUTO);

	lcd_bck_info.level = lcd_bck_info.ac_lvl;
	break;

      case LCD_ON_BATT_LEVEL:
	logdebug("LCD switching to battery level\n");

	lcd_bck_info.ac_lvl = lcd_bck_info.level;

	if (lcd_bck_info.level > lcd_nv8600mgt_cfg.on_batt)
	  {
	    nv8600mgt_backlight_set(lcd_nv8600mgt_cfg.on_batt);
	    lcd_bck_info.level = lcd_nv8600mgt_cfg.on_batt;

	    mbpdbus_send_lcd_backlight(lcd_bck_info.level, lcd_bck_info.ac_lvl, LCD_AUTO);
	  }
	break;
    }
}


#define PCI_ID_VENDOR_NVIDIA     0x10de
#define PCI_ID_PRODUCT_8600MGT   0x0407

/* Look for an nVidia GeForce 8600M GT */
int
nv8600mgt_backlight_probe(void)
{
  struct pci_access *pacc;
  struct pci_dev *dev;
  int nv_found = 0;

  pacc = pci_alloc();
  if (pacc == NULL)
    {
      logmsg(LOG_ERR, "Could not allocate PCI structs");
      return -1;
    }

  pci_init(pacc);
  pci_scan_bus(pacc);

  /* Iterate over all devices */
  for(dev = pacc->devices; dev; dev = dev->next)
    {
      pci_fill_info(dev, PCI_FILL_IDENT);
      /* nVidia GeForce 8600M GT */
      if ((dev->vendor_id == PCI_ID_VENDOR_NVIDIA)
	  && (dev->device_id == PCI_ID_PRODUCT_8600MGT))
	{
	  nv_found = 1;

	  break;
	}
    }

  pci_cleanup(pacc);

  if (!nv_found)
    {
      logdebug("Failed to detect nVidia GeForce 8600M GT, aborting...\n");
      return -1;
    }

  lcd_bck_info.max = NV8600MGT_BACKLIGHT_MAX;

  if (ioperm(0xB2, 0xB3, 1) < 0)
    {
      logmsg(LOG_ERR, "ioperm() failed: %s", strerror(errno));

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
