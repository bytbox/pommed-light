/*
 * pommed - Apple laptops hotkeys handler daemon
 *
 * $Id$
 *
 * Copyright (C) 2006-2008 Julien BLACHE <jb@jblache.org>
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

#include "pommed.h"
#include "conffile.h"
#include "lcd_backlight.h"
#include "kbd_backlight.h"
#include "cd_eject.h"
#include "beep.h"
#include "audio.h"


struct _general_cfg general_cfg;
struct _lcd_sysfs_cfg lcd_sysfs_cfg;
#ifndef __powerpc__
struct _lcd_x1600_cfg lcd_x1600_cfg;
struct _lcd_gma950_cfg lcd_gma950_cfg;
struct _lcd_nv8600mgt_cfg lcd_nv8600mgt_cfg;
#endif
struct _audio_cfg audio_cfg;
struct _kbd_cfg kbd_cfg;
struct _eject_cfg eject_cfg;
struct _beep_cfg beep_cfg;
#ifndef __powerpc__
struct _appleir_cfg appleir_cfg;
#endif


/* Config file structure */
static cfg_opt_t general_opts[] =
  {
    CFG_INT("fnmode", 1, CFGF_NONE),
    CFG_END()
  };

static cfg_opt_t lcd_sysfs_opts[] =
  {
    CFG_INT("init", -1, CFGF_NONE),
    CFG_INT("step", 8, CFGF_NONE),
    CFG_INT("on_batt", 0, CFGF_NONE),
    CFG_END()
  };


#ifndef __powerpc__
static cfg_opt_t lcd_x1600_opts[] =
  {
    CFG_INT("init", -1, CFGF_NONE),
    CFG_INT("step", 10, CFGF_NONE),
    CFG_INT("on_batt", 0, CFGF_NONE),
    CFG_END()
  };

static cfg_opt_t lcd_gma950_opts[] =
  {
    CFG_INT("init", -1, CFGF_NONE),
    CFG_INT("step", 0x0f, CFGF_NONE),
    CFG_INT("on_batt", 0, CFGF_NONE),
    CFG_END()
  };

static cfg_opt_t lcd_nv8600mgt_opts[] =
  {
    CFG_INT("init", -1, CFGF_NONE),
    CFG_INT("step", 1, CFGF_NONE),
    CFG_INT("on_batt", 0, CFGF_NONE),
    CFG_END()
  };
#endif /* !__powerpc__ */


static cfg_opt_t audio_opts[] =
  {
    CFG_STR("card", "default", CFGF_NONE),
    CFG_INT("init", -1, CFGF_NONE),
    CFG_INT("step", 10, CFGF_NONE),
    CFG_BOOL("beep", 1, CFGF_NONE),
    CFG_STR("volume", "PCM", CFGF_NONE),
    CFG_STR("speakers", "Front", CFGF_NONE),
    CFG_STR("headphones", "Headphone", CFGF_NONE),
    CFG_END()
  };

static cfg_opt_t kbd_opts[] =
  {
    CFG_INT("default", 100, CFGF_NONE),
    CFG_INT("step", 10, CFGF_NONE),
    CFG_INT("on_threshold", 20, CFGF_NONE),
    CFG_INT("off_threshold", 40, CFGF_NONE),
    CFG_BOOL("auto", 1, CFGF_NONE),
    CFG_INT("idle_timer", 60, CFGF_NONE),
    CFG_END()
  };

static cfg_opt_t eject_opts[] =
  {
    CFG_BOOL("enabled", 1, CFGF_NONE),
    CFG_STR("device", "/dev/dvd", CFGF_NONE),
    CFG_END()
  };

static cfg_opt_t beep_opts[] =
  {
    CFG_BOOL("enabled", 0, CFGF_NONE),
    CFG_STR("beepfile", BEEP_DEFAULT_FILE, CFGF_NONE),
    CFG_END()
  };

#ifndef __powerpc__
static cfg_opt_t appleir_opts[] =
  {
    CFG_BOOL("enabled", 0, CFGF_NONE),
    CFG_END()
  };
#endif /* !__powerpc__ */

