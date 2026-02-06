// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_bsp.h 452 2007-05-17 17:11:02Z smite-meister $
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
/// \brief Software renderer, BSP traversal and handling.

#ifndef r_bsp_h
#define r_bsp_h 1

#include "r_defs.h"

extern seg_t*           curline;
extern side_t*          sidedef;
extern line_t*          linedef;
extern sector_t*        frontsector;
extern sector_t*        backsector;


typedef void (*drawfunc_t)(int start, int stop);


void R_ClearClipSegs();
void R_SetupClipSegs();
void R_ClearDrawSegs();

int  R_GetPlaneLight(sector_t* sector, fixed_t  planeheight, bool underside);

#endif
