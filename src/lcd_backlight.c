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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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


unsigned char
lcd_backlight_get()
{
  return INREG(0x7af8) >> 8;
}

void
lcd_backlight_set(unsigned char value)
{
  OUTREG(0x7af8, 0x00000001 | ((unsigned int)value << 8));
}


int
lcd_backlight_map(void)
{
  unsigned int state;

  if ((address == 0) || (length == 0))
    {
      debug("No probing done !\n");
      return -1;
    }

  fd = open("/dev/mem", O_RDWR);
	
  if (fd < 0)
    {
      debug("cannot open /dev/mem: %s\n", strerror(errno));
      return -1;
    }

  memory = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, address);

  if (memory == MAP_FAILED)
    {
      debug("mmap failed: %s\n", strerror(errno));
      return -1;
    }

  /* Is it really necessary ? */
  OUTREG(0x4dc, 0x00000005);
  state = INREG(0x7ae4);
  OUTREG(0x7ae4, state);

  return 0;
}

void
lcd_backlight_unmap(void)
{
  munmap(memory, length);
  memory = NULL;

  close(fd);
  fd = -1;
}


void
lcd_backlight_step(int dir)
{
  int ret;

  int val;
  int newval;

  ret = lcd_backlight_map();
  if (ret < 0)
    return;

  val = lcd_backlight_get();

  if (dir == STEP_UP)
    {
      newval = val + LCD_BCK_STEP;

      if (newval > LCD_BACKLIGHT_MAX)
	newval = LCD_BACKLIGHT_MAX;

      debug("LCD stepping +%d -> %d\n", LCD_BCK_STEP, newval);
    }
  else if (dir == STEP_DOWN)
    {
      newval = val - LCD_BCK_STEP;

      if (newval < LCD_BACKLIGHT_OFF)
	newval = LCD_BACKLIGHT_OFF;

      debug("LCD stepping -%d -> %d\n", LCD_BCK_STEP, newval);
    }
  else
    return;

  lcd_backlight_set((unsigned char)newval);

  lcd_backlight_unmap();
}


#define PCI_ID_VENDOR_ATI        0x1002
#define PCI_ID_PRODUCT_X1600     0x71c5

/* Look for an ATI Radeon Mobility X1600 */
int
lcd_backlight_probe_X1600(void)
{
  struct pci_access *pacc;
  struct pci_dev *dev;

  pacc = pci_alloc();
  if (pacc == NULL)
    return -1;

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
      debug("Failed to detect ATI X1600, aborting...\n");
      return -1;
    }

  return 0;
}
