#ifndef __EVDEV_H__
#define __EVDEV_H__


#define USB_VENDOR_ID_APPLE           0x05ac

/* Apple Geyser III keyboard + trackpad */
#define USB_PRODUCT_ID_GEYSER3_ANSI   0x0217
#define USB_PRODUCT_ID_GEYSER3_ISO    0x0218
#define USB_PRODUCT_ID_GEYSER3_JIS    0x0219

/* Apple Geyser IV keyboard + trackpad */
#define USB_PRODUCT_ID_GEYSER4_ANSI   0x021a
#define USB_PRODUCT_ID_GEYSER4_ISO    0x021b
#define USB_PRODUCT_ID_GEYSER4_JIS    0x021c


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


#define EVDEV_BASE              "/dev/input/event"
#define EVDEV_MAX               32


void
evdev_process_events(int fd);

int
evdev_is_geyser3(unsigned short vendor, unsigned short product);

int
evdev_is_geyser4(unsigned short vendor, unsigned short product);

int
evdev_open(struct pollfd **fds);

void
evdev_close(struct pollfd **fds, int nfds);

int
evdev_reopen(struct pollfd **fds, int nfds);

#endif /* !__EVDEV_H__ */
