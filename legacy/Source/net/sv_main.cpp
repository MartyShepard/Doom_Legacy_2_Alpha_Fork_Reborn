// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: sv_main.cpp 525 2008-01-21 01:07:44Z smite-meister $
//
// Copyright (C) 2004-2007 by DooM Legacy Team.
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
/// \brief Main server code
///
/// Uses OpenTNL


/*
  Masterserver
    -matchmaker between public servers and clients looking for them

  Server (may include local clients)
    -local, no network traffic whatsoever
    -private/LAN, does not announce its existence, just answers to connecting clients
    -public, announces itself to masterserver, keeps sending heartbeat

  Client
    -can look for a server in a specific IP (private), on LAN,
     or ask public server lists from masterserver


legacy -server [-dedicated] [-port XXX]
 startup
 create netint, passwords, set them up
 open console
 read/create levelgraph
 setup first map(s)
 open server, notify master
 before starting game: send heartbeat, don't tick maps
 1: when client joins, send info to all clients, create scope object
 when start condition fulfilled, (lock game), start ticking maps
 when server is closed: make one of the clients new server? / kick clients
 when client disconnects: send info to others, remove pawn/pov, drop relevant items on ground?


legacy -connect IP/LAN [-port XXX]
 startup
 create netint, passwords, set them up
 4: open console/menu, start pinging (several LAN servers? browser like with masterserver)
 2: get server info, connect
 find resources, download if necessary
 send server ping, # of local players, names, teams, colors...
 1: server validates these (all may change)
 setup map, partial spawn
 start ghosting, scopeobj can be either pov (rider, fixed) or the actual pawn (free spectator)
 game starts, spawn the pawn
 when connection is lost: try to renegotiate using the same IP/signatures...
 3: if unable to renegotiate, open menu (destroy old game data)


legacy [no autostart]
 setup, netint, console, menu
 3: multiplayer menu
  client: search LAN 4:, connect masterserver, connect ip
  server: start server, load game, stop server

 singleplayer menu: server with local players, not a netgame



Net tasks: (ghost, rpc, netevent, info packet, raw packet)
 - player info (ghost?, rpc) name color skin autoaim?
 - add/drop player?
 - server info (ghost?, rpc)
 - game object data (ghost)
 - player input (?)
 - hud messages
 - score
 - file transfer (?)


In future, do dll mods like this:
polymorph Map, PlayerInfo? (and Actor descendants, of course)
Have polymorphed class GameType which creates these into GameInfo containers
 */


#include <vector>

#include "doomdef.h"
#include "command.h"
#include "cvars.h"
#include "console.h"

#include "d_event.h"

#include "g_game.h"
#include "g_type.h"
#include "g_level.h"
#include "g_mapinfo.h"
#include "g_map.h"
#include "g_player.h"
#include "g_pawn.h"

#include "n_interface.h"

#include "m_menu.h"

#include "i_system.h"
#include "m_argv.h"
#include "r_data.h"
#include "sounds.h"

#include "w_wad.h"
#include "z_zone.h"


using namespace TNL;


#define DEFAULT_PORT 5029;


//========================================================================
//        Server consvars
//========================================================================

consvar_t cv_playdemospeed  = {"playdemospeed", "0", 0, CV_Unsigned};

#define MS_PORT "28910"
consvar_t cv_masterserver   = {"masterserver", "doomlegacy.dhs.org:"MS_PORT, CV_SAVE, NULL};
consvar_t cv_netstat = {"netstat", "0", 0, CV_OnOff};

// server info
consvar_t cv_publicserver  = {"publicserver", "No", CV_SAVE, CV_YesNo};
consvar_t cv_servername      = {"servername", "DooM Legacy server", CV_SAVE, NULL};
consvar_t cv_allownewplayers = {"allownewplayers", "1", 0, CV_OnOff};
CV_PossibleValue_t maxplayers_cons_t[]={{1, "MIN"},{100, "MAX"},{0, NULL}};
consvar_t cv_maxplayers     = {"maxplayers", "32", CV_NETVAR, maxplayers_cons_t, NULL};

