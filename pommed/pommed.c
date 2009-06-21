/*
 * pommed - Apple laptops hotkeys handler daemon
 *
 * Copyright (C) 2006-2008 Julien BLACHE <jb@jblache.org>
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
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>
#include <signal.h>

#include <sys/utsname.h>

#include <syslog.h>
#include <stdarg.h>

#include <errno.h>

#ifndef __powerpc__
# define check_machine() check_machine_dmi()
#else
# define check_machine() check_machine_pmu()
#endif /* __powerpc__ */

#include <getopt.h>

#include "pommed.h"
#include "kbd_backlight.h"
#include "lcd_backlight.h"
#include "cd_eject.h"
#include "evdev.h"
#include "evloop.h"
#include "conffile.h"
#include "audio.h"
#include "dbus.h"
#include "power.h"
#include "beep.h"


/* Machine-specific operations */
struct machine_ops *mops;


/* --- WARNING ---
 *
 * Be extra-careful here, the list below must come in the same
 * order as the machine_type enum in pommed.h !
 */
#ifdef __powerpc__
/* PowerBook machines */

struct machine_ops pb_mops[] = {
  /* PowerBook3,1 is a G3-based PowerBook */

  {  /* PowerBook3,2 */
    .type = MACHINE_POWERBOOK_32,
    .lcd_backlight_probe = aty128_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step_kernel,
    .lcd_backlight_toggle = sysfs_backlight_toggle_kernel,
    /* .evdev_identify = evdev_is_adb, */
  },

  {  /* PowerBook3,3 */
    .type = MACHINE_POWERBOOK_33,
    .lcd_backlight_probe = r9x00_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_adb, */
  },

  {  /* PowerBook3,4 */
    .type = MACHINE_POWERBOOK_34,
    .lcd_backlight_probe = r9x00_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_adb, */
  },

  {  /* PowerBook3,5 */
    .type = MACHINE_POWERBOOK_35,
    .lcd_backlight_probe = r9x00_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_adb, */
  },

  /* PowerBook4,* -> G3 iBooks */

  {  /* PowerBook5,1 */
    .type = MACHINE_POWERBOOK_51,
    .lcd_backlight_probe = nvidia_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_adb, */
  },

  {  /* PowerBook5,2 */
    .type = MACHINE_POWERBOOK_52,
    .lcd_backlight_probe = r9x00_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_adb, */
  },

  {  /* PowerBook5,3 */
    .type = MACHINE_POWERBOOK_53,
    .lcd_backlight_probe = r9x00_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_adb, */
  },

  {  /* PowerBook5,4 */
    .type = MACHINE_POWERBOOK_54,
    .lcd_backlight_probe = r9x00_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_adb, */
  },

  {  /* PowerBook5,5 */
    .type = MACHINE_POWERBOOK_55,
    .lcd_backlight_probe = r9x00_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_adb, */
  },

  {  /* PowerBook5,6 */
    .type = MACHINE_POWERBOOK_56,
    .lcd_backlight_probe = r9x00_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_fountain, */
  },

  {  /* PowerBook5,7 */
    .type = MACHINE_POWERBOOK_57,
    .lcd_backlight_probe = r9x00_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_fountain, */
  },

  {  /* PowerBook5,8 */
    .type = MACHINE_POWERBOOK_58,
    .lcd_backlight_probe = r9x00_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_geyser, */
  },

  {  /* PowerBook5,9 */
    .type = MACHINE_POWERBOOK_59,
    .lcd_backlight_probe = r9x00_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_geyser, */
  },

  /* G4 iBooks & 12" PowerBooks */

  {  /* PowerBook6,1 */
    .type = MACHINE_POWERBOOK_61,
    .lcd_backlight_probe = nvidia_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_adb, */
  },

  {  /* PowerBook6,2 */
    .type = MACHINE_POWERBOOK_62,
    .lcd_backlight_probe = nvidia_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_adb, */
  },

  {  /* PowerBook6,3 */
    .type = MACHINE_POWERBOOK_63,
    .lcd_backlight_probe = r9x00_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_adb, */
  },

  {  /* PowerBook6,4 */
    .type = MACHINE_POWERBOOK_64,
    .lcd_backlight_probe = nvidia_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_adb, */
  },

  {  /* PowerBook6,5 */
    .type = MACHINE_POWERBOOK_65,
    .lcd_backlight_probe = r9x00_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_adb, */
  },

  /* Looks like PowerBook6,6 never made it to the market ? */

