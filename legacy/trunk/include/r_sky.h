// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_sky.h 449 2007-05-05 20:45:25Z smite-meister $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2006 by DooM Legacy Team.
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
/// \brief Sky rendering.

#ifndef r_sky_h
#define r_sky_h 1

// The sky map is 256*128*4 maps.
#define ANGLETOSKYSHIFT         22

extern int            skytexturemid;

// call after skytexture is set to adapt for old/new skies
void R_SetupSkyDraw(class Material *skytex);


#endif
