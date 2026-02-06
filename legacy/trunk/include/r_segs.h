// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: r_segs.h 452 2007-05-17 17:11:02Z smite-meister $
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
/// \brief Software renderer, drawing LineSegs from BSP.

#ifndef r_segs_h
#define r_segs_h 1


#ifndef MAXFFLOORS
# define MAXFFLOORS 40
#endif

/// Silhouette, needed for clipping Segs (mainly) and sprites representing things.
enum silhouette_e
{
  SIL_NONE   = 0,
  SIL_BOTTOM = 1,
  SIL_TOP    = 2,
  SIL_BOTH   = 3
};


/// \brief Represents a seg_t to be drawn, allocated on the fly.
/// \ingroup g_sw
struct drawseg_t
{
  seg_t*              curline;
  int                 x1;
  int                 x2;

  fixed_t             scale1;
  fixed_t             scale2;
  fixed_t             scalestep;

  // 0=none, 1=bottom, 2=top, 3=both
  int                 silhouette;

  /// do not clip sprites above this
  fixed_t             bsilheight;

  /// do not clip sprites below this
  fixed_t             tsilheight;

  // Pointers to lists for sprite clipping,
  //  all three adjusted so [x1] is first value.
  short*              sprtopclip;
  short*              sprbottomclip;
  short*              maskedtexturecol;

  struct visplane_t*  ffloorplanes[MAXFFLOORS];
  int                 numffloorplanes;
  ffloor_t*    thicksides[MAXFFLOORS];
  short*              thicksidecol;
  int                 numthicksides;
  fixed_t             frontscale[MAXVIDWIDTH];
};


extern drawseg_t *ds_p;
extern drawseg_t *drawsegs;
extern unsigned   maxdrawsegs;
extern drawseg_t *firstseg;
extern drawseg_t *firstnewseg;


extern int *walllights;

#endif
