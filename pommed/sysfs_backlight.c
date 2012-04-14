/*
 * pommed - Apple laptops hotkeys handler daemon
 *
 * Copyright (C) 2006-2008,2010 Julien BLACHE <jb@jblache.org>
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "pommed.h"
#include "conffile.h"
#include "lcd_backlight.h"


enum {
  SYSFS_DRIVER_NONE,
#ifdef __powerpc__
  SYSFS_DRIVER_ATY128,
  SYSFS_DRIVER_RADEON,
  SYSFS_DRIVER_NVIDIA,
  SYSFS_DRIVER_RIVA,
#else
  SYSFS_DRIVER_MBP,
  SYSFS_DRIVER_APPLE,
  SYSFS_DRIVER_NVIDIA,
  SYSFS_DRIVER_NOUVEAU,
  SYSFS_DRIVER_ACPI,
#endif
  SYSFS_DRIVER_MAX
};


/* sysfs backlight driver in use */
static int bck_driver = SYSFS_DRIVER_NONE;

/* sysfs actual_brightness node path */
static char *actual_brightness[] =
  {
    "/dev/null",
#ifdef __powerpc__
    "/sys/class/backlight/aty128bl0/actual_brightness",
    "/sys/class/backlight/radeonbl0/actual_brightness",
    "/sys/class/backlight/nvidiabl0/actual_brightness",
    "/sys/class/backlight/rivabl0/actual_brightness",
#else
    "/sys/class/backlight/mbp_backlight/actual_brightness",
    "/sys/class/backlight/apple_backlight/actual_brightness",
    "/sys/class/backlight/nvidia_backlight/actual_brightness",
    "/sys/class/backlight/nv_backlight/actual_brightness",
    "/sys/class/backlight/acpi_video0/actual_brightness",
#endif
  };

/* sysfs brightness node path */
static char *brightness[] =
  {
    "/dev/null",
#ifdef __powerpc__
    "/sys/class/backlight/aty128bl0/brightness",
    "/sys/class/backlight/radeonbl0/brightness",
    "/sys/class/backlight/nvidiabl0/brightness",
    "/sys/class/backlight/rivabl0/brightness",
#else
    "/sys/class/backlight/mbp_backlight/brightness",
    "/sys/class/backlight/apple_backlight/brightness",
    "/sys/class/backlight/nvidia_backlight/brightness",
    "/sys/class/backlight/nv_backlight/brightness",
    "/sys/class/backlight/acpi_video0/brightness",
#endif
  };

/* sysfs max_brightness node path */
static char *max_brightness[] =
  {
    "/dev/null",
#ifdef __powerpc__
    "/sys/class/backlight/aty128bl0/max_brightness",
    "/sys/class/backlight/radeonbl0/max_brightness",
    "/sys/class/backlight/nvidiabl0/max_brightness",
    "/sys/class/backlight/rivabl0/max_brightness",
#else
    "/sys/class/backlight/mbp_backlight/max_brightness",
    "/sys/class/backlight/apple_backlight/max_brightness",
    "/sys/class/backlight/nvidia_backlight/max_brightness",
    "/sys/class/backlight/nv_backlight/max_brightness",
    "/sys/class/backlight/acpi_video0/max_brightness",
#endif
  };


struct _lcd_bck_info lcd_bck_info;


static int
sysfs_backlight_get(void)
{
  int fd;
  int n;
  char buffer[8];

  if (bck_driver == SYSFS_DRIVER_NONE)
    return 0;

  fd = open(actual_brightness[bck_driver], O_RDONLY);
  if (fd < 0)
    {
      logmsg(LOG_WARNING, "Could not open sysfs actual_brightness node: %s", strerror(errno));

      return 0;
    }

  memset(buffer, 0, sizeof(buffer));
  n = read(fd, buffer, sizeof(buffer) - 1);
  if (n < 1)
    {
      logmsg(LOG_WARNING, "Could not read sysfs actual_brightness node");

      close(fd);
      return 0;
    }
  close(fd);

  return atoi(buffer);
}

