// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: tables.h 463 2007-05-21 23:32:04Z smite-meister $
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
/// \brief Lookup tables for trigonometric functions.
///
/// fixed_t finetangent[FINEANGLES/2]   - tangens LUT
///  Maps fineangle_t(alpha + pi/2) to fixed_t(tan(angle)).
///  Should work with BAM fairly well (12 of 16bit, effectively, by shifting).
///
/// fixed_t finesine[FINEANGLES * 5/4] - sine/cosine LUT
///  Maps fineangle_t(alpha) to fixed_t(sin(angle)).
///  Remarkable thing is, how to use BAMs with this?
///
/// angle_t tantoangle[SLOPERANGE+1]   - arctan LUT
///  Maps (tan(alpha) * SLOPERANGE) to angle_t(alpha).
///
///  finetangent[i] == FRACUNIT * tan((i - FINEANGLES/4 + 0.5) * (2*pi / FINEANGLES))
///  finesine[i]    == FRACUNIT * sin((i + 0.5) * (2*pi / FINEANGLES))
///  tantoangle[i]  == atan(i/SLOPERANGE) * (2^32 / (2*pi))

#ifndef tables_h
#define tables_h 1

#include "doomtype.h"
#include "vect.h"
#include "m_fixed.h"

typedef Uint32 angle_t;


/// Converts angle_t to degrees, normalized to [0, 360)
inline double Degrees(angle_t a)
{
  return double(a) * (45.0 / double(1 << 29));
}

/// Converts angle_t to degrees, normalized to [-180, 180)
inline double DegreesSigned(angle_t a)
{
  return double(static_cast<Sint32>(a)) * (45.0 / double(1 << 29));
}


extern const fixed_t FloatBobOffsets[64];


#define FINEANGLES              8192
#define FINEMASK                (FINEANGLES-1)
#define ANGLETOFINESHIFT        19      // 0x100000000 to 0x2000


// Effective size is 10240. [5*FINEANGLES/4]
extern const fixed_t* const finesine;

// Re-use data, is just PI/2 phase shift.
extern const fixed_t* const finecosine;

// Effective size is 4096. [FINEANGLES/2]
extern const fixed_t* const finetangent;


const angle_t ANG45  = 0x20000000;
const angle_t ANG90  = 0x40000000;
const angle_t ANG180 = 0x80000000;
const angle_t ANG270 = 0xc0000000;

const angle_t ANGLE_MAX = 0xffffffff;
const angle_t ANGLE_1   = ANG45 / 45;
const angle_t ANGLE_60  = ANG180 / 3;


/// Absolute value of angle difference, always in [0, ANG180].
inline angle_t Abs(angle_t a)
{
  return (a <= ANG180) ? a : -a; // -a is effectively 2pi - a since it wraps
};


/// Encapsulation for tabulated sine, cosine and tangent
//#define AIMINGTOSLOPE(aiming)   finesine[((aiming) >> ANGLETOFINESHIFT) & FINEMASK]
inline fixed_t Sin(angle_t a) { return finesine[a >> ANGLETOFINESHIFT]; }
inline fixed_t Cos(angle_t a) { return finecosine[a >> ANGLETOFINESHIFT]; }
inline fixed_t Tan(angle_t a)
{
  a += ANG90; // wraps around like angles should
  return finetangent[a >> ANGLETOFINESHIFT];
  //return finetangent[(2048 + (a>>ANGLETOFINESHIFT)) & FINEMASK];
}


// to get a global angle from cartesian coordinates, the coordinates are
// flipped until they are in the first octant of the coordinate system, then
// the y (<=x) is scaled and divided by x to get a tangent (slope) value
// which is looked up in the tantoangle[] table.
#define SLOPEBITS   11
#define SLOPERANGE  2048

/// Encapsulation for arcustangent (for the range 0 <= x <= 1)
inline angle_t ArcTan(fixed_t x)
{
  // The +1 size is to handle the case when x==y without additional checking.
  extern angle_t tantoangle[SLOPERANGE+1];

  //#define DBITS (fixed_t::FBITS - SLOPEBITS)
  return tantoangle[(x << SLOPEBITS).floor()];
}

angle_t R_PointToAngle2(fixed_t x2, fixed_t y2, fixed_t x1, fixed_t y1);
fixed_t R_PointToDist2(fixed_t x2, fixed_t y2, fixed_t x1, fixed_t y1);
fixed_t P_AproxDistance(fixed_t dx, fixed_t dy);

inline angle_t R_PointToAngle2(const vec_t<fixed_t>& a, const vec_t<fixed_t>& b)
{
  return R_PointToAngle2(a.x, a.y, b.x, b.y);
}

inline fixed_t P_XYdist(const vec_t<fixed_t>& a, const vec_t<fixed_t>& b)
{
  return P_AproxDistance(a.x - b.x, a.y - b.y);
}


#endif
