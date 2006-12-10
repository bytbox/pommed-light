/*
 * $Id$
 */

#ifndef __LCD_BACKLIGHT_H__
#define __LCD_BACKLIGHT_H__


/* x1600_backlight.c */
void
x1600_backlight_step(int dir);

int
x1600_backlight_probe(void);


/* gma950_backlight.c */
void
gma950_backlight_step(int dir);

int
gma950_backlight_probe(void);

#endif /* !__LCD_BACKLIGHT_H__ */