static cfg_opt_t opts[] =
  {
    CFG_SEC("general", general_opts, CFGF_NONE),
    CFG_SEC("lcd_sysfs", lcd_sysfs_opts, CFGF_NONE),
#ifndef __powerpc__
    CFG_SEC("lcd_x1600", lcd_x1600_opts, CFGF_NONE),
    CFG_SEC("lcd_gma950", lcd_gma950_opts, CFGF_NONE),
    CFG_SEC("lcd_nv8600mgt", lcd_nv8600mgt_opts, CFGF_NONE),
#endif
    CFG_SEC("audio", audio_opts, CFGF_NONE),
    CFG_SEC("kbd", kbd_opts, CFGF_NONE),
    CFG_SEC("eject", eject_opts, CFGF_NONE),
    CFG_SEC("beep", beep_opts, CFGF_NONE),
#ifndef __powerpc__
    CFG_SEC("appleir", appleir_opts, CFGF_NONE),
#endif
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

static int
config_validate_string(cfg_t *cfg, cfg_opt_t *opt)
{
  char *value = cfg_opt_getnstr(opt, cfg_opt_size(opt) - 1);

  if (strlen(value) == 0)
    {
      cfg_error(cfg, "Error: Value for '%s/%s' must be a non-zero string", cfg->name, opt->name);
      return -1;
    }

  return 0;
}


static void
config_print(void)
{
  printf("pommed configuration:\n");
  printf(" + General settings:\n");
  printf("    fnmode: %d\n", general_cfg.fnmode);
  printf(" + sysfs backlight control:\n");
  printf("    initial level: %d\n", lcd_sysfs_cfg.init);
  printf("    step: %d\n", lcd_sysfs_cfg.step);
  printf("    on_batt: %d\n", lcd_sysfs_cfg.on_batt);
#ifndef __powerpc__
  printf(" + ATI X1600 backlight control:\n");
  printf("    initial level: %d\n", lcd_x1600_cfg.init);
  printf("    step: %d\n", lcd_x1600_cfg.step);
  printf("    on_batt: %d\n", lcd_x1600_cfg.on_batt);
  printf(" + Intel GMA950 backlight control:\n");
  printf("    initial level: 0x%x\n", lcd_gma950_cfg.init);
  printf("    step: 0x%x\n", lcd_gma950_cfg.step);
  printf("    on_batt: 0x%x\n", lcd_gma950_cfg.on_batt);
  printf(" + nVidia GeForce 8600M GT backlight control:\n");
  printf("    initial level: %d\n", lcd_nv8600mgt_cfg.init);
  printf("    step: %d\n", lcd_nv8600mgt_cfg.step);
  printf("    on_batt: %d\n", lcd_nv8600mgt_cfg.on_batt);
#endif /* !__powerpc__ */
  printf(" + Audio volume control:\n");
  printf("    card: %s\n", audio_cfg.card);
  printf("    initial volume: %d%s\n", audio_cfg.init, (audio_cfg.init > -1) ? "%" : "");
  printf("    step: %d%%\n", audio_cfg.step);
  printf("    beep: %s\n", (audio_cfg.beep) ? "yes" : "no");
  printf("    volume element: %s\n", audio_cfg.vol);
  printf("    speaker element: %s\n", audio_cfg.spkr);
  printf("    headphones element: %s\n", audio_cfg.head);
  printf(" + Keyboard backlight control:\n");
  printf("    default level: %d\n", kbd_cfg.auto_lvl);
  printf("    step: %d\n", kbd_cfg.step);
  printf("    auto on threshold: %d\n", kbd_cfg.on_thresh);
  printf("    auto off threshold: %d\n", kbd_cfg.off_thresh);
  printf("    auto enable: %s\n", (kbd_cfg.auto_on) ? "yes" : "no");
  printf("    idle timer: %d%s\n", (kbd_cfg.idle * KBD_TIMEOUT) / 1000, (kbd_cfg.idle > 0) ? "s" : "");
  printf(" + CD eject:\n");
  printf("    enabled: %s\n", (eject_cfg.enabled) ? "yes" : "no");
  printf("    device: %s\n", eject_cfg.device);
  printf(" + Beep:\n");
  printf("    enabled: %s\n", (beep_cfg.enabled) ? "yes" : "no");
  printf("    beepfile: %s\n", beep_cfg.beepfile);
#ifndef __powerpc__
  printf(" + Apple Remote IR Receiver:\n");
  printf("    enabled: %s\n", (appleir_cfg.enabled) ? "yes" : "no");
#endif /* !__powerpc__ */
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
  /* general */
  cfg_set_validate_func(cfg, "general|fnmode", config_validate_positive_integer);
  /* lcd_sysfs */
  cfg_set_validate_func(cfg, "lcd_sysfs|step", config_validate_positive_integer);
  cfg_set_validate_func(cfg, "lcd_sysfs|on_batt", config_validate_positive_integer);
#ifndef __powerpc__
  /* lcd_x1600 */
  cfg_set_validate_func(cfg, "lcd_x1600|step", config_validate_positive_integer);
  cfg_set_validate_func(cfg, "lcd_x1600|on_batt", config_validate_positive_integer);
  /* lcd_gma950 */
  cfg_set_validate_func(cfg, "lcd_gma950|step", config_validate_positive_integer);
  cfg_set_validate_func(cfg, "lcd_gma950|on_batt", config_validate_positive_integer);
  /* lcd_nv8600mgt */
  cfg_set_validate_func(cfg, "lcd_nv8600mgt|step", config_validate_positive_integer);
  cfg_set_validate_func(cfg, "lcd_nv8600mgt|on_batt", config_validate_positive_integer);
#endif /* !__powerpc__ */
  /* audio */
  cfg_set_validate_func(cfg, "audio|card", config_validate_string);
  cfg_set_validate_func(cfg, "audio|step", config_validate_positive_integer);
  cfg_set_validate_func(cfg, "audio|volume", config_validate_string);
  cfg_set_validate_func(cfg, "audio|speakers", config_validate_string);
  cfg_set_validate_func(cfg, "audio|headphones", config_validate_string);
  /* kbd */
  cfg_set_validate_func(cfg, "kbd|default", config_validate_positive_integer);
  cfg_set_validate_func(cfg, "kbd|step", config_validate_positive_integer);
  cfg_set_validate_func(cfg, "kbd|on_threshold", config_validate_positive_integer);
  cfg_set_validate_func(cfg, "kbd|off_threshold", config_validate_positive_integer);
  /* CD eject */
  cfg_set_validate_func(cfg, "eject|device", config_validate_string);
  /* beep */
  cfg_set_validate_func(cfg, "beep|beepfile", config_validate_string);

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
  sec = cfg_getsec(cfg, "general");
  general_cfg.fnmode = cfg_getint(sec, "fnmode");

  sec = cfg_getsec(cfg, "lcd_sysfs");
  lcd_sysfs_cfg.init = cfg_getint(sec, "init");
  lcd_sysfs_cfg.step = cfg_getint(sec, "step");
  lcd_sysfs_cfg.on_batt = cfg_getint(sec, "on_batt");
  /* No _fix_config() call here, it's done at probe time */
#ifndef __powerpc__
  sec = cfg_getsec(cfg, "lcd_x1600");
  lcd_x1600_cfg.init = cfg_getint(sec, "init");
  lcd_x1600_cfg.step = cfg_getint(sec, "step");
  lcd_x1600_cfg.on_batt = cfg_getint(sec, "on_batt");
  x1600_backlight_fix_config();

  sec = cfg_getsec(cfg, "lcd_gma950");
  lcd_gma950_cfg.init = cfg_getint(sec, "init");
  lcd_gma950_cfg.step = cfg_getint(sec, "step");
  lcd_gma950_cfg.on_batt = cfg_getint(sec, "on_batt");
  /* No _fix_config() call here, as we're hardware-dependent
   * for the max backlight value */

  sec = cfg_getsec(cfg, "lcd_nv8600mgt");
  lcd_nv8600mgt_cfg.init = cfg_getint(sec, "init");
  lcd_nv8600mgt_cfg.step = cfg_getint(sec, "step");
  lcd_nv8600mgt_cfg.on_batt = cfg_getint(sec, "on_batt");
  nv8600mgt_backlight_fix_config();
#endif /* !__powerpc__ */

  sec = cfg_getsec(cfg, "audio");
  audio_cfg.card = strdup(cfg_getstr(sec, "card"));
  audio_cfg.init = cfg_getint(sec, "init");
  audio_cfg.step = cfg_getint(sec, "step");
  audio_cfg.beep = cfg_getbool(sec, "beep");
  audio_cfg.vol = strdup(cfg_getstr(sec, "volume"));
  audio_cfg.spkr = strdup(cfg_getstr(sec, "speakers"));
  audio_cfg.head = strdup(cfg_getstr(sec, "headphones"));
  audio_fix_config();

  sec = cfg_getsec(cfg, "kbd");
  kbd_cfg.auto_lvl = cfg_getint(sec, "default");
  kbd_cfg.step = cfg_getint(sec, "step");
  kbd_cfg.on_thresh = cfg_getint(sec, "on_threshold");
  kbd_cfg.off_thresh = cfg_getint(sec, "off_threshold");
  kbd_cfg.auto_on = cfg_getbool(sec, "auto");
  kbd_cfg.idle = cfg_getint(sec, "idle_timer");
  kbd_backlight_fix_config();

  sec = cfg_getsec(cfg, "eject");
  eject_cfg.enabled = cfg_getbool(sec, "enabled");
  eject_cfg.device = strdup(cfg_getstr(sec, "device"));
  cd_eject_fix_config();

  sec = cfg_getsec(cfg, "beep");
  beep_cfg.enabled = cfg_getbool(sec, "enabled");
  beep_cfg.beepfile = strdup(cfg_getstr(sec, "beepfile"));
  beep_fix_config();

#ifndef __powerpc__
  sec = cfg_getsec(cfg, "appleir");
  appleir_cfg.enabled = cfg_getbool(sec, "enabled");
#endif

  cfg_free(cfg);

  if (console)
    config_print();

  return 0;
}

void
config_cleanup(void)
{
  free(audio_cfg.card);
  free(audio_cfg.vol);
  free(audio_cfg.spkr);
  free(audio_cfg.head);

  free(eject_cfg.device);

  free(beep_cfg.beepfile);
}
