/*
 * $Id$
 */

#ifndef __KBD_BACKLIGHT_H__
#define __KBD_BACKLIGHT_H__

#ifdef __powerpc__
int
kbd_get_lmuaddr(void);

char*
kbd_get_i2cdev(int addr);

int 
kbd_probe_lmu(int addr, char* i2cdev);

#else
#define KBD_BACKLIGHT           "/sys/class/leds/smc:kbd_backlight/brightness"
#endif /* __powerpc__ */

#define KBD_BACKLIGHT_OFF       0
#define KBD_BACKLIGHT_MAX       255


struct _kbd_bck_info
{
  int level;
  int max;
};

extern struct _kbd_bck_info kbd_bck_info;


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
