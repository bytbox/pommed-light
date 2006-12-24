/*
 * MacBook Backlight Control (Intel GMA950)
 *
 * $Id$
 *
 * Copyright (C) 2006 Ryan Lortie <desrt@desrt.ca>
 * Copyright (C) 2006 Julien BLACHE <jb@jblache.org>
 *  + Adapted for mbpeventd
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
 * This program was written after I reverse engineered the
 * AppleIntelIntegratedFramebuffer.kext kernel extension in Mac OS X and
 * played with the register at the memory location I found therein.
 *
 * From my experiments, the register appears to have two halves.
 *
 * yyyyyyyyyyyyyyy0xxxxxxxxxxxxxxx0
 *
 * The top (y) bits appear to be the maximum brightness level and the
 * bottom (x) bits are the current brightness level.  0s are always 0.
 * The brightness level is, therefore, x/y.
 *
 * As my Macbook boots, y is set to 0x94 and x is set to 0x1f.  Going below
 * 0x1f produces odd results.  For example, if you come from above, the
 * backlight will completely turn off at 0x12 (18).  Coming from below,
 * however, you need to get to 0x15 (21) before the backlight comes back on.
 *
 * Since there is no clear cut boundry, I assume that this value specifies
 * a raw voltage.  Also, it appears that the bootup value of 0x1f corresponds
 * to the lowest level that Mac OS X will set the backlight I choose this
 * value as a minimum.
 *
 * For the maximum I do not let the value exceed the value in the upper 15
 * bits.
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

#include "mbpeventd.h"
#include "conffile.h"
#include "lcd_backlight.h"
#include "dbus.h"


static unsigned int GMA950_BACKLIGHT_MAX;


static int fd = -1;
static char *memory = NULL;
static long address = 0;
static long length = 0;


#define REGISTER_OFFSET       0x00061254


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
  if ((address == 0) || (length == 0))
    {
      logdebug("No probing done !\n");
      return -1;
    }

  fd = open("/dev/mem", O_RDWR);
	
  if (fd < 0)
    {
      logmsg(LOG_WARNING, "Cannot open /dev/mem: %s", strerror(errno));
      return -1;
    }

  memory = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, address);

  if (memory == MAP_FAILED)
    {
      logmsg(LOG_ERR, "mmap failed: %s", strerror(errno));
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

  mbpdbus_send_lcd_backlight(newval, val);

  lcd_bck_info.level = newval;
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
}


#define PCI_ID_VENDOR_INTEL      0x8086
#define PCI_ID_PRODUCT_GMA950    0x27A2

/* Look for an Intel GMA950 */
int
gma950_backlight_probe(void)
{
  struct pci_access *pacc;
  struct pci_dev *dev;

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
      pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_BASES);
      /* GMA950 */
      if ((dev->vendor_id == PCI_ID_VENDOR_INTEL)
	  && (dev->device_id == PCI_ID_PRODUCT_GMA950))
	{
	  address = dev->base_addr[0];
	  length = dev->size[0];
	}
    }

  pci_cleanup(pacc);

  if (!address)
    {
      logdebug("Failed to detect Intel GMA950, aborting...\n");
      return -1;
    }

  /* Get the maximum backlight value */
  ret = gma950_backlight_map();
  if (ret < 0)
    {
      logmsg(LOG_ERR, "Could not determine max backlight value");
      return -1;
    }

  GMA950_BACKLIGHT_MAX = gma950_backlight_get_max();

  /* Now, check the config and fix it if needed */
  gma950_backlight_fix_config();

  /* Set the initial backlight level */
  if (lcd_gma950_cfg.init > -1)
    gma950_backlight_set(lcd_gma950_cfg.init);

  lcd_bck_info.max = GMA950_BACKLIGHT_MAX;
  lcd_bck_info.level = gma950_backlight_get();

  gma950_backlight_unmap();

  return 0;
}
