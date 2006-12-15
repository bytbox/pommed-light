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
#include "audio.h"


int
audio_change(int mode, int value)
{
  snd_mixer_t *handle;
  snd_mixer_elem_t *elem;
  snd_mixer_selem_id_t *sid;

  int playback_switch;
  long vol;
  long max;
  long min;

  int ret;

  ret = snd_mixer_open(&handle, 0);
  if (ret < 0)
    {
      logdebug("Failed to open mixer: %s\n", snd_strerror(ret));
      return -1;
    }

  ret = snd_mixer_attach(handle, "default");
  if (ret < 0)
    {
      logdebug("Failed to attach mixer: %s\n", snd_strerror(ret));

      snd_mixer_close(handle);

      return -1;
    }

  ret = snd_mixer_selem_register(handle, NULL, NULL);
  if (ret < 0)
    {
      logdebug("Failed to register mixer: %s\n", snd_strerror(ret));

      snd_mixer_close(handle);

      return -1;
    }

  ret = snd_mixer_load(handle);
  if (ret < 0)
    {
      logdebug("Failed to load mixer: %s\n", snd_strerror(ret));

      snd_mixer_close(handle);

      return -1;
    }

  int result = -2;
  for (elem = snd_mixer_first_elem(handle); elem; elem = snd_mixer_elem_next(elem)) 
    {
      snd_mixer_selem_id_alloca(&sid);
      snd_mixer_selem_get_id(elem, sid);

      if (!snd_mixer_selem_is_active(elem))
	continue;

      switch(mode)
	{
	  case MODE_VOLUME:
	    if (snd_mixer_selem_has_playback_volume(elem)) 
	      {
		if (strcmp(snd_mixer_selem_id_get_name(sid), "PCM") == 0)
		  {
		    snd_mixer_selem_get_playback_volume(elem, 0, &vol);
		    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);

		    vol += value;

		    if (vol < min)
		      vol = min;
		    else if (vol > max)
		      vol = max;

		    snd_mixer_selem_set_playback_volume(elem, 0, vol);

		    if (snd_mixer_selem_is_playback_mono(elem) == 0)
		      snd_mixer_selem_set_playback_volume(elem, 1, vol);

		    return 0;
		  }
	      }
	    break;

	  case MODE_MUTE:
	    if (snd_mixer_selem_has_playback_switch(elem)) 
	      {
		if ((strcmp(snd_mixer_selem_id_get_name(sid), "Front") == 0)
		    || (strcmp(snd_mixer_selem_id_get_name(sid), "Headphone") == 0))
		  {
		    snd_mixer_selem_get_playback_switch(elem, 0, &playback_switch);

		    playback_switch = 1 - playback_switch;

		    snd_mixer_selem_set_playback_switch(elem, 0, playback_switch);

		    if (snd_mixer_selem_is_playback_mono(elem) == 0)
		      snd_mixer_selem_set_playback_switch(elem, 1, playback_switch);

		    result += 1;
		  }
	      }
	    break;

	  default:
	    break;
	}

    }

  return result;
}
