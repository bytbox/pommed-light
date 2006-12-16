/*
 * mbpeventd - MacBook Pro hotkey handler daemon
 *
 * $Id$
 *
 * Copyright (C) 2006 Romain Beauxis <toots@rastageeks.org>
 * Copyright (C) 2006 Julien BLACHE <jb@jblache.org>
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
#include <string.h>

#include <alsa/asoundlib.h>

#include "mbpeventd.h"
#include "conffile.h"
#include "audio.h"


static snd_mixer_t *mixer_hdl;
static snd_mixer_elem_t *vol_elem;
static snd_mixer_elem_t *spkr_elem;
static snd_mixer_elem_t *head_elem;

static long vol_min;
static long vol_max;
static long vol_step;


void
audio_step(int dir)
{
  long vol;
  long newvol;

  if (mixer_hdl == NULL)
    return;

  if (vol_elem == NULL)
    return;

  snd_mixer_handle_events(mixer_hdl);

  if (!snd_mixer_selem_is_active(vol_elem))
    return;

  snd_mixer_selem_get_playback_volume(vol_elem, 0, &vol);

  logdebug("Mixer volume: %ld\n", vol);

  if (dir == STEP_UP)
    {
      newvol = vol + vol_step;

      if (newvol > vol_max)
	newvol = vol_max;

      logdebug("Audio stepping +%ld -> %ld\n", vol_step, newvol);
    }
  else if (dir == STEP_DOWN)
    {
      newvol = vol - vol_step;

      if (newvol < vol_min)
	newvol = vol_min;

      logdebug("Audio stepping -%ld -> %ld\n", vol_step, newvol);
    }
  else
    return;

  snd_mixer_selem_set_playback_volume(vol_elem, 0, newvol);

  if (snd_mixer_selem_is_playback_mono(vol_elem) == 0)
    snd_mixer_selem_set_playback_volume(vol_elem, 1, newvol);
}


static void
audio_toggle_mute_elem(snd_mixer_elem_t *elem)
{
  int play_sw[2];

  if (snd_mixer_selem_is_active(elem)
      && snd_mixer_selem_has_playback_switch(elem))
    {
      snd_mixer_selem_get_playback_switch(elem, 0, &play_sw[0]);
      play_sw[0] = 1 - play_sw[0];

      if (snd_mixer_selem_is_playback_mono(elem) == 0)
	{
	  snd_mixer_selem_get_playback_switch(elem, 1, &play_sw[1]);
	  play_sw[1] = 1 - play_sw[1];
	}

      snd_mixer_selem_set_playback_switch(elem, 0, play_sw[0]);

      if (snd_mixer_selem_is_playback_mono(elem) == 0)
	snd_mixer_selem_set_playback_switch(elem, 1, play_sw[1]);
    }
}

void
audio_toggle_mute(void)
{
  if (mixer_hdl == NULL)
    return;

  snd_mixer_handle_events(mixer_hdl);

  if (spkr_elem != NULL)
    audio_toggle_mute_elem(spkr_elem);

  if (head_elem != NULL)
    audio_toggle_mute_elem(head_elem);
}


int
audio_init(void)
{
  snd_mixer_elem_t *elem;
  snd_mixer_selem_id_t *sid;

  double dvol;

  int ret;

  vol_elem = NULL;
  spkr_elem = NULL;
  head_elem = NULL;

  ret = snd_mixer_open(&mixer_hdl, 0);
  if (ret < 0)
    {
      logdebug("Failed to open mixer: %s\n", snd_strerror(ret));

      mixer_hdl = NULL;

      return -1;
    }

  ret = snd_mixer_attach(mixer_hdl, audio_cfg.card);
  if (ret < 0)
    {
      logdebug("Failed to attach mixer: %s\n", snd_strerror(ret));

      snd_mixer_close(mixer_hdl);

      return -1;
    }

  ret = snd_mixer_selem_register(mixer_hdl, NULL, NULL);
  if (ret < 0)
    {
      logdebug("Failed to register mixer: %s\n", snd_strerror(ret));

      snd_mixer_detach(mixer_hdl, audio_cfg.card);
      snd_mixer_close(mixer_hdl);

      return -1;
    }

  ret = snd_mixer_load(mixer_hdl);
  if (ret < 0)
    {
      logdebug("Failed to load mixer: %s\n", snd_strerror(ret));

      snd_mixer_detach(mixer_hdl, audio_cfg.card);
      snd_mixer_close(mixer_hdl);

      return -1;
    }


  /* Grab interesting elements */
  snd_mixer_selem_id_alloca(&sid);

  for (elem = snd_mixer_first_elem(mixer_hdl); elem; elem = snd_mixer_elem_next(elem)) 
    {
      snd_mixer_selem_get_id(elem, sid);

      if (strcmp(snd_mixer_selem_id_get_name(sid), audio_cfg.vol) == 0)
	vol_elem = elem;

      if (strcmp(snd_mixer_selem_id_get_name(sid), audio_cfg.spkr) == 0)
	spkr_elem = elem;

      if (strcmp(snd_mixer_selem_id_get_name(sid), audio_cfg.head) == 0)
	head_elem = elem;
    }

  logdebug("Audio init: volume %s, speakers %s, headphones %s\n",
	   (vol_elem == NULL) ? "NOK" : "OK",
	   (spkr_elem == NULL) ? "NOK" : "OK",
	   (head_elem == NULL) ? "NOK" : "OK");

  if ((vol_elem == NULL) || ((spkr_elem == NULL) && (head_elem == NULL)))
    {
      logdebug("Failed to open required mixer elements\n");

      audio_cleanup();

      return -1;
    }

  /* Get min & max volume */
  snd_mixer_selem_get_playback_volume_range(vol_elem, &vol_min, &vol_max);

  dvol = (double)(vol_max - vol_min) / 100.0;
  vol_step = (long)(dvol * (double)audio_cfg.step);

  logdebug("Audio init: min %ld, max %ld, step %ld\n", vol_min, vol_max, vol_step);

  /* Set initial volume if enabled */
  if (audio_cfg.init > -1)
    {
      dvol *= (double)audio_cfg.init;

      if ((long)dvol > vol_max)
	dvol = (double)vol_max;

      snd_mixer_selem_set_playback_volume(vol_elem, 0, (long)dvol);

      if (snd_mixer_selem_is_playback_mono(vol_elem) == 0)
	snd_mixer_selem_set_playback_volume(vol_elem, 1, (long)dvol);      
    }

  return 0;
}

void
audio_cleanup(void)
{
  if (mixer_hdl != NULL)
    {
      snd_mixer_detach(mixer_hdl, audio_cfg.card);
      snd_mixer_close(mixer_hdl);

      mixer_hdl = NULL;
    }
}


void
audio_fix_config(void)
{
  if (audio_cfg.init < 0)
    audio_cfg.init = -1;

  if (audio_cfg.init > 100)
    audio_cfg.init = 100;

  if (audio_cfg.step < 1)
    audio_cfg.step = 1;

  if (audio_cfg.step > 50)
    audio_cfg.step = 50;
}
