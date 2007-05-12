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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <errno.h>

#include <syslog.h>

#include <sys/ioctl.h>
#include <linux/cdrom.h>

#include "pommed.h"
#include "conffile.h"
#include "cd_eject.h"
#include "dbus.h"


static char *
check_mount(char *device)
{
  char line[1024];
  char dev[512];
  char mpoint[512];

  int ret;
  FILE *fp;

  fp = fopen("/proc/mounts", "r");
  if (fp == NULL)
    {
      logmsg(LOG_ERR, "Could not open mount table for reading: %s\n", strerror(errno));
      return NULL;
    }

  while (fgets(line, sizeof(line), fp) != 0)
    {
      ret = sscanf(line, "%511s %511s", dev, mpoint);
      if (ret < 2)
	continue;

      if (strcmp(dev, device) == 0)
	{
	  fclose(fp);
	  return strdup(mpoint);
	}
    }

  fclose(fp);

  return NULL;
}

static int
try_to_unmount(char *device)
{
  char *rdev;
  char *mpoint;
  int ret;

  /* passing NULL is a GNU extension, recommended by POSIX */
  rdev = realpath(device, NULL);
  if (rdev == NULL)
    return -1;

  mpoint = check_mount(rdev);
  if (mpoint == NULL)
    {
      free(rdev);
      return 0;
    }

  logdebug("%s (%s) mounted on %s; attempting to unmount\n", device, rdev, mpoint);

  ret = fork();
  if (ret == 0) /* exec umount */
    {
      /* We need to call umount because mount/umount maintain /etc/mtab ... */
      execl("/bin/umount", "umount", mpoint, NULL);

      logmsg(LOG_ERR, "Could not execute umount: %s", strerror(errno));
      exit(1);
    }
  else if (ret == -1)
    {
      logmsg(LOG_ERR, "Could not fork: %s\n", strerror(errno));
      free(rdev);
      free(mpoint);
      return -1;
    }
  else
    {
      waitpid(ret, &ret, 0);
      if ((WIFEXITED(ret) == 0) || (WEXITSTATUS(ret) != 0))
	{
	  logmsg(LOG_INFO, "umount failed");
	  free(rdev);
	  free(mpoint);
	  return -1;
	}
    }

  free(rdev);
  free(mpoint);
  return 0;
}


void
cd_eject(void)
{
  int fd;
  int ret;

  if (!eject_cfg.enabled)
    return;

  fd = open(eject_cfg.device, O_RDONLY | O_NONBLOCK);
  if (fd < 0)
    {
      logmsg(LOG_ERR, "Could not open CD/DVD device: %s", strerror(errno));

      return;
    }

  /* Check drive status */
  ret = ioctl(fd, CDROM_DRIVE_STATUS);
  close(fd);

  switch (ret)
    {
      case CDS_NO_INFO: /* fall through to CDS_DISC_OK */
	logmsg(LOG_INFO, "Driver does not support CDROM_DRIVE_STATUS, trying to eject anyway");

      case CDS_DISC_OK:
	break;

      case CDS_NO_DISC:
	logmsg(LOG_INFO, "No disc in CD/DVD drive");
	return;

      case CDS_DRIVE_NOT_READY:
	logmsg(LOG_INFO, "Drive not ready, please retry later");
	return;

      case CDS_TRAY_OPEN:
	logmsg(LOG_INFO, "Drive tray already open");
	return;

      default:
	logmsg(LOG_INFO, "CDROM_DRIVE_STATUS returned %d (%s)", ret, strerror(errno));
	return;
    }

  ret = try_to_unmount(eject_cfg.device);
  if (ret < 0)
    {
      logmsg(LOG_ERR, "Could not unmount device");
      return;
    }

  mbpdbus_send_cd_eject();

  fd = open(eject_cfg.device, O_RDONLY | O_NONBLOCK);
  if (fd < 0)
    {
      logmsg(LOG_ERR, "Could not open CD/DVD device: %s", strerror(errno));
      return;
    }

  ret = ioctl(fd, CDROMEJECT);
  if (ret != 0)
    logmsg(LOG_ERR, "Eject command failed: %s", strerror(errno));

  close(fd);
}


void
cd_eject_fix_config(void)
{
  if (eject_cfg.device == NULL)
    {
      eject_cfg.enabled = 0;
      return;
    }
}
