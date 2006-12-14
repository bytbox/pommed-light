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

#include <syslog.h>

#include <confuse.h>

#include "mbpeventd.h"
#include "conffile.h"
#include "lcd_backlight.h"
#include "kbd_backlight.h"
#include "cd_eject.h"


struct _lcd_x1600_cfg lcd_x1600_cfg;
struct _lcd_gma950_cfg lcd_gma950_cfg;
#if 0
struct _audio_cfg audio_cfg;
#endif /* 0 */
struct _kbd_cfg kbd_cfg;
struct _eject_cfg eject_cfg;


/* Config file structure */

static cfg_opt_t lcd_x1600_opts[] =
  {
    CFG_INT("init", 200, CFGF_NONE),
    CFG_INT("step", 10, CFGF_NONE),
    CFG_END()
  };

static cfg_opt_t lcd_gma950_opts[] =
  {
    CFG_INT("init", 0x1f, CFGF_NONE),
    CFG_INT("step", 0x0f, CFGF_NONE),
    CFG_END()
  };

#if 0
static cfg_opt_t audio_opts[] =
  {
    CFG_STR("mixer", "foobar", CFGF_NONE),
    CFG_INT("init", 50, CFGF_NONE),
    CFG_INT("step", 10, CFGF_NONE),
    CFG_END()
  };
#endif /* 0 */

static cfg_opt_t kbd_opts[] =
  {
    CFG_INT("default", 100, CFGF_NONE),
    CFG_INT("step", 10, CFGF_NONE),
    CFG_INT("on_threshold", 20, CFGF_NONE),
    CFG_INT("off_threshold", 40, CFGF_NONE),
    CFG_BOOL("auto", 1, CFGF_NONE),
    CFG_END()
  };

static cfg_opt_t eject_opts[] =
  {
    CFG_BOOL("enabled", 1, CFGF_NONE),
    CFG_STR("device", "/dev/dvd", CFGF_NONE),
    CFG_END()
  };

static cfg_opt_t opts[] =
  {
    CFG_SEC("lcd_x1600", lcd_x1600_opts, CFGF_NONE),
    CFG_SEC("lcd_gma950", lcd_gma950_opts, CFGF_NONE),
#if 0
    CFG_SEC("audio", audio_opts, CFGF_NONE),
#endif /* 0 */
    CFG_SEC("kbd", kbd_opts, CFGF_NONE),
    CFG_SEC("eject", eject_opts, CFGF_NONE),
    CFG_END()
  };


static int
config_validate_positive_integer(cfg_t *cfg, cfg_opt_t *opt)
{
  int value = cfg_opt_getnint(opt, cfg_opt_size(opt) - 1);

  if (value < 0)
    {
      cfg_error(cfg, "Error: Value for '%s/%s' must be positive", cfg->name, opt->name);
      return -1;
    }

  return 0;
}

static void
config_print(void)
{
  printf("mbpeventd configuration:\n");
  printf(" + ATI X1600 backlight control:\n");
  printf("    initial level: %d\n", lcd_x1600_cfg.init);
  printf("    step: %d\n", lcd_x1600_cfg.step);
  printf(" + Intel GMA950 backlight control:\n");
  printf("    initial level: 0x%x\n", lcd_gma950_cfg.init);
  printf("    step: 0x%x\n", lcd_gma950_cfg.step);
#if 0
  printf(" + Audio volume control:\n");
  printf("    mixer device: %s\n", audio_cfg.mixer);
  printf("    initial volume: %d\n", audio_cfg.init);
  printf("    step: %d\n", audio_cfg.step);
#endif /* 0 */
  printf(" + Keyboard backlight control:\n");
  printf("    default level: %d\n", kbd_cfg.auto_lvl);
  printf("    step: %d\n", kbd_cfg.step);
  printf("    auto on threshold: %d\n", kbd_cfg.on_thresh);
  printf("    auto off threshold: %d\n", kbd_cfg.off_thresh);
  printf("    auto enable: %s\n", (kbd_cfg.auto_on) ? "yes" : "no");
  printf(" + CD eject:\n");
  printf("    enabled: %s\n", (eject_cfg.enabled) ? "yes" : "no");
  printf("    device: %s\n", eject_cfg.device);
}


