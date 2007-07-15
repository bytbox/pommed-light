/*
 * $Id$
 */

#ifndef __POMMED_H__
#define __POMMED_H__


#define M_VERSION "1.8"


extern int debug;
extern int console;


void
logmsg(int level, char *fmt, ...);

void
logdebug(char *fmt, ...);


void
kbd_set_fnmode(void);


typedef enum
  {
    MACHINE_ERROR = -3,
    MACHINE_UNKNOWN = -2,
    MACHINE_MAC_UNKNOWN = -1,
#ifndef __powerpc__
    MACHINE_MACBOOKPRO_1,
    MACHINE_MACBOOKPRO_2,
    MACHINE_MACBOOKPRO_3,
    MACHINE_MACBOOK_1,
    MACHINE_MACBOOK_2,
#else
    MACHINE_POWERBOOK_32,
    MACHINE_POWERBOOK_33,
    MACHINE_POWERBOOK_34,
    MACHINE_POWERBOOK_35,

    MACHINE_POWERBOOK_51,
    MACHINE_POWERBOOK_52,
    MACHINE_POWERBOOK_53,
    MACHINE_POWERBOOK_54,
    MACHINE_POWERBOOK_55,
    MACHINE_POWERBOOK_56,
    MACHINE_POWERBOOK_57,
    MACHINE_POWERBOOK_58,
    MACHINE_POWERBOOK_59,

    MACHINE_POWERBOOK_61,
    MACHINE_POWERBOOK_62,
    MACHINE_POWERBOOK_63,
    MACHINE_POWERBOOK_64,
    MACHINE_POWERBOOK_65,
    MACHINE_POWERBOOK_67,
    MACHINE_POWERBOOK_68,
#endif /* !__powerpc__ */
    MACHINE_LAST
  } machine_type;

struct machine_ops
{
  machine_type type;
  int (*lcd_backlight_probe) (void);
  void (*lcd_backlight_step) (int dir);
  int (*evdev_identify) (unsigned short *id);
};

extern struct machine_ops *mops;


#define PIDFILE                "/var/run/pommed.pid"
#define CONFFILE               "/etc/pommed.conf"

#define KBD_FNMODE_FILE        "/sys/module/usbhid/parameters/pb_fnmode"
#define KBD_FNMODE_FILE2620    "/sys/module/hid/parameters/pb_fnmode"

#define STEP_UP                 1
#define STEP_DOWN               -1


#define LOOP_TIMEOUT            200

#endif /* !__POMMED_H__ */
