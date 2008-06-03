/*
 * timerfd syscall numbers and definitions
 */

#ifndef _LINUX_TIMERFD_SYSCALLS_H_
#define _LINUX_TIMERFD_SYSCALLS_H_

#ifndef __NR_timerfd_create
# if defined(__x86_64__)
#  define __NR_timerfd_create  283
#  define __NR_timerfd_settime 286
#  define __NR_timerfd_gettime 287
# elif defined(__i386__)
#  define __NR_timerfd_create  322
#  define __NR_timerfd_settime 325
#  define __NR_timerfd_gettime 326
# elif defined(__powerpc__)
#  define __NR_timerfd_create  306
#  define __NR_timerfd_settime 311
#  define __NR_timerfd_gettime 312
# else
#  error Unsupported architecture
# endif
#endif

/* Defined in include/linux/timerfd.h */
#define TFD_TIMER_ABSTIME (1 << 0)

static inline int
timerfd_create(int clockid, int flags)
{
  return syscall(__NR_timerfd_create, clockid, flags);
}

static inline int
timerfd_settime(int fd, int flags, const struct itimerspec *new_value, struct itimerspec *curr_value)
{
  return syscall(__NR_timerfd_settime, fd, flags, new_value, curr_value);
}

static inline int
timerfd_gettime(int fd, struct itimerspec *curr_value)
{
  return syscall(__NR_timerfd_gettime, fd, curr_value);
}

#endif /* _LINUX_TIMERFD_SYSCALLS_H_ */
