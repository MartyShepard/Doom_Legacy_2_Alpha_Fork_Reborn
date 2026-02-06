// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: g_level.cpp 353 2006-07-27 15:06:52Z smite-meister $
//
// Copyright (C) 2002-2006 by Doom Legacy Team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Implementation of MapCluster class

#include "g_game.h"
#include "g_level.h"
#include "g_mapinfo.h"
#include "g_map.h"
#include "dstrings.h"
#include "sounds.h"

#include "z_zone.h"



// default cluster constructor
MapCluster::MapCluster()
{
  number = 0;
  keepstuff = hub = false;
  episode = 0;

  time = partime = 0;
};

// cluster constructor
MapCluster::MapCluster(int n)
{
  number = n;
  keepstuff = hub = false;
  episode = 0;

  time = partime = 0;
}


// ticks the entire cluster forward in time
void MapCluster::Ticker()
{
  int i, n = maps.size();
  for (i=0; i<n; i++)
    maps[i]->Ticker();
}


// called before moving on to a new cluster
void MapCluster::Finish(int nextmap, int ep)
{
  CONS_Printf("Cluster %d finished!\n", number);
  int n = maps.size();
  for (int i=0; i<n; i++)
    maps[i]->Close(nextmap, ep, true);

  // Z_FreeTags(PU_LEVEL, PU_PURGELEVEL-1); // destroys pawns if they are not Detached
  // P_Initsecnode();  // re-initialize sector node list (the old nodes were just freed)
}
