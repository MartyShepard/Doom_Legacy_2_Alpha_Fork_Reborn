// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_maputl.cpp 518 2007-12-31 02:54:58Z smite-meister $
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
/// \brief Map geometry utility functions, blockmap iterators, traces and intercepts.
///
/// BLOCKMAP iterator functions and some central PIT_* functions to use for iteration.
/// Intercepts and traces.
/// Functions for manipulating msecnode_t threads.

#include <vector>

#include "doomdef.h"
#include "g_actor.h"
#include "g_map.h"
#include "g_blockmap.h"

#include "m_bbox.h"
#include "p_polyobj.h"
#include "r_sky.h"
#include "p_maputl.h"

#include "tables.h"
#include "z_zone.h"

//==========================================================================
//  Simple map geometry utility functions
//==========================================================================

/*!
  \defgroup g_geoutils Simple geometry utility functions

  Distances, point vs. line problems, line intercepts, bounding boxes etc.
  Most of this is 2D stuff.
*/


/// \brief On which side of a 2D line the point is?
/// \ingroup g_geoutils
/*!
  \return side number, 0 (right side == frontside) or 1 (left side == backside, or on the line)
*/
int P_PointOnLineSide(const fixed_t x, const fixed_t y, const line_t *line)
{
  if (!line->dx)
    {
      if (x <= line->v1->x)
	return line->dy > 0;

      return line->dy < 0;
    }
  if (!line->dy)
    {
      if (y <= line->v1->y)
	return line->dx < 0;

      return line->dx > 0;
    }

  fixed_t dx = x - line->v1->x;
  fixed_t dy = y - line->v1->y;

  // original formula, not accurate with short lines
  /*
  fixed_t left = (line->dy >> 16) * dx;  // shift so that it always fits in 32 bits
  fixed_t right = dy * (line->dx >> 16);
  */

  Sint64 left  = line->dy.value() * dx.value();
  Sint64 right = line->dx.value() * dy.value();

  return (right >= left); // backside?
}

/// \brief Do line segments drawn between (x1, y1) and (x2, y2) and
/// between (x3, y3), (x4, y4) cross.
/// \ingroup g_geoutils
/*!
  \return true if segments do cross.
*/
bool divline_t::LinesegsCross(const divline_t *v1) const
{
  // Line segments cross if both pairs of endpoints are on different
  // sides of the line spanned by the other endpoint.

  fixed_t x2 = x + dx;
  fixed_t y2 = y + dy;

  if (v1->PointOnSide(x, y) == v1->PointOnSide(x2, y2))
    return false;

  x2 = v1->x + v1->dx;
  y2 = v1->y + v1->dy;

  if (PointOnSide(v1->x, v1->y) == PointOnSide(x2, y2))
    return false;

  return true;
}


/// \brief On which side of a 2D divline the point is?
/// \ingroup g_geoutils
/*!
  \return side number, 0 (right side == frontside) or 1 (left side == backside) or 2 (on the line)
*/
divline_t::lineside_e divline_t::PointOnSide(const fixed_t px, const fixed_t py) const
{
  if (!dx)
    {
      //if (px == x) return LS_ON;
      return ((px <= x) ^ (dy > 0)) ? LS_FRONT : LS_BACK;
    }

  if (!dy)
    {
      //if (py == y) return LS_ON;
      return ((py > y) ^ (dx > 0)) ? LS_FRONT : LS_BACK;
    }

  // location of point relative to the start of the divline
  fixed_t rx = px - x;
  fixed_t ry = py - y;

  // Try to quickly decide by looking at sign bits (works with 0.5 probability).
  // If the line and d vectors point to adjacent quadrants, we may decide
  // the side of line where d lies by checking which half-planes they point to.
#if 0
  if ((dy.value() ^ dx.value() ^ rx.value() ^ ry.value()) & 0x80000000)
    {
      if ((dy.value() ^ rx.value()) & 0x80000000)
	return LS_BACK; // (left is negative)

      return LS_FRONT;
    }
#endif

  // original formula, not accurate with short lines or near the line
  /*
  fixed_t left = (dy >> 8) * (rx >> 8); // shift so result always fits in 32 bits
  fixed_t right = (ry >> 8) * (dx >> 8);

  // or, in R_PointOnSide,
  fixed_t left = (node->dy >> fixed_t::FBITS) * dx;
  fixed_t right = dy * (node->dx >> fixed_t::FBITS);
  */

  Sint64 left  = dy.value() * rx.value();
  Sint64 right = dx.value() * ry.value();

  if (right < left)
    return LS_FRONT; // frontside
  /*
  if (left == right)
    return LS_ON; // on the line
  */
  return LS_BACK; // backside
}


