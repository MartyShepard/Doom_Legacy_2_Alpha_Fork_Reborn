// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2007 by DooM Legacy Team.
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
/// \brief Map environmental effects.

#ifndef p_effects_h
#define p_effects_h 1

#include <vector>

#include "doomdef.h"
#include "r_defs.h"

using namespace std;


/// \brief Class for Map environmental effects.
/*!
  Currently just the Hexen lightning effect.
*/
class MapEffect
{
  class Map *mp; ///< parent Map

  int flash_duration; ///< remaining tics for the current flash
  int flash_delay;    ///< tics before next flash
  struct sectorflash_t
  {
    sector_t *sec;
    short orig_light;
  };
  vector<sectorflash_t> flash_sectors; ///< sectors affected by the flashes, with additional info

 public:
  MapEffect(Map *m);

  bool Force();
  void LightningFlash();  
};


#endif
