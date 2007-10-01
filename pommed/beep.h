/*
 * $Id$
 */

#ifndef __BEEP_H__
#define __BEEP_H__

struct _beep_info
{
  int fd;
};

extern struct _beep_info beep_info;


#define BEEP_DEFAULT_FILE    "/usr/share/pommed/beep.wav"
#define BEEP_DEVICE_NAME     "Pommed beeper device"

void
beep_beep(void);

int
beep_open_device(void);

void
beep_close_device(void);

int
beep_init(void);

void
beep_cleanup(void);

void
beep_fix_config(void);


/* Beep thread data definitions */
struct sample {
  char *audiodata;
  int audiodatalen;
  int format;
  unsigned int channels;
  unsigned int speed;
  unsigned int framesize;
  int framecount;
  unsigned int periods;
  unsigned long buffersize;
};

enum {
  AUDIO_COMMAND_NONE = -2,
  AUDIO_COMMAND_QUIT = -1,
  AUDIO_CLICK = 0,
  AUDIO_N /* keep this one last */
};

struct dspdata {
  int command;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  pthread_t thread;
  struct sample *sample[AUDIO_N];   /* sound to play */
};


#endif /* !__BEEP_H__ */
