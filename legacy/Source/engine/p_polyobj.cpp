// Emacs style mode select -*- C++ -*-
//---------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2005-2007 by Doom Legacy Team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------

/// \file
/// \brief Polyobjects. New implementation.

#include <set>
#include <math.h>

#include "p_polyobj.h"

#include "g_actor.h"
#include "g_map.h"
#include "g_blockmap.h"

#include "p_spec.h"

#include "sounds.h"
#include "tables.h"
#include "z_zone.h"

using namespace std;

#define PO_MAXLINES 64


IMPLEMENT_CLASS(polyobject_t, Thinker);
polyobject_t::polyobject_t() {}


//==========================================================================
//   Poly rotator
//==========================================================================

IMPLEMENT_CLASS(polyrotator_t, polyobject_t);
polyrotator_t::polyrotator_t() {}


float polyrotator_t::PushForce()
{
  return fabs(ang_vel) / 32.0f; // max 32
}


void polyrotator_t::Think()
{
  if (dist == -1) // perpetual rotator
    {
      poly->Rotate(ang_vel << ANGLETOFINESHIFT);
      return;
    }

  // not perpetual
  int speed = abs(ang_vel);
  if (dist < speed)
    {
      ang_vel = ang_vel < 0 ? -dist : dist;
      speed = dist;
    }

  if (poly->Rotate(ang_vel << ANGLETOFINESHIFT))
    {
      dist -= speed;
      if (dist <= 0)
	{
	  if (poly->thinker == this)
	    poly->thinker = NULL;

	  mp->SN_StopSequence(&poly->origin);
	  mp->PO_Finished(poly->id);
	  mp->RemoveThinker(this);
	}
    }
}



//==========================================================================
//   Poly mover
//==========================================================================

IMPLEMENT_CLASS(polymover_t, polyobject_t);
polymover_t::polymover_t() {}

polymover_t::polymover_t(polyobj_t *p, float sp, angle_t ang, float d)
  : polyobject_t(p), speed(sp), angle(ang), dist(d)
{
  xs = speed * Cos(angle);
  ys = speed * Sin(angle);
}


float polymover_t::PushForce() { return speed / 8.0f; }


void polymover_t::Think()
{
  if (dist < speed) // speed is always nonnegative
    {
      speed = dist;
      xs = speed * Cos(angle);
      ys = speed * Sin(angle);
    }

  if (poly->Move(xs, ys))
    {
      dist -= speed;
      if (dist <= 0)
	{
	  if (poly->thinker == this)
	    poly->thinker = NULL;

	  mp->SN_StopSequence(&poly->origin);
	  mp->PO_Finished(poly->id);
	  mp->RemoveThinker(this);
	}
    }
}



//==========================================================================
//   Poly doors
//==========================================================================

IMPLEMENT_CLASS(polydoor_swing_t, polyrotator_t);
polydoor_swing_t::polydoor_swing_t() {}

IMPLEMENT_CLASS(polydoor_slide_t, polymover_t);
polydoor_slide_t::polydoor_slide_t() {}


polydoor_swing_t::polydoor_swing_t(polyobj_t *p, float av, float d, int delay)
  : polyrotator_t(p, av, d)
{
  closing = false;
  open_delay = delay;
  delay = 0;
  initial_dist = dist;
}


void polydoor_swing_t::Think()
{
  if (delay > 0)
    {
      if (!--delay)
	mp->SN_StartSequence(&poly->origin, SEQ_DOOR + poly->sound_seq);

      return;
    }

  int speed = abs(ang_vel);
  if (dist < speed)
    {
      ang_vel = ang_vel < 0 ? -dist : dist;
      speed = dist;
    }

  if (poly->Rotate(ang_vel << ANGLETOFINESHIFT))
    {
      dist -= speed;
      if (dist <= 0)
	{
	  if (!closing)
	    {
	      closing = true;
	      delay = open_delay;
	      dist = initial_dist;
	      ang_vel = -ang_vel;
	    }
	  else
	    {
	      // done
	      if (poly->thinker == this)
		poly->thinker = NULL;
	      mp->PO_Finished(poly->id);
	      mp->RemoveThinker(this);
	    }
	  mp->SN_StopSequence(&poly->origin);
	}
    }
  else
    {
      if (poly->damage || !closing)
	return; // opening doors are persistent
      else
	{
	  // closing doors bounce open again if disturbed
	  closing = false;
	  dist = initial_dist - dist;
	  ang_vel = -ang_vel;
	  mp->SN_StartSequence(&poly->origin, SEQ_DOOR + poly->sound_seq);
	}
    }
}