  {  /* PowerBook6,7 */
    .type = MACHINE_POWERBOOK_67,
    .lcd_backlight_probe = r9x00_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_adb, */
  },

  {  /* PowerBook6,8 */
    .type = MACHINE_POWERBOOK_68,
    .lcd_backlight_probe = nvidia_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_adb, */
  }
};

#else

struct machine_ops mb_mops[] = {
  /* MacBook Pro machines */

  {  /* MacBookPro1,1 / MacBookPro1,2 (Core Duo) */
    .type = MACHINE_MACBOOKPRO_1,
    .lcd_backlight_probe = x1600_backlight_probe,
    .lcd_backlight_step = x1600_backlight_step,
    .lcd_backlight_toggle = x1600_backlight_toggle,
    /* .evdev_identify = evdev_is_geyser3, */
  },

  {  /* MacBookPro2,1 / MacBookPro2,2 (Core2 Duo) */
    .type = MACHINE_MACBOOKPRO_2,
    .lcd_backlight_probe = x1600_backlight_probe,
    .lcd_backlight_step = x1600_backlight_step,
    .lcd_backlight_toggle = x1600_backlight_toggle,
    /* .evdev_identify = evdev_is_geyser4, */
  },

  {  /* MacBookPro3,1 (15" & 17", Core2 Duo, June 2007) */
    .type = MACHINE_MACBOOKPRO_3,
    .lcd_backlight_probe = mbp_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_geyser4, */
  },

  {  /* MacBookPro4,1 (15" & 17", Core2 Duo, February 2008) */
    .type = MACHINE_MACBOOKPRO_4,
    .lcd_backlight_probe = mbp_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_wellspring2, */
  },

  {  /* MacBookPro5,1 (15" & 17", Core2 Duo, October 2008)
      * MacBookPro5,5 (13" June 2009) */
    .type = MACHINE_MACBOOKPRO_5,
    .lcd_backlight_probe = mbp_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_wellspring3, */
  },

  /* MacBook machines */

  {  /* MacBook1,1 (Core Duo) */
    .type = MACHINE_MACBOOK_1,
    .lcd_backlight_probe = gma950_backlight_probe,
    .lcd_backlight_step = gma950_backlight_step,
    .lcd_backlight_toggle = gma950_backlight_toggle,
    /* .evdev_identify = evdev_is_geyser3, */
  },

  {  /* MacBook2,1 (Core2 Duo) */
    .type = MACHINE_MACBOOK_2,
    .lcd_backlight_probe = gma950_backlight_probe,
    .lcd_backlight_step = gma950_backlight_step,
    .lcd_backlight_toggle = gma950_backlight_toggle,
    /* .evdev_identify = evdev_is_geyser4, */
  },

  {  /* MacBook3,1 (Core2 Duo Santa Rosa, November 2007) */
    .type = MACHINE_MACBOOK_3,
    .lcd_backlight_probe = gma950_backlight_probe, /* gma950 supports the gma965 */
    .lcd_backlight_step = gma950_backlight_step,
    .lcd_backlight_toggle = gma950_backlight_toggle,
    /* .evdev_identify = evdev_is_geyser4hf, */
  },

  {  /* MacBook4,1 (Core2 Duo, February 2008) */
    .type = MACHINE_MACBOOK_4,
    .lcd_backlight_probe = gma950_backlight_probe, /* gma950 supports the gma965 */
    .lcd_backlight_step = gma950_backlight_step,
    .lcd_backlight_toggle = gma950_backlight_toggle,
    /* .evdev_identify = evdev_is_geyser4hf, */
  },

  {  /* MacBook5,1 (Core2 Duo, October 2008) */
    .type = MACHINE_MACBOOK_5,
    .lcd_backlight_probe = mbp_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_wellspring3, */
  },

  /* MacBook Air machines */

  {  /* MacBookAir1,1 (January 2008) */
    .type = MACHINE_MACBOOKAIR_1,
    .lcd_backlight_probe = gma950_backlight_probe, /* gma950 supports the gma965 */
    .lcd_backlight_step = gma950_backlight_step,
    .lcd_backlight_toggle = gma950_backlight_toggle,
    /* .evdev_identify = evdev_is_wellspring, */
  },

  {  /* MacBookAir2,1 (October 2008) */
    .type = MACHINE_MACBOOKAIR_2,
    .lcd_backlight_probe = mbp_sysfs_backlight_probe,
    .lcd_backlight_step = sysfs_backlight_step,
    .lcd_backlight_toggle = sysfs_backlight_toggle,
    /* .evdev_identify = evdev_is_wellspring3, */
  }
};
#endif /* __powerpc__ */


