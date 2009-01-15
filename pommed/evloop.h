/*
 * pommed - evloop.h
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

typedef void(*pommed_timer_cb)(int id, uint64_t ticks);

struct pommed_timer_job
{
  int id;
  pommed_timer_cb cb;

  struct pommed_timer_job *next;
};

struct pommed_timer
{
  int fd;
  int timeout;
  struct pommed_timer_job *jobs;

  struct pommed_timer *next;
};


int
evloop_add(int fd, uint32_t events, pommed_event_cb cb);

int
evloop_remove(int fd);

int
evloop_add_timer(int timeout, pommed_timer_cb cb);

int
evloop_remove_timer(int id);

int
evloop_iteration(void);

void
evloop_stop(void);

int
evloop_init(void);

void
evloop_cleanup(void);


#endif /* __EVLOOP_H__ */
