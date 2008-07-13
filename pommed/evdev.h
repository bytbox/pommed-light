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

/* Fountain & Geyser devices : AppleUSBTopCase.kext/Contents/PlugIns/AppleUSBTrackpad.kext */

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

/* WellSpring devices : AppleUSBMultitouch.kext */

/* Apple WellSpring keyboard + trackpad */
#define USB_PRODUCT_ID_WELLSPRING_ANSI   0x0223
#define USB_PRODUCT_ID_WELLSPRING_ISO    0x0224
#define USB_PRODUCT_ID_WELLSPRING_JIS    0x0225

/* Apple WellSpring II keyboard + trackpad */
#define USB_PRODUCT_ID_WELLSPRING2_ANSI   0x0230
#define USB_PRODUCT_ID_WELLSPRING2_ISO    0x0231
#define USB_PRODUCT_ID_WELLSPRING2_JIS    0x0232


/* Apple external USB keyboard, white */
#define USB_PRODUCT_ID_APPLE_EXTKBD_WHITE   0x020c

/* Apple external USB keyboard, aluminium */
#define USB_PRODUCT_ID_APPLE_EXTKBD_ALU_ANSI 0x0220
#define USB_PRODUCT_ID_APPLE_EXTKBD_ALU_ISO  0x0221
#define USB_PRODUCT_ID_APPLE_EXTKBD_ALU_JIS  0x0222

/* Apple external wireless keyboard, aluminium */
#define USB_PRODUCT_ID_APPLE_EXTKBD_ALU_WL_ANSI 0x022c
#define USB_PRODUCT_ID_APPLE_EXTKBD_ALU_WL_ISO  0x022d
#define USB_PRODUCT_ID_APPLE_EXTKBD_ALU_WL_JIS  0x022e

/* Apple Remote IR Receiver */
#define USB_PRODUCT_ID_APPLEIR        0x8240
#define USB_PRODUCT_ID_APPLEIR_2      0x8242


#define EVDEV_DIR               "/dev/input"
#define EVDEV_BASE              "/dev/input/event"
#define EVDEV_MAX               32


int
evdev_init(void);

void
evdev_cleanup(void);

#endif /* !__EVDEV_H__ */
