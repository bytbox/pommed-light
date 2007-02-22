/*
 * gpomme - GTK application for use with pommed
 *
 * $Id$
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
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <libintl.h>

#include <gtk/gtk.h>

#include <dbus/dbus.h>

#include "gpomme.h"
#include "theme.h"
#include "audio.h"

#include "../dbus-client/dbus-client.h"


#define _(str) gettext(str)


struct
{
  GtkWidget *window;     /* The window itself */

  GtkWidget *img_align;  /* Image container */
  GtkWidget *image;      /* Current image, if any */

  GtkWidget *label;      /* Text label */

  GtkWidget *pbar_align; /* Progress bar container */
  GtkWidget *pbar;       /* Progress bar */
  int pbar_state;

  guint timer;
} mbp_w;

struct
{
  int muted;
} mbp;

DBusError dbus_err;
DBusConnection *conn;


/* Timer callback */
gboolean
hide_window(gpointer userdata)
{
  gtk_widget_hide(mbp_w.window);

  mbp_w.timer = 0;

  return FALSE;
}


void
draw_window_bg(void)
{
  GtkWidget *window = mbp_w.window;

  GdkWindow *root_win;
  GdkScreen *screen;
  GdkRectangle mon_size;
  GdkPixbuf *pixbuf = NULL;
  GdkPixmap *pixmap = NULL;

  int x, y;
  int monitor;

  screen = gtk_window_get_screen(GTK_WINDOW(window));

  /* Find which monitor the mouse cursor is on */
  root_win = gdk_screen_get_root_window(screen);
  gdk_window_get_pointer(root_win, &x, &y, NULL);

  monitor = gdk_screen_get_monitor_at_point(screen, x, y);
  gdk_screen_get_monitor_geometry(screen, monitor, &mon_size);

  /* Move the window to the bottom center of the screen */
  x = mon_size.x + (mon_size.width - theme.width) / 2;
  y = mon_size.y + (mon_size.height - 100 - theme.height);

  gtk_window_move(GTK_WINDOW(window), x, y);


  /* Redraw the window background, compositing the background pixmap with
   * the portion of the root window that's beneath the window
   */
  pixbuf = gdk_pixbuf_get_from_drawable(NULL,
					gdk_get_default_root_window(), gdk_colormap_get_system(),
					x, y, 0, 0, theme.width, theme.height);

  /* render the combined pixbuf to a pixmap with alpha control */
  pixmap = gdk_pixmap_new(GTK_WIDGET(window)->window, theme.width, theme.height, -1);
  gdk_draw_pixbuf(pixmap, NULL, pixbuf, 0, 0, 0, 0,
		  theme.width, theme.height, GDK_RGB_DITHER_NONE, 0, 0);
  gdk_draw_pixbuf(pixmap, NULL, theme.background, 0, 0, 0, 0,
		   theme.width, theme.height, GDK_RGB_DITHER_NONE, 0, 0);

  gdk_window_set_back_pixmap(GTK_WIDGET(window)->window, pixmap, FALSE);

  g_object_unref(pixbuf);
  g_object_unref(pixmap);
}

void
show_window(int img, char *label, double fraction)
{
  char *m_label;
  char *u_label;

  GtkWidget *window = mbp_w.window;

  if (img >= IMG_NIMG)
    return;

  /* Cancel timer */
  if (mbp_w.timer > 0)
    g_source_remove(mbp_w.timer);

  if (!GTK_WIDGET_VISIBLE(window))
    draw_window_bg();

  /* Put the appropriate image in there */
  if (mbp_w.image != theme.images[img])
    {
      if (mbp_w.image != NULL)
	gtk_container_remove(GTK_CONTAINER(mbp_w.img_align), mbp_w.image);

      gtk_container_add(GTK_CONTAINER(mbp_w.img_align), theme.images[img]);
    }

  mbp_w.image = theme.images[img];

  /* Set the text label */
  u_label = g_locale_to_utf8(label, -1, NULL, NULL, NULL);

  if (u_label == NULL)
    m_label = "";
  else /* accepts only UTF-8 input ... segfaults otherwise */
    m_label = g_markup_printf_escaped("<span weight=\"bold\" foreground=\"white\">%s</span>", u_label);

  gtk_label_set_markup(GTK_LABEL(mbp_w.label), m_label);

  if (u_label != NULL)
    {
      g_free(u_label);
      g_free(m_label);
    }

  /* Set the progress bar */
  if (fraction >= 0.0)
    {
      gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(mbp_w.pbar), fraction);

      if (!mbp_w.pbar_state)
	{
	  gtk_container_add(GTK_CONTAINER(mbp_w.pbar_align), mbp_w.pbar);
	  mbp_w.pbar_state = 1;
	}
    }
  else if (mbp_w.pbar_state)
    {
      gtk_container_remove(GTK_CONTAINER(mbp_w.pbar_align), mbp_w.pbar);
      mbp_w.pbar_state = 0;
    }

  gtk_widget_show_all(window);

  mbp_w.timer = g_timeout_add(900, hide_window, NULL);
}


