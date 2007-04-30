/*
 * pommed - Apple laptops hotkeys handler daemon
 *
 * $Id$
 *
 * Copyright (C) 2006-2007 Julien BLACHE <jb@jblache.org>
 * Copyright (C) 2006 Yves-Alexis Perez <corsac@corsac.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */ 


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include <syslog.h>

#include <errno.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>

#include <ofapi/of_api.h>

#include "../pommed.h"
#include "../conffile.h"
#include "../kbd_backlight.h"
#include "../ambient.h"
#include "../dbus.h"


#define I2C_DEV       "/dev/i2c-7"
#define I2C_SLAVE     0x0703


struct _kbd_bck_info kbd_bck_info;


static int lmuaddr;  /* i2c bus address */
static char *i2cdev; /* i2c bus device */


int
has_kbd_backlight(void)
{
  return ((mops->type == MACHINE_POWERBOOK_51)
	  || (mops->type == MACHINE_POWERBOOK_52)
	  || (mops->type == MACHINE_POWERBOOK_53)
	  || (mops->type == MACHINE_POWERBOOK_54)
	  || (mops->type == MACHINE_POWERBOOK_55)
	  || (mops->type == MACHINE_POWERBOOK_56)
	  || (mops->type == MACHINE_POWERBOOK_57)
	  || (mops->type == MACHINE_POWERBOOK_58)
	  || (mops->type == MACHINE_POWERBOOK_59));
}

static int
kbd_backlight_get(void)
{
  if (lmuaddr == 0)
    return 0;

  return kbd_bck_info.level;
}

static void
kbd_backlight_set(int val, int who)
{
  int curval;

  int i;
  float fadeval;
  float step;
  struct timespec fade_step;

  int fd;
  int ret;
  unsigned char buf[8];

  if (kbd_bck_info.inhibit ^ KBD_INHIBIT_CFG)
    return;

  curval = kbd_backlight_get();

  if (val == curval)
    return;

  if ((val < KBD_BACKLIGHT_OFF) || (val > KBD_BACKLIGHT_MAX))
    return;

  if (lmuaddr == 0)
    return;

  fd = open (i2cdev, O_RDWR);
  if (fd < 0)
    {
      logmsg(LOG_ERR, "Could not open %s: %s\n", I2C_DEV, strerror(errno));

      return;
    }

  ret = ioctl(fd, I2C_SLAVE, lmuaddr);
  if (ret < 0)
    {
      logmsg(LOG_ERR, "Could not ioctl the i2c bus: %s\n", strerror(errno));

      close(fd);
      return;
    }

  buf[0] = 0x01;   /* i2c register */

  if (who == KBD_AUTO)
    {
      fade_step.tv_sec = 0;
      fade_step.tv_nsec = (KBD_BACKLIGHT_FADE_LENGTH / KBD_BACKLIGHT_FADE_STEPS) * 1000000;

      fadeval = (float)curval;
      step = (float)(val - curval) / (float)KBD_BACKLIGHT_FADE_STEPS;

      for (i = 0; i < KBD_BACKLIGHT_FADE_STEPS; i++)
	{
	  fadeval += step;

	  /* See below for the format */
	  buf[1] = (unsigned char) fadeval >> 4;
	  buf[2] = (unsigned char) fadeval << 4;

	  if (write (fd, buf, 3) < 0)
	    {
	      logmsg(LOG_ERR, "Could not set kbd brightness: %s\n", strerror(errno));

	      continue;
	    }

	  logdebug("KBD backlight value faded to %d\n", (int)fadeval);

	  nanosleep(&fade_step, NULL);
	}
    }
  
  /* The format appears to be: (taken from pbbuttonsd)
   *          byte 1   byte 2
   *         |<---->| |<---->|
   *         xxxx7654 3210xxxx
   *             |<----->|
   *                 ^-- brightness
   */
  
  buf[1] = (unsigned char) val >> 4;
  buf[2] = (unsigned char) val << 4;

  if (write (fd, buf, 3) < 0)
    logmsg(LOG_ERR, "Could not set kbd brightness: %s\n", strerror(errno));

  close(fd);

  mbpdbus_send_kbd_backlight(val, kbd_bck_info.level, who);

  kbd_bck_info.level = val;
}