static int
sysfs_backlight_get_max(void)
{
  int fd;
  int n;
  char buffer[8];

  if (bck_driver == SYSFS_DRIVER_NONE)
    return 0;

  fd = open(max_brightness[bck_driver], O_RDONLY);
  if (fd < 0)
    {
      logmsg(LOG_WARNING, "Could not open sysfs max_brightness node: %s", strerror(errno));

      return 0;
    }

  memset(buffer, 0, sizeof(buffer));
  n = read(fd, buffer, sizeof(buffer) - 1);
  if (n < 1)
    {
      logmsg(LOG_WARNING, "Could not read sysfs max_brightness node");

      close(fd);
      return 0;
    }
  close(fd);

  return atoi(buffer);
}


static void
sysfs_backlight_set(int value)
{
  FILE *fp;

  if (bck_driver == SYSFS_DRIVER_NONE)
    return;

  fp = fopen(brightness[bck_driver], "a");
  if (fp == NULL)
    {
      logmsg(LOG_WARNING, "Could not open sysfs brightness node: %s", strerror(errno));

      return;
    }

  fprintf(fp, "%d", value);

  fclose(fp);
}

void
sysfs_backlight_step(int dir)
{
  int val;
  int newval;

  if (bck_driver == SYSFS_DRIVER_NONE)
    return;

  val = sysfs_backlight_get();

  if (dir == STEP_UP)
    {
      newval = val + lcd_sysfs_cfg.step;

      if (newval > lcd_bck_info.max)
	newval = lcd_bck_info.max;

      logdebug("LCD stepping +%d -> %d\n", lcd_sysfs_cfg.step, newval);
    }
  else if (dir == STEP_DOWN)
    {
      newval = val - lcd_sysfs_cfg.step;

      if (newval < SYSFS_BACKLIGHT_OFF)
	newval = SYSFS_BACKLIGHT_OFF;

      logdebug("LCD stepping -%d -> %d\n", lcd_sysfs_cfg.step, newval);
    }
  else
    return;

  sysfs_backlight_set(newval);

  lcd_bck_info.level = newval;
}


void
sysfs_backlight_toggle(int lvl)
{
  int val;

  if (bck_driver == SYSFS_DRIVER_NONE)
    return;

  if (lcd_sysfs_cfg.on_batt == 0)
    return;

  val = sysfs_backlight_get();
  if (val != lcd_bck_info.level)
    {
      lcd_bck_info.level = val;
    }

  if (lcd_bck_info.level == 0)
    return;

  switch (lvl)
    {
      case LCD_ON_AC_LEVEL:
	if (lcd_bck_info.level >= lcd_bck_info.ac_lvl)
	  break;

	logdebug("LCD switching to AC level\n");

	sysfs_backlight_set(lcd_bck_info.ac_lvl);

	lcd_bck_info.level = lcd_bck_info.ac_lvl;
	break;

      case LCD_ON_BATT_LEVEL:
	if (lcd_bck_info.level <= lcd_sysfs_cfg.on_batt)
	  break;

	logdebug("LCD switching to battery level\n");

	lcd_bck_info.ac_lvl = lcd_bck_info.level;

	sysfs_backlight_set(lcd_sysfs_cfg.on_batt);

	lcd_bck_info.level = lcd_sysfs_cfg.on_batt;
	break;
    }
}


#ifdef __powerpc__
/* When brightness keys are handled by the kernel itself,
 * we're only updating our internal buffers
 */
void
sysfs_backlight_step_kernel(int dir)
{
  int val;

  val = sysfs_backlight_get();

  logdebug("LCD stepping: %d -> %d\n", lcd_bck_info.level, val);

  lcd_bck_info.level = val;
}

void
sysfs_backlight_toggle_kernel(int lvl)
{
  return;
}
#endif /* __powerpc__ */


