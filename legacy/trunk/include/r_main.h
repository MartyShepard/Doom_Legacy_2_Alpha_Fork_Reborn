// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: r_main.h 502 2007-12-13 01:50:51Z smite-meister $
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
/// \brief Software renderer state variables. Lights.


#ifndef r_main_h
#define r_main_h 1

#include "r_defs.h"
#include "screen.h"
#include "tables.h"

extern  int     viewangleoffset;

//
// POV related.
//
extern fixed_t          viewcos;
extern fixed_t          viewsin;

extern int              viewwidth;
extern int              viewheight;
extern int              viewwindowx;
extern int              viewwindowy;

extern int              centerx;
extern int              centery;

extern int      centerypsp;

extern fixed_t          centerxfrac;
extern fixed_t          centeryfrac;
extern fixed_t          projection;
extern fixed_t          projectiony;    //added:02-02-98:aspect ratio test...

extern int              validcount;

extern int              linecount;
extern int              loopcount;

extern int      framecount;



extern angle_t          clipangle;

extern int              viewangletox[FINEANGLES/2];
extern angle_t          xtoviewangle[MAXVIDWIDTH+1];

extern fixed_t          rw_distance;
extern angle_t          rw_normalangle;

// angle to line origin
extern int              rw_angle1;



//==========================================
//               Lights
//==========================================


// This could be wider for >8 bit display.
// Indeed, true color support is posibble
//  precalculating 24bpp lightmap/colormap LUT.
//  from darkening PLAYPAL to all black.
// Could even use more than 32 levels.
typedef byte lighttable_t;



//Hurdler: 04/12/2000: for now, only used in hardware mode
//                     maybe later for software as well?
//                     that's why it's moved here
struct light_t
{
  Uint16  type;           // light,... (cfr #define in hwr_light.c)

  float   light_xoffset;
  float   light_yoffset;  // y offset to adjust corona's height

  Uint32  corona_color;   // color of the light for static lighting
  float   corona_radius;  // radius of the coronas

  Uint32  dynamic_color;  // color of the light for dynamic lighting
  float   dynamic_radius; // radius of the light ball
  float   dynamic_sqrradius; // radius^2 of the light ball
};

struct lightmap_t
{
  float       s[2], t[2];
  light_t    *light;
  lightmap_t *next;
};


/// \brief Information for shadows casted by 3D floors.
///
/// This information is contained inside the sector_t and is used as the base
/// information for casted shadows.
struct lightlist_t
{
  fixed_t           height;
  short            *lightlevel;
  fadetable_t      *extra_colormap;
  struct ffloor_t  *caster;
};


/// Used for rendering walls with shadows cast on them.
struct r_lightlist_t
{
  fixed_t           height;
  fixed_t           heightstep;
  fixed_t           botheight;
  fixed_t           botheightstep;
  short             lightlevel;
  fadetable_t      *extra_colormap;
  lighttable_t     *rcolormap;
  int               flags;
};




//SoM: 3/30/2000: Boom colormaps.
//SoM: 4/7/2000: Had to put a limit on colormaps :(
#define                 MAXCOLORMAPS 30


// Lighting LUT.
// Used for z-depth cuing per column/row,
//  and other lighting effects (sector ambient, flash).

// Lighting constants.
// Now why not 32 levels here?
#define LIGHTLEVELS             16
#define LIGHTSEGSHIFT            4

#define MAXLIGHTSCALE           48
#define LIGHTSCALESHIFT         4 // was 12
#define MAXLIGHTZ              128
#define LIGHTZSHIFT              4 // without any fixed_t stuff

/// Precalculated fadetable offsets
extern int  scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
extern int  scalelightfixed[MAXLIGHTSCALE];
extern int  zlight[LIGHTLEVELS][MAXLIGHTZ];
extern int  fixedcolormap;

extern int              extralight;


// Number of diminishing brightness levels.
// There a 0-31, i.e. 32 LUT in the COLORMAP lump.
#define NUMCOLORMAPS            32



// REFRESH - the actual rendering functions.

// initializes the client part of the renderer
void R_Init();

/// Updates internal renderer state to the given tic.
void R_Update(tic_t t);

// just sets setsizeneeded true
extern bool setsizeneeded;
void   R_SetViewSize();

// do it (sometimes explicitly called)
void   R_ExecuteSetViewSize();

// add commands related to engine, at game startup
void   R_RegisterEngineStuff();

#endif
