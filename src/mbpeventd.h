/*
 * $Id$
 */

#ifndef __MBPEVENTD_H__
#define __MBPEVENTD_H__

#define DEBUG

#ifdef DEBUG
# define debug(fmt, args...) printf(fmt, ##args);
#else
# define debug(fmt, args...)
#endif


#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)    ((array[LONG(bit)] >> OFF(bit)) & 1)


#define USB_VENDOR_ID_APPLE           0x05ac
#define USB_PRODUCT_ID_GEYSER4_ISO    0x021b


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
#define EVDEV_TIMEOUT           200

#define LCD_BCK_STEP            10
#define LCD_BACKLIGHT_OFF       0
#define LCD_BACKLIGHT_MAX       255

#define KBD_BCK_STEP            10
#define KBD_BACKLIGHT           "/sys/class/leds/smc:kbd_backlight/brightness"
#define KBD_AMBIENT_SENSOR      "/sys/devices/platform/applesmc/light"
#define KBD_AMBIENT_THRESHOLD   20
#define KBD_BACKLIGHT_DEFAULT   100
#define KBD_BACKLIGHT_OFF       0
#define KBD_BACKLIGHT_MAX       255


#endif /* !__MBPEVENTD_H__ */
