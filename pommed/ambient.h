/*
 * pommed - ambient.h
 */

#ifndef __AMBIENT_H__
#define __AMBIENT_H__

#define KBD_AMBIENT_MIN         0
#define KBD_AMBIENT_MAX         255

#ifdef __powerpc__
/* I2C ioctl */
# define I2C_SLAVE           0x0703

# define ADB_DEVICE          "/dev/adb"
# define ADB_BUFFER_SIZE     32

struct _lmu_info
{
  unsigned int lmuaddr;  /* i2c bus address */
  char i2cdev[16];       /* i2c bus device */
};

extern struct _lmu_info lmu_info;

#endif /* !__powerpc__ */


struct _ambient_info
{
  int left;
  int right;
  int max;
};

extern struct _ambient_info ambient_info;


void
ambient_get(int *r, int *l);

void
ambient_init(int *r, int *l);


#endif /* !__AMBIENT_H__ */
