/*
 * pommed - Apple laptops hotkeys handler daemon
 *
 * Copyright (C) 2006-2008 Julien BLACHE <jb@jblache.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include <syslog.h>

#include <errno.h>

#include <sys/epoll.h>

#ifndef NO_SYS_TIMERFD_H
# include <sys/timerfd.h>
#else
# include "timerfd-syscalls.h"
#endif

#include "pommed.h"
#include "evloop.h"


/* epoll fd */
static int epfd;

/* event sources registered on the main loop */
static struct pommed_event *sources;

/* timers */
static struct pommed_timer *timers;
static int timer_job_id;

static int running;


int
evloop_add(int fd, uint32_t events, pommed_event_cb cb)
{
  int ret;

  struct epoll_event epoll_ev;
  struct pommed_event *pommed_ev;

  pommed_ev = (struct pommed_event *)malloc(sizeof(*pommed_ev));

  if (pommed_ev == NULL)
    {
      logmsg(LOG_ERR, "Could not allocate memory for new source");

      return -1;
    }

  pommed_ev->fd = fd;
  pommed_ev->cb = cb;
  pommed_ev->next = sources;

  epoll_ev.events = events;
  epoll_ev.data.ptr = pommed_ev;

  ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &epoll_ev);

  if (ret < 0)
    {
      logmsg(LOG_ERR, "Could not add source to epoll: %s", strerror(errno));

      free(pommed_ev);
      return -1;
    }

  sources = pommed_ev;

  return 0;
}

int
evloop_remove(int fd)
{
  int ret;

  struct pommed_event *p;
  struct pommed_event *e;

  ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);

  if (ret < 0)
    {
      logmsg(LOG_ERR, "Could not remove source from epoll: %s", strerror(errno));

      return -1;
    }

  for (p = NULL, e = sources; e != NULL; p = e, e = e->next)
    {
      if (e->fd != fd)
	continue;

      if (p != NULL)
	p->next = e->next;
      else
	sources = e->next;

      free(e);

      break;
    }

  return 0;
}


static void
evloop_timer_callback(int fd, uint32_t events)
{
  uint64_t ticks;

  struct pommed_timer *t;
  struct pommed_timer_job *j;

  /* Acknowledge timer */
  read(fd, &ticks, sizeof(ticks));

  j = NULL;
  for (t = timers; t != NULL; t = t->next)
    {
      if (t->fd == fd)
	{
	  j = t->jobs;

	  break;
	}
    }

  while (j != NULL)
    {
      j->cb(j->id, ticks);

      j = j->next;
    }
}

static int
evloop_create_timer(int timeout)
{
  int fd;
  int ret;

  struct itimerspec timing;

  fd = timerfd_create(CLOCK_MONOTONIC, 0);
  if (fd < 0)
    {
      logmsg(LOG_ERR, "Could not create timer: %s", strerror(errno));

      return -1;
    }

  timing.it_interval.tv_sec = (timeout >= 1000) ? timeout / 1000 : 0;
  timing.it_interval.tv_nsec = (timeout - (timing.it_interval.tv_sec * 1000)) * 1000000;

  ret = clock_gettime(CLOCK_MONOTONIC, &timing.it_value);
  if (ret < 0)
    {
      logmsg(LOG_ERR, "Could not get current time: %s", strerror(errno));

      close(fd);
      return -1;
    }

  timing.it_value.tv_sec += timing.it_interval.tv_sec;
  timing.it_value.tv_nsec += timing.it_interval.tv_nsec;
  if (timing.it_value.tv_nsec > 1000000000)
    {
      timing.it_value.tv_sec++;
      timing.it_value.tv_nsec -= 1000000000;
    }

  ret = timerfd_settime(fd, TFD_TIMER_ABSTIME, &timing, NULL);
  if (ret < 0)
    {
      logmsg(LOG_ERR, "Could not setup timer: %s", strerror(errno));

      close(fd);
      return -1;
    }

  ret = evloop_add(fd, EPOLLIN, evloop_timer_callback);
  if (ret < 0)
    {
      close(fd);
      return -1;
    }

  return fd;
}

