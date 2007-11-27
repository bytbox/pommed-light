/*
 * video-client.c -- shared video switch routines for pommed clients
 *
 * $Id$
 *
 * Copyright (C) 2007 Julien BLACHE <jb@jblache.org>
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
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>

#include "video-client.h"


static char *vsw_user = NULL;

/*
 * NOTE: you MUST install a SIGCHLD handler if you use this function
 */
void
mbp_video_switch(void)
{
  struct passwd *pw;
  char *vsw = NULL;

  int ret;

  if (vsw_user == NULL)
    {
      pw = getpwuid(getuid());
      if (pw == NULL)
        {
          fprintf(stderr, "Could not get user information\n");

          return;
        }

      vsw_user = (char *) malloc(strlen(pw->pw_dir) + strlen(VIDEO_SWITCH_USER) + 1);
      if (vsw_user == NULL)
        {
          fprintf(stderr, "Could not allocate memory\n");

          return;
        }

      strncpy(vsw_user, pw->pw_dir, strlen(pw->pw_dir) + 1);
      strncat(vsw_user, VIDEO_SWITCH_USER, strlen(VIDEO_SWITCH_USER));
    }

  if (access(vsw_user, R_OK | X_OK) == 0)
    {
      vsw = vsw_user;
    }
  else if (access(VIDEO_SWITCH_SYSTEM, R_OK | X_OK) == 0)
    {
      vsw = VIDEO_SWITCH_SYSTEM;
    }
  else
    {
      fprintf(stderr, "No video switch script available\n");
      return;
    }

  ret = fork();
  if (ret == 0) /* exec video switch script */
    {
      execl(vsw, "videoswitch", NULL);

      fprintf(stderr, "Could not execute video switch script: %s", strerror(errno));
      exit(1);
    }
  else if (ret == -1)
    {
      fprintf(stderr, "Could not fork: %s\n", strerror(errno));
      return;
    }
}

