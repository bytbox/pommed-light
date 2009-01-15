/*
 * pommed - power.h
 */

#ifndef __POWER_H__
#define __POWER_H__


#define AC_STATE_ERROR   -1
#define AC_STATE_UNKNOWN -2

#define AC_STATE_ONLINE   1
#define AC_STATE_OFFLINE  0

#define POWER_TIMEOUT 200

#ifdef __powerpc__
# define SYSFS_POWER_AC_STATE  "/sys/class/power_supply/pmu-ac/online"
#else
# define SYSFS_POWER_AC_STATE  "/sys/class/power_supply/ADP1/online"
#endif


void
power_init(void);

void
power_cleanup(void);

#endif /* !__POWER_H__ */

