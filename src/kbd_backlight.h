/*
 * $Id$
 */

#ifndef __KBD_BACKLIGHT_H__
#define __KBD_BACKLIGHT_H__

void
kbd_backlight_off(void);

void
kbd_backlight_step(int dir);

void
kbd_backlight_status_init(void);

void
kbd_backlight_ambient_check(void);

#endif /* !__KBD_BACKLIGHT_H__ */
