/*
 * gpomme - GTK application for use with pommed
 *
 * Copyright (C) 2007 Julien BLACHE <jb@jblache.org>
 * Copyright (C) 2007 daniel g. siegel <dgsiegel@gmail.com>
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
#include <stdlib.h>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <pwd.h>
#include <dirent.h>
#include <libintl.h>

#ifndef NO_SYS_INOTIFY_H
# include <sys/inotify.h>
#else
# include <linux/inotify.h>
# include "inotify-syscalls.h"
#endif

#include <confuse.h>

#include "conffile.h"
#include "gpomme.h"
#include "theme.h"

#include <gtk/gtk.h>
#include <glade/glade.h>


#define _(str) gettext(str)

#define GLADE_FILE  "/usr/share/gpomme/gpomme.glade" // FIXME: check for path
#define CONFFILE    "/.gpommerc"


static cfg_opt_t cfg_opts[] =
  {
    CFG_STR("theme", DEFAULT_THEME, CFGF_NONE),
    CFG_INT("timeout", 900, CFGF_NONE),
    CFG_END()
  };

GladeXML *gxml;     
GtkWidget *app_window; 

void
on_gpomme_window_close_cb (GtkWidget *widget, gpointer user_data);

void
update_gui_config (void);


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



void 
config_gui(void)
{
  GtkWidget *cb_theme;
  GtkWidget *hs_timeout;
  struct dirent **namelist; 
  int n; 

  glade_init();

  /* initialize glade and the window */
  gxml = glade_xml_new(GLADE_FILE, NULL, NULL);
  app_window = glade_xml_get_widget(gxml, "gpomme_window");

  /* setting the strings (for translation) */
  gtk_window_set_title(GTK_WINDOW(app_window), _("gpomme preferences"));

  GtkWidget *s;
  s = glade_xml_get_widget(gxml, "lb_theme");
  gtk_label_set_text(GTK_LABEL(s), _("Theme:"));

  s = glade_xml_get_widget(gxml, "lb_timeout");
  gtk_label_set_text(GTK_LABEL(s), _("Timeout (seconds):"));

  /* set the default settings */
  hs_timeout = glade_xml_get_widget(gxml, "hs_timeout");
  gtk_range_set_value(GTK_RANGE(hs_timeout), (gdouble)cfg_getint(cfg, "timeout") / 1000.0);

  /* TODO: check for theme-previews */
  cb_theme = glade_xml_get_widget(gxml, "cb_theme");
  gtk_combo_box_remove_text(GTK_COMBO_BOX(cb_theme), 0); /* remove dummy-text */

  gtk_combo_box_append_text(GTK_COMBO_BOX(cb_theme), cfg_getstr(cfg, "theme"));
  gtk_combo_box_set_active(GTK_COMBO_BOX(cb_theme), 0);

  n = scandir(THEME_BASE, &namelist, 0, alphasort); 

  if (n < 0)
    {
      fprintf(stderr, "Could not open theme directory: %s\n", strerror(errno));

      exit(1);
    }

  while(n--)
    {
      if (strcmp(namelist[n]->d_name, cfg_getstr(cfg, "theme"))
	  && (namelist[n]->d_name[0] != '.'))
	{
	  /* printf("%s\n", namelist[n]->d_name); */
	  gtk_combo_box_append_text(GTK_COMBO_BOX(cb_theme), namelist[n]->d_name); 
	}
    }

  /* signals... */
  glade_xml_signal_connect(gxml, "on_bt_close_clicked",         
			   G_CALLBACK(on_gpomme_window_close_cb));

  glade_xml_signal_connect(gxml, "on_gpomme_window_close",         
			   G_CALLBACK(on_gpomme_window_close_cb));

  glade_xml_signal_connect(gxml, "on_gpomme_window_destroy",         
			   G_CALLBACK(on_gpomme_window_close_cb));

  glade_xml_signal_connect(gxml, "on_hs_timeout_value_changed",         
			   G_CALLBACK(update_gui_config));

  glade_xml_signal_connect(gxml, "on_cb_theme_changed",         
			   G_CALLBACK(update_gui_config));

  gtk_widget_show(app_window);

  gtk_main();
}

/* window is closed, so write the settings to the config-file */
void
on_gpomme_window_close_cb (GtkWidget *widget, gpointer user_data)
{
  update_gui_config();
  
  gtk_widget_hide(app_window);
  gtk_main_quit();
}

void
update_gui_config(void)
{
  GtkWidget *cb_themes;
  GtkWidget *hs_timeout;

  /* get the actual settings */
  hs_timeout = glade_xml_get_widget(gxml, "hs_timeout");
  cb_themes = glade_xml_get_widget(gxml, "cb_theme");

  gdouble timeout = gtk_range_get_value(GTK_RANGE(hs_timeout)) * 1000.0;
  //g_print("setting timeout to %gs\n", timeout);
  cfg_setint(cfg, "timeout", timeout);

  //g_print("setting theme to %s\n", gtk_combo_box_get_active_text(GTK_COMBO_BOX(cb_themes)));
  cfg_setstr(cfg, "theme", gtk_combo_box_get_active_text(GTK_COMBO_BOX(cb_themes)));

  /* actually write them */
  config_write();
}
