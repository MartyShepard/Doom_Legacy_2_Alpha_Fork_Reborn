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

#ifndef p_polyobj_h
#define p_polyobj_h 1

#include <vector>
#include "g_think.h"
#include "r_defs.h"

/// \brief Physical polyobject definition.
///
/// The corresponding Thinker is the polyobject_t class.
struct polyobj_t
{
  Map *mp;            ///< Map to which the polyobj belongs
  unsigned id;        ///< polyobj number (should be nonzero)
  unsigned mirror_id; ///< polyobj mirroring this one, or zero
  unsigned sound_seq; ///< sound sequence
  int      damage;    ///< crushing damage
  angle_t  angle;     ///< current rotation of the polyobj

  mappoint_t origin;  ///< current origin, also sound source

  std::vector<line_t *> lines; ///< pointers to the lines constituting the polyobj
  seg_t *segs; ///< segs generated from the lines

  int num_points;            ///< number of unique vertices in the polyobj
  vertex_t *base_points;     ///< copies of the unrotated vertices relative to the origin
  vertex_t **current_points; ///< current vertices
  vertex_t center;           ///< average of the current vertices, the "location" of the polyobj
  int      bbox[4];          ///< bounding box in blockmap coordinates

  class polyobject_t *thinker; ///< pointer to a Thinker, if the polyobj is moving

  bool bad; ///< is the polyobj invalid?
  int validcount;

  bool Build();
  /// Move polyobj by (dx, dy).
  bool Move(fixed_t dx, fixed_t dy);
  /// Rotate polyobj around its origin by da.
  bool Rotate(angle_t da);
  void PushActor(Actor *a, seg_t *seg);
};

/// \brief Polyblockmap list element.
///
/// One polyobj can be linked to many adjacent blockmap cells.
struct polyblock_t
{
  polyobj_t *polyobj;
  polyblock_t *prev;
  polyblock_t *next;
};




/// Polyobject ABC
class polyobject_t : public Thinker
{
  friend class Map;
  DECLARE_CLASS(polyobject_t)
protected:
  polyobj_t *poly;

public:
  inline polyobject_t(polyobj_t *p) { poly = p; }

  virtual void    Think() {}
  virtual float PushForce() { return 0; }

  enum po_type_e
  {
    po_rotate,
    po_move,
    po_door_swing,
    po_door_slide,
  };
};


/// Polyobject Rotator
class polyrotator_t : public polyobject_t
{
  friend class Map;
  DECLARE_CLASS(polyrotator_t)
protected:  
  int ang_vel; ///< angular velocity (in fineangle units)
  int    dist; ///< remaining angle to turn (in fineangle units)

public:
  polyrotator_t(polyobj_t *p, float av, float d) : polyobject_t(p), ang_vel(int(av)), dist(int(d)) {}
  
  virtual void    Think();
  virtual float PushForce();
};



/// Polyobject Mover
class polymover_t : public polyobject_t
{
  friend class Map;
  DECLARE_CLASS(polymover_t)
protected:
  float   speed;
  angle_t angle;
  float   dist;

  fixed_t xs, ys;

public:
  polymover_t(polyobj_t *p, float speed, angle_t ang, float dist);
  
  virtual void    Think();
  virtual float PushForce();
};



/// \brief Polyobject swinging door
///
/// A timed polyrotator.
class polydoor_swing_t : public polyrotator_t
{
  friend class Map;
  DECLARE_CLASS(polydoor_swing_t)
public:

protected:
  bool    closing;
  int     open_delay, delay;
  int     initial_dist; ///< in fineangle units

public:
  polydoor_swing_t(polyobj_t *p, float ang_vel, float dist, int delay);
  virtual void Think();
};



/// \brief Polyobject sliding door
///
/// A polymover with a timer.
class polydoor_slide_t : public polymover_t
{
  friend class Map;
  DECLARE_CLASS(polydoor_slide_t)
public:

protected:
  bool    closing;
  int     open_delay, delay;
  float   initial_dist;

public:
  polydoor_slide_t(polyobj_t *p, float speed, angle_t ang, float dist, int delay);
  virtual void Think();
};


#endif
