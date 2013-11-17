/*
 * pommed - Apple laptops hotkeys handler daemon
 *
 * MacBook Backlight Control (Intel GMA950 & GMA965)
 *
 * Copyright (C) 2006-2007 Ryan Lortie <desrt@desrt.ca>
 * Copyright (C) 2006-2007 Julien BLACHE <jb@jblache.org>
 *  + Adapted for pommed
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
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
 * The GMA950 has a backlight control register at offset 0x00061254 in its
 * PCI memory space (512K region):
 *  - bits 0-15 represent the backlight value
 *  - bits 16 indicates legacy mode is in use when set
 *  - bits 17-31 hold the max backlight value << 1
 *
 * Bit 16 indicates whether the backlight control should be used in legacy
 * mode or not. This bit is 0 on MacBooks, indicating native mode should be
 * used. This is the only method supported here.
 *
 *
 * The GMA965 is slightly different; the backlight control register is at
 * offset 0x00061250 in its PCI memory space (first 512K in the 1M region):
 *  - bits 0-15 represent the backlight value
 *  - bits 16-31 hold the max backlight value
 *  - bit 30 indicates legacy mode is in use when set
 *
 *
 * For BOTH cards, the register for the backlight value is at offset 0x00061254.
 *
 *
 * For both cards, in the code below, max value and current value are expressed
 * on 15 bits; the values are shifted as appropriate when appropriate.
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

#include "../pommed-light.h"
#include "../conffile.h"
#include "../lcd_backlight.h"


static unsigned int GMA950_BACKLIGHT_MAX;

static int fd = -1;
static char *memory = NULL;
static char sysfs_resource[64];
static long length = 0;

#define REGISTER_OFFSET           0x00061254

#define GMA950_LEGACY_MODE        (1 << 16)
#define GMA950_CONTROL_REGISTER   0x00061254

#define GMA965_LEGACY_MODE        (1 << 30)
#define GMA965_CONTROL_REGISTER   0x00061250

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


static unsigned int
gma950_backlight_get(void)
{
  return (INREG(REGISTER_OFFSET) >> 1) & 0x7fff;
}

static unsigned int
gma950_backlight_get_max(void)
{
  return (INREG(REGISTER_OFFSET) >> 17);
}

static void
gma950_backlight_set(unsigned int value)
{
  OUTREG(REGISTER_OFFSET, (GMA950_BACKLIGHT_MAX << 17) | (value << 1));
}


static int
gma950_backlight_map(void)
{
  if (length == 0)
    {
      logdebug("No probing done !\n");
      return -1;
    }

  fd = open(sysfs_resource, O_RDWR);
	
  if (fd < 0)
    {
      logmsg(LOG_WARNING, "Cannot open %s: %s", sysfs_resource, strerror(errno));
      return -1;
    }

  memory = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

  if (memory == MAP_FAILED)
    {
      logmsg(LOG_ERR, "mmap failed: %s", strerror(errno));

      close(fd);
      fd = -1;

      return -1;
    }

  return 0;
}

static void
gma950_backlight_unmap(void)
{
  munmap(memory, length);
  memory = NULL;

  close(fd);
  fd = -1;
}


void
gma950_backlight_step(int dir)
{
  int ret;

  unsigned int val;
  unsigned int newval = 0;

  ret = gma950_backlight_map();
  if (ret < 0)
    return;

  val = gma950_backlight_get();

  if (dir == STEP_UP)
    {
      newval = val + lcd_gma950_cfg.step;

      if (newval < GMA950_BACKLIGHT_MIN)
	newval = GMA950_BACKLIGHT_MIN;

      if (newval > GMA950_BACKLIGHT_MAX)
	newval = GMA950_BACKLIGHT_MAX;

      logdebug("LCD stepping +%d -> %d\n", lcd_gma950_cfg.step, newval);
    }
  else if (dir == STEP_DOWN)
    {
      /* val is unsigned */
      if (val > lcd_gma950_cfg.step)
	newval = val - lcd_gma950_cfg.step;

      if (newval < GMA950_BACKLIGHT_MIN)
	newval = 0x00;

      logdebug("LCD stepping -%d -> %d\n", lcd_gma950_cfg.step, newval);
    }
  else
    return;

  gma950_backlight_set(newval);

  gma950_backlight_unmap();

  lcd_bck_info.level = newval;
}


void
gma950_backlight_toggle(int lvl)
{
  int val;
  int ret;

  if (lcd_gma950_cfg.on_batt == 0)
    return;

  ret = gma950_backlight_map();
  if (ret < 0)
    return;

  val = gma950_backlight_get();
  if (val != lcd_bck_info.level)
    {
      lcd_bck_info.level = val;
    }

  if (lcd_bck_info.level == 0)
    {
      gma950_backlight_unmap();
      return;
    }

  switch (lvl)
    {
      case LCD_ON_AC_LEVEL:
	if (lcd_bck_info.level >= lcd_bck_info.ac_lvl)
	  break;

	logdebug("LCD switching to AC level\n");

	gma950_backlight_set(lcd_bck_info.ac_lvl);

	lcd_bck_info.level = lcd_bck_info.ac_lvl;
	break;

      case LCD_ON_BATT_LEVEL:
	if (lcd_bck_info.level <= lcd_gma950_cfg.on_batt)
	  break;

	logdebug("LCD switching to battery level\n");

	lcd_bck_info.ac_lvl = lcd_bck_info.level;

	gma950_backlight_set(lcd_gma950_cfg.on_batt);

	lcd_bck_info.level = lcd_gma950_cfg.on_batt;
	break;
    }

  gma950_backlight_unmap();
}