/* debug mode */
int debug = 0;
int console = 0;


void
logmsg(int level, char *fmt, ...)
{
  va_list ap;
  FILE *where = stdout;

  va_start(ap, fmt);

  if (console)
    {
      switch (level)
	{
	  case LOG_INFO:
	    fprintf(where, "I: ");
	    break;

	  case LOG_WARNING:
	    fprintf(where, "W: ");
	    break;

	  case LOG_ERR:
	    where = stderr;
	    fprintf(where, "E: ");
	    break;

	  default:
	    break;
	}
      vfprintf(where, fmt, ap);
      fprintf(where, "\n");
    }
  else
    {
      vsyslog(level | LOG_DAEMON, fmt, ap);
    }

  va_end(ap);
}

void
logdebug(char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);

  if (debug)
    vfprintf(stderr, fmt, ap);

  va_end(ap);
}


void
kbd_set_fnmode(void)
{
  char *fnmode_node[] =
    {
      "/sys/module/hid_apple/parameters/fnmode", /* 2.6.28 & up */
      "/sys/module/hid/parameters/pb_fnmode",    /* 2.6.20 & up */
      "/sys/module/usbhid/parameters/pb_fnmode"
    };
  FILE *fp;
  int i;

  if ((general_cfg.fnmode < 1) || (general_cfg.fnmode > 2))
    general_cfg.fnmode = 1;

  for (i = 0; i < sizeof(fnmode_node) / sizeof(*fnmode_node); i++)
    {
      logdebug("Trying %s\n", fnmode_node[i]);

      fp = fopen(fnmode_node[i], "a");
      if (fp != NULL)
	break;

      if (errno == ENOENT)
	continue;

      logmsg(LOG_INFO, "Could not open %s: %s", fnmode_node[i], strerror(errno));
      return;
    }

  fprintf(fp, "%d", general_cfg.fnmode);

  fclose(fp);
}

