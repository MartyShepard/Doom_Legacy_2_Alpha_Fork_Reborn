// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: hwr_bsp.h 433 2007-02-07 19:06:40Z smite-meister $
//
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
//
//-----------------------------------------------------------------------------

#ifndef hwr_bsp_h
#define hwr_bsp_h 1

#include <vector>
#include "r_defs.h"
#include "z_zone.h"
#include "hardware/hwr_states.h"
#include "hardware/hwr_geometry.h"

/**
  \brief a vertex of a Doom 'plane' polygon
*/
struct PolyVertex
  {
    float x;
    float y;
  };

/**
  \brief a convex 'plane' polygon, clockwise order (GL_TRIANGLE_FAN)
*/
struct Poly
  {
    int        numpts;
    PolyVertex pts[0];
  };

/**
  \brief the floating point (instead of fixed) version of divline_t
*/
struct DivLine
  {
    float x;
    float y;
    float dx;
    float dy;
  };

/**
  \brief Handles sub-sectors (which are leaves of the BSP) using GL friendly structure.
*/
class Subsector
{
private:
  subsector_t *sub;  // pointer to the software structure
  std::vector<Geometry *> geometries;
  std::vector<State *> states;

  void AddWall(seg_t *line, seg_t *prev_line, seg_t *next_line, fixed_t floor, fixed_t ceiling);

public:
  Subsector(int num, Poly *poly);
  ~Subsector();

  /// Render a sub-sector
  void Render();
};

/**
  \brief Handles the BSP with GL friendly structures.
*/
class HWBsp
{
private:
  std::vector<Subsector *> subsectors;
  unsigned int num_planepolys;
  std::vector<Poly *> planepolys;
  float bspfrac;                        //TODO: this is ugly, but it's better for now than a global variable

  void AddSubsector(unsigned int num, Poly *poly);
  /// Return the sub-sector which have the number "num"
  inline Subsector *GetSubsector(int num)
  {
      return subsectors[num];
  }

  // Legacy of the old renderer (we might rewrite all this one day, with support of glBSP, but for now it's not a priority)
  inline Poly *AllocPoly(int numpts);
  inline void FreePoly(Poly *poly);
  inline PolyVertex *AllocVertex ();
  inline void FreeVertex(PolyVertex *polyvertex);
  PolyVertex *FracDivLine(DivLine *bsp, PolyVertex *v1, PolyVertex *v2);
  bool SameVertice(PolyVertex *p1, PolyVertex *p2);
  void SplitPoly(DivLine *bsp, Poly *poly, Poly **frontpoly, Poly **backpoly);
  Poly *CutOutSubsecPoly(seg_t *lseg, int count, Poly *poly);
  void SearchDivLine(node_t *bsp, DivLine *divline);
  bool PointInSeg(PolyVertex *a, PolyVertex *v1, PolyVertex *v2);
  void SearchSegInBSP(int bspnum, PolyVertex *p, Poly *poly);
  void AdjustSegs();
  void SolveTProblem();

public:
  HWBsp(int size, int bspnum);
  ~HWBsp();

  /// Traverse recursively the BSP nodes to create sub-sectors
  void Traverse(int bspnum, Poly* poly, Uint32 *leafnode, bbox_t &bbox);

  /// Render recursively the BSP nodes
  void Render(int bspnum);
};

inline Poly* HWBsp::AllocPoly(int numpts)
{
  Poly *p = (Poly *) Z_Malloc(sizeof(Poly) + sizeof(PolyVertex) * numpts, PU_OPENGL_GEOMETRY, NULL);
  p->numpts = numpts;
  return p;
}

//TODO: polygons should be freed in reverse order for efficiency,
inline void HWBsp::FreePoly(Poly* poly)
{
  Z_Free(poly);
}

inline PolyVertex* HWBsp::AllocVertex()
{
  PolyVertex *p = (PolyVertex *) Z_Malloc(sizeof(PolyVertex), PU_OPENGL_GEOMETRY, NULL);
  return p;
}

//TODO: not used for now, but we should see why memory allocated by AllocVertex is never explicity freed
//      (maybe it's implicitely freed or freed by the software code)
inline void HWBsp::FreeVertex(PolyVertex *polyvertex)
{
  Z_Free(polyvertex);
}

#endif
