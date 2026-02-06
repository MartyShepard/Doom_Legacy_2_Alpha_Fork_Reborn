// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_setup.h 306 2006-02-11 16:26:27Z jussip $
//
// Copyright (C) 1998-2005 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Linedef conversion to Hexen system

#ifndef p_setup_h
#define p_setup_h 1


typedef unsigned char byte;


// linedef conversion lookup table entry
struct xtable_t
{
  byte type;
  byte args[5];
  byte trigger;
};


// used in linedef conversion table
enum trigger_e
{
  T_REPEAT = 0x01, // bit 0

  T_AMASK  = 0x0e, // bits 1--3
  T_ASHIFT = 1,

  T_CROSS  = 0, // W
  T_USE    = 1,	// P, S
  T_MCROSS = 2,	// when monster crosses line
  T_IMPACT = 3,	// G
  T_PUSH   = 4,	// when player/monster pushes line
  T_PCROSS = 5, // when projectile crosses line

  T_ALLOWMONSTER = 0x10, // bit 4

  //T_TAG_1 = 0x20 // bit 5
};


extern xtable_t *linedef_xtable;
extern int  linedef_xtable_size;

#endif
