/*
 * gpomme - GTK application for use with pommed
 *
 * Copyright (C) 2006, 2008 Soeren SONNENBURG <debian@nn7.de>
 * Copyright (C) 2006-2008 Julien BLACHE <jb@jblache.org>
 * Copyright (C) 2007 daniel g. siegel <dgsiegel@gmail.com>
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

#include <sys/types.h>
#include <sys/wait.h>

#ifndef NO_SYS_INOTIFY_H
# include <sys/inotify.h>
#else
# include <linux/inotify.h>
# include "inotify-syscalls.h"
#endif

#include <libintl.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "gpomme.h"
#include "theme.h"
#include "conffile.h"

#include "../client-common/dbus-client.h"
#include "../client-common/video-client.h"


#define _(str) gettext(str)


struct _mbp_w mbp_w;

struct
{
  int muted;
} mbp;

DBusError dbus_err;
DBusConnection *conn;

dbus_uint32_t mute_serial = 0;


/* Timer callback */
static gboolean
hide_window(gpointer userdata)
{
  gtk_widget_hide(mbp_w.window);

  mbp_w.timer = 0;

  return FALSE;
}

static gboolean
window_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
  GdkRegion *region;
  GtkWidget *child;
  cairo_t *cr;

  /* get our child (in this case, the event box) */ 
  child = gtk_bin_get_child (GTK_BIN (widget));

  /* create a cairo context to draw to the window */
  cr = gdk_cairo_create (widget->window);

  /* the source data is the (composited) event box */
  gdk_cairo_set_source_pixmap (cr, child->window,
			       child->allocation.x,
			       child->allocation.y);

  /* draw no more than our expose event intersects our child */
  region = gdk_region_rectangle (&child->allocation);
  gdk_region_intersect (region, event->region);
  gdk_cairo_region (cr, region);
  cairo_clip (cr);

  /* composite, with a 50% opacity */
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_paint_with_alpha (cr, 1.0);

  /* we're done */
  cairo_destroy (cr);

  return FALSE;
}

static void
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
  GdkGC *gc = gdk_gc_new(pixmap);
  gdk_draw_rectangle(pixmap, gc, TRUE, 0, 0, theme.width, theme.height);
  if (!gdk_screen_is_composited(screen))
  {
    gdk_draw_pixbuf(pixmap, NULL, pixbuf, 0, 0, 0, 0,
		    theme.width, theme.height, GDK_RGB_DITHER_NONE, 0, 0);
  }

  gdk_draw_pixbuf(pixmap, NULL, theme.background, 0, 0, 0, 0,
		  theme.width, theme.height, GDK_RGB_DITHER_NONE, 0, 0);
  gdk_window_set_back_pixmap(GTK_WIDGET(window)->window, pixmap, FALSE);

  g_object_unref(pixbuf);
  g_object_unref(pixmap);
}

static void
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

  mbp_w.timer = g_timeout_add(mbp_w.timeout, hide_window, NULL);
}


static void
create_window(void)
{
  GdkScreen *screen;
  GdkColormap *rgba;

  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *align;

  window = gtk_window_new(GTK_WINDOW_POPUP);
  screen = gtk_widget_get_screen (window);

  if (gdk_screen_is_composited (screen))
    {
      rgba = gdk_screen_get_rgba_colormap (screen);
      gtk_widget_set_colormap (window, rgba);
    }

  gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
  gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, FALSE);
  gtk_widget_set_app_paintable(GTK_WIDGET(window), TRUE);

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

  gtk_widget_realize(GTK_WIDGET(window));

  if (gdk_screen_is_composited(screen))
    {
      gdk_window_set_opacity(window->window, 0.8);
      g_signal_connect_after(window, "expose-event", G_CALLBACK(window_expose_event), NULL);
    }
}


static gboolean
mbp_dbus_reconnect(gpointer userdata);

