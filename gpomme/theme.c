/*
 * gpomme - GTK application for use with pommed
 *
 * Copyright (C) 2006 Soeren SONNENBURG <debian@nn7.de>
 * Copyright (C) 2006-2007 Julien BLACHE <jb@jblache.org>
 *
 * Portions of the GTK code below were shamelessly
 * stolen from pbbuttonsd. Thanks ! ;-)
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

#include <gtk/gtk.h>

#include "gpomme.h"
#include "theme.h"


struct gpomme_theme theme;


static GtkWidget *
load_image(const char *name, const char *img)
{
  GError *error = NULL;
  GdkPixbuf *pixbuf;

  char file[PATH_MAX];
  int ret;

  ret = snprintf(file, PATH_MAX, "%s/%s/%s", THEME_BASE, name, img);
  if (ret >= PATH_MAX)
    return NULL;

  pixbuf = gdk_pixbuf_new_from_file(file, &error);

  if (error != NULL)
    {
      printf("Error loading theme file %s: %s\n", name, error->message);

      g_error_free(error);
      return NULL;
    }

  return gtk_image_new_from_pixbuf(pixbuf);
}

int
theme_load(const char *name)
{
  GError *error = NULL;

  char file[PATH_MAX];
  int i;
  int ret;

  ret = snprintf(file, PATH_MAX, "%s/%s/background.png", THEME_BASE, name);
  if (ret >= PATH_MAX)
    return -1;

  if (theme.background)
    g_object_unref(G_OBJECT(theme.background));

  theme.background = gdk_pixbuf_new_from_file(file, &error);

  if (error != NULL)
    {
      printf("Error loading theme background: %s\n", error->message);

      g_error_free(error);
      return -1;
    }

  theme.width = gdk_pixbuf_get_width (theme.background);
  theme.height = gdk_pixbuf_get_height (theme.background);

  /*
   * We need to up the refcount to prevent GTK from destroying
   * the images by itself when we start adding/removing them
   * to/from a GtkContainer.
   */

  for (i = 0; i < IMG_NIMG; i++)
    {
      if (theme.images[i])
	g_object_unref(G_OBJECT(theme.images[i]));
    }

  theme.images[IMG_LCD_BCK] = load_image(name, "brightness.png");
  theme.images[IMG_KBD_BCK] = load_image(name, "kbdlight.png");
  theme.images[IMG_AUDIO_VOL_ON] = load_image(name, "volume.png");
  theme.images[IMG_AUDIO_VOL_OFF] = load_image(name, "mute.png");
  theme.images[IMG_AUDIO_MUTE] = load_image(name, "noaudio.png");
  theme.images[IMG_CD_EJECT] = load_image(name, "cdrom.png");

  for (i = 0; i < IMG_NIMG; i++)
    {
      g_object_ref(G_OBJECT(theme.images[i]));
    }

  return 0;
}
