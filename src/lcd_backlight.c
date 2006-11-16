/*
 * Apple Macbook Pro LCD backlight control
 *
 * $Id$
 *
 * Copyright (C) 2006 Nicolas Boichat <nicolas@boichat.ch>
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

#include <pci/pci.h>

#include "mbpeventd.h"


static char *memory;


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


void
lcd_backlight_step(int dir)
{
  int val;
  int newval;

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

  lcd_backlight_set((unsigned char)val);
}

#define PCI_ID_VENDOR_ATI        0x1002
#defien PCI_ID_PRODUCT_X1600     0x71c5

/* Look for an ATI Radeon Mobility X1600 */
int
lcd_backlight_probe_X1600(void)
{
  char* endptr;
  int ret = 0;
  long address = 0;
  long length = 0;
  int fd;
  int state;

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
      printf("Failed to detect ATI X1600, aborting...\n");
      return 1;
    }

  fd = open("/dev/mem", O_RDWR);
	
  if (fd < 0) {
    perror("cannot open /dev/mem");
    return 1;
  }

  memory = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, address);

  if (memory == MAP_FAILED)
    {
      perror("mmap failed");
      return 1;
    }

  /* Is it really necessary ? */
  OUTREG(0x4dc, 0x00000005);
  state = INREG(0x7ae4);
  OUTREG(0x7ae4, state);

  if (argc == 2)
    {
      if (argv[1][0] == '+')
	{
	  long value = strtol(&argv[1][1], &endptr, 10);
	  if ((value < 0) || (value > 255) || (endptr[0]))
	    {
	      printf("Invalid value \"%s\" (should be an integer between 0 and 255).\n", &argv[1][1]);
	      usage(argv[0]);
	      ret = 1;
	    }
	  else
	    {
	      value = read_backlight()+value;
	      if (value > 255)
		value = 255;
	      write_backlight(value);
	    }
	}
      else if (argv[1][0] == '-')
	{
	  long value = strtol(&argv[1][1], &endptr, 10);
	  if ((value < 0) || (value > 255) || (endptr[0]))
	    {
	      printf("Invalid value \"%s\" (should be an integer between 0 and 255).\n", &argv[1][1]);
	      usage(argv[0]);
	      ret = 1;
	    }
	  else
	    {
	      value = read_backlight()-value;
	      if (value < 0)
		value = 0;
	      write_backlight(value);
	    }
	}
      else
	{
	  long value = strtol(argv[1], &endptr, 10);
	  if ((value < 0) || (value > 255) || (endptr[0]))
	    {
	      printf("Invalid value \"%s\" (should be an integer between 0 and 255).\n", argv[1]);
	      usage(argv[0]);
	      ret = 1;
	    }
	  else
	    {
	      write_backlight(value);
	    }
	}
    }
  else
    {
      printf("%d\n", read_backlight());
    }

  close(fd);

  return ret;
}

void
lcd_backlight_release(void)
{
  munmap(memory, length);
}
