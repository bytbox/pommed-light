/*
 * $Id$
 */

#ifndef __MBPEVENTD_H__
#define __MBPEVENTD_H__


#define M_VERSION "0.7"


void
logmsg(int level, char *fmt, ...);

#ifdef DEBUG
# define logdebug(fmt, args...) printf(fmt, ##args);
# define debug 1
#else
# define logdebug(fmt, args...)
extern int debug;
#endif


typedef enum
  {
    MACHINE_ERROR = -1,
    MACHINE_UNKNOWN,
    MACHINE_MAC_UNKNOWN,
    MACHINE_MACBOOKPRO_1,
    MACHINE_MACBOOKPRO_2,
    MACHINE_MACBOOK_1,
    MACHINE_MACBOOK_2,
  } machine_type;

struct machine_ops
{
  machine_type type;
  int (*lcd_backlight_probe) (void);
  void (*lcd_backlight_step) (int dir);
  int (*evdev_identify) (unsigned short vendor, unsigned short product);
};

extern struct machine_ops *mops;


#define USB_VENDOR_ID_APPLE           0x05ac

/* Apple Geyser III keyboard + trackpad */
#define USB_PRODUCT_ID_GEYSER3_ANSI   0x0217
#define USB_PRODUCT_ID_GEYSER3_ISO    0x0218
#define USB_PRODUCT_ID_GEYSER3_JIS    0x0219

/* Apple Geyser IV keyboard + trackpad */
#define USB_PRODUCT_ID_GEYSER4_ANSI   0x021a
#define USB_PRODUCT_ID_GEYSER4_ISO    0x021b
#define USB_PRODUCT_ID_GEYSER4_JIS    0x021c


#define PIDFILE                "/var/run/mbpeventd.pid"


#define K_LCD_BCK_DOWN          0xe0
#define K_LCD_BCK_UP            0xe1
#define K_AUDIO_MUTE            0x71
#define K_AUDIO_DOWN            0x72
#define K_AUDIO_UP              0x73
#define K_VIDEO_TOGGLE          0xe3
#define K_KBD_BCK_OFF           0xe4
#define K_KBD_BCK_DOWN          0xe5
#define K_KBD_BCK_UP            0xe6
#define K_CD_EJECT              0xa1

#define STEP_UP                 1
#define STEP_DOWN               -1

#define EVDEV_BASE              "/dev/input/event"
#define EVDEV_MAX               32

#define LOOP_TIMEOUT            200

#define CD_DVD_DEVICE           "/dev/dvd"
#define CD_CDROM_DEVICE         "/dev/cdrom"
#define CD_EJECT_CMD            "/usr/bin/eject"

#endif /* !__MBPEVENTD_H__ */
