// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_telept.cpp 533 2009-06-11 08:31:10Z smite-meister $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2007 by DooM Legacy Team.
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
/// \brief Teleportation.

#include "doomdef.h"
#include "g_game.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "g_map.h"

#include "p_spec.h"
#include "m_random.h"
#include "p_maputl.h"

#include "r_defs.h"
#include "sounds.h"
#include "tables.h"

bool Actor::Teleport(const vec_t<fixed_t> &p, angle_t nangle, bool silent)
{
  // silent means "Boom-style"

  vec_t<fixed_t> oldpos = pos;

  if (!TeleportMove(p))
    return false;

  if (!silent)
    {
      // spawn teleport fog at source and destination

      fixed_t fogHeight = (game.mode >= gm_heretic && !(flags & MF_MISSILE)) ? TELEFOGHEIGHT : 0;

      // source
      oldpos.z += fogHeight;
      DActor *fog = mp->SpawnDActor(oldpos, MT_TFOG);
      S_StartSound(fog, sfx_teleport);

      // destination
      vec_t<fixed_t> fogDelta(20*Cos(nangle), 20*Sin(nangle), fogHeight);
      fog = mp->SpawnDActor(pos + fogDelta, MT_TFOG);
      S_StartSound(fog, sfx_teleport);

      if ((flags2 & MF2_FOOTCLIP) && (subsector->sector->floortype >= FLOOR_LIQUID))
	floorclip = FOOTCLIPSIZE;
      else
	floorclip = 0;

      if (flags & MF_MISSILE)
	{
	  fixed_t speed = P_AproxDistance(vel.x, vel.y);
	  vel.x = speed * Cos(nangle);
	  vel.y = speed * Sin(nangle);
	}
      else
	vel.Set(0,0,0);
    }
  else
    {
      angle_t ang = nangle - yaw;
      // Sine, cosine of angle adjustment
      fixed_t s = Sin(ang);
      fixed_t c = Cos(ang);

      fixed_t tpx = vel.x;
      fixed_t tpy = vel.y;
      // Rotate thing's momentum to come out of exit just like it entered
      vel.x = (tpx * c) - (tpy * s);
      vel.y = (tpy * c) + (tpx * s);
    }

  yaw = nangle;

  return true;
}



//=========================================================================
//                            TELEPORTATION
//=========================================================================

bool Map::EV_Teleport(unsigned tag, line_t *line, Actor *thing, int type, int flags)
{
  if (!line)
    return false;

  // don't teleport missiles
  if (thing->flags2 & MF2_NOTELEPORT)
    return false;

  Actor *m;
  int i;

  bool noplayer = flags & TP_noplayer;
  bool silent   = flags & TP_silent;
  bool reldir   = flags & TP_reldir;
  bool reverse  = flags & TP_flip;

  if (noplayer && thing->flags & MF_PLAYER)
    return false;

  vec_t<fixed_t> delta(0, 0, thing->Feet() - thing->floorz); // preserve the relative altitude

  if (type == TP_toTID) // go to thing with correct TID (Hexen system)
    {
      Iterate_TID iter(this, tag);
      i = iter.Count();
      if (i > 0)
	{
	  i = P_Random() % i;

	  for ( ; i > 0; i--)
	    iter.Next(); // skip i things

	  m = iter.Next();
	  if (!m)
	    I_Error("Can't find teleport mapspot\n");

	  return thing->Teleport(m->pos + delta, m->yaw, silent); // does the angle change work correctly?
	}
    }
  else if (type == TP_toThingInSector) // go to teleport thing in tagged sector
    {
      for (i = -1; (i = FindSectorFromTag(tag, i)) >= 0;)
	for (m = sectors[i].thinglist; m != NULL; m = m->snext)
	  {
	    DActor *dm = m->Inherits<DActor>();
	    if (!dm)
	      continue;

	    // not a teleportman
	    if (dm->type != MT_TELEPORTMAN)
	      continue;

	    if (silent || reldir)
	      {
		// Get the angle between the exit thing and source linedef.
		// Rotate 90 degrees, so that walking perpendicularly across
		// teleporter linedef causes thing to exit in the direction
		// indicated by the exit thing.
		angle_t ang = m->yaw + thing->yaw - R_PointToAngle2(0, 0, line->dx, line->dy) - ANG90;
		return thing->Teleport(m->pos + delta, ang, true);
	      }
	    else
	      return thing->Teleport(m->pos + delta, m->yaw, false);
	  }
    }
  else // if (type == TP_toLine)
    return EV_SilentLineTeleport(tag, line, thing, reverse);

  return false;
}



