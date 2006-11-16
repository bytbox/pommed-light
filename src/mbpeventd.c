#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>

#include <errno.h>

#include <linux/input.h>

#define DEBUG

#ifdef DEBUG
# define debug(fmt, args...) printf(fmt, ##args);
#else
# define debug(fmt, args...)
#endif


#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)    ((array[LONG(bit)] >> OFF(bit)) & 1)


#define USB_VENDOR_ID_APPLE           0x05ac
#define USB_PRODUCT_ID_GEYSER4_ISO    0x021b


#define K_LCD_BCK_DOWN          0xe0
#define K_LCD_BCK_UP            0xe1
#define K_AUDIO_MUTE            0x71
#define K_AUDIO_DOWN            0x72
#define K_AUDIO_UP              0x73
#define K_VIDEO_TOGGLE          0xe3
#define K_KBD_BCK_OFF           0xe4
#define K_KBD_BCK_DOWN          0xe5
#define K_KBD_BCK_UP            0xe6
#define K_CD_EJECT              0xa1

#define LCD_BCK_STEP            10
#define KBD_BCK_STEP            10

#define EVDEV_BASE              "/dev/input/event"
#define MAX_EVDEV               32

#define KBD_BACKLIGHT           "/sys/class/leds/smc:kbd_backlight/brightness"
#define KBD_AMBIENT             "/sys/devices/platform/applesmc/light"
#define KBD_AMBIENT_THRESHOLD   20
#define KBD_BACKLIGHT_DEFAULT   100

#define MBP_EVDEV_TIMEOUT       100


struct
{
  int off;    /* turned off ? */
  int r_sens; /* right sensor */
  int l_sens; /* left sensor */
} kbd_bck_status;


int
kbd_backlight_get(void)
{
  int fd;
  int ret;
  char buf[8];

  fd = open(KBD_BACKLIGHT, O_RDONLY);
  if (fd < 0)
    return -1;

  memset(buf, 0, 8);

  ret = read(fd, buf, 8);

  close(fd);

  if ((ret < 1) || (ret > 7))
    return -1;

  ret = atoi(buf);

  debug("KBD backlight value is %d\n", ret);

  return ret;
}

void
kbd_backlight_set(int val)
{
  int fd;
  int ret;
  int curval;
  char buf[8];

  curval = kbd_backlight_get();

  if (val == curval)
    return;

  fd = open(KBD_BACKLIGHT, O_RDWR | O_APPEND);
  if (fd < 0)
    return;

  ret = snprintf(buf, 8, "%d\n", val);

  if ((ret <= 0) || (ret > 7))
    return;

  ret = write(fd, buf, ret);

  debug("KBD backlight value set to %d\n", val);

  close(fd);
}

void
kbd_backlight_step(int dir)
{
  int val;
  int newval;

  val = kbd_backlight_get();

  if (val < 0)
    return;

  if (dir > 0)
    {
      newval = val + KBD_BCK_STEP;

      if (newval > 255)
	newval = 255;

      debug("KBD stepping +%d -> %d\n", KBD_BCK_STEP, newval);
    }
  else
    {
      newval = val - KBD_BCK_STEP;

      if (newval < 0)
	newval = 0;

      debug("KBD stepping -%d -> %d\n", KBD_BCK_STEP, newval);
    }

  kbd_backlight_set(newval);
}

