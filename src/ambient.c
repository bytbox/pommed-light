/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "mbpeventd.h"
#include "ambient.h"

void
ambient_get(int *r, int *l)
{
  int fd;
  int ret;
  char buf[16];
  char *p;

  fd = open(KBD_AMBIENT_SENSOR, O_RDONLY);
  if (fd < 0)
    {
      *r = -1;
      *l = -1;

      return;
    }

  ret = read(fd, buf, 16);

  if ((ret <= 0) || (ret > 15))
    {
      *r = -1;
      *l = -1;

      return;
    }

  buf[strlen(buf)] = '\0';

  p = strchr(buf, ',');
  *p++ = '\0';
  *r = atoi(p);

  p = buf + 1;
  *l = atoi(p);

  debug("Ambient light: right %d, left %d\n", *r, *l);
  
  close(fd);
}
