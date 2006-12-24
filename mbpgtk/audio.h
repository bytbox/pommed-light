/*
 * $Id$
 */

#ifndef __AUDIO_H__
#define __AUDIO_H__

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


void
audio_command(int command);

void
audio_cleanup(void);

int
audio_init_thread(void);


#endif /* !__AUDIO_H__ */