void
create_window(void)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *align;

  window = gtk_window_new(GTK_WINDOW_POPUP);
  gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
  gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, FALSE);
  gtk_widget_set_app_paintable(GTK_WIDGET(window), TRUE);
  gtk_widget_realize (GTK_WIDGET(window));

  gtk_window_set_default_size(GTK_WINDOW(window), theme.width, theme.height);
  gtk_widget_set_size_request(GTK_WIDGET(window), theme.width, theme.height);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  /* Image */
  mbp_w.img_align = gtk_alignment_new(0.5, 0.7, 0.0, 0.0);
  gtk_box_pack_start(GTK_BOX(vbox), mbp_w.img_align, TRUE, TRUE, 0);

  /* Text message */
  align = gtk_alignment_new(0.5, 0.0, 0.0, 0.0);
  gtk_box_pack_start(GTK_BOX(vbox), align, TRUE, TRUE, 0);
	
  mbp_w.label = gtk_label_new("");
  gtk_container_add(GTK_CONTAINER(align), mbp_w.label);

  /* Progress bar */
  mbp_w.pbar_align = gtk_alignment_new(0.5, 0.0, 0.0, 0.0);
  gtk_box_pack_start(GTK_BOX(vbox), mbp_w.pbar_align, TRUE, TRUE, 0);

  mbp_w.pbar = gtk_progress_bar_new();
  /* make it 10px high */
  gtk_widget_set_size_request(mbp_w.pbar, -1, 10);
  gtk_container_add(GTK_CONTAINER(mbp_w.pbar_align), mbp_w.pbar);
  /* Up the refcount to prevent GTK from freeing the widget */
  gtk_widget_ref(mbp_w.pbar);

  mbp_w.pbar_state = 1;
  mbp_w.window = window;
  mbp_w.image = NULL;
  mbp_w.timer = 0;
}


void
audio_getmute_cb(DBusPendingCall *pending, void *status)
{
  DBusMessage *msg;

  msg = dbus_pending_call_steal_reply(pending);

  if (msg == NULL)
    {
      printf("Could not steal reply\n");

      dbus_pending_call_unref(pending);

      return;
    }

  dbus_pending_call_unref(pending);

  if (!mbp_dbus_check_error(msg))
    {
      dbus_message_get_args(msg, &dbus_err,
			    DBUS_TYPE_BOOLEAN, &mbp.muted,
			    DBUS_TYPE_INVALID);
    }
  else
    *(int *)status = -1;

  dbus_message_unref(msg);
}


int
mbp_dbus_connect(void)
{
  unsigned int signals;
  int ret;
  int cbret;

  signals = MBP_DBUS_SIG_LCD | MBP_DBUS_SIG_KBD
    | MBP_DBUS_SIG_VOL | MBP_DBUS_SIG_MUTE
    | MBP_DBUS_SIG_EJECT;

  conn = mbp_dbus_init(&dbus_err, signals);

  if (conn == NULL)
    return -1;

  ret = mbp_call_audio_getmute(audio_getmute_cb, &cbret);
  if ((ret < 0) || (cbret < 0))
    {
      printf("audio getMute call failed !\n");
    }

  return 0;
}


