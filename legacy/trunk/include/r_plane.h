// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: r_plane.h 449 2007-05-05 20:45:25Z smite-meister $
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
/// SW renderer, visplane stuff (floor, ceilings).

#ifndef r_plane_h
#define r_plane_h 1

#include "screen.h"     //needs MAXVIDWIDTH/MAXVIDHEIGHT
#include "r_segs.h"

//
// Now what is a visplane, anyway?
// Simple : kinda floor/ceiling polygon optimised for Doom rendering.
// 4124 bytes!
//
struct visplane_t
{
  visplane_t*    next;//SoM: 3/17/2000

  fixed_t               height;
  fixed_t               viewz;
  angle_t               viewangle;
  Material*             pic;
  int                   lightlevel;
  int                   minx;
  int                   maxx;

  //SoM: 4/3/2000: Colormaps per sector!
  class fadetable_t    *extra_colormap;

  // leave pads for [minx-1]/[maxx+1]

  //faB: words sucks .. should get rid of that.. but eats memory
  //added:08-02-98: THIS IS UNSIGNED! VERY IMPORTANT!!
  unsigned short         pad1;
  unsigned short         top[MAXVIDWIDTH];
  unsigned short         pad2;
  unsigned short         pad3;
  unsigned short         bottom[MAXVIDWIDTH];
  unsigned short         pad4;

  int                    high, low; // SoM: R_PlaneBounds should set these.

  fixed_t xoffs, yoffs;  // SoM: 3/6/2000: Srolling flats.

  // SoM: frontscale should be stored in the first seg of the subsector
  // where the planes themselves are stored. I'm doing this now because
  // the old way caused trouble with the drawseg array was re-sized.
  int    scaleseg;

  ffloor_t* ffloor;
  bool sky; // sky plane?
};


extern visplane_t*    floorplane;
extern visplane_t*    ceilingplane;

#ifdef OLDWATER
extern visplane_t*    waterplane;
#endif


// Visplane related.
extern  short*          lastopening;

typedef void (*planefunction_t) (int top, int bottom);

extern planefunction_t  floorfunc;
extern planefunction_t  ceilingfunc_t;

extern short            floorclip[MAXVIDWIDTH];
extern short            ceilingclip[MAXVIDWIDTH];
extern short            waterclip[MAXVIDWIDTH];   //added:18-02-98:WATER!
extern fixed_t          frontscale[MAXVIDWIDTH];
extern fixed_t          yslopetab[MAXVIDHEIGHT*4];

extern fixed_t*         yslope;
extern fixed_t          distscale[MAXVIDWIDTH];

void R_InitPlanes();
void R_MapPlane(int y, int x1, int x2);
void R_MakeSpans(int x, int t1, int b1, int t2, int b2);


// SoM: Draws a single visplane. If !handlesource, it won't allocate or
// remove ds_source.
//void R_DrawSinglePlane(visplane_t* pl, bool handlesource);

visplane_t* R_CheckPlane( visplane_t*   pl, int           start, int           stop );

void R_ExpandPlane(visplane_t*  pl, int start, int stop);

void R_PlaneBounds(visplane_t* plane);


struct planemgr_t
{
  visplane_t*  plane;
  fixed_t      height;
  bool      mark;
  fixed_t      f_pos;  // `F' for `Front sector'.
  fixed_t      b_pos;  // `B' for `Back sector'
  fixed_t      f_frac;
  fixed_t      f_step;
  fixed_t      b_frac;
  fixed_t      b_step;
  short        f_clip[MAXVIDWIDTH];
  short        c_clip[MAXVIDWIDTH];

  ffloor_t  *ffloor;
};

extern planemgr_t    ffloor[MAXFFLOORS];
extern int           numffloors;
#endif