int
evloop_add_timer(int timeout, pommed_timer_cb cb)
{
  int fd;

  struct pommed_timer *t;
  struct pommed_timer_job *j;

  j = (struct pommed_timer_job *)malloc(sizeof(struct pommed_timer_job));
  if (j == NULL)
    {
      logmsg(LOG_ERR, "Could not allocate memory for timer job");
      return -1;
    }

  j->cb = cb;
  j->id = timer_job_id;
  timer_job_id++;

  for (t = timers; t != NULL; t = t->next)
    {
      if (t->timeout == timeout)
	break;
    }

  if (t == NULL)
    {
      t = (struct pommed_timer *)malloc(sizeof(struct pommed_timer));
      if (t == NULL)
	{
	  logmsg(LOG_ERR, "Could not allocate memory for timer");
	  return -1;
	}

      fd = evloop_create_timer(timeout);
      if (fd < 0)
	{
	  free(t);
	  return -1;
	}

      t->fd = fd;
      t->timeout = timeout;
      t->jobs = NULL;
      t->next = timers;
      timers = t;
    }

  j->next = t->jobs;
  t->jobs = j;

  return 0;
}

int
evloop_remove_timer(int id)
{
  int found;
  int ret;

  struct pommed_timer *t;
  struct pommed_timer *pt;
  struct pommed_timer_job *j;
  struct pommed_timer_job *pj;

  found = 0;
  for (pt = NULL, t = timers; t != NULL; pt = t, t = t->next)
    {
      for (pj = NULL, j = t->jobs; j != NULL; pj = j, j = j->next)
	{
	  if (j->id == id)
	    {
	      if (pj != NULL)
		pj->next = j->next;
	      else
		t->jobs = j->next;

	      free(j);

	      found = 1;

	      break;
	    }
	}

      if (found)
	break;
    }

  if (t == NULL)
    return 0;

  if (t->jobs == NULL)
    {
      ret = evloop_remove(t->fd);
      if (ret < 0)
	return ret;

      close(t->fd);

      if (pt != NULL)
	pt->next = t->next;
      else
	timers = t->next;

      free(t);
    }

  return 0;
}


int
evloop_iteration(void)
{
  int i;
  int nfds;

  struct epoll_event epoll_ev[MAX_EPOLL_EVENTS];
  struct pommed_event *pommed_ev;

  if (!running)
    return -1;

  nfds = epoll_wait(epfd, epoll_ev, MAX_EPOLL_EVENTS, -1);

  if (nfds < 0)
    {
      if (errno == EINTR)
	return 0; /* pommed.c will continue */
      else
	{
	  logmsg(LOG_ERR, "epoll_wait() error: %s", strerror(errno));

	  return -1; /* pommed.c will exit */
	}
    }

  for (i = 0; i < nfds; i++)
    {
      pommed_ev = epoll_ev[i].data.ptr;
      pommed_ev->cb(pommed_ev->fd, epoll_ev[i].events);
    }

  return nfds;
}

void
evloop_stop(void)
{
  running = 0;
}


int
evloop_init(void)
{
  sources = NULL;

  timers = NULL;
  timer_job_id = 0;

  running = 1;

  epfd = epoll_create(MAX_EPOLL_EVENTS);
  if (epfd < 0)
    {
      logmsg(LOG_ERR, "Could not create epoll fd: %s", strerror(errno));

      return -1;
    }

  return 0;
}

void
evloop_cleanup(void)
{
  struct pommed_event *p;
  struct pommed_timer *t;
  struct pommed_timer_job *j;
  struct pommed_timer_job *jobs;

  close(epfd);

  while (sources != NULL)
    {
      p = sources;
      sources = sources->next;

      close(p->fd);

      free(p);
    }

  while (timers != NULL)
    {
      t = timers;
      timers = timers->next;

      jobs = t->jobs;
      while (jobs != NULL)
	{
	  j = jobs;
	  jobs = jobs->next;

	  free(j);
	}

      free(t);
    }
}