#ifdef __powerpc__
static machine_type
check_machine_pmu(void)
{
  int fd;
  int n;
  int ret = MACHINE_UNKNOWN;

  char buffer[128];

  /* Check copyright node, look for "Apple Computer, Inc." */
  fd = open("/proc/device-tree/copyright", O_RDONLY);
  if (fd < 0)
    {
      logmsg(LOG_ERR, "Could not open /proc/device-tree/copyright");

      return ret;
    }

  n = read(fd, buffer, sizeof(buffer) - 1);
  if (n < 1)
    {
      logmsg(LOG_ERR, "Error reading /proc/device-tree/copyright");

      close(fd);
      return ret;
    }
  close(fd);
  buffer[n] = '\0';

  logdebug("device-tree copyright node: [%s]\n", buffer);

  if (strstr(buffer, "Apple Computer, Inc.") == NULL)
    return ret;


  ret = MACHINE_MAC_UNKNOWN;

  /* Grab machine identifier string */
  fd = open("/proc/device-tree/model", O_RDONLY);
  if (fd < 0)
    {
      logmsg(LOG_ERR, "Could not open /proc/device-tree/model");

      return ret;
    }

  n = read(fd, buffer, sizeof(buffer) - 1);
  if (n < 1)
    {
      logmsg(LOG_ERR, "Error reading /proc/device-tree/model");

      close(fd);
      return ret;
    }
  close(fd);
  buffer[n] = '\0';

  logdebug("device-tree model node: [%s]\n", buffer);

  /* PowerBook G4 Titanium 15" (December 2000) */
  if (strncmp(buffer, "PowerBook3,2", 12) == 0)
    ret = MACHINE_POWERBOOK_32;
  /* PowerBook G4 Titanium 15" (October 2001) */
  else if (strncmp(buffer, "PowerBook3,3", 12) == 0)
    ret = MACHINE_POWERBOOK_33;
  /* PowerBook G4 Titanium 15" (April 2002) */
  else if (strncmp(buffer, "PowerBook3,4", 12) == 0)
    ret = MACHINE_POWERBOOK_34;
  /* PowerBook G4 Titanium 15" */
  else if (strncmp(buffer, "PowerBook3,5", 12) == 0)
    ret = MACHINE_POWERBOOK_35;

  /* PowerBook G4 Aluminium 17" */
  else if (strncmp(buffer, "PowerBook5,1", 12) == 0)
    ret = MACHINE_POWERBOOK_51;
  /* PowerBook G4 Aluminium 15" (September 2003) */
  else if (strncmp(buffer, "PowerBook5,2", 12) == 0)
    ret = MACHINE_POWERBOOK_52;
  /* PowerBook G4 Aluminium 17" (September 2003) */
  else if (strncmp(buffer, "PowerBook5,3", 12) == 0)
    ret = MACHINE_POWERBOOK_53;
  /* PowerBook G4 Aluminium 15" (April 2004) */
  else if (strncmp(buffer, "PowerBook5,4", 12) == 0)
    ret = MACHINE_POWERBOOK_54;
  /* PowerBook G4 Aluminium 17" (April 2004) */
  else if (strncmp(buffer, "PowerBook5,5", 12) == 0)
    ret = MACHINE_POWERBOOK_55;
  /* PowerBook G4 Aluminium 15" (February 2005) */
  else if (strncmp(buffer, "PowerBook5,6", 12) == 0)
    ret = MACHINE_POWERBOOK_56;
  /* PowerBook G4 Aluminium 17" (February 2005) */
  else if (strncmp(buffer, "PowerBook5,7", 12) == 0)
    ret = MACHINE_POWERBOOK_57;
  /* PowerBook G4 Aluminium 15" */
  else if (strncmp(buffer, "PowerBook5,8", 12) == 0)
    ret = MACHINE_POWERBOOK_58;
  /* PowerBook G4 Aluminium 17" */
  else if (strncmp(buffer, "PowerBook5,9", 12) == 0)
    ret = MACHINE_POWERBOOK_59;

  /* PowerBook G4 12" (January 2003) */
  else if (strncmp(buffer, "PowerBook6,1", 12) == 0)
    ret = MACHINE_POWERBOOK_61;
  /* PowerBook G4 12" (September 2003) */
  else if (strncmp(buffer, "PowerBook6,2", 12) == 0)
    ret = MACHINE_POWERBOOK_61;
  /* iBook G4 (October 2003) */
  else if (strncmp(buffer, "PowerBook6,3", 12) == 0)
    ret = MACHINE_POWERBOOK_63;
  /* PowerBook G4 12" (April 2004) */
  else if (strncmp(buffer, "PowerBook6,4", 12) == 0)
    ret = MACHINE_POWERBOOK_64;
  /* iBook G4 (October 2004) */
  else if (strncmp(buffer, "PowerBook6,5", 12) == 0)
    ret = MACHINE_POWERBOOK_65;
  /* iBook G4 */
  else if (strncmp(buffer, "PowerBook6,7", 12) == 0)
    ret = MACHINE_POWERBOOK_67;
  /* PowerBook G4 12" */
  else if (strncmp(buffer, "PowerBook6,8", 12) == 0)
    ret = MACHINE_POWERBOOK_68;

  else
    logmsg(LOG_ERR, "Unknown Apple machine: %s", buffer);
  
  if (ret != MACHINE_MAC_UNKNOWN)
    logmsg(LOG_INFO, "PMU machine check: running on a %s", buffer);

  return ret;
}

#else

