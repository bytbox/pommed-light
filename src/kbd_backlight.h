/*
 * $Id$
 */

#ifndef __KBD_BACKLIGHT_H__
#define __KBD_BACKLIGHT_H__


#define KBD_BACKLIGHT           "/sys/class/leds/smc:kbd_backlight/brightness"

#define KBD_BACKLIGHT_OFF       0
#define KBD_BACKLIGHT_MAX       255


int
has_kbd_backlight(void);

void
kbd_backlight_off(void);

void
kbd_backlight_step(int dir);

void
kbd_backlight_init(void);

void
kbd_backlight_ambient_check(void);

void
kbd_backlight_fix_config(void);


#endif /* !__KBD_BACKLIGHT_H__ */
