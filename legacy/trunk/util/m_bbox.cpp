// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: m_bbox.cpp 476 2007-06-14 18:31:04Z smite-meister $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2004 by DooM Legacy Team.
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
//
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Bounding boxes

#include "doomtype.h"
#include "m_bbox.h"
#include "p_maputl.h"

void bbox_t::Clear()
{
  box[BOXTOP] = box[BOXRIGHT] = fixed_t::FMIN;
  box[BOXBOTTOM] = box[BOXLEFT] = fixed_t::FMAX;
}


void bbox_t::Add(fixed_t x, fixed_t y)
{
  if (x<box[BOXLEFT  ])   box[BOXLEFT  ] = x;
  if (x>box[BOXRIGHT ])   box[BOXRIGHT ] = x;

  if (y<box[BOXBOTTOM])   box[BOXBOTTOM] = y;
  if (y>box[BOXTOP   ])   box[BOXTOP   ] = y;
}


void bbox_t::Move(fixed_t x, fixed_t y)
{
  box[BOXLEFT] += x;
  box[BOXRIGHT] += x;

  box[BOXTOP] += y;
  box[BOXBOTTOM] += y;
}



bool bbox_t::PointInBox(fixed_t x, fixed_t y) const
{
  if (x < box[BOXLEFT]   || x > box[BOXRIGHT] ||
      y < box[BOXBOTTOM] || y > box[BOXTOP])
    return false;

  return true;
}

bool bbox_t::CircleTouchBox(fixed_t x, fixed_t y, fixed_t radius) const
{
  if (box[BOXLEFT  ]-radius > x ||
      box[BOXRIGHT ]+radius < x ||
      box[BOXBOTTOM]-radius > y ||
      box[BOXTOP   ]+radius < y)
    return false;

  return true;
}

bool bbox_t::BoxTouchBox(const bbox_t &other) const
{
  if (box[BOXRIGHT] <= other[BOXLEFT]
      || box[BOXLEFT] >= other[BOXRIGHT]
      || box[BOXTOP] <= other[BOXBOTTOM]
      || box[BOXBOTTOM] >= other[BOXTOP])
    return false;

  return true;
}

// Returns true if line drawn between given points intercepts any edge
// of the box.

bool bbox_t::LineCrossesEdge(const fixed_t x1, const fixed_t y1, const fixed_t x2, const fixed_t y2) const
{
  divline_t line;
  line.x = x1;
  line.y = y1;
  line.dx = x2-x1;
  line.dy = y2-y1;

  int s1 = line.PointOnSide(box[BOXRIGHT], box[BOXTOP]); 
  int s2 = line.PointOnSide(box[BOXLEFT], box[BOXTOP]); 

  if (s1 != s2)
    // see if line crosses the top edge
    if ((y1 <= box[BOXTOP]) ^ (y2 <= box[BOXTOP]))
      return true;

  int s3 = line.PointOnSide(box[BOXLEFT], box[BOXBOTTOM]); 

  if (s2 != s3)
    if ((x1 <= box[BOXLEFT]) ^ (x2 <= box[BOXLEFT]))
      return true;

  int s4 = line.PointOnSide(box[BOXRIGHT], box[BOXBOTTOM]); 

  if (s3 != s4)
    if ((y1 <= box[BOXBOTTOM]) ^ (y2 <= box[BOXBOTTOM]))
      return true;

  if (s4 != s1)
    if ((x1 <= box[BOXRIGHT]) ^ (x2 <= box[BOXRIGHT]))
      return true;

  return false;
}

// set temp location and boundingbox
void bbox_t::Set(fixed_t x, fixed_t y, fixed_t r)
{
  box[BOXTOP]    = y + r;
  box[BOXBOTTOM] = y - r;
  box[BOXRIGHT]  = x + r;
  box[BOXLEFT]   = x - r;
}