// game rules
CV_PossibleValue_t teamplay_cons_t[]={{0, "Off"},{1, "Color"},{2, "Skin"},{3, NULL}};
CV_PossibleValue_t deathmatch_cons_t[]={{0, "Coop"},{1, "1"},{2, "2"},{3, "3"},{0, NULL}};
CV_PossibleValue_t fraglimit_cons_t[]={{0, "MIN"},{1000, "MAX"},{0, NULL}};
CV_PossibleValue_t exitmode_cons_t[]={{0, "no"},{1, "any"},{2, "personal"},{3, "all"},{4, NULL}};

void TeamPlay_OnChange();
void Deathmatch_OnChange();
void FragLimit_OnChange();
void TimeLimit_OnChange();
void FastMonster_OnChange();
void ExitMode_OnChange();

consvar_t cv_deathmatch = {"deathmatch", "0", CV_NETVAR | CV_NOINIT | CV_CALL, deathmatch_cons_t, Deathmatch_OnChange};
consvar_t cv_teamplay   = {"teamplay"  , "0", CV_NETVAR | CV_CALL, teamplay_cons_t, TeamPlay_OnChange};
consvar_t cv_teamdamage    = {"teamdamage", "0", CV_NETVAR, CV_OnOff};
consvar_t cv_hiddenplayers = {"hiddenplayers", "0", CV_NETVAR, CV_YesNo};
consvar_t cv_exitmode      = {"exitmode", "1", CV_NETVAR | CV_CALL, exitmode_cons_t, ExitMode_OnChange};
consvar_t cv_intermission  = {"intermission", "1", CV_NETVAR, CV_OnOff};
consvar_t cv_fraglimit     = {"fraglimit" , "0", CV_NETVAR | CV_CALL | CV_NOINIT,fraglimit_cons_t, FragLimit_OnChange};
consvar_t cv_timelimit     = {"timelimit" , "0", CV_NETVAR | CV_CALL | CV_NOINIT, CV_Unsigned, TimeLimit_OnChange};

consvar_t cv_jumpspeed       = {"jumpspeed", "6", CV_NETVAR | CV_FLOAT};
consvar_t cv_fallingdamage   = {"fallingdamage", "0", CV_NETVAR, CV_Unsigned};
consvar_t cv_allowrocketjump = {"allowrocketjump", "0", CV_NETVAR, CV_YesNo};
consvar_t cv_allowautoaim    = {"allowautoaim", "1", CV_NETVAR, CV_YesNo};
consvar_t cv_allowmlook      = {"allowfreelook", "1", CV_NETVAR, CV_YesNo};
consvar_t cv_allowpause      = {"allowpause", "1", CV_NETVAR, CV_YesNo};

consvar_t cv_itemrespawn     = {"respawnitem"    , "0", CV_NETVAR, CV_OnOff};
consvar_t cv_itemrespawntime = {"respawnitemtime", "30", CV_NETVAR, CV_Unsigned};
consvar_t cv_respawnmonsters = {"respawnmonsters", "0", CV_NETVAR, CV_OnOff};
consvar_t cv_respawnmonsterstime = {"respawnmonsterstime", "12", CV_NETVAR, CV_Unsigned};
consvar_t cv_fragsweaponfalling  = {"fragsweaponfalling", "0", CV_NETVAR, CV_OnOff};
consvar_t cv_bodyqueue_size  = {"bodyqueue_size", "32", CV_NETVAR, CV_Unsigned};
consvar_t cv_bodyqueue_monsters = {"bodyqueue_monsters", "0", CV_NETVAR, CV_YesNo};

