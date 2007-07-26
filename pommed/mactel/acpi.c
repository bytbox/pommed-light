/*
 * pommed - Apple laptops hotkeys handler daemon
 *
 * $Id$
 *
 * Copyright (C) 2006-2007 Julien BLACHE <jb@jblache.org>
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

#include "../pommed.h"
#include "../power.h"


#define ACPI_AC_STATE   "/proc/acpi/ac_adapter/ADP1/state"
#define ACPI_AC_ONLINE  "on-line\n"
#define ACPI_AC_OFFLINE "off-line\n"


/* Internal API */
int
check_ac_state(void)
{
  FILE *fp;
  char buf[128];
  int ret;

  fp = fopen(ACPI_AC_STATE, "r");
  if (fp == NULL)
    return AC_STATE_ERROR;

  ret = fread(buf, 1, 127, fp);

  if (ferror(fp) != 0)
    {
      logdebug("acpi: Error reading AC state: %s\n", strerror(errno));
      return AC_STATE_ERROR;
    }

  if (feof(fp) == 0)
    {
      logdebug("acpi: Error reading AC state: buffer too small\n");
      return AC_STATE_ERROR;
    }

  fclose(fp);

  buf[ret] = '\0';

  if (strstr(buf, ACPI_AC_ONLINE) != NULL)
    return AC_STATE_ONLINE;

  if (strstr(buf, ACPI_AC_OFFLINE) != NULL)
    return AC_STATE_OFFLINE;

  return AC_STATE_UNKNOWN;
}
