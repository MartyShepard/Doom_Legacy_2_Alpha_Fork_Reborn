// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2007-2008 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Map environmental effects.

#include "g_map.h"
#include "g_mapinfo.h"
#include "g_actor.h"
#include "g_decorate.h"
#include "p_effects.h"
#include "p_spec.h"

#include "m_random.h"
#include "r_data.h"
#include "sounds.h"



/// When something disturbs a liquid surface, we get a splash.
DActor *Map::SpawnSplash(const vec_t<fixed_t>& pos, fixed_t z, int sound, mobjtype_t base, mobjtype_t chunk, bool randtics)
{
  // spawn a base splash
  DActor *p = SpawnDActor(pos.x, pos.y, z, base);
  S_StartSound(p, sound);

  if (randtics)
    {
      p->tics -= P_Random() & 3;

      if (p->tics < 1)
	p->tics = 1;
    }

  if (chunk == MT_NONE)
    return p;

  // and possibly an additional chunk
  p = SpawnDActor(pos.x, pos.y, z, chunk);
  return p;
}


/*!
  Spawn a blood sprite with falling z movement, at given location.
  The duration and first sprite frame depends on the damage level.
  The more damage, the longer is the sprite animation
*/
DActor *Map::SpawnBlood(const vec_t<fixed_t>& r, int damage)
{
  DActor *th = SpawnDActor(r.x, r.y, r.z + 4*RandomS(), MT_BLOOD);

  th->vel.Set(16*RandomS(), 16*RandomS(), 2.0f);
  th->tics -= P_Random()&3;

  if (th->tics < 1)
    th->tics = 1;

  if (damage <= 12 && damage >= 9)
    th->SetState(S_BLOOD2);
  else if (damage < 9)
    th->SetState(S_BLOOD3);

  return th;
}


/// When player gets hurt by lava/slime, spawn at feet.
void Map::SpawnSmoke(const vec_t<fixed_t>& r)
{
  // x,y offsets were (P_Random() & 8) - 4, meaning either -4 or 4
  DActor *th = SpawnDActor(r + vec_t<fixed_t>(8*Random()-4, 8*Random()-4, 3*Random()), MT_SMOK);
  th->vel.z = 1;
  th->tics -= P_Random() & 3;

  if (th->tics < 1)
    th->tics = 1;
}


/// Spawn a "puff" sprite denoting a weapon hitting a thing/wall.
void Map::SpawnPuff(const vec_t<fixed_t>& r, mobjtype_t pufftype, bool hit_thing)
{
  vec_t<fixed_t> p = r;
  p.z += 4*RandomS();

  DActor *puff = SpawnDActor(p, pufftype);

  if (hit_thing && puff->info->seesound)
    S_StartSound(puff, puff->info->seesound); // Hit thing sound
  else if (puff->info->attacksound)
    S_StartSound(puff, puff->info->attacksound);


  switch (pufftype)
    {
    case MT_PUFF:
      puff->tics -= P_Random()&3;
      if (puff->tics < 1)
	puff->tics = 1;
        
      // TODO Doom fist puffs used this (smaller puff, avoid sparks): puff->SetState(S_PUFF3);
      // fallthru
    case MT_PUNCHPUFF:
    case MT_BEAKPUFF:
    case MT_STAFFPUFF:
      puff->vel.z = 1;
      break;
    case MT_HAMMERPUFF:
    case MT_GAUNTLETPUFF1:
    case MT_GAUNTLETPUFF2:
      puff->vel.z = 0.8f;
      break;
    default:
      break;
    }
}




//========================================================
//  Hexen lightning effect
//========================================================

MapEffect::MapEffect(Map *m)
{
  mp = m;

  if (!m->info->lightning)
    return;

  sector_t *s = m->sectors;
  sectorflash_t f = {NULL, 0};
  for (int i = 0; i < m->numsectors; i++)
    {
      if (s[i].SkyCeiling()
	  || s[i].special == SS_IndoorLightning1
	  || s[i].special == SS_IndoorLightning2)
	{
	  f.sec = &s[i];
	  flash_sectors.push_back(f);
	}
    }

  if (flash_sectors.empty())
    {
      m->info->lightning = false; // no way to see the flashes
      return;
    }

  flash_duration = 0;
  flash_delay = ((P_Random() & 15) + 5) * TICRATE; // first flash
}



bool MapEffect::Force()
{
  flash_delay = 0;
  return true;
}



void MapEffect::LightningFlash()
{
  flash_duration--;
  if (flash_duration > 0)
    {
      // flash is fading
      int n = flash_sectors.size();
      for (int i = 0; i < n; i++)
	{
	  sector_t *s = flash_sectors[i].sec;
	  if (flash_sectors[i].orig_light < s->lightlevel - 4)
	    s->lightlevel -= 4;
	}
    }					
  else if (flash_duration == 0)
    {
      // flash is over, return pre-flash light levels
      int n = flash_sectors.size();
      for (int i = 0; i < n; i++)
	flash_sectors[i].sec->lightlevel = flash_sectors[i].orig_light;
      
      mp->skytexture = materials.Get(mp->info->sky1.c_str(), TEX_wall); // set default sky
    }

  flash_delay--;
  if (flash_delay > 0)
    return;

  // new flash
  flash_duration = (P_Random() & 7) + 8;
  short flash = 200 + (P_Random() & 31);

  int n = flash_sectors.size();
  for (int i = 0; i < n; i++)
    {
      sector_t *s = flash_sectors[i].sec;
      flash_sectors[i].orig_light = s->lightlevel; // record current lightlevel

      // add more light
      if (s->special == SS_IndoorLightning1)
	s->lightlevel += 64;
      else if (s->special == SS_IndoorLightning2)
	s->lightlevel += 32;
      else
	s->lightlevel = flash;

      // clamp between original light level and flash level
      s->lightlevel = max(min(s->lightlevel, flash), flash_sectors[i].orig_light);
    }

  mp->skytexture = materials.Get(mp->info->sky2.c_str(), TEX_wall); // set alternate sky
  S_StartAmbSound(NULL, SFX_THUNDER_CRASH);

  // how soon will we get the next flash?
  if (P_Random() < 50)
    flash_delay = (P_Random() & 15) + 16; // rapid second flash
  else
    {
      if (P_Random() < 128 && !(mp->maptic & 32))
	flash_delay = ((P_Random() & 7) + 2) * TICRATE;
      else
	flash_delay = ((P_Random() & 15) + 5) * TICRATE;
    }
}