void
kbd_backlight_step(int dir)
{
  int val;
  int newval;

  if (kbd_bck_info.inhibit ^ KBD_INHIBIT_CFG)
    return;

  if (lmuaddr == 0)
    return;

  val = kbd_backlight_get();

  if (val < 0)
    return;

  if (dir == STEP_UP)
    {
      newval = val + kbd_cfg.step;

      if (newval > KBD_BACKLIGHT_MAX)
	newval = KBD_BACKLIGHT_MAX;

      logdebug("KBD stepping +%d -> %d\n", kbd_cfg.step, newval);
    }
  else if (dir == STEP_DOWN)
    {
      newval = val - kbd_cfg.step;

      if (newval < KBD_BACKLIGHT_OFF)
	newval = KBD_BACKLIGHT_OFF;

      logdebug("KBD stepping -%d -> %d\n", kbd_cfg.step, newval);
    }
  else
    return;

  kbd_backlight_set(newval, KBD_USER);
}


/* Include automatic backlight routines */
#include "../kbd_auto.c"


void
kbd_backlight_init(void)
{
  int ret;

  if (kbd_cfg.auto_on)
    kbd_bck_info.inhibit = 0;
  else
    kbd_bck_info.inhibit = KBD_INHIBIT_CFG;

  kbd_bck_info.toggle_lvl = kbd_cfg.auto_lvl;

  kbd_bck_info.inhibit_lvl = 0;

  kbd_bck_info.auto_on = 0;

  lmuaddr = kbd_get_lmuaddr();
  i2cdev = "/dev/i2c-7";

  ret = kbd_probe_lmu(lmuaddr, i2cdev);

  if ((!has_kbd_backlight()) || (ret < 0))
    {
      lmuaddr = 0;
      i2cdev = NULL;

      kbd_bck_info.r_sens = 0;
      kbd_bck_info.l_sens = 0;

      kbd_bck_info.level = 0;

      ambient_info.left = 0;
      ambient_info.right = 0;
      ambient_info.max = 0;

      return;
    }

  kbd_bck_info.level = kbd_backlight_get();

  if (kbd_bck_info.level < 0)
    kbd_bck_info.level = 0;

  kbd_bck_info.max = KBD_BACKLIGHT_MAX;

  ambient_init(&kbd_bck_info.r_sens, &kbd_bck_info.l_sens);
}


void
kbd_backlight_fix_config(void)
{
  if (kbd_cfg.auto_lvl > KBD_BACKLIGHT_MAX)
    kbd_cfg.auto_lvl = KBD_BACKLIGHT_MAX;

  if (kbd_cfg.step < 1)
    kbd_cfg.step = 1;

  if (kbd_cfg.step > (KBD_BACKLIGHT_MAX / 2))
    kbd_cfg.step = KBD_BACKLIGHT_MAX / 2;
}

int
kbd_get_lmuaddr(void)
{
  struct device_node *node;
  int plen, lmuaddr = -1;
  long *reg = NULL;

  of_init();

  node = of_find_node_by_type("lmu-controller", 0);
  if (node == NULL)
    return -1;

  reg = of_find_property(node, "reg", &plen);
  lmuaddr = (int) (*reg >> 1);

  free(reg);
  of_free_node(node);

  return lmuaddr;
}

#if 0 /* Old code */
int
kbd_get_lmuaddr2(void)
{
  int fd;
  int ret;
  long reg;

  fd = open(LMU_REG, O_RDONLY);
  if (fd < 0)
    {
      logmsg(LOG_ERR, "Could not open lmu %s: %s\n", LMU_REG, strerror(errno));

      fd = open(LMU_REG_55, O_RDONLY);
      if (fd < 0)
	{
	  logmsg(LOG_ERR, "Could not open lmu %s: %s\n", LMU_REG_55, strerror(errno));

	  return -1;
	}
    }

  ret = read(fd, &reg, sizeof(long));
  close(fd);

  if (ret == sizeof(long))
    return (int)(reg >> 1);

  return 0;
}
#endif /* 0 */


char *
kbd_get_i2cdev(int addr)
{
  return I2C_DEV;
}

int
kbd_probe_lmu(int addr, char *dev)
{
  int fd;
  int ret;
  char buffer[4];

  fd = open(dev, O_RDWR);
  if (fd < 0)
    {
      logmsg(LOG_WARNING, "Could not open device %s: %s\n", dev, strerror(errno));

      return -1;
    }

  ret = ioctl(fd, I2C_SLAVE, addr);
  if (ret < 0)
    {
      logmsg(LOG_ERR, "ioctl failed on %s: %s\n", dev, strerror(errno));

      close(fd);
      return -1;
    }

  ret = read(fd, buffer, 4);
  if (ret != 4)
    {
      logmsg(LOG_WARNING, "Probing failed on %s: %s\n", dev, strerror(errno));

      close(fd);
      return -1;
    }
  close(fd);

  logdebug("Probing successful on %s\n", dev);

  return 0;
}