polydoor_slide_t::polydoor_slide_t(polyobj_t *p, float sp, angle_t ang, float d, int delay)
  : polymover_t(p, sp, ang, d)
{
  closing = false;
  open_delay = delay;
  delay = 0;
  initial_dist = dist;
}


void polydoor_slide_t::Think()
{
  if (delay > 0)
    {
      if (!--delay)
	mp->SN_StartSequence(&poly->origin, SEQ_DOOR + poly->sound_seq);

      return;
    }

  if (dist < speed) // speed is always nonnegative
    {
      speed = dist;
      xs = speed * Cos(angle);
      ys = speed * Sin(angle);
    }

  if (poly->Move(xs, ys))
    {
      dist -= speed;
      if (dist <= 0)
	{
	  if (!closing)
	    {
	      // fully open, now wait and then close again
	      closing = true;
	      delay = open_delay;
	      dist = initial_dist;
	      angle += ANG180;
	      xs = -xs;
	      ys = -ys;
	    }
	  else
	    {
	      // done
	      if (poly->thinker == this)
		poly->thinker = NULL;
	      mp->PO_Finished(poly->id);
	      mp->RemoveThinker(this);
	    }
	  mp->SN_StopSequence(&poly->origin);
	}
    }
  else
    {
      if (poly->damage || !closing)
	return; // opening doors are persistent
      else
	{
	  // closing doors bounce open again if disturbed
	  closing = false;
	  dist = initial_dist - dist;
	  angle += ANG180;
	  xs = -xs;
	  ys = -ys;
	  mp->SN_StartSequence(&poly->origin, SEQ_DOOR + poly->sound_seq);
	}
    }
}



bool Map::EV_ActivatePolyobj(unsigned id, int type, float speed, angle_t angle, float dist, int delay, bool override)
{
  polyobj_t *p = PO_FindPolyobj(id);

  if (!p)
    {
      I_Error("EV_ActivatePolyobj: Unknown polyobj %d\n", id);
      return false;
    }

  // max. angular velocity +-pi/4 per tic, min 4 fineangleunits/tic!
  if (type == polyobject_t::po_rotate)
    {
      // Tales from the cryptic, again
      if (angle == 0)
	dist = (ANGLE_MAX >> ANGLETOFINESHIFT) + 1; // full turn
      else if ((angle >> 24) == 255)
	dist = -1; // special case, perpetual rotator
      else
	dist = angle >> ANGLETOFINESHIFT;
    }

  bool ret = false;

  do
    {
      if (p->thinker && !override)
	return ret; // already moving, ignore action TODO continue would be more logical

      polyobject_t *obj;
      switch (type)
	{
	case polyobject_t::po_rotate:
	  obj = new polyrotator_t(p, speed / (1 << 19), dist); // HACK, to fineangle
	  speed = -speed;
	  break;
	case polyobject_t::po_move:
	  obj = new polymover_t(p, speed, angle, dist);
	  angle += ANG180;
	  break;
	case polyobject_t::po_door_swing:
	  obj = new polydoor_swing_t(p, speed / (1 << 19), angle >> ANGLETOFINESHIFT, delay);
	  speed = -speed;
	  break;
	case polyobject_t::po_door_slide:
	  obj = new polydoor_slide_t(p, speed, angle, dist, delay);
	  angle += ANG180;
	  break;
	}

      ret = true;
      AddThinker(obj);
      p->thinker = obj;
      SN_StartSequence(&p->origin, SEQ_DOOR + p->sound_seq);

      p = PO_FindPolyobj(p->mirror_id);
    }
  while (p);

  return ret;
}


//====================================================================================
//   polyobj_t methods
//====================================================================================


