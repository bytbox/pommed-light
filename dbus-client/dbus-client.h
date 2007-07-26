/*
 * $Id$
 */
#ifndef __MBP_DBUS_CLIENT_H__
#define __MBP_DBUS_CLIENT_H__

/* Signals to listen to */
#define MBP_DBUS_SIG_NONE   0
#define MBP_DBUS_SIG_LCD    (1 << 0)
#define MBP_DBUS_SIG_KBD    (1 << 1)
#define MBP_DBUS_SIG_VOL    (1 << 2)
#define MBP_DBUS_SIG_MUTE   (1 << 3)
#define MBP_DBUS_SIG_EJECT  (1 << 4)
#define MBP_DBUS_SIG_LIGHT  (1 << 5)


#define LCD_USER      0
#define LCD_AUTO      1

#define KBD_USER      0
#define KBD_AUTO      1


/* Method calls */
int
mbp_call_lcd_getlevel(DBusPendingCallNotifyFunction cb, void *userdata);

int
mbp_call_kbd_getlevel(DBusPendingCallNotifyFunction cb, void *userdata);

int
mbp_call_ambient_getlevel(DBusPendingCallNotifyFunction cb, void *userdata);

int
mbp_call_audio_getvolume(DBusPendingCallNotifyFunction cb, void *userdata);

int
mbp_call_audio_getmute(DBusPendingCallNotifyFunction cb, void *userdata);


/* Error checking */
int
mbp_dbus_check_error(DBusMessage *msg);


/* Connection init and cleanup */
DBusConnection *
mbp_dbus_init(DBusError *error, unsigned int signals);

void
mbp_dbus_cleanup(void);


#endif /* !__MBP_DBUS_CLIENT_H__ */
