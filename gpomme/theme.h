/*
 * $Id$
 */
#ifndef __THEME_H__
#define __THEME_H__

#define DEFAULT_THEME     "CrystalLarge"

enum
  {
    IMG_LCD_BCK = 0,
    IMG_KBD_BCK,
    IMG_AUDIO_VOL_ON,
    IMG_AUDIO_VOL_OFF,
    IMG_AUDIO_MUTE,
    IMG_CD_EJECT,

    IMG_NIMG /* Keep this one last */
  };

struct gpomme_theme
{
  int width;
  int height;
  GdkPixbuf *background;

  GtkWidget *images[IMG_NIMG];
};

extern struct gpomme_theme theme;


int theme_load(const char *name);

#endif /* !__THEME_H__ */
