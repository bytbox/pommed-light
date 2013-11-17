/*
 * pommed - lcd_backlight.h
 */

#ifndef __LCD_BACKLIGHT_H__
#define __LCD_BACKLIGHT_H__


struct _lcd_bck_info
{
  int level;
  int ac_lvl;
  int max;
};

extern struct _lcd_bck_info lcd_bck_info;


#define LCD_USER    0
#define LCD_AUTO    1

#define LCD_ON_AC_LEVEL    0
#define LCD_ON_BATT_LEVEL  1


#ifndef __powerpc__
/* x1600_backlight.c */
#define X1600_BACKLIGHT_OFF       0
#define X1600_BACKLIGHT_MAX       255

void
x1600_backlight_step(int dir);

void
x1600_backlight_toggle(int lvl);

int
x1600_backlight_probe(void);

void
x1600_backlight_fix_config(void);


/* gma950_backlight.c */
#define GMA950_BACKLIGHT_MIN       0x1f
/* Beware, GMA950_BACKLIGHT_MAX is dynamic, see source */

void
gma950_backlight_step(int dir);

void
gma950_backlight_toggle(int lvl);

int
gma950_backlight_probe(void);


/* nv8600mgt_backlight.c */
#define NV8600MGT_BACKLIGHT_OFF    0
#define NV8600MGT_BACKLIGHT_MAX    15

void
nv8600mgt_backlight_step(int dir);

void
nv8600mgt_backlight_toggle(int lvl);

int
nv8600mgt_backlight_probe(void);

void
nv8600mgt_backlight_fix_config(void);
#endif /* !__powerpc__ */


/* sysfs_backlight.c */
#define SYSFS_BACKLIGHT_OFF	0

void
sysfs_backlight_step(int dir);

void
sysfs_backlight_toggle(int lvl);

#ifdef __powerpc__
void
sysfs_backlight_step_kernel(int dir);

void
sysfs_backlight_toggle_kernel(int lvl);

int
aty128_sysfs_backlight_probe(void);

int
r9x00_sysfs_backlight_probe(void);

int
nvidia_sysfs_backlight_probe(void);
#else
int
mbp_sysfs_backlight_probe(void);
#endif


#endif /* !__LCD_BACKLIGHT_H__ */