/* We can't fix the config until we know the max backlight value,
 * so, here, fix_config() is static and called at probe time
 */
static void
sysfs_backlight_fix_config(void)
{
  if (lcd_sysfs_cfg.init < 0)
    lcd_sysfs_cfg.init = -1;

  if (lcd_sysfs_cfg.init > lcd_bck_info.max)
    lcd_sysfs_cfg.init = lcd_bck_info.max;

  if (lcd_sysfs_cfg.step < 1)
    lcd_sysfs_cfg.step = 1;

  if (lcd_sysfs_cfg.step > (lcd_bck_info.max / 2))
    lcd_sysfs_cfg.step = lcd_bck_info.max / 2;

  if ((lcd_sysfs_cfg.on_batt > lcd_bck_info.max)
      || (lcd_sysfs_cfg.on_batt < SYSFS_BACKLIGHT_OFF))
    lcd_sysfs_cfg.on_batt = 0;
}


/* Look for the backlight driver */
static int
sysfs_backlight_probe(int driver)
{
  if (access(brightness[driver], W_OK) != 0)
    {
      logdebug("Failed to access brightness node: %s\n", strerror(errno));
      return -1;
    }

  if (access(actual_brightness[driver], R_OK) != 0)
    {
      logdebug("Failed to access actual_brightness node: %s\n", strerror(errno));
      return -1;
    }

  if (access(max_brightness[driver], R_OK) != 0)
    {
      logdebug("Failed to access max_brightness node: %s\n", strerror(errno));
      return -1;
    }

  bck_driver = driver;

  lcd_bck_info.max = sysfs_backlight_get_max();

  /* Now we can fix the config */
  sysfs_backlight_fix_config();

  /*
   * Set the initial backlight level
   * The value has been sanity checked already
   */
  if (lcd_sysfs_cfg.init > -1)
    {
      sysfs_backlight_set(lcd_sysfs_cfg.init);
    }

  lcd_bck_info.level = sysfs_backlight_get();
  lcd_bck_info.ac_lvl = lcd_bck_info.level;

  return 0;
}


#ifdef __powerpc__
int
aty128_sysfs_backlight_probe(void)
{
  return sysfs_backlight_probe(SYSFS_DRIVER_ATY128);
}

int
r9x00_sysfs_backlight_probe(void)
{
  return sysfs_backlight_probe(SYSFS_DRIVER_RADEON);
}

int
nvidia_sysfs_backlight_probe(void)
{
  return sysfs_backlight_probe(SYSFS_DRIVER_NVIDIA);
}

#else

int
mbp_sysfs_backlight_probe(void)
{
  int drv;
  int ret;

  for (drv = SYSFS_DRIVER_NONE + 1; drv < SYSFS_DRIVER_MAX; drv++)
    {
      ret = sysfs_backlight_probe(drv);
      if (ret == 0)
	return 0;
    }

  /* Probe failed, wire up native driver instead */
  switch (mops->type)
    {
      case MACHINE_MACBOOKPRO_3:
      case MACHINE_MACBOOKPRO_4:
      case MACHINE_MACBOOKPRO_5:
      case MACHINE_MACBOOKPRO_6:
      case MACHINE_MACBOOK_5:
      case MACHINE_MACBOOK_6:
      case MACHINE_MACBOOKAIR_2:
      case MACHINE_MACBOOKAIR_3:
	logmsg(LOG_INFO, "sysfs backlight probe failed, falling back to nv8600mgt");

	ret = nv8600mgt_backlight_probe();
	if (ret == 0)
	  {
	    /* Wire up fallback native driver */
	    mops->lcd_backlight_step = nv8600mgt_backlight_step;
	    mops->lcd_backlight_toggle = nv8600mgt_backlight_toggle;
	  }
	return ret;

      default:
	logmsg(LOG_ERR, "sysfs backlight probe failed, no fallback for this machine");
	return -1;
    }

  return -1;
}
#endif
