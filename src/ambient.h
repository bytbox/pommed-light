/*
 * $Id$
 */

#ifndef __AMBIENT_H__
#define __AMBIENT_H__


#define KBD_AMBIENT_SENSOR      "/sys/devices/platform/applesmc/light"
#define KBD_AMBIENT_MIN         0
#define KBD_AMBIENT_MAX         255


struct _ambient_info
{
  int left;
  int right;
  int max;
};

extern struct _ambient_info ambient_info;


void
ambient_get(int *r, int *l);

void
ambient_init(int *r, int *l);


#endif /* !__AMBIENT_H__ */