consvar_t cv_gravity = {"gravity", "1", CV_NETVAR | CV_FLOAT | CV_ANNOUNCE};
consvar_t cv_nomonsters = {"nomonsters", "0", CV_NETVAR, CV_OnOff};
consvar_t cv_fastmonsters = {"fastmonsters", "0", CV_NETVAR | CV_CALL, CV_OnOff,FastMonster_OnChange};
consvar_t cv_solidcorpse  = {"solidcorpse", "0", CV_NETVAR, CV_OnOff};
consvar_t cv_voodoodolls  = {"voodoodolls", "1", CV_NETVAR, CV_OnOff};
consvar_t cv_infighting  = {"infighting", "1", CV_NETVAR, CV_OnOff};


void TeamPlay_OnChange()
{
  /*
  int value = cv_teamplay.value;

  // Change the name of the teams
  game.teams.resize(game.maxteams);

  if (cv_teamplay.value == 1)
    {
      // color
      for (i=0; i<game.maxteams; i++)
	{
	  game.teams[i] = new TeamInfo;
	  game.teams[i]->name = Color_Names[i];
	  game.teams[i]->color = i;
        }
    }
  else if (cv_teamplay.value == 2)
    {
      // skins
      for(i=0; i<game.maxteams; i++)
        {
	  game.teams[i] = new TeamInfo;
	  game.teams[i]->name = skins[i].name;
	  game.teams[i]->color = i; // kinda irrelevant
        }
    }
  */
}


void Deathmatch_OnChange()
{
  int value = cv_deathmatch.value;

  // dm 1: original weapons are respawned at start, stay when picked up, yield 2.5*ammo (because ammo does not respawn...)
  // dm 2: items respawn, also original weapons are removed when picked up
  // dm 3: items respawn, original weapons are respawned at start, stay when picked up

  // give all keys to all players
  if (value)
    {
      GameInfo::player_iter_t t = game.Players.begin();
      for ( ; t != game.Players.end(); t++)
	{
	  PlayerInfo *p = t->second;
	  if (p && p->pawn)
	    p->pawn->keycards = it_allkeys;
	}
    }

  if (value >= 2)
    cv_itemrespawn.Set(1);
  else
    cv_itemrespawn.Set(0);

  if (value == 1 || value == 3)
    {
      GameInfo::mapinfo_iter_t t = game.mapinfo.begin();
      for ( ; t != game.mapinfo.end(); t++)
	{
	  MapInfo *p = t->second;
	  if (p && p->me)
	    p->me->RespawnWeapons();
	}
    }
}


void FragLimit_OnChange()
{
  if (cv_fraglimit.value > 0)
    {
      cv_exitmode.Set(0); // no exit
    }
}


void TimeLimit_OnChange()
{
  if (cv_timelimit.value)
    CONS_Printf("Levels will end after %d minute(s).\n", cv_timelimit.value);
  else
    CONS_Printf("Time limit disabled\n");
}


void ExitMode_OnChange()
{
  if (cv_exitmode.value == 3)
    cv_intermission.Set(0);
  else
    cv_intermission.Set(1);
}



//========================================================================
//    Main server code
//========================================================================


void GameInfo::TryRunTics(tic_t elapsed)
{
  extern  bool singletics;

  // max time step
  if (elapsed > TICRATE/7)
    {
      if (server)
	elapsed = 1;
      else
	elapsed = TICRATE/7;
    }
  
  if (singletics)
    elapsed = 1;

  if (elapsed >= 1)
    COM.BufExecute(); // process command buffer

  I_GetEvent();


  if (!dedicated)
    {
      Menu::Ticker();
      con.Ticker();

      // translate inputs (keyboard/mouse/joystick) into a ticcmd
      for (int i=0; i < NUM_LOCALPLAYERS; i++)
	LocalPlayers[i].GetInput(elapsed);
    }

  D_ProcessEvents(); // read control events, feed them to responders

  // get packets? (maybe connection is lost...)

  // queue ticcmds to server_con equipped with delta_t's (elapsed)
  // try resending them a few times until we get confirmation of arrival from server

  // run client simulation/server simulation

  net->Update();

  while (elapsed-- > 0)
    Ticker();
}