static machine_type
check_machine_dmi(void)
{
  int ret;

  int fd;
  char buf[32];
  int i;

  char *vendor_node[] =
    {
      "/sys/class/dmi/id/sys_vendor",
      "/sys/class/dmi/id/board_vendor",
      "/sys/class/dmi/id/chassis_vendor",
      "/sys/class/dmi/id/bios_vendor"
    };

  /* Check vendor name */
  for (i = 0; i < sizeof(vendor_node) / sizeof(vendor_node[0]); i++)
    {
      fd = open(vendor_node[i], O_RDONLY);
      if (fd > 0)
	break;

      logmsg(LOG_INFO, "Could not open %s: %s", vendor_node[i], strerror(errno));
    }

  if (fd < 0)
    {
      logmsg(LOG_ERR, "Could not determine vendor name");

      return MACHINE_ERROR;
    }

  memset(buf, 0, sizeof(buf));

  ret = read(fd, buf, sizeof(buf) - 1);
  if (ret < 0)
    {
      logmsg(LOG_ERR, "Could not read from %s: %s", vendor_node[i], strerror(errno));

      close(fd);
      return MACHINE_ERROR;
    }

  close(fd);

  if (buf[ret - 1] == '\n')
    buf[ret - 1] = '\0';

  logdebug("DMI vendor name: [%s]\n", buf);

  if ((strcmp(buf, "Apple Computer, Inc.") != 0)
      && (strcmp(buf, "Apple Inc.") != 0))
    return MACHINE_UNKNOWN;

  /* Check product name */
  fd = open("/sys/class/dmi/id/product_name", O_RDONLY);
  if (fd < 0)
    {
      logmsg(LOG_INFO, "Could not open /sys/class/dmi/id/product_name: %s", strerror(errno));

      return MACHINE_MAC_UNKNOWN;
    }

  memset(buf, 0, sizeof(buf));

  ret = read(fd, buf, sizeof(buf) - 1);
  if (ret < 0)
    {
      logmsg(LOG_ERR, "Could not read from /sys/class/dmi/id/product_name: %s", strerror(errno));

      close(fd);
      return MACHINE_MAC_UNKNOWN;
    }

  close(fd);

  if (buf[ret - 1] == '\n')
    buf[ret - 1] = '\0';

  logdebug("DMI product name: [%s]\n", buf);

  ret = MACHINE_MAC_UNKNOWN;

  /* Core Duo MacBook Pro 15" (January 2006) & 17" (April 2006) */
  if ((strcmp(buf, "MacBookPro1,1") == 0) || (strcmp(buf, "MacBookPro1,2") == 0))
    ret = MACHINE_MACBOOKPRO_1;
  /* Core2 Duo MacBook Pro 17" & 15" (October 2006) */
  else if ((strcmp(buf, "MacBookPro2,1") == 0) || (strcmp(buf, "MacBookPro2,2") == 0))
    ret = MACHINE_MACBOOKPRO_2;
  /* Core2 Duo MacBook Pro 15" & 17" (June 2007) */
  else if (strcmp(buf, "MacBookPro3,1") == 0)
    ret = MACHINE_MACBOOKPRO_3;
  /* Core2 Duo MacBook Pro 15" & 17" (February 2008) */
  else if (strcmp(buf, "MacBookPro4,1") == 0)
    ret = MACHINE_MACBOOKPRO_4;
  /* Core2 Duo MacBook Pro 15" & 17" (October 2008)
   * MacBook Pro 13" (June 2009) */
  else if ((strcmp(buf, "MacBookPro5,1") == 0)
	   || (strcmp(buf, "MacBookPro5,5") == 0))
    ret = MACHINE_MACBOOKPRO_5;
  /* Core Duo MacBook (May 2006) */
  else if (strcmp(buf, "MacBook1,1") == 0)
    ret = MACHINE_MACBOOK_1;
  /* Core2 Duo MacBook (November 2006) */
  else if (strcmp(buf, "MacBook2,1") == 0)
    ret = MACHINE_MACBOOK_2;
  /* Core2 Duo Santa Rosa MacBook (November 2007) */
  else if (strcmp(buf, "MacBook3,1") == 0)
    ret = MACHINE_MACBOOK_3;
  /* Core2 Duo MacBook (February 2008) */
  else if (strcmp(buf, "MacBook4,1") == 0)
    ret = MACHINE_MACBOOK_4;
  /* Core2 Duo MacBook (October 2008) (5,2 white MacBook) */
  else if ((strcmp(buf, "MacBook5,1") == 0) || (strcmp(buf, "MacBook5,2") == 0))
    ret = MACHINE_MACBOOK_5;
  /* MacBook Air (January 2008) */
  else if (strcmp(buf, "MacBookAir1,1") == 0)
    ret = MACHINE_MACBOOKAIR_1;
  /* MacBook Air (October 2008) */
  else if (strcmp(buf, "MacBookAir2,1") == 0)
    ret = MACHINE_MACBOOKAIR_2;
  else
    logmsg(LOG_ERR, "Unknown Apple machine: %s", buf);

  if (ret != MACHINE_MAC_UNKNOWN)
    logmsg(LOG_INFO, "DMI machine check: running on a %s", buf);

  return ret;
}
#endif /* __powerpc__ */


static void
usage(void)
{
  printf("pommed v" M_VERSION " Apple laptops hotkeys handler\n");
  printf("Copyright (C) 2006-2009 Julien BLACHE <jb@jblache.org>\n");

  printf("Usage:\n");
  printf("\tpommed\t-- start pommed as a daemon\n");
  printf("\tpommed -v\t-- print version and exit\n");
  printf("\tpommed -f\t-- run in the foreground with log messages\n");
  printf("\tpommed -d\t-- run in the foreground with debug messages\n");
}


void
sig_int_term_handler(int signal)
{
  evloop_stop();
}