void polyobj_t::PushActor(Actor *a, seg_t *seg)
{
  float force;
  if (thinker)
    {
      force = thinker->PushForce();

      if (force < 1)
	force = 1;
      else if (force > 4)
	force = 4;
    }
  else
    force = 1;

  // subtract 90 degrees from line angle to get the normal
  angle_t ang = seg->angle - ANG90;
  vec_t<fixed_t> dv(force * Cos(ang), force * Sin(ang), 0);
  a->vel += dv;

  // if the actor cannot be pushed aside, it will be crushed
  if (damage && (a->flags & MF_SHOOTABLE) && !a->TestLocation(a->pos + dv))
    a->Damage(NULL, NULL, damage, dt_crushing);
}



/// Moves the polyobj's origin by (dx,dy)
bool polyobj_t::Move(fixed_t dx, fixed_t dy)
{
  // move line bboxes
  int n_lines = lines.size();
  for (int i = 0; i < n_lines; i++)
    lines[i]->bbox.Move(dx, dy);

  // move vertices
  for (int i = 0; i < num_points; i++)
    {
      current_points[i]->x += dx;
      current_points[i]->y += dy;
    }

  if (mp->blockmap->PO_ClipActors(this))
    {
      // collided with something, undo move
      for (int i = 0; i < n_lines; i++)
	lines[i]->bbox.Move(-dx, -dy);

      for (int i = 0; i < num_points; i++)
	{
	  current_points[i]->x -= dx;
	  current_points[i]->y -= dy;
	}

      return false;
    }

  // otherwise update center and origin, and relink into world at new location
  center.x += dx;
  center.y += dy;

  origin.x += dx;
  origin.y += dy;

  mp->blockmap->PO_Unlink(this);
  mp->blockmap->PO_Link(this);

  subsector_t *sub = mp->GetSubsector(center.x, center.y);
  if (sub->poly && sub->poly != this)
    CONS_Printf("Multiple polyobjs (%d, %d) in subsector %d!\n", id, sub->poly->id, sub - mp->subsectors);
  sub->poly = this;
  return true;
}



static vertex_t RotateVertex(vertex_t v, angle_t a)
{
  vertex_t out;
  out.x = v.x * Cos(a) - v.y * Sin(a);
  out.y = v.x * Sin(a) + v.y * Cos(a);
  return out;
}


bool polyobj_t::Rotate(angle_t da)
{
  vertex_t temp[num_points];
  vertex_t orig = {origin.x, origin.y}; // TODO stupid mappoint_t

  for (int i = 0; i < num_points; i++)
    {
      temp[i] = *current_points[i]; // store the current position
      *current_points[i] = RotateVertex(base_points[i], angle + da);
      *current_points[i] += orig;
    }

  // recompute line bboxes
  int n_lines = lines.size();
  for (int i = 0; i < n_lines; i++)
    lines[i]->SetDims();

  // check collisions
  if (mp->blockmap->PO_ClipActors(this))
    {
      // collided with something, undo move
      for (int i = 0; i < num_points; i++)
	*current_points[i] = temp[i];

      for (int i = 0; i < n_lines; i++)
	lines[i]->SetDims();

      return false;
    }

  // otherwise update center and angle, and relink into world at new location
  center -= orig;
  center = RotateVertex(center, angle + da);
  center += orig;

  angle += da;
  for (int i = 0; i < n_lines; i++)
     segs[i].angle += da;

  mp->blockmap->PO_Unlink(this);
  mp->blockmap->PO_Link(this);
  // Rotate can move center, so TODO subsector re-link
  return true;
}



