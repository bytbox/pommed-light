/*
 * pommed - Apple laptops hotkeys handler daemon
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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <syslog.h>

#include <sys/ioctl.h>
#include <linux/cdrom.h>

#include "pommed.h"
#include "conffile.h"
#include "cd_eject.h"
#include "dbus.h"


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
      logmsg(LOG_ERR, "Could not open CD/DVD device");

      return;
    }

  /* Check drive status */
  ret = ioctl(fd, CDROM_DRIVE_STATUS);
  switch (ret)
    {
      case CDS_NO_INFO: /* fall through */
	logmsg(LOG_INFO, "Driver does not support CDROM_DRIVE_STATUS, trying to eject anyway");

      case CDS_DISC_OK:
	mbpdbus_send_cd_eject();

	ret = ioctl(fd, CDROMEJECT);
	if (ret != 0)
	  logmsg(LOG_ERR, "Eject command failed");
	break;

      case CDS_NO_DISC:
	logmsg(LOG_INFO, "No disc in CD/DVD drive");
	break;

      case CDS_DRIVE_NOT_READY:
	logmsg(LOG_INFO, "Drive not ready, please retry later");
	break;

      case CDS_TRAY_OPEN:
	logmsg(LOG_INFO, "Drive tray already open");
	break;

      default:
	logmsg(LOG_INFO, "CDROM_DRIVE_STATUS returned %d", ret);
    }

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
