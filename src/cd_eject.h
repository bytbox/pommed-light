/*
 * $Id$
 */

#ifndef __CD_EJECT_H__
#define __CD_EJECT_H__

#define CD_DVD_DEVICE           "/dev/dvd"
#define CD_CDROM_DEVICE         "/dev/cdrom"
#define CD_EJECT_CMD            "/usr/bin/eject"


void
cd_eject(void);

#endif /* !__CD_EJECT_H__ */