int
main (int argc, char **argv)
{
  int ret;
  int c;

  FILE *pidfile;
  struct utsname sysinfo;

  machine_type machine;

  while ((c = getopt(argc, argv, "fdv")) != -1)
    {
      switch (c)
	{
	  case 'f':
	    console = 1;
	    break;

	  case 'd':
	    debug = 1;
	    console = 1;
	    break;

	  case 'v':
	    printf("pommed v" M_VERSION " Apple laptops hotkeys handler\n");
	    printf("Copyright (C) 2006-2009 Julien BLACHE <jb@jblache.org>\n");

	    exit(0);
	    break;

	  default:
	    usage();

	    exit(1);
	    break;
	}
    }

  if (geteuid() != 0)
    {
      logmsg(LOG_ERR, "pommed needs root privileges to operate");

      exit(1);
    }

  if (!console)
    {
      openlog("pommed", LOG_PID, LOG_DAEMON);
    }

  logmsg(LOG_INFO, "pommed v" M_VERSION " Apple laptops hotkeys handler");
  logmsg(LOG_INFO, "Copyright (C) 2006-2009 Julien BLACHE <jb@jblache.org>");

  /* Load our configuration */
  ret = config_load();
  if (ret < 0)
    {
      exit(1);
    }

  /* Identify the machine we're running on */
  machine = check_machine();
  switch (machine)
    {
      case MACHINE_MAC_UNKNOWN:
	logmsg(LOG_ERR, "Unknown Apple machine");

	exit(1);
	break;

      case MACHINE_UNKNOWN:
	logmsg(LOG_ERR, "Unknown non-Apple machine");

	exit(1);
	break;

      case MACHINE_ERROR:
	exit(1);
	break;

      default:
	if (machine < MACHINE_LAST)
	  {
#ifdef __powerpc__
	    mops = &pb_mops[machine];
#else
	    mops = &mb_mops[machine];
#endif /* __powerpc__ */
	  }
	break;
    }

  /* Runtime sanity check: catch errors in the mb_mops and pb_mops arrays */
  if (mops->type != machine)
    {
      logmsg(LOG_ERR, "machine_ops mismatch: expected %d, found %d", machine, mops->type);

      exit(1);
    }

  if (debug)
    {
      ret = uname(&sysinfo);

      if (ret < 0)
	logmsg(LOG_ERR, "uname() failed: %s", strerror(errno));
      else
	logdebug("System: %s %s %s\n", sysinfo.sysname, sysinfo.release, sysinfo.machine);
    }

  ret = evloop_init();
  if (ret < 0)
    {
      logmsg(LOG_ERR, "Event loop initialization failed");
      exit (1);
    }

  ret = mops->lcd_backlight_probe();
  if (ret < 0)
    {
      logmsg(LOG_ERR, "LCD backlight probe failed, check debug output");

      exit(1);
    }

  ret = evdev_init();
  if (ret < 1)
    {
      logmsg(LOG_ERR, "No suitable event devices found");

      exit(1);
    }

  kbd_backlight_init();

  ret = audio_init();
  if (ret < 0)
    {
      logmsg(LOG_WARNING, "Audio initialization failed, audio support disabled");
    }

  ret = mbpdbus_init();
  if (ret < 0)
    {
      logmsg(LOG_WARNING, "Could not connect to DBus system bus");
    }

  power_init();

  if (!console)
    {
      /*
       * Detach from the console
       */
      if (daemon(0, 0) != 0)
	{
	  logmsg(LOG_ERR, "daemon() failed: %s", strerror(errno));

	  evdev_cleanup();

	  exit(1);
	}
    }

  pidfile = fopen(PIDFILE, "w");
  if (pidfile == NULL)
    {
      logmsg(LOG_WARNING, "Could not open pidfile %s: %s", PIDFILE, strerror(errno));

      evdev_cleanup();

      exit(1);
    }
  fprintf(pidfile, "%d\n", getpid());
  fclose(pidfile);

  /* Spawn the beep thread */
  beep_init();

  signal(SIGINT, sig_int_term_handler);
  signal(SIGTERM, sig_int_term_handler);


  do
    {
      ret = evloop_iteration();
    }
  while (ret >= 0);

  evdev_cleanup();

  beep_cleanup();

  mbpdbus_cleanup();

  kbd_backlight_cleanup();

  power_cleanup();

  evloop_cleanup();

  config_cleanup();

  logmsg(LOG_INFO, "Exiting");

  if (!console)
    closelog();

  unlink(PIDFILE);

  return 0;
}
