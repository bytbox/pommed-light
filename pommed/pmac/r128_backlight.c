/*
 * pommed - Apple laptops hotkeys handler daemon
 *
 * $Id$
 *
 * Copyright (C) 2006 Julien BLACHE <jb@jblache.org>
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


#define SYSFS_BACKLIGHT "/sys/class/backlight/aty128bl0"

static int probed = 0;


static int
r128_backlight_get(void)
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
r128_backlight_set(int value)
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


/* The brightness keys are handled by the kernel itself, so
 * we're only updating our internal buffers
 */
void
r128_backlight_step(int dir)
{
  int val;

  val = r128_backlight_get();

  logdebug("LCD stepping: %d -> %d\n", lcd_bck_info.level, val);

  mbpdbus_send_lcd_backlight(val, lcd_bck_info.level);

  lcd_bck_info.level = val;
}


#define PCI_ID_VENDOR_ATI        0x1002
#define PCI_ID_PRODUCT_R128     0x4c46

/* Look for an ATI Rage128 Mobility */
int
r128_backlight_probe(void)
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
      /* ATI r128 */
      if ((dev->vendor_id == PCI_ID_VENDOR_ATI)
	  && (dev->device_id == PCI_ID_PRODUCT_R128))
	{
	  probed = 1;
	}
    }

  pci_cleanup(pacc);

  if (!probed)
    {
      logdebug("Failed to detect ATI Rage128, aborting...\n");
      return -1;
    }

  lcd_bck_info.max = R128_BACKLIGHT_MAX;

  /*
   * Set the initial backlight level
   * The value has been sanity checked already
   */
  if (lcd_r128_cfg.init > -1)
    {
      r128_backlight_set(lcd_r128_cfg.init);
    }

  lcd_bck_info.level = r128_backlight_get();

  return 0;
}

void
r128_backlight_fix_config(void)
{
  if (lcd_r128_cfg.init < 0)
    lcd_r128_cfg.init = -1;

  if (lcd_r128_cfg.init > R128_BACKLIGHT_MAX)
    lcd_r128_cfg.init = R128_BACKLIGHT_MAX;
}

