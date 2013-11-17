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
#include <string.h>
#include <errno.h>

#include "../pommed-light.h"
#include "../power.h"


#define PROC_PMU_AC_STATE_FILE  "/proc/pmu/info"
#define PROC_PMU_AC_STATE       "AC Power"
#define PROC_PMU_AC_ONLINE      '1'
#define PROC_PMU_AC_OFFLINE     '0'


/* Internal API - procfs PMU */
int
procfs_check_ac_state(void)
{
  FILE *fp;
  char buf[128];
  char *ac_state;
  int ret;

  fp = fopen(PROC_PMU_AC_STATE_FILE, "r");
  if (fp == NULL)
    return AC_STATE_ERROR;

  ret = fread(buf, 1, 127, fp);

  if (ferror(fp) != 0)
    {
      logdebug("pmu: Error reading AC state: %s\n", strerror(errno));
      return AC_STATE_ERROR;
    }

  if (feof(fp) == 0)
    {
      logdebug("pmu: Error reading AC state: buffer too small\n");
      return AC_STATE_ERROR;
    }

  fclose(fp);

  buf[ret] = '\0';

  ac_state = strstr(buf, PROC_PMU_AC_STATE);
  if (ac_state == NULL)
    return AC_STATE_ERROR;

  ac_state = strchr(ac_state, '\n');
  if ((ac_state == NULL) || (ac_state == buf))
    return AC_STATE_ERROR;

  if (ac_state[-1] == PROC_PMU_AC_ONLINE)
    return AC_STATE_ONLINE;

  if (ac_state[-1] == PROC_PMU_AC_OFFLINE)
    return AC_STATE_OFFLINE;

  return AC_STATE_UNKNOWN;
}
