/*
 * pommed - Apple laptops hotkeys handler daemon
 *
 * $Id$
 *
 * Copyright (C) 2006-2008 Julien BLACHE <jb@jblache.org>
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


static int kbd_timer;


/* simple backlight toggle */
void
kbd_backlight_toggle(void)
{
  int curval;

  curval = kbd_backlight_get();

  if (curval != KBD_BACKLIGHT_OFF)
    {
      kbd_bck_info.toggle_lvl = curval;
      kbd_backlight_set(KBD_BACKLIGHT_OFF, KBD_USER);
    }
  else
    {
      if (kbd_bck_info.toggle_lvl < kbd_cfg.auto_lvl)
	kbd_bck_info.toggle_lvl = kbd_cfg.auto_lvl;

      kbd_backlight_set(kbd_bck_info.toggle_lvl, KBD_USER);
    }
}


/* Automatic backlight */
void
kbd_backlight_inhibit_set(int mask)
{
  if (!kbd_bck_info.inhibit)
    kbd_bck_info.inhibit_lvl = kbd_bck_info.level;

  kbd_backlight_set(KBD_BACKLIGHT_OFF,
		    (mask & KBD_MASK_AUTO) ? (KBD_AUTO) : (KBD_USER));

  kbd_bck_info.inhibit |= mask;

  logdebug("KBD: inhibit set 0x%02x -> 0x%02x\n", mask, kbd_bck_info.inhibit);
}

void
kbd_backlight_inhibit_clear(int mask)
{
  int flag;

  flag = kbd_bck_info.inhibit & mask;

  kbd_bck_info.inhibit &= ~mask;

  logdebug("KBD: inhibit clear 0x%02x -> 0x%02x\n", mask, kbd_bck_info.inhibit);

  if (kbd_bck_info.inhibit || !flag)
    return;

  if (kbd_bck_info.auto_on)
    {
      kbd_bck_info.auto_on = 0;
      kbd_bck_info.inhibit_lvl = 0;
    }

  kbd_backlight_set(kbd_bck_info.inhibit_lvl,
		    (mask & KBD_MASK_AUTO) ? (KBD_AUTO) : (KBD_USER));
}

void
kbd_backlight_inhibit_toggle(int mask)
{
  if (kbd_bck_info.inhibit & mask)
    kbd_backlight_inhibit_clear(mask);
  else
    kbd_backlight_inhibit_set(mask);
}


void
kbd_backlight_ambient_check(void)
{
  int amb_r, amb_l;

  ambient_get(&amb_r, &amb_l);

  if ((amb_r < 0) || (amb_l < 0))
    return;

  mbpdbus_send_ambient_light(amb_l, kbd_bck_info.l_sens, amb_r, kbd_bck_info.r_sens);

  kbd_bck_info.r_sens = amb_r;
  kbd_bck_info.l_sens = amb_l;

  /* Inhibited */
  if (kbd_bck_info.inhibit)
    return;

  if ((amb_r < kbd_cfg.on_thresh) && (amb_l < kbd_cfg.on_thresh))
    {
      logdebug("Ambient light lower threshold reached\n");

      /* backlight already on */
      if (kbd_backlight_get() > KBD_BACKLIGHT_OFF)
	return;

      /* turn on backlight */
      kbd_bck_info.auto_on = 1;

      kbd_backlight_set(kbd_cfg.auto_lvl, KBD_AUTO);
    }
  else if (kbd_bck_info.auto_on)
    {
      if ((amb_r > kbd_cfg.off_thresh) || (amb_l > kbd_cfg.off_thresh))
	{
	  logdebug("Ambient light upper threshold reached\n");

	  kbd_bck_info.auto_on = 0;

	  kbd_backlight_set(KBD_BACKLIGHT_OFF, KBD_AUTO);
	}
    }
}


static void
kbd_auto_process(int id, uint64_t ticks)
{
  /* Increment keyboard backlight idle timer */
  kbd_bck_info.idle++;
  if ((kbd_cfg.idle > 0) && (kbd_bck_info.idle > kbd_cfg.idle))
    kbd_backlight_inhibit_set(KBD_INHIBIT_IDLE);

  kbd_backlight_ambient_check();
}


static int
kbd_auto_init(void)
{
  kbd_timer = evloop_add_timer(KBD_TIMEOUT, kbd_auto_process);
  if (kbd_timer < 0)
    return -1;

  return 0;
}

static void
kbd_auto_cleanup(void)
{
  if (kbd_timer > 0)
    evloop_remove_timer(kbd_timer);
}
