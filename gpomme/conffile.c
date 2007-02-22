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

#include <confuse.h>

#include "gpomme.h"
#include "theme.h"

/*
 * TODO
 *  - print config with cfg_print() (need to create the path if it doesn't exist)
 *  - build config file path
 */

#define CONFFILE "/dev/null"

static cfg_opt_t cfg_opts[] =
  {
    CFG_STR("theme", DEFAULT_THEME, CFGF_NONE),
    CFG_INT("timeout", 900, CFGF_NONE),
    CFG_END()
  };


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
  cfg_t *cfg;

  int ret;

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
  ret = cfg_parse(cfg, CONFFILE);
  if ((ret != CFG_SUCCESS) && (ret != CFG_FILE_ERROR))
    {
      cfg_free(cfg);

      fprintf(stderr, "Failed to parse configuration file\n");

      return -1;
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

  cfg_free(cfg);

  return 0;
}