/// \brief Finds the intercept point of two 2D line segments
/// \ingroup g_geoutils
/*!
  This is only called by the addthings and addlines traversers.
  \return fractional intercept point along the first divline
*/
float divline_t::InterceptVector(const divline_t *other) const
{
  float den = other->dy.value() * dx.value() - other->dx.value() * dy.value();
  if (den == 0)
    return 0; // parallel lines

  float num = (y.value() - other->y.value()) * other->dx.value() -(x.value() - other->x.value()) * other->dy.value();

  return num/den;

  // Original formula, not accurate with short divlines.
  /*
  fixed_t den = (other->dy >> 8) * v2->dx - (other->dx >> 8) * v2->dy;
  if (den == 0)
    return 0;
  fixed_t num = ((other->x - v2->x) >> 8) * other->dy + ((v2->y - other->y) >> 8) * other->dx;
  return num / den;
  */
}


divline_t::divline_t(const line_t *li)
{
  x = li->v1->x;
  y = li->v1->y;
  dx = li->dx;
  dy = li->dy;
}


divline_t::divline_t(const seg_t *s)
{
  x = s->v1->x;
  y = s->v1->y;
  dx = s->v2->x - x;
  dy = s->v2->y - y;
}


divline_t::divline_t(const Actor *a)
{
  x = a->pos.x;
  y = a->pos.y;
  dx = a->vel.x;
  dy = a->vel.y;
}



/// \brief On which side of a 2D line the bounding box is?
/// \ingroup g_geoutils
/*!
  Considers the line to be infinite in length.
  \return side number, 0 or 1, or -1 if box crosses the line
*/
int bbox_t::BoxOnLineSide(const line_t *ld) const
{
  int         p1;
  int         p2;

  switch (ld->slopetype)
    {
    case ST_HORIZONTAL:
      p1 = box[BOXTOP] > ld->v1->y;
      p2 = box[BOXBOTTOM] > ld->v1->y;
      if (ld->dx < 0)
        {
	  p1 ^= 1;
	  p2 ^= 1;
        }
      break;

    case ST_VERTICAL:
      p1 = box[BOXRIGHT] < ld->v1->x;
      p2 = box[BOXLEFT] < ld->v1->x;
      if (ld->dy < 0)
        {
	  p1 ^= 1;
	  p2 ^= 1;
        }
      break;

    case ST_POSITIVE:
      p1 = P_PointOnLineSide (box[BOXLEFT], box[BOXTOP], ld);
      p2 = P_PointOnLineSide (box[BOXRIGHT], box[BOXBOTTOM], ld);
      break;

    case ST_NEGATIVE:
      p1 = P_PointOnLineSide (box[BOXRIGHT], box[BOXTOP], ld);
      p2 = P_PointOnLineSide (box[BOXLEFT], box[BOXBOTTOM], ld);
      break;
    default :
      I_Error("P_BoxOnLineSide: unknow slopetype %d\n",ld->slopetype);
      return -1;
    }

  if (p1 == p2)
    return p1;
  return -1;
}



//==========================================================================
//  BSP utilities
//==========================================================================

/// Returns the subsector_t corresponding to the BSP leaf to which the point (x,y) belongs.
/// NOTE that (x,y) may be outside the actual subsector_t, yet inside the leaf node.
subsector_t *Map::GetSubsector(fixed_t x, fixed_t y)
{
  // single subsector is a special case
  if (!numnodes)
    return subsectors;

  int nodenum = numnodes-1;

  while (!(nodenum & NF_SUBSECTOR))
    {
      node_t *node = &nodes[nodenum];
      int side = node->PointOnSide(x, y);
      nodenum = node->children[side];
    }

  return &subsectors[nodenum & ~NF_SUBSECTOR];
}


