// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2002-2007 by DooM Legacy Team.
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
/// \brief blockmap_t class definition.

#ifndef g_blockmap_h
#define g_blockmap_h 1

#include "vect.h"
#include "m_fixed.h"

typedef bool (*traverser_t)(struct intercept_t *in);
typedef bool (*line_iterator_t)(struct line_t *l);
typedef bool (*thing_iterator_t)(class Actor *a);


/// \brief Blockmap, used for collision detection.
/*!
  Created from axis aligned bounding box of the map, a rectangular array of
  blocks of size 128 map units square. See blockmapheader_t.
  Used to speed up collision detection agains lines and things by a spatial subdivision in 2D.
*/
class blockmap_t
{
private:
#define MAPBLOCKUNITS   128
#define MAPBLOCKBITS    7
#define MAPBLOCK_END    static_cast<Uint16>(0xFFFF) ///< terminator for a blocklist

  fixed_t  orgx, orgy;    ///< origin (lower left corner) of block map in map coordinates
  int      width, height; ///< size of the blockmap in mapblocks

  Uint16  *lists;  ///< packed array of -1 terminated blocklists

  /// Each blockmap cell contains a line_t, polyobj_t and Actor list.
  struct blockmapcell_t
  {
    Uint16  *blocklist;  ///< blocklist pointer (for line_t's)
    struct polyblock_t *polys; ///< polyblock chain
    Actor   *actors;   ///< thing chain
  };

  blockmapcell_t *cells; ///< width*height array of cells

  inline int BlockX(fixed_t x) const { return (x - orgx).floor() >> MAPBLOCKBITS; }
  inline int BlockY(fixed_t y) const { return (y - orgy).floor() >> MAPBLOCKBITS; }
  
  bool LinesIterator(int x, int y, line_iterator_t func);
  bool ThingsIterator(int x, int y, thing_iterator_t func);

public:
  class Map *parent_map;  ///< TODO unnecessary if we used line_t*'s instead of Uint16 indices in the blocklists...

  blockmap_t(int lump);
  blockmap_t(Map *mp);
  ~blockmap_t();

  /// Replaces the Actor * in the cell of (x,y), returns the old one.
  Actor *Replace(fixed_t x, fixed_t y, Actor *a)
  {
    int bx = BlockX(x);
    int by = BlockY(y);

    if (bx >= 0 && bx < width && by >= 0 && by < height)
      {
	Actor **p = &cells[by * width + bx].actors;
	Actor *ret = *p;
	*p = a;
	return ret;
      }
    return NULL; // off the blockmap
  }

  /// Remove the polyobj from the blockmap.
  void PO_Unlink(polyobj_t *p);
  /// Add the polyobj to the blockmap.
  void PO_Link(polyobj_t *p);
  /// Check if the polyobj collides with any Actors, push and damage the Actors accordingly.
  bool PO_ClipActors(polyobj_t *p);

  bool IterateLinesRadius(fixed_t x, fixed_t y, fixed_t radius, line_iterator_t func);
  bool IterateThingsRadius(fixed_t x, fixed_t y, fixed_t radius, thing_iterator_t func);
  bool RoughBlockSearch(Actor *center, int distance, thing_iterator_t func);
  bool PathTraverse(const vec_t<fixed_t>& p1, const vec_t<fixed_t>& p2, int flags, traverser_t trav);

  inline fixed_t FracX(fixed_t x) const { return (x - orgx) % MAPBLOCKUNITS; }
  inline fixed_t FracY(fixed_t y) const { return (y - orgy) % MAPBLOCKUNITS; }
};



#endif