// base_points, current_points, origin, center, segs
bool polyobj_t::Build()
{
  // find the unique vertices of the polyobj (with LINE_PO_EXPLICIT you can have subcycles and hence shared vertices!)
  set<vertex_t *> points; 
  int n_lines = lines.size();
  //if (explicit)
  for (int k=0; k<n_lines; k++)
    if (!lines[k])
      {
	bad = true;
	CONS_Printf("Error: Polyobj %d: Missing LINE_PO_EXPLICIT order number %d!\n", id, k+1);
	return false;
      }
    else
      {
	points.insert(lines[k]->v1);
	points.insert(lines[k]->v2); // TODO with LINE_PO_EXPLICIT, the polyobj can in principle consist of disjoint lines... bad?
      }

  // TEST Generate new segs, one per line to make things easy. TODO what about old ones? rendered or not? in BSP?
  segs = static_cast<seg_t *>(Z_Malloc(n_lines * sizeof(seg_t), PU_LEVEL, 0));
  memset(segs, 0, n_lines * sizeof(seg_t)); // basic initialization to zero
  for (int k=0; k<n_lines; k++)
    {
      seg_t *s = &segs[k];
      line_t *l = lines[k];

      s->v1 = l->v1;
      s->v2 = l->v2;
      s->linedef = l;
      s->side = 0; // TODO check if ok
      s->sidedef = l->sideptr[0];

      float dx = (s->v2->x - s->v1->x).Float();
      float dy = (s->v2->y - s->v1->y).Float();

      if (dx == 0)
	s->angle = (dy > 0) ? ANG90 : ANG270;
      else
	s->angle = angle_t((atan2(dy, dx) * ANG180) / M_PI);

      s->length = sqrt(dx*dx + dy*dy);
      s->frontsector = l->frontsector;
      s->backsector = l->backsector;
    }

  // Now store unique vertices, and calculate center (initialized to zero).
  num_points = points.size();
  base_points    = static_cast<vertex_t *>(Z_Malloc(num_points * sizeof(vertex_t), PU_LEVEL, 0));
  current_points = static_cast<vertex_t **>(Z_Malloc(num_points * sizeof(vertex_t *), PU_LEVEL, 0));

  vertex_t orig = {origin.x, origin.y}; // TODO stupid mappoint_t
  set<vertex_t *>::iterator i = points.begin();
  for (int k = 0; k < num_points; k++, i++)
    {
      // make a copy of the unique vertices, using the anchor as origin
      base_points[k] = *(*i);
      base_points[k] -= orig;

      current_points[k] = *i; // TODO if not spawned?
      center += *current_points[k]; // calculate the average of the vertex coords
    }

  center.x /= num_points;
  center.y /= num_points;

  return true;
}




//====================================================================================
//   blockmap_t methods
//====================================================================================


void blockmap_t::PO_Unlink(polyobj_t *p)
{
  // remove the polyobj from each blockmap section its bounding box touches
  for (int j = p->bbox[BOXBOTTOM]; j <= p->bbox[BOXTOP]; j++)
    {
      int index = j*width;
      for (int i = p->bbox[BOXLEFT]; i <= p->bbox[BOXRIGHT]; i++)
	{
	  polyblock_t *link = cells[index+i].polys;
	  while (link && link->polyobj != p)
	    link = link->next;

	  if (link)
	    link->polyobj = NULL; // free the link TODO use a freelist instead, like msecnode_t?
	}
    }
}


void blockmap_t::PO_Link(polyobj_t *p)
{
  bbox_t bbox;
  bbox.Clear();

  int n = p->num_points;
  for (int i = 0; i < n; i++)
    bbox.Add(p->current_points[i]->x, p->current_points[i]->y);

  // calculate the polyobj bbox (always limited to the area of the blockmap!)
  p->bbox[BOXLEFT]   = max(BlockX(bbox[BOXLEFT]), 0);
  p->bbox[BOXRIGHT]  = min(BlockX(bbox[BOXRIGHT]), width-1);
  p->bbox[BOXBOTTOM] = max(BlockY(bbox[BOXBOTTOM]), 0);
  p->bbox[BOXTOP]    = min(BlockY(bbox[BOXTOP]), height-1);

  // link the polyobj to each blockmap cell its bounding box touches
  for (int j = p->bbox[BOXBOTTOM]*width; j <= p->bbox[BOXTOP]*width; j += width)
    {
      for (int i = p->bbox[BOXLEFT]; i <= p->bbox[BOXRIGHT]; i++)
	{
	  polyblock_t *link = cells[j+i].polys;
	  if (!link)
	    {
	      // allocate a new link
	      cells[j+i].polys = link = static_cast<polyblock_t*>(Z_Malloc(sizeof(polyblock_t), PU_LEVEL, 0));
	      link->next = NULL;
	      link->prev = NULL;
	      link->polyobj = p;
	      continue;
	    }

	  while (link->next && link->polyobj)
	    link = link->next;

	  if (!link->polyobj)
	    {
	      // reclaim unused link
	      link->polyobj = p;
	      continue;
	    }

	  // allocate new link to the end of the chain
	  link->next = static_cast<polyblock_t*>(Z_Malloc(sizeof(polyblock_t), PU_LEVEL, 0));
	  link->next->next = NULL;
	  link->next->prev = link;
	  link->next->polyobj = p;
	}
    }
}