/// Same as Map::GetSubsector, but returns NULL if not inside the subsector_t.
subsector_t* Map::FindSubsector(fixed_t x, fixed_t y)
{
  // single subsector is a special case
  if (!numnodes)
    return subsectors;

  int nodenum = numnodes-1;

  while (!(nodenum & NF_SUBSECTOR))
    {
      node_t *node = &nodes[nodenum];
      int side = node->PointOnSide(x, y);
      nodenum = node->children[side];
    }

  subsector_t *ret = &subsectors[nodenum & ~NF_SUBSECTOR];
  for (unsigned i=0; i < ret->num_segs; i++)
    {
      if (divline_t(&segs[ret->first_seg + i]).PointOnSide(x, y))
	return NULL;
    }

  return ret;
}



//==========================================================================
//  Line openings and vertical ranges
//==========================================================================


sector_t::zcheck_t sector_t::CheckZ(fixed_t z)
{
  if (z < floorheight)
    return SkyFloor() ? z_Sky : z_Wall;

  if (z > ceilingheight)
    return SkyCeiling() ? z_Sky : z_Wall;

  for (ffloor_t *rover = ffloors; rover; rover = rover->next)
    {
      if (!(rover->flags & FF_SOLID))
	continue;

      if (z > *rover->bottomheight && z < *rover->topheight)
	return z_FFloor;
    }

  return z_Open;
}



static list<range_t> Line_openings; // sorted from low to high


list<range_t> *sector_t::FindLineOpeningsInRange(const range_t& in)
{
  Line_openings.clear();

  // start chopping the range up with sector planes

  range_t r = in;

  if (floorheight > r.low)
    r.low = floorheight;

  if (ceilingheight < r.high)
    r.high = ceilingheight;

  if (r.high <= r.low)
    return &Line_openings; // no opening

  Line_openings.push_back(r);

  for (ffloor_t *rover = ffloors; rover; rover = rover->next)
    {
      if (!(rover->flags & FF_SOLID))
	continue;

      fixed_t h = *rover->topheight;
      fixed_t l = *rover->bottomheight;

      list<range_t>::iterator a, b, o, t;

      // find range of openings [a,b) which this ffloor changes, starting from the lowest
      for (a = Line_openings.begin(); l >= a->high && a != Line_openings.end(); a++)
	; // a is the first affected one

      for (b = a; h > b->low && b != Line_openings.end(); b++)
	; // b is past-the-end

      for (o = a; o != b; )
	{
	  if (l <= o->low)
	    {
	      // cut from below
	      if (h >= o->high)
		{
		  // entire opening is closed
		  t = o;
		  o++ ;
		  Line_openings.erase(t);
		  continue; // skip o++ at end of block
		}
	      else
		o->low = h; // raise bottom
	    }
	  else if (h >= o->high)
	    {
	      // cut from above
	      o->high = l; // lower top
	    }
	  else
	    {
	      // floor splits the opening into two: [o->low, l] and [h, o->high]
	      r.low = o->low;
	      r.high = l;
	      Line_openings.insert(o, r); // inserted just before o
	      o->low = h;
	    }

	  o++;
	}
    }

  return &Line_openings;
}



range_t sector_t::FindZRange(fixed_t z)
{
  range_t r;

  // NOTE z is assumed to be always between absolute floor and ceiling
  r.low  = floorheight;
  r.high = ceilingheight;

  // see if fake floors make the range narrower
  for (ffloor_t *rover = ffloors; rover; rover = rover->next)
    {
      if (!(rover->flags & FF_SOLID))
	continue;

      fixed_t h = *rover->topheight;
      if (h <= z && h > r.low)
	r.low = h;

      h = *rover->bottomheight;
      if (h >= z && h < r.high)
	r.high = h;
    }

  return r;
}



