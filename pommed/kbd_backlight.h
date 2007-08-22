/*
 * $Id$
 */

#ifndef __KBD_BACKLIGHT_H__
#define __KBD_BACKLIGHT_H__

#ifndef __powerpc__
#define KBD_BACKLIGHT           "/sys/class/leds/smc:kbd_backlight/brightness"
#endif /* !__powerpc__ */


#define KBD_BACKLIGHT_OFF       0
#define KBD_BACKLIGHT_MAX       255

/* fading duration in milliseconds */
#define KBD_BACKLIGHT_FADE_LENGTH 350
#define KBD_BACKLIGHT_FADE_STEPS 20

#define KBD_INHIBIT_USER        (1 << 0)
#define KBD_INHIBIT_LID         (1 << 1)
#define KBD_INHIBIT_CFG         (1 << 2)
#define KBD_INHIBIT_IDLE        (1 << 3)

#define KBD_MASK_AUTO           (KBD_INHIBIT_LID | KBD_INHIBIT_IDLE)


#define KBD_USER     0
#define KBD_AUTO     1


struct _kbd_bck_info
{
  int level;
  int max;

  int inhibit;
  int inhibit_lvl;

  int toggle_lvl; /* backlight level for simple toggle */

  int auto_on;  /* automatic */
  int idle;     /* idle timer */
  int r_sens;   /* right sensor */
  int l_sens;   /* left sensor */
};

extern struct _kbd_bck_info kbd_bck_info;


int
has_kbd_backlight(void);

void
kbd_backlight_step(int dir);

void
kbd_backlight_init(void);

void
kbd_backlight_fix_config(void);


/* In kbd_auto.c */
void
kbd_backlight_toggle(void);

void
kbd_backlight_inhibit_set(int mask);

void
kbd_backlight_inhibit_clear(int mask);

void
kbd_backlight_inhibit_toggle(int mask);

void
kbd_backlight_ambient_check(void);


#endif /* !__KBD_BACKLIGHT_H__ */
