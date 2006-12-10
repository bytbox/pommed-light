/*
 * Macbook Backlight Control
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
 *
 * Turning the backlight off entirely is not supported (as this is supported
 * by the kernel itself).  This utility is only for setting the brightness
 * of the backlight when it is enabled.
 *
 */

/* REMOVE ME
 * The base address of the video card is hard-coded below.  This is the
 * value on my Macbook.  If you need to change this, please write me an
 * email about it and I'll develop an autodetection.
 *
 * Update: several people have mailed about the location on the 2nd
 *         generation model of Macbook being at 0x50380000.
 */

#if 0 /* REMOVE ME */
#ifdef GENERATION_2
#define VIDEO_CARD_ADDRESS    0x50380000
#else
#define VIDEO_CARD_ADDRESS    0x90380000 /* taken from lspci, 512K region */
#endif
#endif /* 0 */


/* REMOVE ME */
#define DEBUG 1

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
#include "lcd_backlight.h"


#define LCD_BCK_STEP            0x0f
#define LCD_BACKLIGHT_MIN       0x1f
static unsigned int LCD_BACKLIGHT_MAX;


static int fd = -1;
static char *memory = NULL;
static long address = 0;
static long length = 0;


#define REGISTER_OFFSET       0x00061254


#if 0 /* REMOVE ME */
#define REGISTER_ADDRESS      (VIDEO_CARD_ADDRESS+REGISTER_OFFSET)
#define PAGE_SIZE             4096
#define PAGE_MASK             (PAGE_SIZE - 1)

#define ACCESS_PAGE           (REGISTER_ADDRESS & ~PAGE_MASK)
#define ACCESS_OFFSET         (REGISTER_ADDRESS & PAGE_MASK)
#define ACCESS_INDEX          (ACCESS_OFFSET >> 2)

#endif /* 0 */



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
  OUTREG(REGISTER_OFFSET, (LCD_BACKLIGHT_MAX << 17) | (value << 1));
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
  unsigned int newval;

  ret = gma950_backlight_map();
  if (ret < 0)
    return;

  val = gma950_backlight_get();

  /* REMOVE ME */
  logdebug("Backlight val: 0x%x\n", val);

  if (dir == STEP_UP)
    {
      newval = val + LCD_BCK_STEP;

      if (newval > LCD_BACKLIGHT_MAX)
	newval = LCD_BACKLIGHT_MAX;

      logdebug("LCD stepping +%d -> %d\n", LCD_BCK_STEP, newval);
    }
  else if (dir == STEP_DOWN)
    {
      newval = val - LCD_BCK_STEP;

      if (newval < LCD_BACKLIGHT_MIN)
	newval = LCD_BACKLIGHT_MIN;

      logdebug("LCD stepping -%d -> %d\n", LCD_BCK_STEP, newval);
    }
  else
    return;

  /* REMOVE ME */
  logdebug("Backlight new: 0x%x\n", newval);

  gma950_backlight_set(newval);

  gma950_backlight_unmap();
}


#define PCI_ID_VENDOR_INTEL      0x8086
#define PCI_ID_PRODUCT_GMA950    0x27A6

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
	  address = dev->base_addr[2];
	  length = dev->size[2];
	}
    }

  pci_cleanup(pacc);

  /* REMOVE ME */
  logdebug("Found Intel GMA950 at 0x%lx, size 0x%lx\n", address, length);

  if (!address)
    {
      logdebug("Failed to detect Intel GMA950, aborting...\n");
      return -1;
    }

  /* Get the maximum backlight value */
  ret = gma950_backlight_map();
  if (ret < 0)
    return -1;

  LCD_BACKLIGHT_MAX = gma950_backlight_get_max();

  gma950_backlight_unmap();

  if (ret < 0)
    {
      logmsg(LOG_ERR, "Could not determine max backlight value");
      return -1;
    }

  /* REMOVE ME */
  logdebug("Max backlight value: 0x%x\n", LCD_BACKLIGHT_MAX);

  return 0;
}