range_t sector_t::FindZRange(const Actor *a)
{
  range_t r;

  // start with real ceil and floor
  r.high = ceilingheight;
  r.low  = floorheight;

  if (!ffloors)
    return r;

  // Check fake floors

  fixed_t thingbot = a->Feet();
  fixed_t thingtop = a->Top();

  for (ffloor_t *rover = ffloors; rover; rover = rover->next)
    {
      if (!(rover->flags & FF_SOLID))
	continue;

      fixed_t ffcenter = (*rover->topheight + *rover->bottomheight)/2;
      fixed_t delta1 = abs(thingbot - ffcenter);
      fixed_t delta2 = abs(thingtop - ffcenter);

      // NOTE: this logic works only when max climbing height is less than Actor.height/2
	    
      if (delta1 > delta2)
	{
	  // ffloor acts as a ceiling
	  if (*rover->bottomheight < r.high)
	    r.high = *rover->bottomheight;
	}
      else
	{
	  // ffloor acts as a floor
	  if (*rover->topheight > r.low)
	    r.low = *rover->topheight;
	}
    }

  return r;
}




void line_opening_t::SubtractFromOpening(const Actor *a, sector_t *s)
{
  range_t r = s->FindZRange(a);

  // now see if the opening is changed
  if (r.high < top)
    {
      top = r.high;
      top_sky = (s->SkyCeiling() && r.high == s->ceilingheight);
    }

#if 0
  // TEST originally lowfloor is the lowest floor encountered, now it is the second-highest.
  // This makes monsters a little more bold in stairs etc.
  if (r.low > bottom)
    {
      lowfloor = bottom;
      bottom = r.low;
      bottom_sky = (s->SkyFloor() && r.low == s->floorheight);
      bottompic = s->floorpic;
    }
  else if (r.low > lowfloor)
    lowfloor = r.low;
#else
  // original lowfloor logic
  if (r.low > bottom)
    {
      bottom = r.low;
      bottom_sky = (s->SkyFloor() && r.low == s->floorheight);
      bottompic = s->floorpic;
    }
  else if (r.low < lowfloor)
    lowfloor = r.low;
#endif
}


static line_opening_t Opening;

/// Sets Opening to the window through a two sided line.
line_opening_t *line_opening_t::Get(line_t *line, Actor *thing)
{
  Opening.Reset();

  if (line->sideptr[1] == NULL)
    {
      // single sided line
      Opening.bottom = Opening.top = 0;
      return &Opening;
    }

  Opening.SubtractFromOpening(thing, line->frontsector);
  Opening.SubtractFromOpening(thing, line->backsector);

  return &Opening;
}



//==========================================================================
//   Blockmap iterators
//==========================================================================

/// \brief Iterate a blockmap cell for line_t'
/// \ingroup g_iterators
/*!
  For each line in the given mapblock, call the passed \ref g_pit PIT function.
  If the function returns false, exit with false without checking anything else.
 
  The validcount flags are used to avoid checking lines that are marked in multiple mapblocks,
  so increment validcount before the first call to LinesIterator, then make one or more calls to it.
*/
bool blockmap_t::LinesIterator(int x, int y, line_iterator_t func)
{
  blockmapcell_t *cell = &cells[y*width + x];

  // first iterate through polyblockmap
  for (polyblock_t *p = cell->polys; p; p = p->next)
    {
      if (p->polyobj && p->polyobj->validcount != validcount)
	{
	  polyobj_t *temp = p->polyobj;
	  temp->validcount = validcount;
	  int n = temp->lines.size();
	  for (int i=0; i < n; i++)
	    {
	      if (!func(temp->lines[i]))
		return false;
	    }
	}
    }

  line_t *lines = parent_map->lines;

  // iterate through the blocklist
  for (Uint16 *p = cell->blocklist; *p != MAPBLOCK_END; p++) // index skips the initial zero marker
    {
      line_t *ld = &lines[*p];

      if (ld->validcount == validcount)
	continue;   // line has already been checked

      ld->validcount = validcount;

      if (!func(ld))
	return false;
    }
  return true;        // everything was checked
}


