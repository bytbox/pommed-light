/*
 * pommed - evdev.h
 */

#ifndef __EVDEV_H__
#define __EVDEV_H__


/****** ADB Devices ******/

#define ADB_PRODUCT_ID_KEYBOARD_ANSI  0x22c3
#define ADB_PRODUCT_ID_KEYBOARD_ISO   0x22c4
#define ADB_PRODUCT_ID_KEYBOARD_JIS   0x22c5
/* Special PowerBook buttons */
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

/* Apple WellSpring III keyboard + trackpad */
#define USB_PRODUCT_ID_WELLSPRING3_ANSI   0x0236
#define USB_PRODUCT_ID_WELLSPRING3_ISO    0x0237
#define USB_PRODUCT_ID_WELLSPRING3_JIS    0x0238

/* Apple WellSpring IV keyboard + trackpad */
#define USB_PRODUCT_ID_WELLSPRING4_ANSI   0x023f
#define USB_PRODUCT_ID_WELLSPRING4_ISO    0x0240
#define USB_PRODUCT_ID_WELLSPRING4_JIS    0x0241

/* Apple WellSpring IVa keyboard + trackpad */
#define USB_PRODUCT_ID_WELLSPRING4A_ANSI  0x0242
#define USB_PRODUCT_ID_WELLSPRING4A_ISO   0x0243
#define USB_PRODUCT_ID_WELLSPRING4A_JIS   0x0244

/* Apple WellSpring V keyboard + trackpad */
#define USB_PRODUCT_ID_WELLSPRING5_ANSI   0x0245
#define USB_PRODUCT_ID_WELLSPRING5_ISO    0x0246
#define USB_PRODUCT_ID_WELLSPRING5_JIS    0x0247

/* 2011 Macbook Air */
#define USB_PRODUCT_ID_2011MBA_ANSI       0x024a
#define USB_PRODUCT_ID_2011MBA_ISO        0x024b
#define USB_PRODUCT_ID_2011MBA_JIS        0x024c
#define USB_PRODUCT_ID_2011MBA_EUR        0x024d

/* 2012 Macbook Air */
#define USB_PRODUCT_ID_2012MBA_ANSI       0x024a
#define USB_PRODUCT_ID_2012MBA_ISO        0x024b
#define USB_PRODUCT_ID_2012MBA_JIS        0x024c
#define USB_PRODUCT_ID_2012MBA_EUR        0x024d

/* Apple Inc. Apple Internal Keyboard / Trackpad */
#define USB_PRODUCT_ID_APPLE_INTERNAL_KEYBOARD_ANSI   0x0262

/* Apple WellSpring VI keyboard + trackpad */
#define USB_PRODUCT_ID_WELLSPRING6_ANSI   0x0259

/* Apple external USB keyboard, white */
#define USB_PRODUCT_ID_APPLE_EXTKBD_WHITE   0x020c

/* Apple external USB mini keyboard, aluminium */
#define USB_PRODUCT_ID_APPLE_EXTKBD_MINI_ALU_ANSI 0x021d
#define USB_PRODUCT_ID_APPLE_EXTKBD_MINI_ALU_ISO  0x021e
#define USB_PRODUCT_ID_APPLE_EXTKBD_MINI_ALU_JIS  0x021f

/* Apple external USB keyboard, aluminium */
#define USB_PRODUCT_ID_APPLE_EXTKBD_ALU_ANSI 0x0220
#define USB_PRODUCT_ID_APPLE_EXTKBD_ALU_ISO  0x0221
#define USB_PRODUCT_ID_APPLE_EXTKBD_ALU_JIS  0x0222

/* Apple external USB keyboard, aluminium, revision B */
#define USB_PRODUCT_ID_APPLE_EXTKBD_ALU_REV_B_ANSI 0x024f
#define USB_PRODUCT_ID_APPLE_EXTKBD_ALU_REV_B_ISO  0x0250
#define USB_PRODUCT_ID_APPLE_EXTKBD_ALU_REV_B_JIS  0x0251

/* Apple external wireless keyboard, aluminium */
#define USB_PRODUCT_ID_APPLE_EXTKBD_ALU_WL_ANSI 0x022c
#define USB_PRODUCT_ID_APPLE_EXTKBD_ALU_WL_ISO  0x022d
#define USB_PRODUCT_ID_APPLE_EXTKBD_ALU_WL_JIS  0x022e

/* Apple external wireless keyboard, aluminium, newer model */
#define USB_PRODUCT_ID_APPLE_EXTKBD_ALU_WL_2_ANSI 0x0239
#define USB_PRODUCT_ID_APPLE_EXTKBD_ALU_WL_2_ISO  0x023a
#define USB_PRODUCT_ID_APPLE_EXTKBD_ALU_WL_2_JIS  0x023b

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