bool blockmap_t::PO_ClipActors(polyobj_t *p)
{
  bool blocked = false;
  int n = p->lines.size();
  for (int k = 0; k < n; k++)
    {
      line_t *line = p->lines[k];

      int left =   max(BlockX(line->bbox[BOXLEFT] - MAXRADIUS), 0);
      int right =  min(BlockX(line->bbox[BOXRIGHT] + MAXRADIUS), width-1);
      int bottom = max(BlockY(line->bbox[BOXBOTTOM] - MAXRADIUS), 0);
      int top =    min(BlockY(line->bbox[BOXTOP] + MAXRADIUS), height-1);

      for (int j = bottom*width; j <= top*width; j += width)
	for (int i = left; i <= right; i++)
	  for (Actor *a = cells[j+i].actors; a; a = a->bnext)
	    {
	      if (a->flags & MF_SOLID)
		{
		  bbox_t box;
		  box.Set(a->pos.x, a->pos.y, a->radius);

		  if (!box.BoxTouchBox(line->bbox))
		    continue;

		  if (box.BoxOnLineSide(line) != -1)
		    continue;

		  p->PushActor(a, &p->segs[k]);
		  blocked = true;
		}
	    }
    }
  return blocked;
}


//====================================================================================
//   Map methods
//====================================================================================


polyobj_t *Map::PO_FindPolyobj(unsigned id)
{
  PO_iter_t i = PO_map.find(id);
  if (i == PO_map.end())
    return NULL;

  return i->second;
}


bool Map::PO_Busy(unsigned id)
{
  polyobj_t *p = PO_FindPolyobj(id);
  if (!p || !p->thinker)
    return false;
  else
    return true;
}



vector<mapthing_t *> polyspawn; // temporary list of PO spawn mapthings used during map loading
vector<mapthing_t *> polyanchor; // temporary list of PO anchor mapthings


/// Find a LINE_PO_START cycle for polyobj p, starting from 'line'.
/// \return true if succesful
bool Map::PO_FindLines(polyobj_t *p, line_t *line)
{
  // find the lines in the PO (assume it is a simple cycle, or else...)
  vertex_t start = *line->v1;
  vertex_t end   = *line->v2;

  if (end == start)
    {
      CONS_Printf("Error: Polyobj %d: LINE_PO_START linedef %d has identical endpoints.\n", p->id, line - lines);
      return false;
    }

  p->lines.push_back(line);

  for (int k = 0; k < numlines; k++)
    if (*lines[k].v1 == end)
      {
	if (*lines[k].v2 == end)
	  {
	    CONS_Printf("Error: Polyobj %d: Linedef %d in a LINE_PO_START cycle starting from linedef %d has identical endpoints.\n", p->id, k, line - lines);
	    return false;
	  }

	p->lines.push_back(&lines[k]);
	end = *lines[k].v2;

	if (end == start)
	  return true; // found a complete cycle

	// if PO lines do not form a simple cycle, we may get stuck in infinite loop...
	if (p->lines.size() >= PO_MAXLINES)
	  {
	    CONS_Printf("Error: Polyobj %d: Cycle starting from LINE_PO_START linedef %d has more than %d lines (or a subcycle).\n", p->id, line - lines, PO_MAXLINES);
	    return false;
	  }

	k = -1; // back to the beginning
      }

  CONS_Printf("Error: Polyobj %d, starting from LINE_PO_START linedef %d, is not closed.\n", p->id, line - lines);
  return false;
}