/// \brief Iterate a blockmap cell for Actor's
/// \ingroup g_iterators
/*!
  For each Actor in the given mapblock, call the passed \ref g_pit PIT function.
  If the function returns false, exit with false without checking anything else.
*/
bool blockmap_t::ThingsIterator(int x, int y, thing_iterator_t func)
{
  // iterate through the actor list
  for (Actor *mobj = cells[y*width + x].actors; mobj; mobj = mobj->bnext)
    if (!func(mobj))
      return false;

  return true;
}



/// \brief Searches though the surrounding mapblocks for Actors.
/// \ingroup g_iterators
/*!
  \param[in] distance is in MAPBLOCKUNITS
  \return true if an iterator returns false (which immediately stops the iteration)
*/
bool blockmap_t::RoughBlockSearch(Actor *center, int distance, thing_iterator_t func)
{
  extern Actor *blocksearch_self;
  blocksearch_self = center; // for the iterators

  // searches from (x,y) outwards in increasing-radius mapblock squares
  int startX = BlockX(center->pos.x);
  int startY = BlockY(center->pos.y);

  if (startX >= 0 && startX < width && startY >= 0 && startY < height)
    if (!ThingsIterator(startX, startY, func))
      return true; // found a target right away

  for (int count = 1; count <= distance; count++)
    {
      int xl = startX - count;
      if (xl < 0)
	xl = 0;
      else if (xl >= width)
	continue;

      int yl = startY - count;
      if (yl < 0)
	yl = 0;
      else if (yl >= height)
	continue;

      int xh = startX + count;
      if (xh < 0)
	continue;
      if (xh >= width)
	xh = width-1;

      int yh = startY + count;
      if (yh < 0)
	continue;
      if (yh >= height)
	yh = height-1;

      // y 3 2
      // ^  s
      // | b 1
      // +-->x

      int cx = xl;
      int cy = yl;

      // Trace the first block section (low y)
      for ( ; cx <= xh; cx++)
	if (!ThingsIterator(cx, cy, func))
	  return true;

      // Trace the second block section (high x)
      for (cx--, cy++; cy <= yh; cy++)
	if (!ThingsIterator(cx, cy, func))
	  return true;

      // Trace the third block section (high y)
      for (cy--, cx--; cx >= xl; cx--)
	if (!ThingsIterator(cx, cy, func))
	  return true;

      // Trace the final block section (low x)
      for (cx++, cy--; cy >= yl; cy--)
	if (!ThingsIterator(cx, cy, func))
	  return true;
    }
  return false; // no target found
}



//==========================================================================
//  Thinker iteration
//==========================================================================

/// \brief Iterate the Thinker list
/*!
  Iterates through all the Thinkers in the Map, calling 'func' for each.
  If the function returns false, exit with false without checking anything else.
*/
bool Map::IterateThinkers(thinker_iterator_t func)
{
  Thinker *t, *n;
  for (t = thinkercap.next; t != &thinkercap; t = n)
    {
      n = t->next; // if t is removed while it thinks, its 'next' pointer will no longer be valid.
      if (!func(t))
	return false;
    }
  return true;
}


/// \brief Iterate the Actors in the Thinker list
/*!
  Iterates through all the Actors in the Map, calling 'func' for each.
  If the function returns false, exit with false without checking anything else.
*/
bool Map::IterateActors(thing_iterator_t func)
{
  Thinker *t, *n;
  for (t = thinkercap.next; t != &thinkercap; t = n)
    {
      n = t->next; // if t is removed while it thinks, its 'next' pointer will no longer be valid.
      Actor *a = t->Inherits<Actor>();
      if (a && !func(a))
	return false;
    }
  return true;
}


//==========================================================================
//  Trace/intercept routines
//==========================================================================

trace_t trace;        ///< changed by Map::PathTraverse ONLY
static bool earlyout;

