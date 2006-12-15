/*
 * $Id$
 */

#ifndef __MBPEVENTD_H__
#define __MBPEVENTD_H__


#define M_VERSION "0.9"


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


#define PIDFILE                "/var/run/mbpeventd.pid"
#define CONFFILE               "/etc/mbpeventd.conf"


#define STEP_UP                 1
#define STEP_DOWN               -1


#define LOOP_TIMEOUT            200

#endif /* !__MBPEVENTD_H__ */
