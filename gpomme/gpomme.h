/*
 * $Id$
 */

#ifndef __GPOMME_H__
#define __GPOMME_H__

#include <gtk/gtk.h>

#define THEME_BASE    "/usr/share/gpomme/themes"

#define M_VERSION     "0.7"


struct _mbp_w
{
  GtkWidget *window;     /* The window itself */

  GtkWidget *img_align;  /* Image container */
  GtkWidget *image;      /* Current image, if any */

  GtkWidget *label;      /* Text label */

  GtkWidget *pbar_align; /* Progress bar container */
  GtkWidget *pbar;       /* Progress bar */
  int pbar_state;

  int timeout;
  guint timer;
};

extern struct _mbp_w mbp_w;


#endif /* !__GPOMME_H__ */
