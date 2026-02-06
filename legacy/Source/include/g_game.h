// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: g_game.h 508 2007-12-18 21:00:41Z jussip $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
/// \brief GameInfo class definition

#ifndef g_game_h
#define g_game_h 1

#include <vector>
#include <map>
#include <string>

#include "doomtype.h"

class GameType;
class LNetInterface;

using namespace std;



/// Game mode. For game-specific rules, IWAD dependent animations etc.
enum gamemode_t
{
  gm_none,
  gm_doom1,   // DOOM 1 shareware (E1M9), registered (E3M27), retail (Ultimate DOOM) (E4M36)
  gm_doom2,   // DOOM 2 retail (commercial), E1 M34
  gm_heretic,
  gm_hexen
};

// "Mission packs". Used only during startup.
enum gamemission_t
{
//gmi_none = -1,  
  gmi_prealpha,
  gmi_alpha,
  gmi_beta,
  gmi_presdemo,  
  gmi_shareware,  // DOOM 1 shareware (E1M9)
  gmi_doom1,      // registered (E3M27)
  gmi_ultimate,   // retail (Ultimate DOOM) (E4M36)
  gmi_doom2,      // DOOM 2, default
  gmi_tnt,        // TNT Evilution mission pack
  gmi_plut,       // Plutonia Experiment pack
  gmi_heretic,    // Heretic
  gmi_hexen,      // Hexen
  gmi_hacx,       // HacX
  gmi_strife,
  gmi_fdoomu,
  gmi_fdoom2  
};

/// skill levels
enum skill_t
{
  sk_baby,
  sk_easy,
  sk_medium,
  sk_hard,
  sk_nightmare
};

/// \brief Game info common to all players.
/// \ingroup g_central
/*!
  There is only one instance in existence, called 'game'.
  It stores all relevant data concerning one game,
  including game state, flags, players, teams, maps etc.
*/
class GameInfo
{
  friend class Intermission;
  friend class PlayerInfo;
  friend class LNetInterface;
  friend class LConnection;
  friend class GameType;

public:
  /// current state of the game
  enum gamestate_t
    {
      GS_NULL = 0,      ///< only used during game startup
      GS_INTRO,         ///< no game running, playing intro loop
      GS_WAIT,
      GS_LEVEL,           ///< we are playing
      GS_INTERMISSION,    ///< gazing at the intermission screen
      GS_FINALE,          ///< game final animation
      GS_DEMOPLAYBACK     ///< watching a demo
    };

  gamestate_t   state;  ///< gamestate

  // demoversion is the 'dynamic' version number, this should be == game VERSION.
  // When playing back demos, 'demoversion' receives the version number of the
  // demo. At each change to the game play, demoversion is compared to
  // the game version, if it's older, the changes are not done, and the older
  // code is used for compatibility.
  unsigned demoversion;

  gamemode_t    mode;   ///< Which game? Doom? Heretic? Hexen?
  skill_t       skill;  ///< skill level

  bool server;      ///< are we the game authority?
  bool dedicated;   ///< if so, do we allow local players?

  bool netgame;     ///< only true in a netgame (nonlocal players possible)
  bool multiplayer; ///< only true if >1 players. netgame => multiplayer but not (multiplayer => netgame)
  bool modified;    ///< an external modification-dll is in use
  bool paused;      ///< is the game currently paused?

  bool inventory;   ///< PlayerPawns have an inventory

  // Intro sequences
  int   pagetic;    ///< how many tics left until demo is changed?
  int   demosequence;
  const char *pagename;

  int  screenwipe; ///< screen wipe progress: 0: inactive, 1: needed, 2: ongoing
  bool refresh_viewborder;

  unsigned time;  ///< how long (in ms) has the game been running?
  unsigned tic;   ///< how many times has the game been ticked?

public:
  int maxplayers; ///< max # of players allowed
  int maxteams;   ///< max # of teams

  typedef map<int, class PlayerInfo*>::iterator player_iter_t;
  map<int, PlayerInfo*> Players;     ///< mapping from player number to PlayerInfo

  vector<class TeamInfo*> teams;     ///< the teams in the game

  /// \name MAPINFO data
  //@{
  typedef map<int, class MapInfo*>::iterator mapinfo_iter_t;
  typedef map<int, class MapCluster*>::iterator cluster_iter_t;

  string              mapinfo_lump;  ///< MAPINFO lump used
  map<int, MapInfo*>       mapinfo;  ///< all the maps of the current game
  map<int, MapCluster*> clustermap;  ///< map clusters or hubs of the current game
  vector<class Episode*>  episodes;  ///< game entrypoints
  //@}

  MapInfo       *initial_map;  ///< New players will go into this Map,
  int             initial_ep;  ///< entering through this entrypoint.
  MapCluster *currentcluster;  ///< currently active MapCluster (contains active Maps)

  GameType     *gtype; ///< TEST
  LNetInterface  *net; ///< our network interface (netstate and connections)

public:

  // in g_game.cpp
  GameInfo();
  ~GameInfo();

  void SetState(gamestate_t s);
  void StartIntro();
  void AdvanceIntro();

  void Display();
  void Drawer();
  bool Responder(struct event_t *ev);

  /// returns the player if he is in the game, otherwise NULL
  PlayerInfo *FindPlayer(int number);
  PlayerInfo *FindPlayer(const char *name);

  PlayerInfo *AddPlayer(PlayerInfo *p); ///< tries to add a player to the game
  bool RemovePlayer(int number);        ///< erases player from game
  void ClearPlayers();                  ///< erases all players

  void Pause(bool on); ///< pauses or unpauses the game

  // in sv_main.cpp
public:
  bool Playing();
  void SV_Reset(bool clear_mapinfo = true);
  bool SV_SpawnServer(bool force_mapinfo);
  void SV_SetServerState(bool open);
  bool SV_StartGame(skill_t skill, int mapnumber, int ep = 0);
  void TryRunTics(tic_t realtics);
private:
  void ReadResourceLumps();
  void SV_ResetScripting();


  // in cl_main.cpp
public:
  void CL_Reset();
  bool CL_SpawnClient();
  bool CL_StartGame();


  int  Serialize(class LArchive &a);
  int  Unserialize(LArchive &a);
  void LoadGame(int slot);
  void SaveGame(int slot, const char* description);

  int  GetFrags(struct fragsort_t **fs, int type);
  bool CheckScoreLimit();

  // in g_mapinfo.cpp
  int  Read_MAPINFO();
  void Clear_mapinfo_clustermap();
  MapCluster *FindCluster(int number);
  MapInfo *FindMapInfo(int number);
  MapInfo *FindMapInfo(const char *name);
  MapInfo *FindNextMap(int mapnum, int incr = 1);
  const Episode *GetEpisode(int n);

  // in g_state.cpp
  void Ticker(); ///< ticks the game forward in time
  void StartIntermission();
  void EndIntermission();
  void StartFinale(MapCluster *next);
  void EndFinale();

  // in g_demo.cpp
  bool Downgrade(int version);
  void BeginRecording();
  void PlayDemo(char *defdemoname);
  void ReadDemoTiccmd(struct ticcmd_t* cmd, int playernum);
  void WriteDemoTiccmd(ticcmd_t* cmd, int playernum);
  void StopDemo();
  bool CheckDemoStatus();
};


extern GameInfo game;

#endif
