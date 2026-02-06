// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: d_ticcmd.h 306 2006-02-11 16:26:27Z jussip $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
/// \brief ticcmd_t definition

#ifndef d_ticcmd_h
#define d_ticcmd_h 1

#include "m_fixed.h" // fixed_t
#include "doomtype.h"

/// \brief Player input.
///
/// Client input data gathered during a tick and sent
/// to server for processing.

struct ticcmd_t
{
  /// Button-type actions
  enum buttoncode_t
  {
    BT_ATTACK   = 1, ///< Press "Fire".
    BT_USE      = 2, ///< Use button, to open doors, activate switches.
    BT_JUMP     = 4, ///< Jump or fly/swim up.
    BT_FLYDOWN  = 8, ///< Fly/swim down.

    // The weapon mask and shift, for convenience.
    WEAPONMASK  = 0xFFF0,
    WEAPONSHIFT = 4,
  };

  short buttons; ///< buttons and weapon changes
  char  item;    ///< if nonzero, using an inventory item
  char  forward; ///< "push" forward-backward
  char  side;    ///< "push" right-left

  // rotation: << 16 for angle_t
  short yaw;   ///< left-right   
  short pitch; ///< up-down



  ticcmd_t() : buttons(0), item(0), forward(0), side(0), yaw(0), pitch(0) {};

  /// sets the ticcmd into a neutral state
  void Clear();

  /// Fills the ticcmd_t with local input data.
  void Build(class LocalPlayerInfo *info, int elapsedtics);
};

#endif