// Silent linedef-based TELEPORTATION, by Lee Killough
// Primarily for rooms-over-rooms etc.
// This is the complete player-preserving kind of teleporter.
// It has advantages over the teleporter with thing exits.

bool Map::EV_SilentLineTeleport(unsigned lineid, line_t *line, Actor *thing, bool reverse)
{
  if (!line)
    return false;

  if (thing->flags2 & MF2_NOTELEPORT)
    return false;

  line_t *l;
  for (int i = -1; (l = FindLineFromID(lineid, &i)) != NULL;)
    if (l != line && l->backsector)
      {
        // Get the thing's position along the source linedef
        fixed_t pos = abs(line->dx) > abs(line->dy) ?
          (thing->pos.x - line->v1->x) / line->dx :
	  (thing->pos.y - line->v1->y) / line->dy ;

        // Get the angle between the two linedefs, for rotating
        // orientation and momentum. Rotate 180 degrees, and flip
        // the position across the exit linedef, if reversed.
        angle_t angle = (reverse ? pos = 1-pos, 0 : ANG180) +
          R_PointToAngle2(0, 0, l->dx, l->dy) -
          R_PointToAngle2(0, 0, line->dx, line->dy);


        // Maximum distance thing can be moved away from interpolated
        // exit, to ensure that it is on the correct side of exit linedef

	// maximum fixed_t units to move object to avoid hiccups
#define FUDGEFACTOR 10
        int fudge = FUDGEFACTOR;

        // Whether walking towards first side of exit linedef steps down
        int stepdown = l->frontsector->floorheight < l->backsector->floorheight;

        // Interpolate position across the exit linedef.
        // Adjust z position to be same height above ground as before.
        // Ground level at the exit is measured as the higher of the
        // two floor heights at the exit linedef.

        vec_t<fixed_t> p(l->v2->x - (pos * l->dx),
			 l->v2->y - (pos * l->dy),
			 l->sideptr[stepdown]->sector->floorheight +(thing->Feet() - thing->floorz));

        // Side to exit the linedef on positionally.
        //
        // Notes:
        //
        // This flag concerns exit position, not momentum. Due to
        // roundoff error, the thing can land on either the left or
        // the right side of the exit linedef, and steps must be
        // taken to make sure it does not end up on the wrong side.
        //
        // Exit momentum is always towards side 1 in a reversed
        // teleporter, and always towards side 0 otherwise.
        //
        // Exiting positionally on side 1 is always safe, as far
        // as avoiding oscillations and stuck-in-wall problems,
        // but may not be optimum for non-reversed teleporters.
        //
        // Exiting on side 0 can cause oscillations if momentum
        // is towards side 1, as it is with reversed teleporters.
        //
        // Exiting on side 1 slightly improves player viewing
        // when going down a step on a non-reversed teleporter.

        //int side = reverse || (player && stepdown);
	// FIXME why treat players differently?
	int side = reverse || stepdown;

        // Make sure we are on correct side of exit linedef.
        while (P_PointOnLineSide(p.x, p.y, l) != side && --fudge>=0)
          if (abs(l->dx) > abs(l->dy))
            p.y -= (l->dx < 0) != side ? -fixed_epsilon : fixed_epsilon;
          else
            p.x += (l->dy < 0) != side ? -fixed_epsilon : fixed_epsilon;

	return thing->Teleport(p, thing->yaw + angle, true);
      }
  return false;
}
