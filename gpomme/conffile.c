/*
 * gpomme - GTK application for use with pommed
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
#include <string.h>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <pwd.h>

#include <linux/inotify.h>
#include "inotify-syscalls.h"

#include <confuse.h>

#include "conffile.h"
#include "gpomme.h"
#include "theme.h"

#define CONFFILE        "/.gpommerc"

static cfg_opt_t cfg_opts[] =
  {
    CFG_STR("theme", DEFAULT_THEME, CFGF_NONE),
    CFG_INT("timeout", 900, CFGF_NONE),
    CFG_END()
  };


cfg_t *cfg = NULL;
static char *conffile = NULL;


static int
config_validate_positive_integer(cfg_t *cfg, cfg_opt_t *opt)
{
  int value = cfg_opt_getnint(opt, cfg_opt_size(opt) - 1);

  if (value < 0)
    {
      cfg_error(cfg, "Error: Value for '%s' must be positive", opt->name);
      return -1;
    }

  return 0;
}

static int
config_validate_string(cfg_t *cfg, cfg_opt_t *opt)
{
  char *value = cfg_opt_getnstr(opt, cfg_opt_size(opt) - 1);

  if (strlen(value) == 0)
    {
      cfg_error(cfg, "Error: Value for '%s' must be a non-zero string", opt->name);
      return -1;
    }

  return 0;
}


int
config_load(void)
{
  struct passwd *pw;

  int ret;

  if (conffile == NULL)
    {
      pw = getpwuid(getuid());
      if (pw == NULL)
	{
	  fprintf(stderr, "Could not get user information\n");

	  return -1;
	}

      conffile = (char *) malloc(strlen(pw->pw_dir) + strlen(CONFFILE) + 1);
      if (conffile == NULL)
	{
	  fprintf(stderr, "Could not allocate memory\n");

	  return -1;
	}

      strncpy(conffile, pw->pw_dir, strlen(pw->pw_dir) + 1);
      strncat(conffile, CONFFILE, strlen(CONFFILE));
    }

  if (cfg != NULL)
    cfg_free(cfg);

  cfg = cfg_init(cfg_opts, CFGF_NONE);

  if (cfg == NULL)
    {
      fprintf(stderr, "Failed to initialize configuration parser\n");

      return -1;
    }

  /* Set up config values validation */
  cfg_set_validate_func(cfg, "theme", config_validate_string);
  cfg_set_validate_func(cfg, "timeout", config_validate_positive_integer);

  /* 
   * Do the actual parsing.
   * If the file does not exist or cannot be opened,
   * we'll be using the default values defined in the cfg_opt_t array.
   */
  ret = cfg_parse(cfg, conffile);
  if (ret != CFG_SUCCESS)
    {
      if (ret == CFG_FILE_ERROR)
	{
	  config_write();
	}
      else
	{
	  cfg_free(cfg);

	  fprintf(stderr, "Failed to parse configuration file\n");

	  return -1;
	}
    }

  /* Fill up the structs */
  mbp_w.timeout = cfg_getint(cfg, "timeout");

  ret = theme_load(cfg_getstr(cfg, "theme"));
  if (ret < 0)
    {
      fprintf(stderr, "Failed to load theme '%s', using '%s' instead\n",
	      cfg_getstr(cfg, "theme"), DEFAULT_THEME);

      ret = theme_load(DEFAULT_THEME);
      if (ret < 0)
	{
	  fprintf(stderr, "Failed to load default theme '%s'\n", DEFAULT_THEME);

	  return -1;
	}
    }

  return 0;
}

int
config_write(void)
{
  FILE *fp;

  fp = fopen(conffile, "w");
  if (fp == NULL)
    {
      fprintf(stderr, "Could not write to config file: %s\n", strerror(errno));

      return -1;
    }

  fprintf(fp, "# gpomme config file\n");
  fprintf(fp, "#  - theme : name of the theme to use\n");
  fprintf(fp, "#  - timeout : time before the window hides\n\n");

  cfg_print(cfg, fp);

  fclose(fp);

  return 0;
}


int
config_monitor(void)
{
  int fd;
  int ret;

  fd = inotify_init();
  if (fd < 0)
    {
      fprintf(stderr, "Error: could not initialize inotify instance: %s\n", strerror(errno));

      return -1;
    }

  ret = fcntl(fd, F_GETFL);
  if (ret < 0)
    {
      close(fd);

      fprintf(stderr, "Error: failed to get inotify fd flags: %s\n", strerror(errno));

      return -1;
    }

  ret = fcntl(fd, F_SETFL, ret | O_NONBLOCK);
  if (ret < 0)
    {
      close(fd);

      fprintf(stderr, "Error: failed to set inotify fd flags: %s\n", strerror(errno));

      return -1;
    }

  ret = inotify_add_watch(fd, conffile, IN_CLOSE_WRITE);
  if (ret < 0)
    {
      close(fd);

      fprintf(stderr, "Error: could not add inotify watch: %s\n", strerror(errno));

      return -1;
    }

  return fd;
}
