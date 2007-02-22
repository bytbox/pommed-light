/*
 * $Id$
 */

#ifndef __LCD_BACKLIGHT_H__
#define __LCD_BACKLIGHT_H__


struct _lcd_bck_info
{
  int level;
  int max;
};

extern struct _lcd_bck_info lcd_bck_info;

#ifndef __powerpc__
/* x1600_backlight.c */
#define X1600_BACKLIGHT_OFF       0
#define X1600_BACKLIGHT_MAX       255

void
x1600_backlight_step(int dir);

int
x1600_backlight_probe(void);

void
x1600_backlight_fix_config(void);


/* gma950_backlight.c */
#define GMA950_BACKLIGHT_MIN       0x1f
/* Beware, GMA950_BACKLIGHT_MAX is dynamic, see source */

void
gma950_backlight_step(int dir);

int
gma950_backlight_probe(void);


#else


/* sysfs_backlight.c */
#define SYSFS_BACKLIGHT_OFF	0

void
sysfs_backlight_step(int dir);

int
r9x00_sysfs_backlight_probe(void);

int
nvidia_sysfs_backlight_probe(void);


/* r128_backlight.c */
#define R128_BACKLIGHT_OFF	0
#define R128_BACKLIGHT_MAX	127
void
r128_backlight_step(int dir);

int
r128_backlight_probe(void);

void
r128_backlight_fix_config(void);
#endif /* !__powerpc__ */

#endif /* !__LCD_BACKLIGHT_H__ */
