/*
 * pommed - Apple laptops hotkeys handler daemon
 *
 * $Id$
 *
 * Copyright (C) 2006-2007 Julien BLACHE <jb@jblache.org>
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

#include <syslog.h>

#include <dbus/dbus.h>

#include "pommed.h"
#include "dbus.h"
#include "lcd_backlight.h"
#include "kbd_backlight.h"
#include "ambient.h"
#include "audio.h"
#include "cd_eject.h"


static DBusError err;
static DBusConnection *conn;


void
mbpdbus_send_lcd_backlight(int cur, int prev)
{
  DBusMessage *msg;

  int ret;

  if (conn == NULL)
    return;

  logdebug("DBus lcdBacklight: %d %d\n", cur, prev);

  msg = dbus_message_new_signal("/org/pommed/notify/lcdBacklight",
				"org.pommed.signal.lcdBacklight",
				"lcdBacklight");
  if (msg == NULL)
    {
      logdebug("Failed to create DBus message\n");

      return;
    }

  ret = dbus_message_append_args(msg,
				 DBUS_TYPE_UINT32, &cur,
				 DBUS_TYPE_UINT32, &prev,
				 DBUS_TYPE_UINT32, &lcd_bck_info.max,
				 DBUS_TYPE_INVALID);
  if (ret == FALSE)
    {
      logdebug("Failed to add arguments\n");

      dbus_message_unref(msg);

      return;
    }

  ret = dbus_connection_send(conn, msg, NULL);
  if (ret == FALSE)
    {
      logdebug("Could not send lcdBacklight signal\n");

      dbus_message_unref(msg);

      return;
    }

  dbus_connection_flush(conn);

  dbus_message_unref(msg);
}

void
mbpdbus_send_kbd_backlight(int cur, int prev, int who)
{
  DBusMessage *msg;

  int ret;

  if (conn == NULL)
    return;

  logdebug("DBus kbdBacklight: %d %d\n", cur, prev);

  msg = dbus_message_new_signal("/org/pommed/notify/kbdBacklight",
				"org.pommed.signal.kbdBacklight",
				"kbdBacklight");
  if (msg == NULL)
    {
      logdebug("Failed to create DBus message\n");

      return;
    }

  ret = dbus_message_append_args(msg,
				 DBUS_TYPE_UINT32, &cur,
				 DBUS_TYPE_UINT32, &prev,
				 DBUS_TYPE_UINT32, &kbd_bck_info.max,
				 DBUS_TYPE_UINT32, &who,
				 DBUS_TYPE_INVALID);
  if (ret == FALSE)
    {
      logdebug("Failed to add arguments\n");

      dbus_message_unref(msg);

      return;
    }

  ret = dbus_connection_send(conn, msg, NULL);
  if (ret == FALSE)
    {
      logdebug("Could not send kbdBacklight signal\n");

      dbus_message_unref(msg);

      return;
    }

  dbus_connection_flush(conn);

  dbus_message_unref(msg);
}

void
mbpdbus_send_ambient_light(int l, int l_prev, int r, int r_prev)
{
  DBusMessage *msg;

  int ret;

  if (conn == NULL)
    return;

  logdebug("DBus ambientLight: %d %d %d %d\n", l, l_prev, r, r_prev);

  msg = dbus_message_new_signal("/org/pommed/notify/ambientLight",
				"org.pommed.signal.ambientLight",
				"ambientLight");
  if (msg == NULL)
    {
      logdebug("Failed to create DBus message\n");

      return;
    }

  ret = dbus_message_append_args(msg,
				 DBUS_TYPE_UINT32, &l,
				 DBUS_TYPE_UINT32, &l_prev,
				 DBUS_TYPE_UINT32, &r,
				 DBUS_TYPE_UINT32, &r_prev,
				 DBUS_TYPE_UINT32, &ambient_info.max,
				 DBUS_TYPE_INVALID);
  if (ret == FALSE)
    {
      logdebug("Failed to add arguments\n");

      dbus_message_unref(msg);

      return;
    }

  ret = dbus_connection_send(conn, msg, NULL);
  if (ret == FALSE)
    {
      logdebug("Could not send kbdBacklight signal\n");

      dbus_message_unref(msg);

      return;
    }

  dbus_connection_flush(conn);

  dbus_message_unref(msg);
}

void
mbpdbus_send_audio_volume(int cur, int prev)
{
  DBusMessage *msg;

  int ret;

  if (conn == NULL)
    return;

  logdebug("DBus audioVolume: %d %d\n", cur, prev);

  msg = dbus_message_new_signal("/org/pommed/notify/audioVolume",
				"org.pommed.signal.audioVolume",
				"audioVolume");
  if (msg == NULL)
    {
      logdebug("Failed to create DBus message\n");

      return;
    }

  ret = dbus_message_append_args(msg,
				 DBUS_TYPE_UINT32, &cur,
				 DBUS_TYPE_UINT32, &prev,
				 DBUS_TYPE_UINT32, &audio_info.max,
				 DBUS_TYPE_INVALID);
  if (ret == FALSE)
    {
      logdebug("Failed to add arguments\n");

      dbus_message_unref(msg);

      return;
    }

  ret = dbus_connection_send(conn, msg, NULL);
  if (ret == FALSE)
    {
      logdebug("Could not send audioVolume signal\n");

      dbus_message_unref(msg);

      return;
    }

  dbus_connection_flush(conn);

  dbus_message_unref(msg);
}

void
mbpdbus_send_audio_mute(int mute)
{
  DBusMessage *msg;

  int ret;

  if (conn == NULL)
    return;

  logdebug("DBus audioMute: %d\n", mute);

  msg = dbus_message_new_signal("/org/pommed/notify/audioMute",
				"org.pommed.signal.audioMute",
				"audioMute");
  if (msg == NULL)
    {
      logdebug("Failed to create DBus message\n");

      return;
    }

  ret = dbus_message_append_args(msg,
				 DBUS_TYPE_BOOLEAN, &mute,
				 DBUS_TYPE_INVALID);
  if (ret == FALSE)
    {
      logdebug("Failed to add arguments\n");

      dbus_message_unref(msg);

      return;
    }

  ret = dbus_connection_send(conn, msg, NULL);
  if (ret == FALSE)
    {
      logdebug("Could not send audioMute signal\n");

      dbus_message_unref(msg);

      return;
    }

  dbus_connection_flush(conn);

  dbus_message_unref(msg);
}

void
mbpdbus_send_cd_eject(void)
{
  DBusMessage *msg;

  int ret;

  if (conn == NULL)
    return;

  logdebug("DBus CD eject\n");

  msg = dbus_message_new_signal("/org/pommed/notify/cdEject",
				"org.pommed.signal.cdEject",
				"cdEject");
  if (msg == NULL)
    {
      logdebug("Failed to create DBus message\n");

      return;
    }

  ret = dbus_connection_send(conn, msg, NULL);
  if (ret == FALSE)
    {
      logdebug("Could not send cdEject signal\n");

      dbus_message_unref(msg);

      return;
    }

  dbus_connection_flush(conn);

  dbus_message_unref(msg);
}


static void
process_lcd_getlevel_call(DBusMessage *req)
{
  DBusMessage *msg;
  DBusMessageIter args;

  int ret;

  logdebug("Got lcdBacklight getLevel call\n");

  if (dbus_message_iter_init(req, &args))
    {
      logdebug("lcdBacklight getLevel call with arguments ?!\n");

      return;
    }

  msg = dbus_message_new_method_return(req);

  ret = dbus_message_append_args(msg,
				 DBUS_TYPE_UINT32, &lcd_bck_info.level,
				 DBUS_TYPE_UINT32, &lcd_bck_info.max,
				 DBUS_TYPE_INVALID);
  if (ret == FALSE)
    {
      logdebug("Failed to add arguments\n");

      dbus_message_unref(msg);

      return;
    }

  ret = dbus_connection_send(conn, msg, NULL);
  if (ret == FALSE)
    {
      logdebug("Could not send lcdBacklight getLevel reply\n");

      dbus_message_unref(msg);

      return;
    }

  dbus_message_unref(msg);
}


static void
process_kbd_getlevel_call(DBusMessage *req)
{
  DBusMessage *msg;
  DBusMessageIter args;

  int ret;

  logdebug("Got kbdBacklight getLevel call\n");

  if (dbus_message_iter_init(req, &args))
    {
      logdebug("kbdBacklight getLevel call with arguments ?!\n");

      return;
    }

  msg = dbus_message_new_method_return(req);

  ret = dbus_message_append_args(msg,
				 DBUS_TYPE_UINT32, &kbd_bck_info.level,
				 DBUS_TYPE_UINT32, &kbd_bck_info.max,
				 DBUS_TYPE_INVALID);
  if (ret == FALSE)
    {
      logdebug("Failed to add arguments\n");

      dbus_message_unref(msg);

      return;
    }

  ret = dbus_connection_send(conn, msg, NULL);
  if (ret == FALSE)
    {
      logdebug("Could not send kbdBacklight getLevel reply\n");

      dbus_message_unref(msg);

      return;
    }

  dbus_message_unref(msg);
}


static void
process_ambient_getlevel_call(DBusMessage *req)
{
  DBusMessage *msg;
  DBusMessageIter args;

  int ret;

  logdebug("Got ambient getLevel call\n");

  if (dbus_message_iter_init(req, &args))
    {
      logdebug("ambient getLevel call with arguments ?!\n");

      return;
    }

  msg = dbus_message_new_method_return(req);

  ret = dbus_message_append_args(msg,
				 DBUS_TYPE_UINT32, &ambient_info.left,
				 DBUS_TYPE_UINT32, &ambient_info.right,
				 DBUS_TYPE_UINT32, &ambient_info.max,
				 DBUS_TYPE_INVALID);
  if (ret == FALSE)
    {
      logdebug("Failed to add arguments\n");

      dbus_message_unref(msg);

      return;
    }

  ret = dbus_connection_send(conn, msg, NULL);
  if (ret == FALSE)
    {
      logdebug("Could not send ambient getLevel reply\n");

      dbus_message_unref(msg);

      return;
    }

  dbus_message_unref(msg);
}

static void
process_audio_getvolume_call(DBusMessage *req)
{
  DBusMessage *msg;
  DBusMessageIter args;

  int ret;

  logdebug("Got audio getVolume call\n");

  if (dbus_message_iter_init(req, &args))
    {
      logdebug("audio getVolume call with arguments ?!\n");

      return;
    }

  msg = dbus_message_new_method_return(req);

  ret = dbus_message_append_args(msg,
				 DBUS_TYPE_UINT32, &audio_info.level,
				 DBUS_TYPE_UINT32, &audio_info.max,
				 DBUS_TYPE_INVALID);
  if (ret == FALSE)
    {
      logdebug("Failed to add arguments\n");

      dbus_message_unref(msg);

      return;
    }

  ret = dbus_connection_send(conn, msg, NULL);
  if (ret == FALSE)
    {
      logdebug("Could not send audio getVolume reply\n");

      dbus_message_unref(msg);

      return;
    }

  dbus_message_unref(msg);
}

static void
process_audio_getmute_call(DBusMessage *req)
{
  DBusMessage *msg;
  DBusMessageIter args;

  int ret;

  logdebug("Got audio getMute call\n");

  if (dbus_message_iter_init(req, &args))
    {
      logdebug("audio getMute call with arguments ?!\n");

      return;
    }

  msg = dbus_message_new_method_return(req);

  ret = dbus_message_append_args(msg,
				 DBUS_TYPE_BOOLEAN, &audio_info.muted,
				 DBUS_TYPE_INVALID);
  if (ret == FALSE)
    {
      logdebug("Failed to add arguments\n");

      dbus_message_unref(msg);

      return;
    }

  ret = dbus_connection_send(conn, msg, NULL);
  if (ret == FALSE)
    {
      logdebug("Could not send audio getMute reply\n");

      dbus_message_unref(msg);

      return;
    }

  dbus_message_unref(msg);
}


static void
process_lcd_backlight_step_call(DBusMessage *req, int dir)
{
  DBusMessage *msg;

  int ret;

  logdebug("Got lcdBacklight levelUp/levelDown call\n");

  mops->lcd_backlight_step(dir);

  msg = dbus_message_new_method_return(req);

  ret = dbus_connection_send(conn, msg, NULL);
  if (ret == FALSE)
    {
      logdebug("Could not send lcdBacklight levelUp/levelDown reply\n");

      dbus_message_unref(msg);

      return;
    }

  dbus_message_unref(msg);
}

static void
process_kbd_backlight_inhibit_call(DBusMessage *req, int inhibit)
{
  DBusMessage *msg;

  int ret;

  logdebug("Got kbdBacklight inhibit call\n");

  if (inhibit)
    kbd_backlight_inhibit_set(KBD_INHIBIT_USER);
  else
    kbd_backlight_inhibit_clear(KBD_INHIBIT_USER);

  msg = dbus_message_new_method_return(req);

  ret = dbus_connection_send(conn, msg, NULL);
  if (ret == FALSE)
    {
      logdebug("Could not send kbdBacklight inhibit reply\n");

      dbus_message_unref(msg);

      return;
    }

  dbus_message_unref(msg);
}

static void
process_audio_volume_step_call(DBusMessage *req, int dir)
{
  DBusMessage *msg;

  int ret;

  logdebug("Got audio volumeUp/volumeDown call\n");

  audio_step(dir);

  msg = dbus_message_new_method_return(req);

  ret = dbus_connection_send(conn, msg, NULL);
  if (ret == FALSE)
    {
      logdebug("Could not send audio volumeUp/volumeDown reply\n");

      dbus_message_unref(msg);

      return;
    }

  dbus_message_unref(msg);
}

static void
process_audio_toggle_mute_call(DBusMessage *req)
{
  DBusMessage *msg;

  int ret;

  logdebug("Got audio toggleMute call\n");

  audio_toggle_mute();

  msg = dbus_message_new_method_return(req);

  ret = dbus_connection_send(conn, msg, NULL);
  if (ret == FALSE)
    {
      logdebug("Could not send audio toggleMute reply\n");

      dbus_message_unref(msg);

      return;
    }

  dbus_message_unref(msg);
}

static void
process_cd_eject_call(DBusMessage *req)
{
  DBusMessage *msg;

  int ret;

  logdebug("Got cd eject call\n");

  cd_eject();

  msg = dbus_message_new_method_return(req);

  ret = dbus_connection_send(conn, msg, NULL);
  if (ret == FALSE)
    {
      logdebug("Could not send cd eject reply\n");

      dbus_message_unref(msg);

      return;
    }

  dbus_message_unref(msg);
}


void
mbpdbus_process_requests(void)
{
  DBusMessage *msg;

  int nmsg;

  if (conn == NULL)
    {
      if (mbpdbus_init() < 0)
	return;
    }

  logdebug("Processing DBus requests\n");

  nmsg = 0;
  while (1)
    {
      logdebug("Checking messages\n");

      dbus_connection_read_write(conn, 0);

      msg = dbus_connection_pop_message(conn);

      if (msg == NULL)
	{
	  break;
	}

      // Get methods
      if (dbus_message_is_method_call(msg, "org.pommed.lcdBacklight", "getLevel"))
	process_lcd_getlevel_call(msg);
      else if (dbus_message_is_method_call(msg, "org.pommed.kbdBacklight", "getLevel"))
	process_kbd_getlevel_call(msg);
      else if (dbus_message_is_method_call(msg, "org.pommed.ambient", "getLevel"))
	process_ambient_getlevel_call(msg);
      else if (dbus_message_is_method_call(msg, "org.pommed.audio", "getVolume"))
	process_audio_getvolume_call(msg);
      else if (dbus_message_is_method_call(msg, "org.pommed.audio", "getMute"))
	process_audio_getmute_call(msg);
      // Set methods
      else if (dbus_message_is_method_call(msg, "org.pommed.lcdBacklight", "levelUp"))
        process_lcd_backlight_step_call(msg, STEP_UP);
      else if (dbus_message_is_method_call(msg, "org.pommed.lcdBacklight", "levelDown"))
        process_lcd_backlight_step_call(msg, STEP_DOWN);
      else if (dbus_message_is_method_call(msg, "org.pommed.kbdBacklight", "inhibit"))
        process_kbd_backlight_inhibit_call(msg, 1);
      else if (dbus_message_is_method_call(msg, "org.pommed.kbdBacklight", "disinhibit"))
        process_kbd_backlight_inhibit_call(msg, 0);
      else if (dbus_message_is_method_call(msg, "org.pommed.audio", "volumeUp"))
        process_audio_volume_step_call(msg, STEP_UP);
      else if (dbus_message_is_method_call(msg, "org.pommed.audio", "volumeDown"))
        process_audio_volume_step_call(msg, STEP_DOWN);
      else if (dbus_message_is_method_call(msg, "org.pommed.audio", "toggleMute"))
        process_audio_toggle_mute_call(msg);
      else if (dbus_message_is_method_call(msg, "org.pommed.cd", "eject"))
        process_cd_eject_call(msg);
      else if (dbus_message_is_signal(msg, DBUS_INTERFACE_LOCAL, "Disconnected"))
	{
	  logmsg(LOG_INFO, "DBus disconnected");

	  mbpdbus_cleanup();

	  dbus_message_unref(msg);

	  return;
	}

      dbus_message_unref(msg);

      nmsg++;
    }

  if (nmsg > 0)
    dbus_connection_flush(conn);

  logdebug("Done with DBus requests\n");
}


int
mbpdbus_init(void)
{
  int ret;

  dbus_error_init(&err);

  conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
  if (dbus_error_is_set(&err))
    {
      logmsg(LOG_ERR, "DBus system bus connection failed: %s", err.message);

      dbus_error_free(&err);

      conn = NULL;

      return -1;
    }

  dbus_connection_set_exit_on_disconnect(conn, FALSE);

  ret = dbus_bus_request_name(conn, "org.pommed", 0, &err);

  if (dbus_error_is_set(&err))
    {
      logmsg(LOG_ERR, "Failed to request DBus name: %s", err.message);

      mbpdbus_cleanup();

      return -1;
    }

  if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
    {
      logmsg(LOG_ERR, "Not primary DBus name owner");

      mbpdbus_cleanup();

      return -1;
    }

  return 0;
}

void
mbpdbus_cleanup(void)
{
  if (conn == NULL)
    return;

  dbus_error_free(&err);

  /* This is a shared connection owned by libdbus
   * Do not close it, only unref
   */
  dbus_connection_unref(conn);
  conn = NULL;
}
