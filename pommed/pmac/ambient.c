/*
 * pommed - Apple laptops hotkeys handler daemon
 *
 * Copyright (C) 2006-2007 Julien BLACHE <jb@jblache.org>
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

#include <linux/adb.h>

#include "../pommed.h"
#include "../ambient.h"


struct _ambient_info ambient_info;


#define LMU_AMBIENT_MAX_RAW    1600

#define PMU_AMBIENT_MAX_RAW    2048


static void
lmu_ambient_get(int *r, int *l)
{
  int fd;
  int ret;
  char buf[4];

  fd = open(lmu_info.i2cdev, O_RDONLY);
  if (fd < 0)
    {
      *r = -1;
      *l = -1;

      ambient_info.right = 0;
      ambient_info.left = 0;

      logmsg(LOG_ERR, "Could not open i2c device %s: %s", lmu_info.i2cdev, strerror(errno));
      return;
    }

  ret = ioctl(fd, I2C_SLAVE, lmu_info.lmuaddr);
  if (ret < 0)
    {
      close(fd);

      *r = -1;
      *l = -1;

      ambient_info.right = 0;
      ambient_info.left = 0;

      logmsg(LOG_ERR, "ioctl failed on %s: %s", lmu_info.i2cdev, strerror(errno));

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
  *r = (int) (((buf[0] << 8) | buf[1]) * KBD_AMBIENT_MAX) / LMU_AMBIENT_MAX_RAW;
  *l = (int) (((buf[2] << 8) | buf[3]) * KBD_AMBIENT_MAX) / LMU_AMBIENT_MAX_RAW;

  logdebug("Ambient light: right %d, left %d\n", *r, *l);

  ambient_info.right = *r;
  ambient_info.left = *l;
}

static void
pmu_ambient_get(int *r, int *l)
{
  int fd;
  int ret;
  char buf[ADB_BUFFER_SIZE];

  fd = open(ADB_DEVICE, O_RDWR);
  if (fd < 0)
    {
      *r = -1;
      *l = -1;

      ambient_info.right = 0;
      ambient_info.left = 0;

      logmsg(LOG_ERR, "Could not open ADB device %s: %s", ADB_DEVICE, strerror(errno));
      return;
    }

  buf[0] = PMU_PACKET;
  buf[1] = 0x4f; /* PMU command */
  buf[2] = 1;

  ret = write(fd, buf, 3);
  if (ret != 3)
    {
      *r = -1;
      *l = -1;

      ambient_info.right = 0;
      ambient_info.left = 0;

      logmsg(LOG_ERR, "Could not send PMU command: %s", strerror(errno));
      close(fd);
      return;
    }

  ret = read(fd, buf, ADB_BUFFER_SIZE);
  if (ret != 5)
    {
      *r = -1;
      *l = -1;

      ambient_info.right = 0;
      ambient_info.left = 0;

      logmsg(LOG_ERR, "Could not read PMU reply: %s", strerror(errno));
      close(fd);
      return;
    }

  close(fd);
  
  /* Taken from pbbuttonsd */
  *l = (int) (((buf[2] << 8) | buf[1]) * KBD_AMBIENT_MAX) / PMU_AMBIENT_MAX_RAW;
  *r = (int) (((buf[4] << 8) | buf[3]) * KBD_AMBIENT_MAX) / PMU_AMBIENT_MAX_RAW;

  logdebug("Ambient light: right %d, left %d\n", *r, *l);

  ambient_info.right = *r;
  ambient_info.left = *l;
}

void
ambient_get(int *r, int *l)
{
  if ((mops->type == MACHINE_POWERBOOK_58)
      || (mops->type == MACHINE_POWERBOOK_59))
    {
      pmu_ambient_get(r, l);
    }
  else
    {
      lmu_ambient_get(r, l);
    }
}


void
ambient_init(int *r, int *l)
{
  ambient_get(r, l);

  ambient_info.max = KBD_AMBIENT_MAX;
  ambient_info.left = *l;
  ambient_info.right = *r;
}
