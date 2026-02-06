// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_hacks.cpp 522 2008-01-20 22:42:23Z smite-meister $
//
// Copyright (C) 2004-2006 by DooM Legacy Team.
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
/// \brief Re-implementation of some original Doom "features"

#include "p_hacks.h"

#include "g_player.h"
#include "g_map.h"

#include "r_defs.h"


VoodooDoll::VoodooDoll(const PlayerPawn &p)
  : PlayerPawn(p)
{
  flags  = MF_SOLID | MF_SHOOTABLE | MF_DROPOFF | MF_PICKUP | MF_PLAYER;
  flags2 = MF2_WINDTHRUST | MF2_PUSHABLE | MF2_SLIDE | MF2_TELESTOMP | MF2_PUSHWALL;
  eflags = 0;
  //pres->color = 2;

  // NOTE: because of the default copy constructor, all pointer members are potential trouble!

  player = NULL; // so ~PlayerPawn does not cause trouble
 
  // victim's geometry links (will get its own in Map::SpawnActor()) TODO should be part of Actor's copy constructor...
  sprev = snext = bprev = bnext = NULL;
  touching_sectorlist = NULL; // msecnode_t chain
  subsector = NULL;
}



VoodooDoll::~VoodooDoll()
{
  pres = NULL; // (has no presentation of its own, so Actor must not delete it!)
}



void VoodooDoll::CheckPointers()
{
  if (owner && (owner->eflags & MFE_REMOVE))
    owner = NULL;

  if (target && (target->eflags & MFE_REMOVE))
    target = NULL;

  if (attacker && (attacker->eflags & MFE_REMOVE))
    attacker = NULL;

  if (victim && (victim->eflags & MFE_REMOVE))
    {
      victim = NULL;
      Remove(); // deleted when its victim is deleted
    }
}



void VoodooDoll::Spawn(PlayerInfo *p, mapthing_t *mthing)
{
  VoodooDoll *d = new VoodooDoll(*p->pawn); // a copy of the real pawn

  d->pos.x = mthing->x;
  d->pos.y = mthing->y;
  d->pos.z = ONFLOORZ;

  // set player, team?
  d->victim = p->pawn;

  Map *m = p->mp;
  m->SpawnActor(d, mthing->height);

  //d->angle = ANG45 * (mthing->angle/45);

  d->spawnpoint = mthing;
  mthing->mobj = d;

  // set the timer
  mthing->tid = short((m->maptic + 20) & 0xFFFF);
}



bool VoodooDoll::Damage(Actor *inflictor, Actor *source, int damage, int dtype)
{
  // tricky. No recoil is given to the victim!
  return victim->Damage(inflictor, source, damage, dtype | dt_norecoil);
}



bool VoodooDoll::Touch(Actor *a)
{
  //PlayerPawn::Touch(a);
  return victim->Touch(a);
}



void VoodooDoll::Think()
{
  if (!victim || victim->health <= 0 || victim->flags & MF_CORPSE)
    {
      Remove(); // deleted when its victim is deleted or just dead
      return;
    }

  // TODO most power counters are probably unnecessary, since Damage and Touch are
  // automatically rerouted to the victim, and monsters never target voodoo dolls.
  // Only powers affecting movement and collision logic are needed?

  if (victim->powers[pw_flight])
    {
      eflags |= MFE_FLY;
      flags  |= MF_NOGRAVITY;
    }
  else
    {
      eflags &= ~MFE_FLY;
      flags  &= ~MF_NOGRAVITY;
    }

  // this is where the "actor part" of the thinking begins
  // we call Actor::Think(), because a playerpawn is an actor too
  Actor::Think();
}




void VoodooDoll::XYFriction(fixed_t oldx, fixed_t oldy)
{
  Actor::XYFriction(oldx, oldy);
  // TODO or maybe normal PlayerPawn::XYFriction?
}




// TODO: linedef activation, zombie effect
