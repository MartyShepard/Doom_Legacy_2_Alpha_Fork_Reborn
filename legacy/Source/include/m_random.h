// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: m_random.h 531 2008-11-02 09:03:56Z smite-meister $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2008 by DooM Legacy Team.
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
/// \brief Pseudorandom numbers.


#ifndef m_random_h
#define m_random_h 1

#include "doomtype.h"
#include "m_fixed.h"

/// Uniformly distributed pseudo-random numbers in the range [0,1).
float RandomUniform();
inline float Random() { return RandomUniform(); }
/// Uniformly distributed pseudo-random numbers in the range [min,max).
inline float Random(float min, float max) { return RandomUniform()*(max-min) + min; }

/// Pseudo-random numbers following a pyramid distribution in the range (-1,1).
float RandomS();

/// N(0,1) normally distributed pseudo-random numbers.
float RandomGauss();


/// Returns a number from 0 to 255, from a lookup table.
byte M_Random();

/// As M_Random, but used only by the play simulation.
byte P_Random();
int P_SignedRandom();


// Fix randoms for demos.
void M_ClearRandom();

byte P_GetRandIndex();

void P_SetRandIndex(byte rindex);

#endif