void Map::PO_Init()
{
  // NOTE: HACK, mapthing_t::z stores the polyobj thing type

  // Each polyobj MUST have a single anchor point.
  // Basic Hexen polyobjs allow only one spawn instance per polyobj.

  NumPolyobjs = polyanchor.size();
  CONS_Printf("%d polyobjs.\n", NumPolyobjs);

  // allocate the polyobjects
  polyobjs = static_cast<polyobj_t *>(Z_Malloc(NumPolyobjs * sizeof(polyobj_t), PU_LEVEL, 0));
  memset(polyobjs, 0, NumPolyobjs * sizeof(polyobj_t));

  // generate the polyobj models, one per anchor point
  for (int i = 0; i < NumPolyobjs; i++)
    {
      mapthing_t *mt = polyanchor[i];
      unsigned id = mt->angle; // For a EN_PO_SPAWN* thing, the "angle" field contains the PO number.

      // Is this id already taken?
      if (PO_map.count(id))
	{
	  CONS_Printf("Error: Polyobj %d: Multiple anchor points!\n", id);
	  polyobjs[i].bad = true;
	  continue;
	}

      if (id == 0)
	CONS_Printf("Warning: Polyobj with polyNumber zero found! Strange things may happen.\n");

      polyobj_t *p = &polyobjs[i];
      p->mp = this;
      p->id = id;
      //p->anchor = mt;
      p->origin.x = mt->x;
      p->origin.y = mt->y;

      PO_map[id] = p; // insert it into the map
    }


  // Search linedefs for the polyobj definitions. Single-pass algorithm, with the exception of finding the LINE_PO_START cycles.
  for (int i = 0; i < numlines; i++)
    {
      line_t *line = &lines[i];
      if (line->special == LINE_PO_START)
	{
	  unsigned id = line->args[0];

	  // mark as used
	  line->special = 0;
	  line->args[0] = 0;

	  polyobj_t *p = PO_FindPolyobj(id);
	  if (!p)
	    {
	      CONS_Printf("Error: Polyobj %d (Initialized from LINE_PO_START linedef %d): No anchor point found!\n", id, i);
	      continue;
	    }

	  if (!p->lines.empty())
	    {
	      CONS_Printf("Error: Polyobj %d: Multiple initializations! (LINE_PO_START linedef %d)\n", id, i);
	      continue;
	    }

	  p->bad = !PO_FindLines(p, line);
	  p->mirror_id = line->args[1];
	  p->sound_seq = line->args[2];
	}
      else if (line->special == LINE_PO_EXPLICIT)
	{
	  unsigned id = line->args[0];

	  // mark as used
	  line->special = 0;
	  line->args[0] = 0;

	  polyobj_t *p = PO_FindPolyobj(id);
	  if (!p)
	    {
	      CONS_Printf("Error: Polyobj %d (Initialized from LINE_PO_EXPLICIT linedef %d): No anchor point found!\n", id, i);
	      continue;
	    }

	  unsigned n = line->args[1]; // alkaa 1:stä!!
	  
	  if (n == 0)
	    {
	      CONS_Printf("Error: Polyobj %d: LINE_PO_EXPLICIT linedef %d has zero as its order number!\n", id, i);
	      continue;
	    }
	  else if (n > PO_MAXLINES)
	    {
	      p->bad = true;
	      CONS_Printf("Error: Polyobj %d: LINE_PO_EXPLICIT linedef %d has an order number greater than %d.\n", id, i, PO_MAXLINES);
	      continue;
	    }

	  if (n > p->lines.size())
	    p->lines.resize(n);

	  n--; // zero-based

	  if (p->lines[n])
	    {
	      CONS_Printf("Error: Polyobj %d: The order number of LINE_PO_EXPLICIT linedef %d has already been taken by line %d!\n", id, i, p->lines[n] - lines);
	      continue;
	    }

	  if (n == 0)
	    {
	      p->mirror_id = line->args[2];
	      p->sound_seq = line->args[3];
	    }

	  p->lines[n] = line;
	}
    }

  // check explicits for gaps, generate segs
  for (int i = 0; i < NumPolyobjs; i++)
    {
      polyobj_t *p = &polyobjs[i];
      if (p->bad)
	continue;

      p->Build();
    }

  // Find the spawn spots, and spawn each polyobj.
  int n = polyspawn.size();
  for (int i = 0; i < n; i++)
    {
      mapthing_t *mt = polyspawn[i];
      unsigned id = mt->angle;

      polyobj_t *p = PO_FindPolyobj(id);
      if (!p)
	{
	  CONS_Printf("Error: Trying to spawn an undefined polyobj %d from mapthing %d!\n", id, mt - mapthings);
	  continue;
	}

      p->damage = (mt->height == EN_PO_SPAWNCRUSH) ? 3 : 0;
      if (!p->Move(mt->x - p->origin.x, mt->y - p->origin.y))
	{
	  p->bad = true;
	  CONS_Printf("Error: Polyobj %d: Spawnspot blocked by Actor(s)!\n", id);
	}
    }

  // clean up for next level
  polyanchor.clear();
  polyspawn.clear();
}