/// \brief Find lines intercepted by the trace.
/// \ingroup g_pit
/// \ingroup g_trace
/*!
  Checks if the line_t intercepts the given trace. If so, adds it to the intercepts list.
  A line is crossed if its endpoints are on opposite sides of the trace.
  Iteration is stopped if earlyout is true and a solid line is hit.
*/
static bool PIT_AddLineIntercepts(line_t *ld)
{
  int s1 = trace.dl.PointOnSide(ld->v1->x, ld->v1->y);
  int s2 = trace.dl.PointOnSide(ld->v2->x, ld->v2->y);

  if (s1 == s2)
    return true;    // line isn't crossed

  // hit the line
  divline_t  dl(ld);
  float frac = trace.dl.InterceptVector(&dl);

  if (frac < 0)
    return true;    // behind source

  // try to early out the check
  if (earlyout && frac < 1 && !ld->backsector)
    {
      return false;   // stop checking
    }

  intercept_t in;
  in.frac = frac;
  in.isaline = true;
  in.line = ld;
  trace.intercepts.push_back(in);

  return true;        // continue
}


/// \brief Find Actors intercepted by the trace.
/// \ingroup g_pit
/// \ingroup g_trace
/*!
  Checks if the Actor intercepts the given trace. If so, adds it to the intercepts list.
*/
static bool PIT_AddThingIntercepts(Actor *thing)
{
  fixed_t  x1, y1, x2, y2;
  bool tracepositive = (trace.delta.x.value() ^ trace.delta.y.value()) > 0;

  // check a corner to corner crossection for hit
  if (tracepositive)
    {
      x1 = thing->pos.x - thing->radius;
      y1 = thing->pos.y + thing->radius;

      x2 = thing->pos.x + thing->radius;
      y2 = thing->pos.y - thing->radius;
    }
  else
    {
      x1 = thing->pos.x - thing->radius;
      y1 = thing->pos.y - thing->radius;

      x2 = thing->pos.x + thing->radius;
      y2 = thing->pos.y + thing->radius;
    }

  int s1 = trace.dl.PointOnSide(x1, y1);
  int s2 = trace.dl.PointOnSide(x2, y2);

  if (s1 == s2)
    return true;            // line isn't crossed

  divline_t dl;
  dl.x = x1;
  dl.y = y1;
  dl.dx = x2-x1;
  dl.dy = y2-y1;

  float frac = trace.dl.InterceptVector(&dl);

  if (frac < 0)
    return true;            // behind source

  intercept_t in;
  in.frac = frac;
  in.isaline = false;
  in.thing = thing;
  trace.intercepts.push_back(in);

  return true;                // keep going
}

/// \brief Traverses the accumulated intercepts in order of closeness up to maxfrac.
/// \ingroup g_trace
/*!
  Calls the traverser function on all intercept_t's in the
  intercepts vector, in the nearness-of-intercept order.
  \return true if the traverser function returns true for all lines
*/
bool trace_t::TraverseIntercepts(traverser_t func, float maxfrac)
{
  int count = intercepts.size();
  int i = count;

  // TODO introsort
  intercept_t *in = NULL;
  while (i--)
    {
      float dist = fixed_t::FMAX;

      for (int j = 0; j < count; j++)
	{
	  intercept_t *scan = &intercepts[j];
	  if (scan->frac < dist)
	    {
	      dist = scan->frac;
	      in = scan;
	    }
	}

      if (dist > maxfrac)
	return true;        // checked everything in range

#if 0  // UNUSED
      {
        // don't check these yet, there may be others inserted
        in = scan = intercepts;
        for (scan = intercepts ; scan<intercept_p ; scan++)
	  if (scan->frac > maxfrac)
	    *in++ = *scan;
        intercept_p = in;
        return false;
      }
#endif

      // call the traverser function on the closest intercept_t
      if (!func(in))
	return false; // don't bother going farther

      in->frac = fixed_t::FMAX; // make sure this intercept is not chosen again
    }

  return true; // everything was traversed
}



