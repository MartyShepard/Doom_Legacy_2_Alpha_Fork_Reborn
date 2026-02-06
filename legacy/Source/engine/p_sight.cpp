// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: p_sight.cpp 476 2007-06-14 18:31:04Z smite-meister $
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
/// \brief LineOfSight/Visibility checks, uses REJECT and BSP

#include "doomdef.h"

#include "g_actor.h"
#include "g_map.h"
#include "p_maputl.h"
#include "r_defs.h"


//
// P_CheckSight
//
static fixed_t sightzstart;           // eye z of looker
static fixed_t topslope, bottomslope; // slopes to top and bottom of target

static divline_t strace;                 // from t1 to t2
static fixed_t   t2x, t2y;
static int       sightcounts[2];


// Returns true if strace crosses the given subsector successfully.
bool Map::CrossSubsector(int num)
{
#ifdef RANGECHECK
  if (num>=numsubsectors)
    I_Error ("P_CrossSubsector: ss %i with numss = %i", num, numsubsectors);
#endif

  subsector_t *sub = &subsectors[num];

  // check lines
  int count = sub->num_segs;
  seg_t *seg = &segs[sub->first_seg];

  for ( ; count ; seg++, count--)
    {
      line_t *line = seg->linedef;
      if (!line)
	continue; // miniseg

      // allready checked other side?
      if (line->validcount == validcount)
        continue;

      line->validcount = validcount;

      vertex_t *v1 = line->v1;
      vertex_t *v2 = line->v2;
      int s1 = strace.PointOnSide(v1->x, v1->y);
      int s2 = strace.PointOnSide(v2->x, v2->y);

      // line isn't crossed?
      if (s1 == s2)
        continue;

      divline_t divl;

      divl.x = v1->x;
      divl.y = v1->y;
      divl.dx = v2->x - v1->x;
      divl.dy = v2->y - v1->y;
      s1 = divl.PointOnSide(strace.x, strace.y);
      s2 = divl.PointOnSide(t2x, t2y);

      // line isn't crossed?
      if (s1 == s2)
        continue;

      // stop because it is not two sided anyway
      // might do this after updating validcount?
      if ( !(line->flags & ML_TWOSIDED) )
        return false;

      // crosses a two sided line
      sector_t *front = seg->frontsector;
      sector_t *back = seg->backsector;

      // no wall to block sight with?
      if (front->floorheight == back->floorheight
          && front->ceilingheight == back->ceilingheight)
        continue;

      fixed_t opentop, openbottom;

      // possible occluder
      // because of ceiling height differences
      if (front->ceilingheight < back->ceilingheight)
        opentop = front->ceilingheight;
      else
        opentop = back->ceilingheight;

      // because of ceiling height differences
      if (front->floorheight > back->floorheight)
        openbottom = front->floorheight;
      else
        openbottom = back->floorheight;

      // quick test for totally closed doors
      if (openbottom >= opentop)
        return false;               // stop

      float frac = strace.InterceptVector(&divl);

      if (front->floorheight != back->floorheight)
        {
          fixed_t slope = (openbottom - sightzstart) / frac;
          if (slope > bottomslope)
            bottomslope = slope;
        }

      if (front->ceilingheight != back->ceilingheight)
        {
          fixed_t slope = (opentop - sightzstart) / frac;
          if (slope < topslope)
            topslope = slope;
        }

      if (topslope <= bottomslope)
        return false;               // stop
    }
  // passed the subsector ok
  return true;
}



// Returns true if strace crosses the given node successfully.
bool Map::CrossBSPNode(int bspnum)
{
  if (bspnum & NF_SUBSECTOR)
    {
      if (bspnum == -1)
        return CrossSubsector(0);
      else
        return CrossSubsector(bspnum & (~NF_SUBSECTOR));
    }

  node_t *bsp = &nodes[bspnum];

  // decide which side the start point is on
  int side = bsp->PointOnSide(strace.x, strace.y);
  /*
  if (side == divline_t::LS_ON)
    side = divline_t::LS_FRONT; // an "on" should cross both sides
  */

  // cross the starting side
  if (!CrossBSPNode(bsp->children[side]))
    return false;

  // the partition plane is crossed here
  if (side == bsp->PointOnSide(t2x, t2y))
    {
      // the line doesn't touch the other side
      return true;
    }

  // cross the ending side
  return CrossBSPNode(bsp->children[side^1]);
}


// Returns true if a straight line between t1 and t2 is unobstructed.
// Uses REJECT.
bool Map::CheckSight(Actor *t1, Actor *t2)
{
  if (!t1 || !t2)
    return false;

  if (t1->mp != this || t2->mp != this)
    return false;

  // First check for trivial rejection.

  // Determine sector entries in REJECT table.
  int s1 = (t1->subsector->sector - sectors);
  int s2 = (t2->subsector->sector - sectors);
  int pnum = s1*numsectors + s2;
  int bytenum = pnum >> 3;
  int bitnum = 1 << (pnum&7);

  // Check in REJECT table.
  if (rejectmatrix[bytenum] & bitnum)
    {
      sightcounts[0]++;

      // can't possibly be connected
      return false;
    }

  // An unobstructed LOS is possible.
  // Now look from eyes of t1 to any part of t2.
  sightcounts[1]++;
  validcount++;

  sightzstart = t1->Top() - (t1->height >> 2);
  topslope = t2->Top() - sightzstart;
  bottomslope = t2->Feet() - sightzstart;

  /*
  if (gamemode == gm_heretic)
    return P_SightPathTraverse(t1->x, t1->y, t2->x, t2->y);
  */

  strace.x = t1->pos.x;
  strace.y = t1->pos.y;
  t2x = t2->pos.x;
  t2y = t2->pos.y;
  strace.dx = t2->pos.x - t1->pos.x;
  strace.dy = t2->pos.y - t1->pos.y;

  // the head node is the last node output
  return CrossBSPNode(numnodes-1);
}
