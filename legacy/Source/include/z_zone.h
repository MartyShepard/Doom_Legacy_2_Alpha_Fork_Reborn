// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: z_zone.h 433 2007-02-07 19:06:40Z smite-meister $
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
//---------------------------------------------------------------------

/// \file
/// \brief Zone Memory Allocation
///
/// Perhaps NeXT ObjectiveC inspired.
/// Remark: this was the only stuff that, according
/// to John Carmack, might have been useful for Quake.
///
/// However, now it has been rendered obsolete by OS memory management. Sic transit gloria mundi.

#ifndef z_zone_h
#define z_zone_h 1

#include <stdio.h>

/// \brief Zone Memory tags.
enum memtag_t
{
  PU_STATIC = 0, ///< static entire execution time
  PU_SOUND,      ///< sound effects
  PU_MUSIC,      ///< music
  PU_DAVE,       ///< anything else Dave wants static
  PU_SPRITE,     ///< sprite structures
  PU_MODEL,      ///< 3D models
  PU_TEXTURE,    ///< 2D bitmaps (most graphics)
  PU_LEVEL,      ///< map data
  PU_LEVSPEC,    ///< special thinkers in a level
  PU_OPENGL_GEOMETRY,
  PU_NUMTAGS
};


/// Initialize the memory subsystem.
void Z_Init();

/// Number of bytes allocated for given tag type.
unsigned Z_TagUsage(unsigned tagnum);

/// Tag used by ptr. ptr MUST be allocated using Z_Malloc.
int Z_GetTag(void *ptr);

/// Allocate memory.
void *Z_Malloc(int size, int tag, void **user);
#define Z_MallocAlign(s,t,p,a) Z_Malloc((s),(t),(p))
#define ZZ_Alloc(x) Z_Malloc((x), PU_STATIC, NULL)

/// Free memory.
void Z_Free(void *ptr);

/// Duplicate a string into newly allocated memory.
char *Z_Strdup(const char *s, int tag, void **user);

#endif
