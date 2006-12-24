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
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <syslog.h>

#include "mbpeventd.h"
#include "conffile.h"
#include "cd_eject.h"
#include "dbus.h"


void
cd_eject(void)
{
  char cmd[128];
  int ret;

  if (!eject_cfg.enabled)
    return;

  strcpy(cmd, CD_EJECT_CMD " ");
  strncat(cmd, eject_cfg.device, sizeof(cmd) - 1);

  ret = system(cmd);

  if (WEXITSTATUS(ret) != 0)
    {
      /* 127 means "shell not available" */
      if (WEXITSTATUS(ret) != 127)
	logmsg(LOG_WARNING, "CD ejection failed, eject returned %d", WEXITSTATUS(ret));

      return;
    }

  mbpdbus_send_cd_eject();
}


void
cd_eject_fix_config(void)
{
  if (eject_cfg.device == NULL)
    {
      eject_cfg.enabled = 0;
      return;
    }

  if (strlen(eject_cfg.device) > 100)
    {
      eject_cfg.enabled = 0;

      logmsg(LOG_INFO, "CD/DVD device path too long, CD ejection disabled");

      return;
    }
}
