/*
 * $Id$
 */

#ifndef __EVLOOP_H__
#define __EVLOOP_H__


#define MAX_EPOLL_EVENTS        8

typedef void(*pommed_event_cb)(int fd, uint32_t events);

struct pommed_event
{
  int fd;
  pommed_event_cb cb;
  struct pommed_event *next;
};


int
evloop_add(int fd, uint32_t events, pommed_event_cb cb);

int
evloop_remove(int fd);

int
evloop_iteration(void);

int
evloop_init(void);

void
evloop_cleanup(void);


#endif /* __EVLOOP_H__ */
