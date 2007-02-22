/*
 * $Id$
 */

#ifndef __CONFFILE_H__
#define __CONFFILE_H__

struct _general_cfg {
  int fnmode;
};

#ifdef __powerpc__
struct _lcd_sysfs_cfg {
  int init;
  int step;
};

struct _lcd_r128_cfg {
  int init;
};

#else

struct _lcd_x1600_cfg {
  int init;
  int step;
};

struct _lcd_gma950_cfg {
  unsigned int init;
  unsigned int step;
};
#endif /* __powerpc__ */

struct _audio_cfg {
  char *card;
  int init;
  int step;
  char *vol;
  char *spkr;
  char *head;
};

struct _kbd_cfg {
  int auto_lvl;
  int step;
  int on_thresh;
  int off_thresh;
  int auto_on;
};

struct _eject_cfg {
  int enabled;
  char *device;
};

#ifndef __powerpc__
struct _appleir_cfg {
  int enabled;
};
#endif


extern struct _general_cfg general_cfg;
#ifdef __powerpc__
extern struct _lcd_sysfs_cfg lcd_sysfs_cfg;
extern struct _lcd_r128_cfg lcd_r128_cfg;
#else
extern struct _lcd_x1600_cfg lcd_x1600_cfg;
extern struct _lcd_gma950_cfg lcd_gma950_cfg;
#endif
extern struct _audio_cfg audio_cfg;
extern struct _kbd_cfg kbd_cfg;
extern struct _eject_cfg eject_cfg;
#ifndef __powerpc__
extern struct _appleir_cfg appleir_cfg;
#endif


int
config_load(void);

void
config_cleanup(void);


#endif /* !__CONFFILE_H__ */