int
config_load(void)
{
  cfg_t *cfg;
  cfg_t *sec;

  int ret;

  cfg = cfg_init(opts, CFGF_NONE);

  if (cfg == NULL)
    {
      logmsg(LOG_ERR, "Failed to initialize configuration parser");

      return -1;
    }

  /* Set up config values validation */
  /* lcd_x1600 */
  cfg_set_validate_func(cfg, "lcd_x1600|init", config_validate_positive_integer);
  cfg_set_validate_func(cfg, "lcd_x1600|step", config_validate_positive_integer);
  /* lcd_gma950 */
  cfg_set_validate_func(cfg, "lcd_gma950|init", config_validate_positive_integer);
  cfg_set_validate_func(cfg, "lcd_gma950|step", config_validate_positive_integer);
#if 0
  /* audio */
  cfg_set_validate_func(cfg, "audio|init", config_validate_positive_integer);
  cfg_set_validate_func(cfg, "audio|step", config_validate_positive_integer);
#endif /* 0 */
  /* kbd */
  cfg_set_validate_func(cfg, "kbd|default", config_validate_positive_integer);
  cfg_set_validate_func(cfg, "kbd|step", config_validate_positive_integer);
  cfg_set_validate_func(cfg, "kbd|on_threshold", config_validate_positive_integer);
  cfg_set_validate_func(cfg, "kbd|off_threshold", config_validate_positive_integer);

  /* 
   * Do the actual parsing.
   * If the file does not exist or cannot be opened,
   * we'll be using the default values defined in the cfg_opt_t arrays.
   */
  ret = cfg_parse(cfg, CONFFILE);
  if (ret != CFG_SUCCESS)
    {
      if (ret == CFG_FILE_ERROR)
	{
	  logmsg(LOG_INFO, "Configuration file not found, using defaults");
	}
      else
	{
	  cfg_free(cfg);

	  logmsg(LOG_ERR, "Failed to parse configuration file");

	  return -1;
	}
    }

  /* Fill up the structs */
  sec = cfg_getsec(cfg, "lcd_x1600");
  lcd_x1600_cfg.init = cfg_getint(sec, "init");
  lcd_x1600_cfg.step = cfg_getint(sec, "step");
  x1600_backlight_fix_config();

  sec = cfg_getsec(cfg, "lcd_gma950");
  lcd_gma950_cfg.init = cfg_getint(sec, "init");
  lcd_gma950_cfg.step = cfg_getint(sec, "step");
  /* No _fix_config() call here, as we're hardware-dependent
   * for the max backlight value */

#if 0
  sec = cfg_getsec(cfg, "audio");
  audio_cfg.mixer = strdup(cfg_getstr(sec, "mixer"));
  audio_cfg.init = cfg_getint(sec, "init");
  audio_cfg.step = cfg_getint(sec, "step");
  audio_fix_config();
#endif /* 0 */

  sec = cfg_getsec(cfg, "kbd");
  kbd_cfg.auto_lvl = cfg_getint(sec, "default");
  kbd_cfg.step = cfg_getint(sec, "step");
  kbd_cfg.on_thresh = cfg_getint(sec, "on_threshold");
  kbd_cfg.off_thresh = cfg_getint(sec, "off_threshold");
  kbd_cfg.auto_on = cfg_getbool(sec, "auto");
  kbd_backlight_fix_config();

  sec = cfg_getsec(cfg, "eject");
  eject_cfg.enabled = cfg_getbool(sec, "enabled");
  eject_cfg.device = strdup(cfg_getstr(sec, "device"));
  cd_eject_fix_config();

  cfg_free(cfg);

  if (debug)
    config_print();

  return 0;
}

void
config_cleanup(void)
{
#if 0
  free(audio_cfg.mixer);
#endif /* 0 */
  free(eject_cfg.device);
}
