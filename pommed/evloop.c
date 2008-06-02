/*
 * pommed - Apple laptops hotkeys handler daemon
 *
 * $Id$
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

#include <syslog.h>

#include <errno.h>

#include <sys/epoll.h>

#include "pommed.h"
#include "evloop.h"


/* epoll fd */
static int epfd;

/* event sources registered on the main loop */
static struct pommed_event *sources;


int
evloop_add(int fd, uint32_t events, pommed_event_cb cb)
{
  int ret;

  struct epoll_event epoll_ev;
  struct pommed_event *pommed_ev;

  pommed_ev = (struct pommed_event *)malloc(sizeof(*pommed_ev));

  if (pommed_ev == NULL)
    {
      logmsg(LOG_ERR, "Could not allocate memory for new event");

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
      logmsg(LOG_ERR, "Could not add device to epoll: %s", strerror(errno));

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
      logmsg(LOG_ERR, "Could not remove device from epoll: %s", strerror(errno));

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


int
evloop_iteration(void)
{
  int i;
  int nfds;

  struct epoll_event epoll_ev[MAX_EPOLL_EVENTS];
  struct pommed_event *pommed_ev;

  nfds = epoll_wait(epfd, epoll_ev, MAX_EPOLL_EVENTS, LOOP_TIMEOUT);

  if (nfds < 0)
    {
      if (errno == EINTR)
	return 1; /* pommed.c will go on with the events management */
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


int
evloop_init(void)
{
  sources = NULL;

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

  close(epfd);

  while (sources != NULL)
    {
      p = sources;
      sources = sources->next;

      close(p->fd);

      free(p);
    }
}
