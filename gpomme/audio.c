/*
 * mbpgtk - GTK application for use with mbpeventd
 *
 * $Id$
 *
 * Copyright (C) 2006 Soeren SONNENBURG <debian@nn7.de>
 * Copyright (C) 2006 Julien BLACHE <jb@jblache.org>
 *
 * Portions of the code below were shamelessly
 * stolen from pbbuttonsd. Thanks ! ;-)
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
#include <limits.h>

#include <pthread.h>

#include <alsa/asoundlib.h>

#include <audiofile.h>

#include "mbpgtk.h"
#include "audio.h"


struct dspdata _dsp;

/* Called from the main thread */
static struct sample *
load_sample(char *theme, char *name)
{
  AFfilehandle affd;     /* filehandle for soundfile from libaudiofile */
  AFframecount framecount;
  int dummy, channels, byteorder, framesize, precision, err = 0;
  struct sample *sample;

  char file[PATH_MAX];
  int ret;

  ret = snprintf(file, PATH_MAX, "%s/%s/%s", THEME_BASE, theme, name);
  if (ret >= PATH_MAX)
    return NULL;

  sample = (struct sample *) malloc(sizeof(struct sample));
  if (sample == NULL)
    return NULL;

  affd = afOpenFile(file, "r", 0);
  if (affd > 0)
    {
      afGetSampleFormat(affd, AF_DEFAULT_TRACK, &dummy, &precision);
      channels = afGetChannels(affd, AF_DEFAULT_TRACK);
      byteorder = afGetVirtualByteOrder(affd, AF_DEFAULT_TRACK);
      framesize = (int) afGetFrameSize(affd, AF_DEFAULT_TRACK, 0);
      framecount = afGetFrameCount(affd, AF_DEFAULT_TRACK);
      sample->speed = (int) afGetRate(affd, AF_DEFAULT_TRACK);

      if (channels <= 2)
	sample->channels = channels;
      else
	err = -1;

      if (precision == 8)
	sample->format = SND_PCM_FORMAT_S8;
      else if (precision == 16)
	{
	  if (byteorder == AF_BYTEORDER_LITTLEENDIAN)
	    sample->format = SND_PCM_FORMAT_S16_LE;
	  else
	    sample->format = SND_PCM_FORMAT_S16_BE;
	}
      else
	err = -1;

      if (err == 0)
	{
	  sample->framesize = framesize;
	  sample->framecount = framecount;
	  sample->audiodatalen = framecount * framesize;

	  sample->audiodata = (char *) malloc(sample->audiodatalen);
	  if (sample->audiodata != NULL)
	    {
	      ret = afReadFrames(affd, AF_DEFAULT_TRACK, sample->audiodata, framecount);
	      if (ret != framecount)
		{
		  free(sample->audiodata);
		  err = -1;
		}
	    }
	  else
	    ret = -1;
	}
      afCloseFile(affd);
    }

  if (err == -1)
    {
      free(sample);
      return NULL;
    }

  sample->periods = sample->framesize;
  sample->buffersize = (sample->periods * 8192) >> 2;

  return sample;
}


