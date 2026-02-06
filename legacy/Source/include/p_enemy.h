// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_enemy.h 504 2007-12-16 17:58:26Z smite-meister $
//
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
/// \brief Common function prototypes for monster AI

#ifndef p_enemy_h
#define p_enemy_h 1


#define HITDICE(a) ((1+(P_Random() & 7))*(a))

const int MELEERANGE   = 64;
const int AIMRANGE     = 16*64;
const int MISSILERANGE = 32*64;

const float   FLOATSPEED = 4;

enum dirtype_t
{
  DI_EAST,
  DI_NORTHEAST,
  DI_NORTH,
  DI_NORTHWEST,
  DI_WEST,
  DI_SOUTHWEST,
  DI_SOUTH,
  DI_SOUTHEAST,
  DI_NODIR,
  NUMDIRS
};

void   P_NoiseAlert(class Actor *target, Actor *emitter);
float  P_BulletSlope(class PlayerPawn *p);

int  P_FaceMobj(Actor *source, Actor *target, angle_t *delta);
void P_MinotaurSlam(Actor *source, Actor *target);
bool P_TouchWhirlwind(Actor *target);

void   A_FaceTarget(class DActor *actor);
void   A_Chase(DActor *actor);
void   A_Fall(DActor *actor);
void   A_Look(DActor *actor);
void   A_Pain(DActor *actor);

#endif
