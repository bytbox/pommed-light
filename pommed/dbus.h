/*
 * $Id$
 */

#ifndef __MBPDBUS_H__
#define __MBPDBUS_H__


void
mbpdbus_send_lcd_backlight(int cur, int prev, int who);

void
mbpdbus_send_kbd_backlight(int cur, int prev, int who);

void
mbpdbus_send_ambient_light(int l, int l_prev, int r, int r_prev);

void
mbpdbus_send_audio_volume(int cur, int prev);

void
mbpdbus_send_audio_mute(int mute);

void
mbpdbus_send_cd_eject(void);

void
mbpdbus_send_video_switch(void);


void
mbpdbus_process_requests(void);


int
mbpdbus_init(void);

void
mbpdbus_cleanup(void);


#endif /* !__MBPDBUS_H__ */
