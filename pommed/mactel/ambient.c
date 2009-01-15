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

#include <syslog.h>

#include "../pommed.h"
#include "../ambient.h"


#define APPLESMC_SYSFS_BASE    "/sys/devices/platform"
static char smcpath[64];


struct _ambient_info ambient_info;


void
ambient_get(int *r, int *l)
{
  int fd;
  int ret;
  char buf[16];
  char *p;

  fd = open(smcpath, O_RDONLY);
  if (fd < 0)
    {
      *r = -1;
      *l = -1;

      ambient_info.right = 0;
      ambient_info.left = 0;

      return;
    }

  ret = read(fd, buf, 16);

  close(fd);

  if ((ret <= 0) || (ret > 15))
    {
      *r = -1;
      *l = -1;

      ambient_info.right = 0;
      ambient_info.left = 0;

      return;
    }

  buf[strlen(buf)] = '\0';

  p = strchr(buf, ',');
  *p++ = '\0';
  *r = atoi(p);

  p = buf + 1;
  *l = atoi(p);

  logdebug("Ambient light: right %d, left %d\n", *r, *l);

  ambient_info.right = *r;
  ambient_info.left = *l;
}


void
ambient_init(int *r, int *l)
{
  DIR *pdev;
  struct dirent *pdevent;

  int ret;

  /* Probe for the applesmc sysfs path */
  pdev = opendir(APPLESMC_SYSFS_BASE);
  if (pdev != NULL)
    {
      while ((pdevent = readdir(pdev)))
	{
	  if (pdevent->d_type != DT_DIR)
	    continue;

	  if (strstr(pdevent->d_name, "applesmc") == pdevent->d_name)
	    {
	      ret = snprintf(smcpath, sizeof(smcpath), "%s/%s/light",
			    APPLESMC_SYSFS_BASE, pdevent->d_name);

	      if ((ret < 0) || (ret >= sizeof(smcpath)))
		logmsg(LOG_ERR, "Failed to build applesmc sysfs path");
	      else
		logmsg(LOG_INFO, "Found applesmc at %s", smcpath);

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
