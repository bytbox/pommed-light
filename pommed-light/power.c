/*
 * pommed - Apple laptops hotkeys handler daemon
 *
 * Copyright (C) 2006-2008 Julien BLACHE <jb@jblache.org>
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
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <syslog.h>

#include "pommed-light.h"
#include "evloop.h"
#include "lcd_backlight.h"
#include "power.h"


/* Internal API - legacy procfs interface, ACPI or PMU */
int
procfs_check_ac_state(void);


static int prev_state;
static int power_timer;


/* sysfs power_supply class */
static int
sysfs_check_ac_state(void)
{
  FILE *fp;
  char ac_state;

  fp = fopen(SYSFS_POWER_AC_STATE, "r");
  if (fp == NULL)
    return AC_STATE_ERROR;

  fread(&ac_state, 1, 1, fp);

  if (ferror(fp) != 0)
    {
      logdebug("power: Error reading sysfs AC state: %s\n", strerror(errno));
      return AC_STATE_ERROR;
    }

  fclose(fp);

  if (ac_state == '1')
    return AC_STATE_ONLINE;

  if (ac_state == '0')
    return AC_STATE_OFFLINE;

  return AC_STATE_UNKNOWN;
}

static int
check_ac_state(void)
{
  int ret;

  ret = sysfs_check_ac_state();
  if (ret == AC_STATE_ERROR)
    return procfs_check_ac_state();

  return ret;
}


static void
power_check_ac_state(int id, uint64_t ticks)
{
  int ac_state;

  ac_state = check_ac_state();

  if (ac_state == prev_state)
    return;
  else
      prev_state = ac_state;

  switch (ac_state)
    {
      case AC_STATE_ONLINE:
	logdebug("power: switched to AC\n");
	mops->lcd_backlight_toggle(LCD_ON_AC_LEVEL);
	break;

      case AC_STATE_OFFLINE:
	logdebug("power: switched to battery\n");
	mops->lcd_backlight_toggle(LCD_ON_BATT_LEVEL);
	break;

      case AC_STATE_ERROR:
	logmsg(LOG_ERR, "power: error reading AC state");
	break;

      case AC_STATE_UNKNOWN:
	logmsg(LOG_INFO, "power: unknown AC state");
	break;
    }
}


void
power_init(void)
{
  prev_state = check_ac_state();

  power_timer = evloop_add_timer(POWER_TIMEOUT, power_check_ac_state);
}

void
power_cleanup(void)
{
  if (power_timer > 0)
    evloop_remove_timer(power_timer);
}
