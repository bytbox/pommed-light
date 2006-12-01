/*
 * Apple Macbook Pro LCD backlight control
 *
 * $Id$
 *
 * Copyright (C) 2006 Nicolas Boichat <nicolas@boichat.ch>
 * Copyright (C) 2006 Julien BLACHE <jb@jblache.org>
 *  + Adapted for mbpeventd
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

#include "mbpeventd.h"
#include "lcd_backlight.h"


static int fd = -1;
static char *memory = NULL;
static long address = 0;
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
      newval = val + LCD_BCK_STEP;

      if (newval > LCD_BACKLIGHT_MAX)
	newval = LCD_BACKLIGHT_MAX;

      logdebug("LCD stepping +%d -> %d\n", LCD_BCK_STEP, newval);
    }
  else if (dir == STEP_DOWN)
    {
      newval = val - LCD_BCK_STEP;

      if (newval < LCD_BACKLIGHT_OFF)
	newval = LCD_BACKLIGHT_OFF;

      logdebug("LCD stepping -%d -> %d\n", LCD_BCK_STEP, newval);
    }
  else
    return;

  x1600_backlight_set((unsigned char)newval);

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
      /* ATI X1600 */
      if ((dev->vendor_id == PCI_ID_VENDOR_ATI)
	  && (dev->device_id == PCI_ID_PRODUCT_X1600))
	{
	  address = dev->base_addr[2];
	  length = dev->size[2];
	}
    }

  pci_cleanup(pacc);

  if (!address)
    {
      logdebug("Failed to detect ATI X1600, aborting...\n");
      return -1;
    }

  return 0;
}
