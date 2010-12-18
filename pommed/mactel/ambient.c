/*
 * pommed - Apple laptops hotkeys handler daemon
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
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <syslog.h>

#include "../pommed.h"
#include "../ambient.h"


#define HWMON_SYSFS_BASE    "/sys/class/hwmon"
static char *smcpath;


struct _ambient_info ambient_info;


void
ambient_get(int *r, int *l)
{
  int fd;
  int ret;
  char buf[16];
  char *p;

  if (!smcpath)
    goto out_error;

  fd = open(smcpath, O_RDONLY);
  if (fd < 0)
    goto out_error;

  ret = read(fd, buf, 16);

  close(fd);

  if ((ret <= 0) || (ret > 15))
    goto out_error;

  buf[strlen(buf)] = '\0';

  p = strchr(buf, ',');
  *p++ = '\0';
  *r = atoi(p);

  p = buf + 1;
  *l = atoi(p);

  logdebug("Ambient light: right %d, left %d\n", *r, *l);

  ambient_info.right = *r;
  ambient_info.left = *l;

  return;

 out_error:
  *r = -1;
  *l = -1;

  ambient_info.right = 0;
  ambient_info.left = 0;
}


void
ambient_init(int *r, int *l)
{
  char devpath[PATH_MAX];
  char devname[9];
  char *p;
  DIR *pdev;
  struct dirent *pdevent;
  int fd;
  int ret;

  smcpath = NULL;

  /* Probe for the applesmc sysfs path */
  pdev = opendir(HWMON_SYSFS_BASE);
  if (pdev != NULL)
    {
      while ((pdevent = readdir(pdev)))
	{
	  if (pdevent->d_name[0] == '.')
	    continue;

	  ret = snprintf(devpath, sizeof(devpath), HWMON_SYSFS_BASE "/%s/device/name", pdevent->d_name);
	  if ((ret < 0) || (ret >= sizeof(devpath)))
	    {
	      logmsg(LOG_WARNING, "Failed to build hwmon probe path");
	      continue;
	    }

	  fd = open(devpath, O_RDONLY);
	  if (fd < 0)
	    {
	      logmsg(LOG_ERR, "Could not open %s: %s", devpath, strerror(errno));
	      continue;
	    }

	  memset(devname, 0, sizeof(devname));
	  ret = read(fd, devname, sizeof(devname) - 1);
	  close(fd);

	  if (ret != (sizeof(devname) - 1))
	    continue;

	  if (strcmp(devname, "applesmc") == 0)
	    {
	      p = strrchr(devpath, '/');
	      *p = '\0';

	      logmsg(LOG_INFO, "Found applesmc at %s", devpath);

	      smcpath = realpath(devpath, NULL);
	      if (!smcpath)
		{
		  logmsg(LOG_ERR, "Could not dereference applesmc device path: %s\n", strerror(errno));
		  break;
		}

	      logmsg(LOG_INFO, "Dereferenced applesmc to %s", smcpath);

	      break;
	    }
	}

      closedir(pdev);
    }

  ambient_get(r, l);

  ambient_info.max = KBD_AMBIENT_MAX;
  ambient_info.left = *l;
  ambient_info.right = *r;
}