/// is there a game running?
bool GameInfo::Playing()
{
  return (state > GS_INTRO);
  //return (net->netstate == LNetInterface::CL_Connected);
}


/// Close net connections, delete players, clear game status.
/// After this you are not Playing().
void GameInfo::SV_Reset(bool clear_mapinfo)
{
  net->SV_Reset(); // disconnect clients etc.

  ClearPlayers();

  if (clear_mapinfo)
    Clear_mapinfo_clustermap();

  // clear teams?
  SV_ResetScripting();

  // reset gamestate?
  SetState(GS_NULL);
  server = true;
  netgame = false;
  multiplayer = cv_splitscreen.value;

  CONS_Printf("\n--- Server reset ---\n");
}


/// Resets and initializes the server.
/// Sets up the game if given a valid MAPINFO lump.
/// If given a negative lump number, does not read any MAPINFO (for loading games).
bool GameInfo::SV_SpawnServer(bool force_mapinfo)
{
  if (Playing())
    {
      I_Error("Tried to spawn a server while playing.\n");
      return false;
    }

  CONS_Printf("Starting a server.\n");

  if (force_mapinfo || mapinfo.empty())
    {
      SV_Reset(true);
      if (Read_MAPINFO() <= 0)
	{
	  CONS_Printf(" Bad MAPINFO lump.\n");
	  return false;
	}
    }
  else
    SV_Reset(false); // keep current MAPINFO data

  ReadResourceLumps(); // SNDINFO etc.
  return true;
}


void P_InitSwitchList();

/// reads some lumps affecting the game behavior
void GameInfo::ReadResourceLumps()
{
  // read these lumps _after_ MAPINFO but not separately for each map
  extern bool nosound;
  if (!nosound)
    {
      S_ClearSounds();
      int n = fc.Size();
      for (int i = 0; i < n; i++)
	{
	  // cumulative reading
	  S_Read_SNDINFO(fc.FindNumForNameFile("SNDINFO", i));
	  S_Read_SNDSEQ(fc.FindNumForNameFile("SNDSEQ", i));
	}

      if (cv_precachesound.value)
	S_PrecacheSounds();
    }

  // set switch texture names/numbers, read "SWITCHES" lump
  P_InitSwitchList();
}


void GameInfo::SV_SetServerState(bool open)
{
  if (open)
    {
      net->SV_Open(false);
      netgame = multiplayer = true;
    }
  // TODO else net->SV_Close()
}



/// starts or restarts the game. assumes that we have set up the clustermap.
bool GameInfo::SV_StartGame(skill_t sk, int mapnumber, int ep)
{
  if (clustermap.empty() || mapinfo.empty())
    return false;

  CONS_Printf("Starting a game\n");

  // close and reset open maps, if any, release and reset players
  if (currentcluster)
    currentcluster->Finish(0);

  for (player_iter_t i = Players.begin(); i != Players.end(); i++)
    i->second->Reset(true, true);
  // TODO need we nuke pawns too?

  initial_map = FindMapInfo(mapnumber);
  initial_ep = ep;
  if (!initial_map)
    initial_map = episodes[0]->minfo;

  if (initial_map)
    currentcluster = FindCluster(initial_map->cluster);
  else
    currentcluster = NULL;

  skill = sk;

  // set cvars
  if (skill == sk_nightmare)
    {
      cv_respawnmonsters.Set(1);
      cv_fastmonsters.Set(1);
    }
  else
    {
      cv_respawnmonsters.Set(0);
      cv_fastmonsters.Set(0);
    }

  cv_deathmatch.Set(0);
  cv_timelimit.Set(0);
  cv_fraglimit.Set(0);

  SV_ResetScripting();

  if (!dedicated)
    CL_StartGame();

  paused = false;
  SetState(GS_LEVEL);

  return true;
}




