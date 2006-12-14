/*
 * $Id$
 */

#ifndef __CONFFILE_H__
#define __CONFFILE_H__

struct _lcd_x1600_cfg {
  int init;
  int step;
};

struct _lcd_gma950_cfg {
  unsigned int init;
  unsigned int step;
};

#if 0
struct _audio_cfg {
  char *mixer;
  int init;
  int step;
};
#endif /* 0 */

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


extern struct _lcd_x1600_cfg lcd_x1600_cfg;
extern struct _lcd_gma950_cfg lcd_gma950_cfg;
#if 0
extern struct _audio_cfg audio_cfg;
#endif /* 0 */
extern struct _kbd_cfg kbd_cfg;
extern struct _eject_cfg eject_cfg;


int
config_load(void);

void
config_cleanup(void);

#endif /* !__CONFFILE_H__ */
