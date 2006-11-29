#ifndef __EVDEV_H__
#define __EVDEV_H__

void
process_evdev_events(int fd);

int
evdev_is_geyser4(unsigned short vendor, unsigned short product);

int
open_evdev(struct pollfd **fds);

void
close_evdev(struct pollfd **fds, int nfds);

int
reopen_evdev(struct pollfd **fds, int nfds);

#endif /* !__EVDEV_H__ */