//=========================================================================
//                           SERVER INIT
//=========================================================================


/// Sets up network interface
void InitNetwork()
{
  CONS_Printf("Initializing the network interface.\n");

  bool ipx = M_CheckParm("-ipx");
  TransportProtocol ptc = ipx ? IPXProtocol : IPProtocol;

  S32 port = DEFAULT_PORT;
  if (M_CheckParm ("-port"))
    port = atoi(M_GetNextParm());

  // set up the interface
  LNetInterface *n = new LNetInterface(Address(ptc, Address::Any, port));
  game.net = n;

  n->ping_address = Address(ptc, Address::Broadcast, port);

  // misc. commandline parameters
  n->nodownload = M_CheckParm("-nodownload");

  // parse network game options,
  if (M_CheckParm("-server") || game.dedicated)
    {
      // TODO give a number of clients to wait for before starting game?
      // if (M_IsNextParm())
      //   doomcom->numnodes = atoi(M_GetNextParm());
    }
  else if (M_CheckParm("-connect"))
    {
      char server_hostname[255];

      if (M_IsNextParm())
	strcpy(server_hostname, M_GetNextParm());
      else
	server_hostname[0] = '\0'; // LAN server, use broadcast to detect it

      // server address only in ip
      if (server_hostname[0] && !ipx)
        {
	  COM.AppendText("connect \"");
	  COM.AppendText(server_hostname);
	  COM.AppendText("\"\n");
        }
      else
        {
	  // so we're on a LAN
	  COM.AppendText("connect any\n");
        }
    }
}


// ------
// protos
// ------

void Command_Listserv_f() {}

void Command_Clear_f();
void Command_Bind_f();

void Command_SaveConfig_f();
void Command_LoadConfig_f();
void Command_ChangeConfig_f();

void Command_Version_f();
void Command_GameInfo_f();
void Command_MapInfo_f();
void Command_Players_f();
void Command_Frags_f();
void Command_TeamFrags_f();

void Command_Pause_f();
void Command_Quit_f ();
void Command_Connect_f();
void Command_Reset_f();

void Command_Kick_f();
void Command_Kill_f();
void Command_Addfile_f();

void Command_NewGame_f();
void Command_StartGame_f();
void Command_Map_f();

void Command_Save_f();
void Command_Load_f();
void Command_Playdemo_f();
void Command_Timedemo_f();
void Command_Stopdemo_f();

void Command_Say_f();
void Command_Sayto_f();
void Command_Sayteam_f();
void Command_Chatmacro_f();

void Command_CheatNoClip_f();
void Command_CheatGod_f();
void Command_CheatGimme_f();

void Command_Meminfo_f();

void Command_RunACS_f();

void COM_FS_DumpScript_f();
void COM_FS_RunScript_f();
void COM_FS_Running_f();
void FS_Init();

void Command_AddBot_f();

void Command_ConvertMap_f();

// set chatmacros cvars point the original or dehacked texts, before config.cfg is executed !!
void HU_HackChatmacros();