void
kbd_backlight_ambient_get(int *r, int *l)
{
  int fd;
  int ret;
  char buf[16];
  char *p;

  fd = open(KBD_AMBIENT, O_RDONLY);
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

void
kbd_backlight_status_init(void)
{
  kbd_bck_status.off = 0;

  kbd_backlight_ambient_get(&kbd_bck_status.r_sens, &kbd_bck_status.l_sens);
}

void
kbd_backlight_ambient_check(void)
{
  int amb_r, amb_l;

  kbd_backlight_ambient_get(&amb_r, &amb_l);

  if ((amb_r < 0) || (amb_l < 0))
    return;

  if ((amb_r < KBD_AMBIENT_THRESHOLD) || (amb_l < KBD_AMBIENT_THRESHOLD))
    {
      kbd_bck_status.off = 0;
      kbd_bck_status.r_sens = amb_r;
      kbd_bck_status.l_sens = amb_l;

      kbd_backlight_set(KBD_BACKLIGHT_DEFAULT);
    }
}


void
process_evdev_events(int fd)
{
  int ret;

  struct input_event ev;

  ret = read(fd, &ev, sizeof(struct input_event));

  if (ret != sizeof(struct input_event))
    return;

  if (ev.type == EV_KEY)
    {
      /* key released - we don't care */
      if (ev.value == 0)
	return;

      switch (ev.code)
	{
	  case K_LCD_BCK_DOWN:
	    debug("\nKEY: LCD backlight down\n");
	    break;

	  case K_LCD_BCK_UP:
	    debug("\nKEY: LCD backlight up\n");
	    break;

	  case K_AUDIO_MUTE:
	    debug("\nKEY: audio mute\n");
	    break;

	  case K_AUDIO_DOWN:
	    debug("\nKEY: audio down\n");
	    break;

	  case K_AUDIO_UP:
	    debug("\nKEY: audio up\n");
	    break;

	  case K_VIDEO_TOGGLE:
	    debug("\nKEY: video toggle\n");
	    break;

	  case K_KBD_BCK_OFF:
	    debug("\nKEY: keyboard backlight off\n");

	    kbd_backlight_set(0);
	    break;

	  case K_KBD_BCK_DOWN:
	    debug("\nKEY: keyboard backlight down\n");

	    kbd_backlight_step(-1);
	    break;

	  case K_KBD_BCK_UP:
	    debug("\nKEY: keyboard backlight up\n");

	    kbd_backlight_step(1);
	    break;

	  case K_CD_EJECT:
	    debug("\nKEY: CD eject\n");
	    break;

	  default:
	    break;
	}
    }
}


int
evdev_is_geyser4(unsigned short vendor, unsigned short product)
{
  if (vendor != USB_VENDOR_ID_APPLE)
    return 0;

  return (product == USB_PRODUCT_ID_GEYSER4_ISO);
}

int
open_evdev(struct pollfd **fds)
{
  int ret;
  int i, j;

  int found = 0;
  int fd[32];

  unsigned short id[4];
  unsigned long bit[EV_MAX][NBITS(KEY_MAX)];
  char evdev[32];

  for (i = 0; i < MAX_EVDEV; i++)
    {
      ret = snprintf(evdev, 32, "%s%d", EVDEV_BASE, i);

      if ((ret <= 0) || (ret > 31))
	return -1;

      fd[i] = open(evdev, O_RDWR);
      if (fd[i] < 0)
	{
	  if (errno != ENOENT)
	    debug("Could not open %s: %s\n", evdev, strerror(errno));

	  continue;
	}

      ioctl(fd[i], EVIOCGID, id);

      if (!evdev_is_geyser4(id[ID_VENDOR], id[ID_PRODUCT]))
	{
	  debug("Discarding evdev %d vid 0x%04x, pid 0x%04x\n", i, id[ID_VENDOR], id[ID_PRODUCT]);

	  close(fd[i]);
	  fd[i] = -1;

	  continue;
	}

      memset(bit, 0, sizeof(bit));

      ioctl(fd[i], EVIOCGBIT(0, EV_MAX), bit[0]);

      if (test_bit(2, bit[0]))
	{
	  debug("Discarding evdev %d with event type > 2 (not a keyboard)\n", i);

	  close(fd[i]);
	  fd[i] = -1;

	  continue;
	}

      found++;
    }

  debug("Found %d devices\n");

  *fds = (struct pollfd *) malloc(found * sizeof(struct pollfd));

  if (*fds == NULL)
    {
      for (i = 0; i < MAX_EVDEV; i++)
	{
	  if (fd[i] > 0)
	    close(fd[i]);
	}

      debug("Out of memory for %d pollfd structs\n", found);

      return -1;
    }

  j = 0;
  for (i = 0; i < MAX_EVDEV && j < found; i++)
    {
      if (fd[i] < 0)
	continue;

      (*fds)[j].fd = fd[i];
      (*fds)[j].events = POLLIN;
      j++;
    }

  return found;
}


int
main (int argc, char **argv)
{
  int ret;
  int i;

  struct pollfd *fds;
  int nfds;

  nfds = open_evdev(&fds);
  if (nfds < 1)
    {
      fprintf(stderr, "Error: no devices found\n");

      exit(1);
    }

  kbd_backlight_status_init();

  while (1)
    {
      ret = poll(fds, nfds, MBP_EVDEV_TIMEOUT);

      if (ret < 0) /* error */
	{
	  debug("Poll error: %s\n", strerror(errno));

	  break;
	}
      else
	{
	  if (ret != 0)
	    {
	      for (i = 0; i < nfds; i++)
		{
		  if (fds[i].revents & POLLIN)
		    process_evdev_events(fds[i].fd);
		}
	    }

	  kbd_backlight_ambient_check();
	}
    }

  if (fds != NULL)
    {
      for (i = 0; i < nfds; i++)
	close(fds[i].fd);

      free(fds);
    }

  return 0;
}
