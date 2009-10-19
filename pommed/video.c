/*
 * pommed - Apple laptops hotkeys handler daemon
 *
 * Copyright (C) 2007, 2009 Julien BLACHE <jb@jblache.org>
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <errno.h>

#include <linux/vt.h>

#include <syslog.h>

#include "pommed.h"
#include "dbus.h"
#include "video.h"


void
video_switch(void)
{
  mbpdbus_send_video_switch();
}

int
video_vt_active(int vt)
{
  int fd;
  char buf[16];
  struct vt_stat vtstat;

  int ret;

  ret = snprintf(buf, sizeof(buf), "/dev/tty%d", vt);
  if ((ret < 0) || (ret >= sizeof(buf)))
    return 1;

  /* Try to open the VT the client's X session is running on */
  fd = open(buf, O_RDWR);

  if ((fd < 0) && (errno == EACCES))
    fd = open(buf, O_RDONLY);

  if ((fd < 0) && (errno == EACCES))
    fd = open(buf, O_WRONLY);

  /* Can't open the VT, this shouldn't happen; maybe X is remote? */
  if (fd < 0)
    return 0;

  /* The VT isn't a tty, WTF?! */
  if (!isatty(fd))
    {
      close(fd);
      return 0;
    }

  /* Get VT state, includes currently-active VT number */
  ret = ioctl(fd, VT_GETSTATE, &vtstat);
  close(fd);

  if (ret < 0)
    return 1;

  return (vt == vtstat.v_active);
}
