/*
 * $Id$
 */

#ifndef __KBD_BACKLIGHT_H__
#define __KBD_BACKLIGHT_H__

int
kbd_backlight_get(void);

void
kbd_backlight_set(int val);

void
kbd_backlight_step(int dir);

void
kbd_backlight_status_init(void);

void
kbd_backlight_ambient_check(void);

#endif /* !__KBD_BACKLIGHT_H__ */
