/*
 * pommed - kbd_backlight.h
 */

#ifndef __KBD_BACKLIGHT_H__
#define __KBD_BACKLIGHT_H__

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

#define KBD_TIMEOUT 200


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

#ifdef __powerpc__
static inline int
has_kbd_backlight(void)
{
  return ((mops->type == MACHINE_POWERBOOK_51)
	  || (mops->type == MACHINE_POWERBOOK_52)
	  || (mops->type == MACHINE_POWERBOOK_53)
	  || (mops->type == MACHINE_POWERBOOK_54)
	  || (mops->type == MACHINE_POWERBOOK_55)
	  || (mops->type == MACHINE_POWERBOOK_56)
	  || (mops->type == MACHINE_POWERBOOK_57)
	  || (mops->type == MACHINE_POWERBOOK_58)
	  || (mops->type == MACHINE_POWERBOOK_59));
}

#else

static inline int
has_kbd_backlight(void)
{
  return ((mops->type == MACHINE_MACBOOKPRO_1)
	  || (mops->type == MACHINE_MACBOOKPRO_2)
	  || (mops->type == MACHINE_MACBOOKPRO_3)
	  || (mops->type == MACHINE_MACBOOKPRO_4)
	  || (mops->type == MACHINE_MACBOOKPRO_5)
	  || (mops->type == MACHINE_MACBOOKPRO_6)
	  || (mops->type == MACHINE_MACBOOKPRO_7)
	  || (mops->type == MACHINE_MACBOOKPRO_8)
	  || (mops->type == MACHINE_MACBOOKPRO_9)
	  || (mops->type == MACHINE_MACBOOKPRO_10)
	  || (mops->type == MACHINE_MACBOOK_5)
	  || (mops->type == MACHINE_MACBOOKAIR_1)
	  || (mops->type == MACHINE_MACBOOKAIR_2)
	  || (mops->type == MACHINE_MACBOOKAIR_3)
	  || (mops->type == MACHINE_MACBOOKAIR_4)
	  || (mops->type == MACHINE_MACBOOKAIR_5)
	  || (mops->type == MACHINE_MACBOOKAIR_6)
	  );
}
#endif /* __powerpc__ */


void
kbd_backlight_step(int dir);

void
kbd_backlight_init(void);

void
kbd_backlight_cleanup(void);

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


#endif /* !__KBD_BACKLIGHT_H__ */
