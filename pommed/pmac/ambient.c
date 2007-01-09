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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <errno.h>
#include <syslog.h>
#include <sys/ioctl.h>

#include "../pommed.h"
#include "../ambient.h"
#include "../dbus.h"


struct _ambient_info ambient_info;


#define I2C_DEV	               "/dev/i2c-7"
#define I2C_SLAVE              0x703
#define LMU_ADDR               0x42
#define KBD_AMBIENT_MAX_RAW    1600


void
ambient_get(int *r, int *l)
{
  int fd;
  int ret;
  char buf[4];

  fd = open(I2C_DEV, O_RDONLY);
  if (fd < 0)
    {
      *r = -1;
      *l = -1;

      ambient_info.right = 0;
      ambient_info.left = 0;

      logmsg(LOG_ERR, "Could not open i2c device %s: %s\n",I2C_DEV, strerror(errno));
      return;
    }

  ret = ioctl(fd, I2C_SLAVE, LMU_ADDR);
  if (ret < 0)
    {
      close(fd);

      *r = -1;
      *l = -1;

      ambient_info.right = 0;
      ambient_info.left = 0;

      logmsg(LOG_ERR, "ioctl failed on %s: %s\n",I2C_DEV, strerror(errno));

      return;
    }
  
  ret = read(fd, buf, 4);
  if (ret != 4)
    {
      close(fd);

      *r = -1;
      *l = -1;

      ambient_info.right = 0;
      ambient_info.left = 0;

      return;
    }
  close(fd);

  /* found in pbbuttonsd.conf */
  *r = (int) (((buf[0] << 8) | buf[1]) * KBD_AMBIENT_MAX) / KBD_AMBIENT_MAX_RAW;
  *l = (int) (((buf[2] << 8) | buf[3]) * KBD_AMBIENT_MAX) / KBD_AMBIENT_MAX_RAW;

  logdebug("Ambient light: right %d, left %d\n", *r, *l);

  ambient_info.right = *r;
  ambient_info.left = *l;
}


void
ambient_init(int *r, int *l)
{
  ambient_get(r, l);

  ambient_info.max = KBD_AMBIENT_MAX;
  ambient_info.left = *l;
  ambient_info.right = *r;
}
