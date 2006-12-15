/*
 * $Id$
 */

#ifndef __AUDIO_H__
#define __AUDIO_H__


void
audio_step(int dir);

void
audio_toggle_mute(void);

int
audio_init(void);

void
audio_cleanup(void);


#endif /* !__AUDIO_H__ */
