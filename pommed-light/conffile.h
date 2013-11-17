/*
 * pommed - conffile.h
 */

#ifndef __CONFFILE_H__
#define __CONFFILE_H__

struct _general_cfg {
  int fnmode;
};

struct _lcd_sysfs_cfg {
  int init;
  int step;
  int on_batt;
};


#ifndef __powerpc__
struct _lcd_x1600_cfg {
  int init;
  int step;
  int on_batt;
};

struct _lcd_gma950_cfg {
  unsigned int init;
  unsigned int step;
  unsigned int on_batt;
};

struct _lcd_nv8600mgt_cfg {
  int init;
  int step;
  int on_batt;
};
#endif /* !__powerpc__ */

struct _audio_cfg {
  int disabled;
  char *card;
  int init;
  int step;
  int beep;
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
  int idle;
  int idle_lvl;
};

struct _eject_cfg {
  int enabled;
  char *device;
};

struct _beep_cfg {
  int enabled;
  char *beepfile;
};

#ifndef __powerpc__
struct _appleir_cfg {
  int enabled;
};
#endif


extern struct _general_cfg general_cfg;
extern struct _lcd_sysfs_cfg lcd_sysfs_cfg;
#ifndef __powerpc__
extern struct _lcd_x1600_cfg lcd_x1600_cfg;
extern struct _lcd_gma950_cfg lcd_gma950_cfg;
extern struct _lcd_nv8600mgt_cfg lcd_nv8600mgt_cfg;
#endif
extern struct _audio_cfg audio_cfg;
extern struct _kbd_cfg kbd_cfg;
extern struct _eject_cfg eject_cfg;
extern struct _beep_cfg beep_cfg;
#ifndef __powerpc__
extern struct _appleir_cfg appleir_cfg;
#endif


int
config_load(void);

void
config_cleanup(void);


#endif /* !__CONFFILE_H__ */
