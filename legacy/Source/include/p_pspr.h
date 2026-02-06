// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: p_pspr.h 396 2006-12-06 22:43:40Z smite-meister $
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
/// \brief  1st person sprites (weapon anims)

#ifndef p_pspr_h
#define p_pspr_h 1

// Basic data types.
// Needs fixed point, and BAM angles.
#include "m_fixed.h"

const fixed_t LOWERSPEED = 6;
const fixed_t RAISESPEED = 6;

#define WEAPONBOTTOM            128
#define WEAPONTOP               32

//
// Overlay psprites are scaled shapes
// drawn directly on the view screen,
// coordinates are given for a 320*200 view screen.
//
typedef enum
{
  ps_weapon,
  ps_flash,
  NUMPSPRITES

} psprnum_t;

struct pspdef_t
{
  struct weaponstate_t *state;  // a NULL state means not active
  int       tics;
  fixed_t   sx, sy;
};


extern struct line_t  *target_line;
extern enum mobjtype_t PuffType;

#endif
