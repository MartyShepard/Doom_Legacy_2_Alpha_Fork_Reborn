// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_tick.cpp 433 2007-02-07 19:06:40Z smite-meister $
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
/// \brief Part of Map class implementation. Thinkers, Map::Ticker().

#include "g_mapinfo.h"
#include "g_map.h"
#include "g_game.h"
#include "g_player.h"
#include "g_pawn.h"
#include "z_zone.h"

#include "r_defs.h"


// Resets the Thinker list
void Map::InitThinkers()
{
  thinkercap.prev = thinkercap.next = &thinkercap;
}


// Adds a new Thinker at the end of the list.
void Map::AddThinker(Thinker *t)
{
  if (!t)
    return;
  t->mp = this;
  thinkercap.prev->next = t;
  t->next = &thinkercap;
  t->prev = thinkercap.prev;
  thinkercap.prev = t;
}


// Removes a Thinker from the Map without deleting it
void Map::DetachThinker(Thinker *t)
{
  t->mp = NULL;
  t->next->prev = t->prev;
  t->prev->next = t->next;
  t->prev = t->next = NULL;

  force_pointercheck = true;
}


// Moves a Thinker to the removal list
void Map::RemoveThinker(Thinker *t)
{
  t->next->prev = t->prev;
  t->prev->next = t->next;

  // Most Thinkers could be deleted right away (it is assumed that there will be
  // no dangling pointers, or that they have already been taken care of.)
  // This does not work for Actors. Hence...
  DeletionList.push_back(t);
  // TODO perhaps this is needed just for Actors, and other Thinkers can be deleted right away?
}


void Map::RunThinkers()
{
  Thinker *t, *next; 

  for (t = thinkercap.next; t != &thinkercap; t = next)
    {
      next = t->next; // if t is removed while it thinks, its next pointer will no longer be valid.
      //if (t->mp == NULL) I_Error("Thinker::mp == NULL! Cannot be!\n");
      t->Think();
    }
}


void Map::PointerCleanup()
{
  int n = DeletionList.size();
  if (n == 0 && !force_pointercheck)
    return;

  for (Thinker *t = thinkercap.next; t != &thinkercap; t = t->next)
    t->CheckPointers();

  // FIXME unfortunate HACK (the entire sound alert system is unrealistic!)
  for (int i=0 ; i<numsectors ; i++)
    if (sectors[i].soundtarget && (sectors[i].soundtarget->eflags & MFE_REMOVE))
      sectors[i].soundtarget = NULL;

  force_pointercheck = false;

  for (int i=0; i<n; i++)
    delete DeletionList[i];

  DeletionList.clear();
}


// Ticks the map forward in time
void Map::Ticker()
{
  //CONS_Printf("Tic begins..");
  int i = 0;

  if (game.server)
    {
      RunThinkers();

      // after a player is respawned, its input should be built before it is used in RunThinkers.
      if (!respawnqueue.empty())
	i = RespawnPlayers();

      //CONS_Printf("think..");
      //RunThinkers();

      //CONS_Printf("specials..");
      UpdateSpecials();

      //CONS_Printf("respawnspecials..");
      RespawnSpecials();

      //CONS_Printf("sound sequences..");
      UpdateSoundSequences();

      //CONS_Printf("FS..");
      FS_DelayedScripts();

      HandlePlayers();
    }
  else
    {
      // client tick 
      if (players.size() == 0)
	{
	  info->state = MapInfo::MAP_FINISHED;
	  //info->state = MapInfo::MAP_INSTASIS; // TODO alternative
	  return;
	}

      for (Thinker *t = thinkercap.next; t != &thinkercap; t = t->next)
	t->ClientThink();
    }

  PointerCleanup(); // this must be done AFTER players have left the Map, BEFORE they enter another

  // for par times etc.
  maptic++;

  //CONS_Printf("tick done\n");
}
