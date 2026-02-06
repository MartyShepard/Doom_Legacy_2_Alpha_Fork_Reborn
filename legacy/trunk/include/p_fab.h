// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_fab.h 355 2006-07-27 22:13:56Z smite-meister $
//
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
/// \brief Miscellaneous oddities

#ifndef p_fab_h
#define p_fab_h 1

// spawn smoke trails behind rockets and skull head attacks
void A_SmokeTrailer(class DActor *actor);

// hack the states table to set Doom Legacy's default translucency on sprites
void P_SetTranslucencies();

// touch functions
int MT_HOLY_FX_touchfunc(class DActor *d, class Actor *p);
int MT_LIGHTNING_touchfunc(DActor *d, Actor *p);
int MT_LIGHTNING_ZAP_touchfunc(DActor *d, Actor *p);
int MT_MSTAFF_FX2_touchfunc(DActor *d, Actor *p);

#endif
