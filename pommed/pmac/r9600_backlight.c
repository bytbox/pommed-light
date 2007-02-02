/*
 * pommed - Apple laptops hotkeys handler daemon
 *
 * $Id$
 *
 * Copyright (C) 2006 Julien BLACHE <jb@jblache.org>
 * Copyright (C) 2006 Yves-Alexis Perez <corsac@corsac.net>
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
#include <syslog.h>
#include <pci/pci.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "../pommed.h"
#include "../conffile.h"
#include "../lcd_backlight.h"
#include "../dbus.h"


struct _lcd_bck_info lcd_bck_info;


#define SYSFS_BACKLIGHT "/sys/class/backlight/radeonbl0"

static int probed = 0;


static int
r9600_backlight_get(void)
{
  int fd;
  int n;
  char buffer[4];

  if (!probed)
    return 0;

  fd = open(SYSFS_BACKLIGHT "/actual_brightness", O_RDONLY);
  if (fd < 0)
    {
      logmsg(LOG_WARNING, "Could not open sysfs actual_brightness node: %s", strerror(errno));

      return 0;
    }

  n = read(fd, buffer, sizeof(buffer) -1);
  if (n < 1)
    {
      logmsg(LOG_WARNING, "Could not read sysfs actual_brightness node");

      close(fd);
      return 0;
    }
  close(fd);

  return atoi(buffer);
}

static void
r9600_backlight_set(int value)
{
  FILE *fp;

  fp = fopen(SYSFS_BACKLIGHT "/brightness", "a");
  if (fp == NULL)
    {
      logmsg(LOG_WARNING, "Could not open sysfs brightness node: %s", strerror(errno));

      return;
    }

  fprintf(fp, "%d", value);

  fclose(fp);
}

void
r9600_backlight_step(int dir)
{
  int val;
  int newval;

  if (!probed)
    return;

  val = r9600_backlight_get();

  if (dir == STEP_UP)
    {
      newval = val + lcd_r9600_cfg.step;

      if (newval > R9600_BACKLIGHT_MAX)
	newval = R9600_BACKLIGHT_MAX;

      logdebug("LCD stepping +%d -> %d\n", lcd_r9600_cfg.step, newval);
    }
  else if (dir == STEP_DOWN)
    {
      newval = val - lcd_r9600_cfg.step;

      if (newval < R9600_BACKLIGHT_OFF)
	newval = R9600_BACKLIGHT_OFF;

      logdebug("LCD stepping -%d -> %d\n", lcd_r9600_cfg.step, newval);
    }
  else
    return;

  r9600_backlight_set(newval);

  mbpdbus_send_lcd_backlight(newval, val);

  lcd_bck_info.level = newval;
}


#define PCI_ID_VENDOR_ATI        0x1002
#define PCI_ID_PRODUCT_R9600     0x4e50
#define PCI_ID_PRODUCT_R9200     0x5c63

/* Look for an ATI Radeon Mobility r9600 */
int
r9600_backlight_probe(void)
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
      /* ATI R9600 */
      if ((dev->vendor_id == PCI_ID_VENDOR_ATI)
	  && ((dev->device_id == PCI_ID_PRODUCT_R9600)
	      || (dev->device_id == PCI_ID_PRODUCT_R9200)))
	{
	  probed = 1;
	}
    }

  pci_cleanup(pacc);

  if (!probed)
    {
      logdebug("Failed to detect ATI R9600, aborting...\n");
      return -1;
    }

  lcd_bck_info.max = R9600_BACKLIGHT_MAX;

  /*
   * Set the initial backlight level
   * The value has been sanity checked already
   */
  if (lcd_r9600_cfg.init > -1)
    {
      r9600_backlight_set(lcd_r9600_cfg.init);
    }

  lcd_bck_info.level = r9600_backlight_get();

    return 0;
}

void
r9600_backlight_fix_config(void)
{
  if (lcd_r9600_cfg.init < 0)
    lcd_r9600_cfg.init = -1;

  if (lcd_r9600_cfg.init > R9600_BACKLIGHT_MAX)
    lcd_r9600_cfg.init = R9600_BACKLIGHT_MAX;

  if (lcd_r9600_cfg.step < 1)
    lcd_r9600_cfg.step = 1;

  if (lcd_r9600_cfg.step > (R9600_BACKLIGHT_MAX / 2))
    lcd_r9600_cfg.step = R9600_BACKLIGHT_MAX / 2;
}

