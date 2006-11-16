/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>


#include "mbpeventd.h"
#include "kbd_backlight.h"
#include "ambient.h"


static struct
{
  int auto_on;  /* automatic */
  int off;      /* turned off ? */
  int r_sens;   /* right sensor */
  int l_sens;   /* left sensor */
} kbd_bck_status;


int
kbd_backlight_get(void)
{
  int fd;
  int ret;
  char buf[8];

  fd = open(KBD_BACKLIGHT, O_RDONLY);
  if (fd < 0)
    return -1;

  memset(buf, 0, 8);

  ret = read(fd, buf, 8);

  close(fd);

  if ((ret < 1) || (ret > 7))
    return -1;

  ret = atoi(buf);

  debug("KBD backlight value is %d\n", ret);

  return ret;
}

void
kbd_backlight_set(int val)
{
  int fd;
  int ret;
  int curval;
  char buf[8];

  curval = kbd_backlight_get();

  if (val == curval)
    return;

  fd = open(KBD_BACKLIGHT, O_RDWR | O_APPEND);
  if (fd < 0)
    return;

  ret = snprintf(buf, 8, "%d\n", val);

  if ((ret <= 0) || (ret > 7))
    return;

  ret = write(fd, buf, ret);

  debug("KBD backlight value set to %d\n", val);

  /* backlight turned off by user */
  if ((val == KBD_BACKLIGHT_OFF) && (kbd_bck_status.auto_on))
    kbd_bck_status.off = 1;

  /* backlight turned on again by user */
  if ((val > KBD_BACKLIGHT_OFF)
      && (kbd_bck_status.auto_on) && (kbd_bck_status.off))
    kbd_bck_status.off = 0;

  close(fd);
}

void
kbd_backlight_step(int dir)
{
  int val;
  int newval;

  val = kbd_backlight_get();

  if (val < 0)
    return;

  if (dir == STEP_UP)
    {
      newval = val + KBD_BCK_STEP;

      if (newval > KBD_BACKLIGHT_MAX)
	newval = KBD_BACKLIGHT_MAX;

      debug("KBD stepping +%d -> %d\n", KBD_BCK_STEP, newval);
    }
  else if (dir == STEP_DOWN)
    {
      newval = val - KBD_BCK_STEP;

      if (newval < KBD_BACKLIGHT_OFF)
	newval = KBD_BACKLIGHT_OFF;

      debug("KBD stepping -%d -> %d\n", KBD_BCK_STEP, newval);
    }
  else
    return;

  kbd_backlight_set(newval);
}

void
kbd_backlight_status_init(void)
{
  kbd_bck_status.auto_on = 0;
  kbd_bck_status.off = 0;

  ambient_get(&kbd_bck_status.r_sens, &kbd_bck_status.l_sens);
}

void
kbd_backlight_ambient_check(void)
{
  int amb_r, amb_l;

  ambient_get(&amb_r, &amb_l);

  if ((amb_r < 0) || (amb_l < 0))
    return;

  if ((amb_r < KBD_AMBIENT_THRESHOLD) || (amb_l < KBD_AMBIENT_THRESHOLD))
    {
      debug("Ambient light lower threshold reached\n");

      /* backlight turned on automatically, then disabled by user */
      if (kbd_bck_status.auto_on && kbd_bck_status.off)
	return;

      /* backlight already on */
      if (kbd_backlight_get() > KBD_BACKLIGHT_OFF)
	return;

      /* turn on backlight */
      kbd_bck_status.auto_on = 1;
      kbd_bck_status.off = 0;

      kbd_backlight_set(KBD_BACKLIGHT_DEFAULT);
    }
  else if (kbd_bck_status.auto_on)
    {
      if ((amb_r > (2 * KBD_AMBIENT_THRESHOLD)) && (amb_l > (2 * KBD_AMBIENT_THRESHOLD)))
	{
	  debug("Ambient light upper threshold reached\n");

	  kbd_bck_status.auto_on = 0;
	  kbd_bck_status.off = 0;

	  kbd_backlight_set(KBD_BACKLIGHT_OFF);
	}
    }

  kbd_bck_status.r_sens = amb_r;
  kbd_bck_status.l_sens = amb_l;
}