/* Called from the audio thread */
static void
play_sample(struct dspdata *dsp, int cmd)
{
  snd_pcm_t *pcm_handle;          
  snd_pcm_hw_params_t *hwparams;

  char *pcm_name = "default";

  struct sample *s = dsp->sample[cmd];

  snd_pcm_hw_params_alloca(&hwparams);

  if (snd_pcm_open(&pcm_handle, pcm_name, SND_PCM_STREAM_PLAYBACK, 0) < 0)
    {
      fprintf(stderr, "Error opening PCM device %s\n", pcm_name);
      return;
    }

  if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0)
    {
      fprintf(stderr, "Can not configure this PCM device.\n");
      return;
    }

  if (snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
    {
      fprintf(stderr, "Error setting access.\n");
      return;
    }

  if (snd_pcm_hw_params_set_format(pcm_handle, hwparams, s->format) < 0)
    {
      fprintf(stderr, "Error setting format.\n");
      return;
    }

  if (snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &s->speed, 0) < 0)
    {
      fprintf(stderr, "Error setting rate.\n");
      return;
    }

  /* Set number of channels */
  if (snd_pcm_hw_params_set_channels_near(pcm_handle, hwparams, &s->channels) < 0)
    {
      fprintf(stderr, "Error setting channels.\n");
      return;
    }

  /* Set number of periods. Periods used to be called fragments. */ 
  if (snd_pcm_hw_params_set_periods_near(pcm_handle, hwparams, &s->periods, 0) < 0)
    {
      fprintf(stderr, "Error setting periods.\n");
      return;
    }

  /* Set buffer size (in frames). The resulting latency is given by */
  /* latency = periodsize * periods / (rate * bytes_per_frame)     */
  if (snd_pcm_hw_params_set_buffer_size_near(pcm_handle, hwparams, &s->buffersize) < 0)
    {
      fprintf(stderr, "Error setting buffersize.\n");
      return;
    }

  /* Apply HW parameter settings to */
  /* PCM device and prepare device  */
  if (snd_pcm_hw_params(pcm_handle, hwparams) < 0)
    {
      fprintf(stderr, "Error setting HW params.\n");
      return;
    }

  int pcmreturn;
  /* Write num_frames frames from buffer data to    */ 
  /* the PCM device pointed to by pcm_handle.       */
  /* Returns the number of frames actually written. */
  while ((pcmreturn = snd_pcm_writei(pcm_handle, s->audiodata, s->framecount)) < 0)
    {
      snd_pcm_prepare(pcm_handle);
      fprintf(stderr, "<<<<<<<<<<<<<<< Buffer Underrun >>>>>>>>>>>>>>>\n");
    }

  /* Stop PCM device and drop pending frames */
  snd_pcm_drop(pcm_handle);

  /* Stop PCM device after pending frames have been played */ 
  if (snd_pcm_close(pcm_handle) < 0)
    {
      fprintf(stderr, "snd_pcm_close error\n");
      return;
    }
}


/* Called from the audio thread
 * Audio thread main loop
 */
void *
audio_thread (void *arg)
{
  struct dspdata *dsp = (struct dspdata *) arg;
  for (;;)
    {
      pthread_mutex_lock(&dsp->mutex);
      pthread_cond_wait(&dsp->cond, &dsp->mutex);
      pthread_mutex_unlock(&dsp->mutex);

      switch (dsp->command)
	{
	  case AUDIO_CLICK:
	    dsp->command = AUDIO_COMMAND_NONE;

	    play_sample(dsp, AUDIO_CLICK);
	    break;
	  case AUDIO_COMMAND_QUIT:
	    pthread_exit(NULL);
	    break;
	  case AUDIO_COMMAND_NONE:
	    break;
	}
    }

  return NULL;
}


/* Called from the main thread
 * This function wakes the audio thread
 */
void
audio_command(int command)
{
  if (!_dsp.sample)
    return;

  pthread_mutex_lock(&(_dsp.mutex));

  _dsp.command = command;

  pthread_cond_signal(&(_dsp.cond));
  pthread_mutex_unlock(&(_dsp.mutex));
}


/* Called from the main thread */
void
audio_cleanup(void)
{
  int i;

  if (!_dsp.sample)
    return;

  for (i = 0; i < AUDIO_N; i++)
    {
      if (_dsp.sample[i] == NULL)
	continue;

      if (_dsp.sample[i]->audiodata != NULL)
	free(_dsp.sample[i]->audiodata);

      free(_dsp.sample[i]);
    }

  pthread_mutex_destroy(&(_dsp.mutex));
  pthread_cond_destroy(&(_dsp.cond));
}

/* Called from the main thread
 * This function sets up the sound playing thread.
 * It starts the thread or if an error occur cleans
 * up all the audio stuff
 */
int
audio_init_thread(void)
{
  pthread_attr_t attr;
  int ret;

  _dsp.sample[AUDIO_CLICK] = load_sample("CrystalLarge", "click.wav");

  if (_dsp.sample[AUDIO_CLICK] == NULL)
    return -1;

  _dsp.thread = 0;

  pthread_mutex_init(&(_dsp.mutex), NULL);
  pthread_cond_init (&(_dsp.cond), NULL);
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  ret = pthread_create(&(_dsp.thread), &attr, audio_thread, (void *) &_dsp);
  if (ret != 0)
    {
      audio_cleanup();
      ret = -1;
    }

  pthread_attr_destroy(&attr);

  return ret;
}