gboolean
mbp_dbus_listen(gpointer userdata)
{
  DBusMessage *msg;

  int ret;

  int scratch;
  int cur;
  int max;
  int who;
  double ratio;

  /* Disconnected, try to reconnect */
  if (conn == NULL)
    {
      ret = mbp_dbus_connect();

      if (ret < 0)
	return TRUE;
    }

  while (1)
    {
      dbus_connection_read_write(conn, 0);

      msg = dbus_connection_pop_message(conn);

      if (msg == NULL)
	return TRUE;

      if (dbus_message_is_signal(msg, "org.pommed.signal.lcdBacklight", "lcdBacklight"))
	{
	  dbus_message_get_args(msg, &dbus_err,
				DBUS_TYPE_UINT32, &cur,
				DBUS_TYPE_UINT32, &scratch, /* previous */
				DBUS_TYPE_UINT32, &max,
				DBUS_TYPE_INVALID);

	  ratio = (double)cur / (double)max;

	  show_window(IMG_LCD_BCK, _("LCD backlight level"), ratio);
	}
      else if (dbus_message_is_signal(msg, "org.pommed.signal.kbdBacklight", "kbdBacklight"))
	{
	  dbus_message_get_args(msg, &dbus_err,
				DBUS_TYPE_UINT32, &cur,
				DBUS_TYPE_UINT32, &scratch, /* previous */
				DBUS_TYPE_UINT32, &max,
				DBUS_TYPE_UINT32, &who,
				DBUS_TYPE_INVALID);

	  if (who == KBD_USER)
	    {
	      ratio = (double)cur / (double)max;

	      show_window(IMG_KBD_BCK, _("Keyboard backlight level"), ratio);
	    }
	}
      else if (dbus_message_is_signal(msg, "org.pommed.signal.audioVolume", "audioVolume"))
	{
	  dbus_message_get_args(msg, &dbus_err,
				DBUS_TYPE_UINT32, &cur,
				DBUS_TYPE_UINT32, &scratch, /* previous */
				DBUS_TYPE_UINT32, &max,
				DBUS_TYPE_INVALID);

	  ratio = (double)cur / (double)max;

	  if (!mbp.muted)
	    {
	      show_window(IMG_AUDIO_VOL_ON, _("Sound volume"), ratio);
	      audio_command(AUDIO_CLICK);
	    }
	  else
	    show_window(IMG_AUDIO_VOL_OFF, _("Sound volume (muted)"), ratio);
	}
      else if (dbus_message_is_signal(msg, "org.pommed.signal.audioMute", "audioMute"))
	{
	  dbus_message_get_args(msg, &dbus_err,
				DBUS_TYPE_BOOLEAN, &mbp.muted,
				DBUS_TYPE_INVALID);

	  if (mbp.muted)
	    show_window(IMG_AUDIO_MUTE, _("Sound muted"), -1.0);
	  else
	    show_window(IMG_AUDIO_MUTE, _("Sound unmuted"), -1.0);
	}
      else if (dbus_message_is_signal(msg, "org.pommed.signal.cdEject", "cdEject"))
	{
	  show_window(IMG_CD_EJECT, _("Eject"), -1.0);
	}
      else if (dbus_message_is_signal(msg, DBUS_INTERFACE_LOCAL, "Disconnected"))
	{
	  printf("DBus disconnected\n");

	  mbp_dbus_cleanup();

	  dbus_message_unref(msg);

	  break;
	}

      dbus_message_unref(msg);
    }

  return TRUE;
}


static void
usage(void)
{
  printf("gpomme v" M_VERSION " ($Rev$) graphical client for pommed\n");
  printf("Copyright (C) 2006 Julien BLACHE <jb@jblache.org> and others\n");

  printf("Usage:\n");
  printf("\tgpomme\t\t-- start gpomme with the default theme\n");
  printf("\tgpomme -v\t-- print version and exit\n");
  printf("\tgpomme -t Tango\t-- start gpomme with the Tango theme\n");
}


void sig_int_term_handler(int signo)
{
  gtk_main_quit();
}

int main(int argc, char **argv)
{
  int c;
  int ret;
  char *theme_name;

  theme_name = DEFAULT_THEME;

  while ((c = getopt(argc, argv, "t:v")) != -1)
    {
      switch (c)
	{
	  case 't':
	    theme_name = optarg;
	    break;

	  case 'v':
	    printf("gpomme v" M_VERSION " ($Rev$) graphical client for pommed\n");
	    printf("Copyright (C) 2006 Julien BLACHE <jb@jblache.org> and others\n");

	    exit(0);
	    break;

	  default:
	    usage();

	    exit(-1);
	    break;
	}
    }

  mbp_dbus_connect();

  signal(SIGINT, sig_int_term_handler);
  signal(SIGTERM, sig_int_term_handler);

  bindtextdomain("gpomme", "/usr/share/locale");
  textdomain("gpomme");

  ret = audio_init_thread();
  if (ret < 0)
    printf("Failed to create audio thread\n");

  gtk_init(&argc, &argv);

  ret = theme_load(theme_name);

  if (ret < 0)
    {
      fprintf(stderr, "Failed to load theme '%s'\n", theme_name);

      exit(1);
    }

  create_window();

  g_timeout_add(100, mbp_dbus_listen, NULL);

  gtk_main();

  audio_command(AUDIO_COMMAND_QUIT);
  audio_cleanup();

  mbp_dbus_cleanup();

  return 0;
}
