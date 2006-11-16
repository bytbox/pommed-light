/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "mbpeventd.h"

void
cd_eject(void)
{
  int ret;

  ret = system(CD_EJECT_CMD " " CD_DVD_DEVICE);

  if (WEXITSTATUS(ret) != 0)
    {
      /* 127 means "shell not available" */
      if (WEXITSTATUS(ret) != 127)
	system(CD_EJECT_CMD " " CD_CDROM_DEVICE);
    }
}
