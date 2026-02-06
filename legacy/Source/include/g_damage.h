// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: g_damage.h 353 2006-07-27 15:06:52Z smite-meister $
//
// Copyright (C) 2002-2006 by DooM Legacy Team.
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
/// \brief Damage types

#ifndef g_damage_h
#define g_damage_h 1


/// Damage type bit field (uses only bits 17-32 !)
enum damage_t
{
  dt_DAMAGEMASK = 0xFFFF,

  /// what kind of damage (4 bits)
  dt_kinetic    = 0x00000, ///< bullets
  dt_crushing   = 0x10000, ///< fist
  dt_concussion = 0x20000, ///< shock wave
  dt_cutting    = 0x30000, ///< claws, shrapnel
  dt_heat       = 0x40000, ///< imp fireball
  dt_cold       = 0x50000, ///< cone of cold
  dt_shock      = 0x60000, ///< electric shock
  dt_corrosive  = 0x70000, ///< chemicals, acid
  dt_magic      = 0x80000, ///< ethereal or otherwise strange
  dt_telefrag   = 0x90000, ///< telefrag

  dt_LIVING     = 0xA0000, ///< damagetypes after this only affect living things
  dt_radiation  = 0xA0000, ///< nukage
  dt_biological = 0xB0000, ///< biowarfare agents
  dt_poison     = 0xC0000, ///< poison clouds, darts
  dt_oxygen     = 0xD0000, ///< lack of oxygen

  dt_TYPEMASK   = 0xF0000,

  // flags: other effects (bit of a hack)
  dt_norecoil   = 0x100000, ///< no recoil momentum is given to the target
  dt_always     = 0x200000, ///< cannot be avoided, even if in god mode
  dt_OTHERMASK  = 0xF00000,

  /*
  // other properties (armor piercing bullets etc.)
  dt_armorpiercing, // replace MF2_RIP?

  // admission methods?
  dt_local,
  dt_enveloping,
  dt_breathed,
  */

  dt_normal = dt_kinetic
};

#endif