static DBusHandlerResult
mbp_dbus_listen(DBusConnection *lconn, DBusMessage *msg, gpointer userdata)
{
  int scratch;
  int cur;
  int max;
  int who;
  double ratio;

  Display *dpy;

  if (dbus_message_is_signal(msg, "org.pommed.signal.lcdBacklight", "lcdBacklight"))
    {
      dbus_message_get_args(msg, &dbus_err,
			    DBUS_TYPE_UINT32, &cur,
			    DBUS_TYPE_UINT32, &scratch, /* previous */
			    DBUS_TYPE_UINT32, &max,
			    DBUS_TYPE_UINT32, &who,
			    DBUS_TYPE_INVALID);

      if (who == LCD_USER)
	{
	  ratio = (double)cur / (double)max;

	  show_window(IMG_LCD_BCK, _("LCD backlight level"), ratio);
	}
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
	show_window(IMG_AUDIO_VOL_ON, _("Sound volume"), ratio);
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
  else if (dbus_message_is_signal(msg, "org.pommed.signal.videoSwitch", "videoSwitch"))
    {
      dpy = GDK_WINDOW_XDISPLAY(GTK_WIDGET(mbp_w.window)->window);
      mbp_video_switch(dpy);
    }
  else if (dbus_message_is_signal(msg, DBUS_INTERFACE_LOCAL, "Disconnected"))
    {
      printf("DBus disconnected\n");

      mbp_dbus_cleanup();

      g_timeout_add(200, mbp_dbus_reconnect, NULL);
    }
  else
    {
      if ((dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_METHOD_RETURN)
	  && (dbus_message_get_reply_serial(msg) == mute_serial))
	{
	  dbus_message_get_args(msg, &dbus_err,
				DBUS_TYPE_BOOLEAN, &mbp.muted,
				DBUS_TYPE_INVALID);
	}
      else
	{
	  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}
    }

  return DBUS_HANDLER_RESULT_HANDLED;
}

static int
mbp_dbus_connect(void)
{
  unsigned int signals;

  DBusMessage *msg;

  int ret;

  signals = MBP_DBUS_SIG_LCD | MBP_DBUS_SIG_KBD
    | MBP_DBUS_SIG_VOL | MBP_DBUS_SIG_MUTE
    | MBP_DBUS_SIG_EJECT | MBP_DBUS_SIG_VIDEO;

  conn = mbp_dbus_init(&dbus_err, signals);

  if (conn == NULL)
    return -1;

  dbus_connection_setup_with_g_main(conn, NULL);

  dbus_connection_add_filter(conn, mbp_dbus_listen, NULL, NULL);

  /* Get the mute state */
  msg = dbus_message_new_method_call("org.pommed", "/org/pommed/audio",
				     "org.pommed.audio", "getMute");

  if (msg == NULL)
    {
      printf("Failed to create method call message for audio getMute\n");

      return 0;
    }

  ret = dbus_connection_send(conn, msg, &mute_serial);
  if (ret == FALSE)
    {
      printf("Could not send method call for audio getMute\n");

      dbus_message_unref(msg);

      return 0;
    }

  dbus_connection_flush(conn);

  dbus_message_unref(msg);

  return 0;
}

static gboolean
mbp_dbus_reconnect(gpointer userdata)
{
  if (mbp_dbus_connect() < 0)
    return TRUE;

  return FALSE;
}


static gboolean
mbp_check_config(GIOChannel *ch, GIOCondition condition, gpointer userdata)
{
  int fd;
  int ret;

  fd = g_io_channel_unix_get_fd(ch);

  if (condition & G_IO_IN)
    {
      ret = config_load();
      if (ret < 0)
	{
	  fprintf(stderr, "Failed to reload config file, exiting\n");

	  gtk_main_quit();
	}
    }

  close(fd);

  fd = config_monitor();

  if (fd > 0)
    {
      ch = g_io_channel_unix_new(fd);
      g_io_channel_set_encoding(ch, NULL, NULL);
      g_io_add_watch(ch, G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL, mbp_check_config, NULL);
    }

  return FALSE;
}


static void
usage(void)
{
  printf("gpomme v" M_VERSION " graphical client for pommed\n");
  printf("Copyright (C) 2006-2008 Julien BLACHE <jb@jblache.org> and others\n");

  printf("Usage:\n");
  printf("\tgpomme\t\t-- start gpomme\n");
  printf("\tgpomme -c\t-- open the configuration panel\n");
  printf("\tgpomme -v\t-- print version and exit\n");
}


void
sig_int_term_handler(int signo)
{
  gtk_main_quit();
}

void
sig_chld_handler(int signo)
{
  int ret;

  do
    {
      ret = waitpid(-1, NULL, WNOHANG);
    }
  while (ret > 0);
}

int
main(int argc, char **argv)
{
  int c;
  int ret;

  GIOChannel *ch;
  int fd;

  gtk_init(&argc, &argv);

  bindtextdomain("gpomme", "/usr/share/locale");
  textdomain("gpomme");

  ret = config_load();
  if (ret < 0)
    {
      fprintf(stderr, "Failed to load configuration\n");

      exit(1);
    }

  while ((c = getopt(argc, argv, "cv")) != -1)
    {
      switch (c)
	{
	  case 'c':
	    config_gui();

	    exit(0);
	    break;

	  case 'v':
	    printf("gpomme v" M_VERSION " graphical client for pommed\n");
	    printf("Copyright (C) 2006-2008 Julien BLACHE <jb@jblache.org> and others\n");

	    exit(0);
	    break;

	  default:
	    usage();

	    exit(-1);
	    break;
	}
    }

  mbp_dbus_connect();

  fd = config_monitor();

  if (fd > 0)
    {
      ch = g_io_channel_unix_new(fd);
      g_io_channel_set_encoding(ch, NULL, NULL);
      g_io_add_watch(ch, G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL, mbp_check_config, NULL);
    }

  signal(SIGINT, sig_int_term_handler);
  signal(SIGTERM, sig_int_term_handler);
  signal(SIGCHLD, sig_chld_handler);

  create_window();

  gtk_main();

  mbp_dbus_cleanup();

  return 0;
}
