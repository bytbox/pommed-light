/*
 * $Id$
 */

#ifndef __AUDIO_H__
#define __AUDIO_H__


struct _audio_info
{
  int level;
  int max;
  int muted;
};

extern struct _audio_info audio_info;


void
audio_step(int dir);

void
audio_toggle_mute(void);

int
audio_init(void);

void
audio_cleanup(void);

void
audio_fix_config(void);


#endif /* !__AUDIO_H__ */