void trace_t::Init(const vec_t<fixed_t>& v1, const vec_t<fixed_t>& v2)
{
  start = v1;
  delta = v2-v1;

  vec_t<float> temp;
  temp.x = delta.x.Float();
  temp.y = delta.y.Float();
  temp.z = delta.z.Float();

  length = temp.Norm();
  sin_pitch = temp.z / length;

  intercepts.clear();

  dl.x = start.x; dl.y = start.y;
  dl.dx = delta.x; dl.dy = delta.y;

  lastz = start.z;
  frac = 0;
}



/// \brief Traces a line through the blockmap.
/// \ingroup g_trace
/*!
  Traces a line from v1 to v2 by stepping through the blockmap
  adding line/thing intercepts and then calling the traverser function for each intercept.
  \return true if the traverser function returns true for all lines
*/
bool blockmap_t::PathTraverse(const vec_t<fixed_t>& v1, const vec_t<fixed_t>& v2, int flags, traverser_t trav)
{
  // small HACK: make local copies so we can change them
  vec_t<fixed_t> p1(v1);
  vec_t<fixed_t> p2(v2);

  earlyout = flags & PT_EARLYOUT;

  validcount++;

#define MAPBLOCKSIZE (MAPBLOCKUNITS * fixed_t::UNIT)

  // TODO why is this needed?
  if (((p1.x-orgx).value() & (MAPBLOCKSIZE-1)) == 0)
    p1.x += 1; // don't side exactly on a line

  if (((p1.y-orgy).value() & (MAPBLOCKSIZE-1)) == 0)
    p1.y += 1; // don't side exactly on a line

  // set up the trace struct
  trace.Init(p1, p2);

  p1.x -= orgx;
  p1.y -= orgy;
  int xt1 = p1.x.floor() >> MAPBLOCKBITS;
  int yt1 = p1.y.floor() >> MAPBLOCKBITS;

  p2.x -= orgx;
  p2.y -= orgy;
  int xt2 = p2.x.floor() >> MAPBLOCKBITS;
  int yt2 = p2.y.floor() >> MAPBLOCKBITS;

  fixed_t     xstep, ystep;
  fixed_t     partial;
  int         mapxstep, mapystep;

  if (xt2 > xt1)
    {
      mapxstep = 1;
      partial = 1 - (p1.x >> MAPBLOCKBITS).frac();
      ystep = (p2.y-p1.y) / abs(p2.x-p1.x);
    }
  else if (xt2 < xt1)
    {
      mapxstep = -1;
      partial = (p1.x >> MAPBLOCKBITS).frac();
      ystep = (p2.y-p1.y) / abs(p2.x-p1.x);
    }
  else
    {
      mapxstep = 0;
      partial = 1;
      ystep = 256;
    }

  fixed_t yintercept = (p1.y >> MAPBLOCKBITS) + (partial * ystep);

  if (yt2 > yt1)
    {
      mapystep = 1;
      partial = 1 - (p1.y >> MAPBLOCKBITS).frac();
      xstep = (p2.x-p1.x) / abs(p2.y-p1.y);
    }
  else if (yt2 < yt1)
    {
      mapystep = -1;
      partial = (p1.y >> MAPBLOCKBITS).frac();
      xstep = (p2.x-p1.x) / abs(p2.y-p1.y);
    }
  else
    {
      mapystep = 0;
      partial = 1;
      xstep = 256;
    }

  fixed_t xintercept = (p1.x >> MAPBLOCKBITS) + (partial * xstep);

  // Step through map blocks.
  // Count is present to prevent a round off error
  // from skipping the break.
  int mapx = xt1;
  int mapy = yt1;

  for (int count = 0 ; count < 64 ; count++)
    {
      if (mapx < 0 || mapx >= width || mapy < 0 || mapy >= height)
	continue; // outside the blockmap, skip

      if (flags & PT_ADDLINES)
        {
	  if (!LinesIterator(mapx, mapy, PIT_AddLineIntercepts))
	    return false;   // early out
        }

      if (flags & PT_ADDTHINGS)
        {
	  if (!ThingsIterator(mapx, mapy, PIT_AddThingIntercepts))
	    return false;   // early out
        }

      if (mapx == xt2 && mapy == yt2)
	break;

      if (yintercept.floor() == mapy)
        {
	  yintercept += ystep;
	  mapx += mapxstep;
        }
      else if (xintercept.floor() == mapx)
        {
	  xintercept += xstep;
	  mapy += mapystep;
        }

    }
  // go through the sorted list
  return trace.TraverseIntercepts(trav, 1);
}





