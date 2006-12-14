#ifndef __EVDEV_H__
#define __EVDEV_H__

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