/// This is needed by both servers and clients
void SV_Init()
{
  CONS_Printf("\n============ SV_Init ============\n");

  // register commands

  // Ideas for new commands from Quake2
  //COM_AddCommand("status", Command_Status_f);
  //COM_AddCommand("heartbeat", SV_Heartbeat_f);
  //COM_AddCommand("killserver", SV_KillServer_f);

  /* ideas from Quake
    "notarget"
    "fly"
    "reconnect"
    "tell"
    "spawn"
    "begin"
    "prespawn"
    "ping"
    "demos"
  */

  // console
  COM.AddCommand("cls", Command_Clear_f);
  COM.AddCommand("bind", Command_Bind_f);

  // config file management  
  COM.AddCommand("saveconfig", Command_SaveConfig_f);
  COM.AddCommand("loadconfig", Command_LoadConfig_f);
  COM.AddCommand("changeconfig", Command_ChangeConfig_f);

  // informational commands
  COM.AddCommand("version", Command_Version_f);
  COM.AddCommand("meminfo", Command_Meminfo_f);
  COM.AddCommand("gameinfo", Command_GameInfo_f);
  COM.AddCommand("mapinfo", Command_MapInfo_f);
  COM.AddCommand("players", Command_Players_f);
  COM.AddCommand("frags", Command_Frags_f);

  // chat commands
  COM.AddCommand("say"    , Command_Say_f);
  COM.AddCommand("sayto"  , Command_Sayto_f);
  COM.AddCommand("sayteam", Command_Sayteam_f);
  COM.AddCommand("chatmacro", Command_Chatmacro_f);

  // basic commands for controlling the game
  COM.AddCommand("pause", Command_Pause_f);
  COM.AddCommand("quit",  Command_Quit_f);
  COM.AddCommand("connect", Command_Connect_f);
  COM.AddCommand("reset", Command_Reset_f);

  // game management (server only)
  COM.AddCommand("save", Command_Save_f);
  COM.AddCommand("load", Command_Load_f);
  COM.AddCommand("playdemo", Command_Playdemo_f);
  COM.AddCommand("stopdemo", Command_Stopdemo_f);
  COM.AddCommand("addfile", Command_Addfile_f);
  COM.AddCommand("kick", Command_Kick_f);
  COM.AddCommand("kill", Command_Kill_f);

  COM.AddCommand("newgame", Command_NewGame_f);
  COM.AddCommand("startgame", Command_StartGame_f);
  COM.AddCommand("map", Command_Map_f);

  COM.AddCommand("runacs", Command_RunACS_f);
  FS_Init();
  COM.AddCommand("fs_dumpscript", COM_FS_DumpScript_f);
  COM.AddCommand("fs_runscript",  COM_FS_RunScript_f);
  COM.AddCommand("fs_running",    COM_FS_Running_f);

  // bots
  COM.AddCommand("addbot", Command_AddBot_f);

  // cheat commands, I'm bored of deh patches renaming the idclev ! :-)
  COM.AddCommand("noclip", Command_CheatNoClip_f);
  COM.AddCommand("god", Command_CheatGod_f);
  COM.AddCommand("gimme", Command_CheatGimme_f);

  //Added by Hurdler for master server connection
  cv_masterserver.Reg();
  COM.AddCommand("listserv", Command_Listserv_f);

  // misc
  COM.AddCommand("convertmap", Command_ConvertMap_f);

  // register console variables
  cv_publicserver.Reg();
  cv_servername.Reg();
  cv_allownewplayers.Reg();
  cv_maxplayers.Reg();

  cv_deathmatch.Reg();
  cv_teamplay.Reg();
  cv_teamdamage.Reg();
  cv_hiddenplayers.Reg();
  cv_intermission.Reg();
  cv_exitmode.Reg();
  cv_fraglimit.Reg();
  cv_timelimit.Reg();

  cv_jumpspeed.Reg();
  cv_fallingdamage.Reg();
  cv_allowrocketjump.Reg();
  cv_allowautoaim.Reg();
  cv_allowmlook.Reg();
  cv_allowpause.Reg();

  cv_itemrespawn.Reg();
  cv_itemrespawntime.Reg();
  cv_respawnmonsters.Reg();
  cv_respawnmonsterstime.Reg();
  cv_fragsweaponfalling.Reg();
  cv_bodyqueue_size.Reg();
  cv_bodyqueue_monsters.Reg();

  cv_gravity.Reg();
  cv_nomonsters.Reg();
  cv_fastmonsters.Reg();
  cv_solidcorpse.Reg();
  cv_voodoodolls.Reg();
  cv_infighting.Reg();

  cv_playdemospeed.Reg();
  cv_netstat.Reg();

  // add chat macro consvars
  HU_HackChatmacros();

  R_ServerInit(); // even server needs to know the texture heights etc.
  InitNetwork();
}