/*
 * We are hardware-dependent for GMA950_BACKLIGHT_MAX,
 * so here _fix_config() is static and called at probe time.
 */
static void
gma950_backlight_fix_config(void)
{
  if (lcd_gma950_cfg.init < 0)
    lcd_gma950_cfg.init = -1;

  if (lcd_gma950_cfg.init > GMA950_BACKLIGHT_MAX)
    lcd_gma950_cfg.init = GMA950_BACKLIGHT_MAX;

  if ((lcd_gma950_cfg.init < GMA950_BACKLIGHT_MIN)
      && (lcd_gma950_cfg.init > 0))
    lcd_gma950_cfg.init = 0x00;

  if (lcd_gma950_cfg.step < 1)
    lcd_gma950_cfg.step = 1;

  if (lcd_gma950_cfg.step > 0x20)
    lcd_gma950_cfg.step = 0x20;

  if ((lcd_gma950_cfg.on_batt > GMA950_BACKLIGHT_MAX)
      || (lcd_gma950_cfg.on_batt < GMA950_BACKLIGHT_MIN))
    lcd_gma950_cfg.on_batt = 0;
}


#define PCI_ID_VENDOR_INTEL      0x8086
#define PCI_ID_PRODUCT_GMA950    0x27A2
#define PCI_ID_PRODUCT_GMA965    0x2A02

/* Look for an Intel GMA950 or GMA965 */
int
gma950_backlight_probe(void)
{
  struct pci_access *pacc;
  struct pci_dev *dev;
  struct stat stbuf;

  int card;
  int ret;

  pacc = pci_alloc();
  if (pacc == NULL)
    {
      logmsg(LOG_ERR, "Could not allocate PCI structs");
      return -1;
    }

  pci_init(pacc);
  pci_scan_bus(pacc);

  card = 0;
  /* Iterate over all devices */
  for(dev = pacc->devices; dev; dev = dev->next)
    {
      pci_fill_info(dev, PCI_FILL_IDENT);
      /* GMA950 or GMA965 */
      if ((dev->vendor_id == PCI_ID_VENDOR_INTEL)
	  && ((dev->device_id == PCI_ID_PRODUCT_GMA950)
	      || (dev->device_id == PCI_ID_PRODUCT_GMA965)))
	{
	  card = dev->device_id;

	  ret = snprintf(sysfs_resource, sizeof(sysfs_resource),
			 "/sys/bus/pci/devices/%04x:%02x:%02x.%1x/resource0",
			 dev->domain, dev->bus, dev->dev, dev->func);

	  break;
	}
    }

  pci_cleanup(pacc);

  if (!dev)
    {
      logdebug("Failed to detect Intel GMA950 or GMA965, aborting...\n");
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

  logdebug("GMA950/GMA965 PCI resource: [%s], length %ldK\n", sysfs_resource, (length / 1024));

  ret = gma950_backlight_map();
  if (ret < 0)
    {
      logmsg(LOG_ERR, "Could not map GMA950/GMA965 memory");
      return -1;
    }

  if (card == PCI_ID_PRODUCT_GMA950)
    {
      if (INREG(GMA950_CONTROL_REGISTER) & GMA950_LEGACY_MODE)
	{
	  logdebug("GMA950 is in legacy backlight control mode, unsupported\n");

	  gma950_backlight_unmap();
	  return -1;
	}
    }
  else if (card == PCI_ID_PRODUCT_GMA965)
    {
      if (INREG(GMA965_CONTROL_REGISTER) & GMA965_LEGACY_MODE)
	{
	  logdebug("GMA965 is in legacy backlight control mode, unsupported\n");

	  gma950_backlight_unmap();
	  return -1;
	}
    }

  /* Get the maximum backlight value */
  GMA950_BACKLIGHT_MAX = gma950_backlight_get_max();

  logdebug("Max LCD backlight value is 0x%x\n", GMA950_BACKLIGHT_MAX);

  /* Now, check the config and fix it if needed */
  gma950_backlight_fix_config();

  /* Set the initial backlight level */
  if (lcd_gma950_cfg.init > -1)
    gma950_backlight_set(lcd_gma950_cfg.init);

  lcd_bck_info.max = GMA950_BACKLIGHT_MAX;
  lcd_bck_info.level = gma950_backlight_get();
  lcd_bck_info.ac_lvl = lcd_bck_info.level;

  gma950_backlight_unmap();

  return 0;
}