//==========================================================================
//  Functions for manipulating msecnode_t threads
//==========================================================================

msecnode_t *msecnode_t::headsecnode = NULL; // freelist for secnodes

void msecnode_t::InitSecnodes()
{
  headsecnode = NULL;
}

// Retrieves a node from the freelist. The calling routine
// should make sure it sets all fields properly.
msecnode_t *msecnode_t::GetNode()
{
  msecnode_t *node;

  if (headsecnode)
    {
      node = headsecnode;
      headsecnode = headsecnode->m_snext;
    }
  else
    node = (msecnode_t*)Z_Malloc(sizeof(*node), PU_LEVEL, NULL);

  return node;
}

// Returns a node to the freelist.
void msecnode_t::Free()
{
  m_snext = headsecnode;
  headsecnode = this;

  // TEST
  /*
  m_thing = NULL;
  m_sector = NULL;
  m_sprev = NULL;
  m_tprev = m_tnext = NULL;
  */
}

// Deletes a sector node from the list of
// sectors this object appears in. Returns a pointer to the next node
// on the linked list, or NULL.
msecnode_t *msecnode_t::Delete()
{
  // Unlink from the Thing thread. The Thing thread begins at
  // sector_list (unavailable here) and not from m_thing->touching_sectorlist.

  if (m_tprev)
    m_tprev->m_tnext = m_tnext;
  if (m_tnext)
    m_tnext->m_tprev = m_tprev;

  // Unlink from the sector thread. This thread begins at
  // m_sector->touching_thinglist.

  if (m_sprev)
    m_sprev->m_snext = m_snext;
  else
    m_sector->touching_thinglist = m_snext;

  if (m_snext)
    m_snext->m_sprev = m_sprev;

  /*
  for (msecnode_t *p = m_sector->touching_thinglist; p; p = p->m_snext)
    if (!p->m_thing || !p->m_sector)
      I_Error("error during msecnode delete");
  */

  // Return this node to the freelist
  Free();
  return m_tnext; // unharmed by Free
}



// Searches the current list to see if this sector is
// already there. If not, it adds a sector node at the head of the list of
// sectors this object appears in. This is called when creating a list of
// nodes that will get linked in later. Returns a pointer to the new node.
msecnode_t *msecnode_t::AddToSectorlist(sector_t *s, Actor *thing, msecnode_t *seclist)
{
  msecnode_t *node = seclist;
  while (node)
    {
      if (node->m_sector == s)   // Already have a node for this sector?
	{
	  node->m_thing = thing; // Yes. Setting m_thing says 'keep it'.
	  return seclist;
	}
      node = node->m_tnext;
    }

  // Couldn't find an existing node for this sector. Add one at the head of the list.
  node = GetNode();

  //mark new nodes unvisited.
  node->visited = false;

  node->m_sector = s;       // sector
  node->m_thing  = thing;   // Actor
  node->m_tprev  = NULL;    // prev node on Thing thread
  node->m_tnext  = seclist;  // next node on Thing thread
  if (seclist)
    seclist->m_tprev = node; // set back link on Thing

  // Add new node at head of sector thread starting at s->touching_thinglist

  node->m_sprev  = NULL;    // prev node on sector thread
  node->m_snext  = s->touching_thinglist; // next node on sector thread
  if (s->touching_thinglist)
    node->m_snext->m_sprev = node;
  s->touching_thinglist = node;

  return node;
}



msecnode_t *msecnode_t::CleanSectorlist(msecnode_t *seclist)
{
  msecnode_t *node = seclist;
  while (node)
    {
      if (node->m_thing == NULL)
	{
	  if (node == seclist)
	    seclist = node->m_tnext;
	  node = node->Delete();
	}
      else
	node = node->m_tnext;
    }

  return seclist;
}
