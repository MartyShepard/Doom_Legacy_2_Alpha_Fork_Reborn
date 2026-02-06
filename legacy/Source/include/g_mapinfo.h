// Emacs style mode select -*- C++ -*-
//---------------------------------------------------------------------------
//
// $Id: g_mapinfo.h 505 2007-12-18 02:57:12Z smite-meister $
//
// Copyright (C) 2002-2007 by Doom Legacy Team
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
//--------------------------------------------------------------------------

/// \file
/// \brief MapInfo class definition

#ifndef g_mapinfo_h
#define g_mapinfo_h 1

#include <string>
#include <vector>
#include <map>

#include "doomtype.h"

using namespace std;


/// \brief A deferred or "queued" ACS script in a currently inactive Map.
struct acs_deferred_t
{
  unsigned script; ///< Script number (on target Map)
  byte    args[4]; ///< 3 byte arguments, padded to 4 for alignment
};


/// \brief Stores all kinds of metadata for an associated Map.
/// \ingroup g_central
/*!
  OK. Doom map format is (undestandably) primitive and has no place for
  general information about maps. There are at least two ways to fix this:
 
  1)  MAPINFO lump (from Hexen and ZDoom)
      There can be one MAPINFO lump per wad. It is a text lump that consists of
      two kinds of blocks: MAP blocks and CLUSTERDEF blocks.
 
  2)  LevelInfo lumps (Legacy, others?)
      There is one LevelInfo lump per Map. It is the same lump as the
      mapname "separator", i.e. MAP16, E6M2, MYOWNMAP... 
      LevelInfo only contains info about the one map to which it belongs.
      It is a text lump that consists of blocks, such as [script], ...
 
  Internally, all the data for each map is combined into one class, MapInfo.
  It contains all the information regarding a Map that doesn't directly affect
  gameplay but can be of interest to human players, such as the Map
  name, author, recommendation for # of players, hints, music that is
  played during the map, partime etc.
*/
class MapInfo
{
  friend class Map;

public:
  /// \name Status variables for the associated Map
  /// 
  //@{
  /// current state of the corresponding Map
  enum mapstate_e
    {
      MAP_UNLOADED = 0, ///< not loaded, "me" should be NULL
      MAP_RUNNING,      ///< currently running, "me" is valid
      MAP_INSTASIS,     ///< presently halted, but "me" is still valid
      MAP_RESET,        ///< (single player) player has died, the map should be reset
      MAP_FINISHED,     ///< Map will be saved or closed after the tick, but currently "me" is still valid
      MAP_SAVED         ///< "me" should be NULL, the Map is saved on disk
    };

  mapstate_e   state;
  class Map   *me;    ///< the actual running Map instance corresponding to this MapInfo
  bool         found; ///< present in the resource pool (WAD files etc.)

  vector<acs_deferred_t> ACS_deferred; ///< Deferred ACS scripts for this map. Will be started when the Map is launched.
  //@}
  
  string lumpname;   ///< map lump name ("MAP04")
  string nicename;   ///< map long nice name ("The Nuclear Plant")
  string savename;   ///< name of the file the map is currently saved in

  int    cluster;    ///< in which cluster does this map belong?
  int    mapnumber;  ///< real map number, used with Teleport_NewMap
  bool   hub;        ///< same as cluster.hub

  string version;     ///< map version string
  string author;      ///< map creator
  string description; ///< optional map description

  string namepic;     ///< texture containing map's nice name
  // string previewpic; // preview picture name

  int   scripts; ///< how many FS scripts in the map?
  int   partime; ///< in seconds
  float gravity; ///< temporary, could be part of ZoneInfo or sector_t?

  // less essential stuff (can be replaced with scripting or other more efficient means)
  string musiclump;

  int    warptrans;    ///< a Hexen quirk, another map numbering for deathmatch
  int    warpnext;     ///< this one uses warptrans numbers
  int    nextlevel;    ///< next map number (normal Doom exit)
  int    secretlevel;  ///< next map number (Doom secret exit)

  bool   doublesky;
  bool   lightning;
  string sky1;
  float  sky1sp;
  string sky2;
  float  sky2sp;

  int    cdtrack;
  string fadetablelump;

  int BossDeathKey;  ///< bit flags to see which bosses end the map when killed.

  // intermission data
  string interpic;   ///< intermission background texture name
  string intermusic; ///< intermission music lumpname

public:
  MapInfo();
  ~MapInfo();

  /// Ticks the map forward in time.
  void Ticker();

  /// Inserts a player into the map and activates it if necessary.
  bool Activate(class PlayerInfo *p);

  /// Throws out all players in the map.
  int  EvictPlayers(int next, int ep = 0, bool force = false);

  /// Shuts down the map, throws out the players, deletes the hubsave.
  void Close(int next, int ep = 0, bool force = false);

  /// Saves the map into a hubsave file
  bool HubSave();

  /// Loads a hubsave
  bool HubLoad();

  /// Reads data from a LevelInfo lump.
  char *Read(int lumpnum);

  int Serialize(class LArchive &a);
  int Unserialize(LArchive &a);
};


#endif
