/*
 * pommed - Apple laptops hotkeys handler daemon
 *
 * Apple Macbook Pro LCD backlight control
 *
 * Copyright (C) 2006 Nicolas Boichat <nicolas@boichat.ch>
 * Copyright (C) 2006-2007 Julien BLACHE <jb@jblache.org>
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

static int fd = -1;
static char *memory = NULL;
static char sysfs_resource[64];
static long length = 0;

static inline unsigned int
readl(const volatile void *addr)
{
  return *(volatile unsigned int*) addr;
}

static inline void
writel(unsigned int b, volatile void *addr)
{
  *(volatile unsigned int*) addr = b;
}

#define INREG(addr)		readl(memory+addr)
#define OUTREG(addr,val)	writel(val, memory+addr)


static unsigned char
x1600_backlight_get()
{
  return INREG(0x7af8) >> 8;
}

static void
x1600_backlight_set(unsigned char value)
{
  OUTREG(0x7af8, 0x00000001 | ((unsigned int)value << 8));
}


static int
x1600_backlight_map(void)
{
  unsigned int state;

  if (length == 0)
    {
      logdebug("No probing done!\n");
      return -1;
    }

  fd = open(sysfs_resource, O_RDWR);
	
  if (fd < 0)
    {
      logmsg(LOG_WARNING, "Cannot open %s: %s", sysfs_resource, strerror(errno));

      close(fd);
      fd = -1;

      return -1;
    }

  memory = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

  if (memory == MAP_FAILED)
    {
      logmsg(LOG_ERR, "mmap failed: %s", strerror(errno));
      return -1;
    }

  /* Is it really necessary ? */
  OUTREG(0x4dc, 0x00000005);
  state = INREG(0x7ae4);
  OUTREG(0x7ae4, state);

  return 0;
}

static void
x1600_backlight_unmap(void)
{
  munmap(memory, length);
  memory = NULL;

  close(fd);
  fd = -1;
}


void
x1600_backlight_step(int dir)
{
  int ret;

  int val;
  int newval;

  ret = x1600_backlight_map();
  if (ret < 0)
    return;

  val = x1600_backlight_get();

  if (dir == STEP_UP)
    {
      newval = val + lcd_x1600_cfg.step;

      if (newval > X1600_BACKLIGHT_MAX)
	newval = X1600_BACKLIGHT_MAX;

      logdebug("LCD stepping +%d -> %d\n", lcd_x1600_cfg.step, newval);
    }
  else if (dir == STEP_DOWN)
    {
      newval = val - lcd_x1600_cfg.step;

      if (newval < X1600_BACKLIGHT_OFF)
	newval = X1600_BACKLIGHT_OFF;

      logdebug("LCD stepping -%d -> %d\n", lcd_x1600_cfg.step, newval);
    }
  else
    return;

  x1600_backlight_set((unsigned char)newval);

  x1600_backlight_unmap();

  mbpdbus_send_lcd_backlight(newval, val, LCD_USER);

  lcd_bck_info.level = newval;
}

void
x1600_backlight_toggle(int lvl)
{
  int val;
  int ret;

  if (lcd_x1600_cfg.on_batt == 0)
    return;

  ret = x1600_backlight_map();
  if (ret < 0)
    return;

  val = x1600_backlight_get();
  if (val != lcd_bck_info.level)
    {
      mbpdbus_send_lcd_backlight(val, lcd_bck_info.level, LCD_AUTO);
      lcd_bck_info.level = val;
    }

  if (lcd_bck_info.level == 0)
    {
      x1600_backlight_unmap();
      return;
    }

  switch (lvl)
    {
      case LCD_ON_AC_LEVEL:
	if (lcd_bck_info.level >= lcd_bck_info.ac_lvl)
	  break;

	logdebug("LCD switching to AC level\n");

	x1600_backlight_set(lcd_bck_info.ac_lvl);

	mbpdbus_send_lcd_backlight(lcd_bck_info.ac_lvl, lcd_bck_info.level, LCD_AUTO);

	lcd_bck_info.level = lcd_bck_info.ac_lvl;
	break;

      case LCD_ON_BATT_LEVEL:
	if (lcd_bck_info.level <= lcd_x1600_cfg.on_batt)
	  break;

	logdebug("LCD switching to battery level\n");

	lcd_bck_info.ac_lvl = lcd_bck_info.level;

	x1600_backlight_set(lcd_x1600_cfg.on_batt);

	mbpdbus_send_lcd_backlight(lcd_x1600_cfg.on_batt, lcd_bck_info.level, LCD_AUTO);

	lcd_bck_info.level = lcd_x1600_cfg.on_batt;
	break;
    }

  x1600_backlight_unmap();
}


#define PCI_ID_VENDOR_ATI        0x1002
#define PCI_ID_PRODUCT_X1600     0x71c5

/* Look for an ATI Radeon Mobility X1600 */
int
x1600_backlight_probe(void)
{
  struct pci_access *pacc;
  struct pci_dev *dev;
  struct stat stbuf;

  int ret;

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
      /* ATI X1600 */
      if ((dev->vendor_id == PCI_ID_VENDOR_ATI)
	  && (dev->device_id == PCI_ID_PRODUCT_X1600))
	{
	  ret = snprintf(sysfs_resource, sizeof(sysfs_resource),
			 "/sys/bus/pci/devices/%04x:%02x:%02x.%1x/resource2",
			 dev->domain, dev->bus, dev->dev, dev->func);

	  break;
	}
    }

  pci_cleanup(pacc);

  if (!dev)
    {
      logdebug("Failed to detect ATI X1600, aborting...\n");
      return -1;
    }

  /* Check snprintf() return value */
  if (ret >= sizeof(sysfs_resource))
    {
      logmsg(LOG_ERR, "Could not build sysfs PCI resource path");
      return -1;
    }

  ret = stat(sysfs_resource, &stbuf);
  if (ret < 0)
    {
      logmsg(LOG_ERR, "Could not determine PCI resource length: %s", strerror(errno));
      return -1;
    }

  length = stbuf.st_size;

  logdebug("ATI X1600 PCI resource: [%s], length %ldK\n", sysfs_resource, (length / 1024));

  lcd_bck_info.max = X1600_BACKLIGHT_MAX;

  ret = x1600_backlight_map();
  if (ret < 0)
    {
      lcd_bck_info.level = 0;

      return 0;
    }

  /*
   * Set the initial backlight level
   * The value has been sanity checked already
   */
  if (lcd_x1600_cfg.init > -1)
    {
      x1600_backlight_set((unsigned char)lcd_x1600_cfg.init);
    }

  lcd_bck_info.level = x1600_backlight_get();
  lcd_bck_info.ac_lvl = lcd_bck_info.level;

  x1600_backlight_unmap();

  return 0;
}


void
x1600_backlight_fix_config(void)
{
  if (lcd_x1600_cfg.init < 0)
    lcd_x1600_cfg.init = -1;

  if (lcd_x1600_cfg.init > X1600_BACKLIGHT_MAX)
    lcd_x1600_cfg.init = X1600_BACKLIGHT_MAX;

  if (lcd_x1600_cfg.step < 1)
    lcd_x1600_cfg.step = 1;

  if (lcd_x1600_cfg.step > (X1600_BACKLIGHT_MAX / 2))
    lcd_x1600_cfg.step = X1600_BACKLIGHT_MAX / 2;

  if ((lcd_x1600_cfg.on_batt > X1600_BACKLIGHT_MAX)
      || (lcd_x1600_cfg.on_batt < X1600_BACKLIGHT_OFF))
    lcd_x1600_cfg.on_batt = 0;
}
