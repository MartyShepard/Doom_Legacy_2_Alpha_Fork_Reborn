// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: p_things.cpp 522 2008-01-20 22:42:23Z smite-meister $
//
// Copyright (C) 2003-2007 by DooM Legacy Team.
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
/// \brief Mapthing-related event functions.

#include "command.h"
#include "cvars.h"
#include "g_actor.h"
#include "g_map.h"
#include "g_decorate.h"

#include "sounds.h"
#include "tables.h"


bool DActor::Activate()
{
  if (flags & MF_MONSTER)
    {
      if (flags2 & MF2_DORMANT)
	{
	  flags2 &= ~MF2_DORMANT;
	  tics = 1;
	  return true;
	}
      return false;
    }

  // NOTE: for non-MF_MONSTERs, meleestate (attacksound) is the active state (sound), seestate (seesound) are the deactivated ones.
  // TODO spikes, bell

  switch (type)
    {
    case MT_THRUSTFLOOR_UP:
    case MT_THRUSTFLOOR_DOWN:
      if (args[0]==0)
	{
	  S_StartSound(this, SFX_THRUSTSPIKE_LOWER);
	  flags2 &= ~MF2_DONTDRAW;
	  if (args[1])
	    SetState(S_BTHRUSTRAISE1);
	  else
	    SetState(S_THRUSTRAISE1);
	}
      break;

    case MT_ZBELL:
      if (health > 0)
	Damage(NULL, NULL, 10); // 'ring' the bell
      break;

    default:
      if (info->attacksound)
	S_StartSound(this, info->attacksound);

      if (info->meleestate)
	SetState(info->meleestate);
      else
	return false;

      break;
    }
  return true;
}



bool DActor::Deactivate()
{
  if (flags & MF_MONSTER)
    {
      if(!(flags2 & MF2_DORMANT))
	{
	  flags2 |= MF2_DORMANT;
	  tics = -1;
	  return true;
	}
      return false;
    }

  switch (type)
    {
    case MT_THRUSTFLOOR_UP:
    case MT_THRUSTFLOOR_DOWN:
      if (args[0]==1)
	{
	  S_StartSound(this, SFX_THRUSTSPIKE_RAISE);
	  if (args[1])
	    SetState(S_BTHRUSTLOWER);
	  else
	    SetState(S_THRUSTLOWER);
	}
      break;

    default:
      //if (info->seesound)
      //  S_StartSound(this, info->seesound);

      if (info->seestate)
	SetState(info->seestate);
      else
	return false;

      break;
    }
  return true;
}


/// Shoot a projectile from all mapthings with the given TID
bool Map::EV_ThingProjectile(int tid, const ActorInfo *ai, angle_t angle, float hspeed, float vspeed, bool gravity)
{
  if (cv_nomonsters.value && (ai->flags & MF_MONSTER))   
    return false;

  // projectile velocity
  vec_t<fixed_t> pvel(Cos(angle)*hspeed, Sin(angle)*hspeed, vspeed);

  bool ret = false;
  Iterate_TID iter(this, tid);
  for (Actor *m = iter.Next(); m; m = iter.Next())
    {
      DActor *p = SpawnDActor(m->pos, ai);
      if (p->info->seesound)
	S_StartSound(p, p->info->seesound);

      p->owner = m;
      p->yaw   = angle;
      p->vel = pvel;
      p->flags |= MF_DROPPED; // no respawning
      if (gravity)
	{
	  p->flags &= ~MF_NOGRAVITY;
	  p->flags2 |= MF2_LOGRAV;
	}

      if (p->CheckMissileSpawn())
	ret = true;
    }
  return ret;
}


/// Spawn a mapthing from a limited selection of types.
bool Map::EV_ThingSpawn(int tid, const ActorInfo *ai, angle_t angle, bool fog)
{
  if (cv_nomonsters.value && (ai->flags & MF_MONSTER))
    return false;

  bool ret = false;
  Iterate_TID iter(this, tid);
  for (Actor *m = iter.Next(); m; m = iter.Next())
    {
      DActor *a = SpawnDActor(m->pos, ai);

      if (!a->TestLocation())
	a->Remove(); // Didn't fit
      else
	{
	  a->yaw = angle;
	  if (fog)
	    {	      
	      DActor *f = SpawnDActor(m->pos.x, m->pos.y, m->pos.z + TELEFOGHEIGHT, MT_TFOG);
	      S_StartSound(f, sfx_teleport);
	    }

	  a->flags |= MF_DROPPED; // no respawning
	  if (a->flags2 & MF2_FLOATBOB)
	    a->special1 = (a->pos.z - a->floorz).floor(); // floating height

	  ret = true;
	}
    }
  return ret;
}


bool Map::EV_ThingActivate(int tid)
{
  bool ret = false;
  Iterate_TID iter(this, tid);
  for (Actor *m = iter.Next(); m; m = iter.Next())
    {
      DActor *d = m->Inherits<DActor>();
      if (d && d->Activate())
	ret = true;
    }
  return ret;
}


bool Map::EV_ThingDeactivate(int tid)
{
  bool ret = false;
  Iterate_TID iter(this, tid);
  for (Actor *m = iter.Next(); m; m = iter.Next())
    {
      DActor *d = m->Inherits<DActor>();
      if (d && d->Deactivate())
	ret = true;
    }
  return ret;
}


bool Map::EV_ThingRemove(int tid)
{
  bool ret = false;
  Iterate_TID iter(this, tid);
  for (Actor *m = iter.Next(); m; m = iter.Next())
    {
      m->Remove();
      ret = true;
    }
  return ret;
}


bool Map::EV_ThingDestroy(int tid)
{
  bool ret = false;
  Iterate_TID iter(this, tid);
  for (Actor *m = iter.Next(); m; m = iter.Next())
    if (m->flags & MF_SHOOTABLE)
      {
	m->Damage(NULL, NULL, 10000, dt_always);
	ret = true;
      }

  return ret;
}
