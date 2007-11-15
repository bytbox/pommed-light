/*
 * $Id$
 */

#ifndef __EVDEV_H__
#define __EVDEV_H__


/****** ADB Devices ******/

/* Keyboard as found on the PowerBook3,2 */
#define ADB_PRODUCT_ID_KEYBOARD       0x22c4
/* Special PowerBook buttons as found on the PowerBook3,2 */
#define ADB_PRODUCT_ID_PBBUTTONS      0x771f


/****** USB Devices ******/

#define USB_VENDOR_ID_APPLE           0x05ac

/* Apple Fountain keyboard + trackpad */
#define USB_PRODUCT_ID_FOUNTAIN_ANSI  0x020e
#define USB_PRODUCT_ID_FOUNTAIN_ISO   0x020f
#define USB_PRODUCT_ID_FOUNTAIN_JIS   0x0210

/* Apple Geyser keyboard + trackpad */
#define USB_PRODUCT_ID_GEYSER_ANSI   0x0214
#define USB_PRODUCT_ID_GEYSER_ISO    0x0215
#define USB_PRODUCT_ID_GEYSER_JIS    0x0216

/* Apple Geyser III keyboard + trackpad */
#define USB_PRODUCT_ID_GEYSER3_ANSI   0x0217
#define USB_PRODUCT_ID_GEYSER3_ISO    0x0218
#define USB_PRODUCT_ID_GEYSER3_JIS    0x0219

/* Apple Geyser IV keyboard + trackpad */
#define USB_PRODUCT_ID_GEYSER4_ANSI   0x021a
#define USB_PRODUCT_ID_GEYSER4_ISO    0x021b
#define USB_PRODUCT_ID_GEYSER4_JIS    0x021c

/* Apple Geyser IV-HF keyboard + trackpad */
#define USB_PRODUCT_ID_GEYSER4HF_ANSI   0x0229
#define USB_PRODUCT_ID_GEYSER4HF_ISO    0x022a
#define USB_PRODUCT_ID_GEYSER4HF_JIS    0x022b

/* Apple Remote IR Receiver */
#define USB_PRODUCT_ID_APPLEIR        0x8240
#define USB_PRODUCT_ID_APPLEIR_2      0x8242


/* Keyboard hotkeys */
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

/* Apple Remote controller keys */
/* audio up/down use the same codes as the keyboard */
#define K_IR_FFWD               0xa3
#define K_IR_REWD               0xa5
#define K_IR_PLAY               0xa4
#define K_IR_MENU               0x8b


#define EVDEV_BASE              "/dev/input/event"
#define EVDEV_MAX               32


void
evdev_process_events(int fd);


#ifdef __powerpc__
int
evdev_is_adb(unsigned short *id);

int
evdev_is_fountain(unsigned short *id);

int
evdev_is_geyser(unsigned short *id);

#else

int
evdev_is_geyser3(unsigned short *id);

int
evdev_is_geyser4(unsigned short *id);

int
evdev_is_geyser4hf(unsigned short *id);
#endif /* __powerpc__ */


int
evdev_open(struct pollfd **fds);

void
evdev_close(struct pollfd **fds, int nfds);

int
evdev_reopen(struct pollfd **fds, int nfds);

#endif /* !__EVDEV_H__ */
