/*
 * mbpeventd - MacBook Pro hotkey handler daemon
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "mbpeventd.h"
#include "ambient.h"


#define KBD_AMBIENT_SENSOR      "/sys/devices/platform/applesmc/light"


void
ambient_get(int *r, int *l)
{
  int fd;
  int ret;
  char buf[16];
  char *p;

  fd = open(KBD_AMBIENT_SENSOR, O_RDONLY);
  if (fd < 0)
    {
      *r = -1;
      *l = -1;

      return;
    }

  ret = read(fd, buf, 16);

  if ((ret <= 0) || (ret > 15))
    {
      *r = -1;
      *l = -1;

      return;
    }

  buf[strlen(buf)] = '\0';

  p = strchr(buf, ',');
  *p++ = '\0';
  *r = atoi(p);

  p = buf + 1;
  *l = atoi(p);

  logdebug("Ambient light: right %d, left %d\n", *r, *l);
  
  close(fd);
}
