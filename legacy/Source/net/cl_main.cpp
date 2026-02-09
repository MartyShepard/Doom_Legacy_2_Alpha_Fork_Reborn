// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: cl_main.cpp 570 2009-11-27 23:57:47Z smite-meister $
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
/// \brief Main client code
///
/// Uses OpenTNL


#include "doomdef.h"
#include "command.h"
#include "cvars.h"

#include "n_interface.h"

#include "g_game.h"
#include "g_player.h"
#include "g_pawn.h"
#include "g_input.h"

#include "p_camera.h"
#include "screen.h"
#include "console.h"
#include "console_log.h" // für CONS_Printf
#include "hud.h"
#include "m_menu.h"
#include "am_map.h"
#include "s_sound.h"
#include "sounds.h"
#include "r_main.h"
#include "m_misc.h"
#include "w_wad.h"
#include "v_video.h"

#include "i_system.h"
#include "i_video.h"



//=========================================================================
//        Client consvars
//=========================================================================

void SplitScreen_OnChange()
{
  // recompute screen size
  R_ExecuteSetViewSize();

  // change the menu
  M_SwitchSplitscreen();

  if (game.state < GameInfo::GS_LEVEL)
    return;

  int n = 1 + cv_splitscreen.value; // this many viewports
  int old = ViewPlayers.size();

  ViewPlayers.resize(n);
  for (int i=old; i < n; i++)
    {
      if (i >= NUM_LOCALPLAYERS)
	i = 0;
      PlayerInfo *p = LocalPlayers[i].info;
      ViewPlayers[i] = p ? p : LocalPlayers[0].info;
    }

  // TODO add/remove local players
  // TODO in a demo, just open another viewport...
}

static CV_PossibleValue_t CV_Splitscreen[] = {{0,"MIN"}, {3,"MAX"}, {0,NULL}};
consvar_t cv_splitscreen = {"splitscreen", "0", CV_CALL, CV_Splitscreen, SplitScreen_OnChange};



//=========================================================================
//        Client commands
//=========================================================================

void Command_Setcontrol_f();
void Command_BindJoyaxis_f();
void Command_UnbindJoyaxis_f();


/// Used for setting player properties.
void Command_Player_f()
{
  if (COM.Argc() != 4)
    {
      CONS_Printf("player <number> <attribute> <value>\n");
      return;
    }

  int n = atoi(COM.Argv(1));
  if (n >= NUM_LOCALHUMANS)
    {
      CONS_Printf("Only %d local players supported.\n", NUM_LOCALHUMANS);
      return;
    }
  LocalPlayerInfo *p = &LocalPlayers[n];

  const char *attr = COM.Argv(2);
  const char *val  = COM.Argv(3);
  int ival = atoi(val);

  if (!strcasecmp(attr, "name"))
    p->name = val;
  else if (!strcasecmp(attr, "color"))
    p->color = ival;
  else if (!strcasecmp(attr, "autoaim"))
    p->autoaim = ival;
  else if (!strcasecmp(attr, "messages"))
    p->messagefilter = ival;
  else if (!strcasecmp(attr, "autorun"))
    p->autorun = ival;
  else if (!strcasecmp(attr, "crosshair"))
    p->crosshair = ival;
  else if (!strcasecmp(attr, "chasecam"))
    {
      PlayerInfo *s = p->info; // FIXME... delayed attribute
      if (!ival)
	{
	  // pov back to first person
	  s->pov = s->pawn;
	  // Fixed cams are never deleted (they're part of the Map), chasecam deletes itself.
	}
      else if (s->pov == s->pawn)
	{
	  // pov to chasecam
	  s->pov = new Camera(s->pawn, s->pawn);
	}
    }
  else
    CONS_Printf("Unknown player attribute '%s'.\n", attr);
}


//=========================================================================
//                           CLIENT INIT
//=========================================================================

void CL_Init()
{
  // contains only stuff that a _dedicated_ server does _not_ need
  CONS_Printf("\n============ CL_Init ============\n");

  // set the video mode, graphics scaling properties, load palette
  vid.Startup();

  // init renderer
  R_Init();

  font_t::Init();

  // we need the HUD font for the console
  // HUD font, crosshairs, say commands
  hud.Startup();

  // startup console
  con.Init(); //-------------------------------------- CONSOLE is on

  // setup menu
  Menu::Startup();

  automap.Startup();

  // set up sound and music
  S.Startup();

  // read the basic legacy.wad sound script lumps
  S_Read_SNDINFO(fc.FindNumForNameFile("SNDINFO", 0));
  S_Read_SNDSEQ(fc.FindNumForNameFile("SNDSEQ", 0));

  COM.AddCommand("player", Command_Player_f);
  COM.AddCommand("setcontrol", Command_Setcontrol_f);
  COM.AddCommand("bindjoyaxis", Command_BindJoyaxis_f);
  COM.AddCommand("unbindjoyaxis", Command_UnbindJoyaxis_f);

  COM.AddCommand("screenshot",M_ScreenShot);

  // client info
  char *temp = I_GetUserName();
  if (temp)
    LocalPlayers[0].name = temp;

  // chasecam properties
  cv_cam_dist.Reg();
  cv_cam_height.Reg();
  cv_cam_speed.Reg();

  // client input
  cv_grabinput.Reg();
  cv_controlperkey.Reg();

  // WARNING : the order is important when initing mouse2, we need the mouse2port
  cv_mouse2port.Reg();
#ifdef LMOUSE2
  cv_mouse2opt.Reg();
#endif

  // we currently support two mice
  for (int i=0; i<2; i++)
    {
      cv_usemouse[i].Reg();
      cv_mousesensx[i].Reg();
      cv_mousesensy[i].Reg();
      cv_automlook[i].Reg();
      cv_mousemove[i].Reg();
      cv_invertmouse[i].Reg();
    }

  cv_mouse_release.Reg();
  // client renderer
  cv_viewheight.Reg();

  cv_scr_width.Reg();
  cv_scr_height.Reg();
  cv_scr_depth.Reg();
  cv_fullscreen.Reg();
  cv_borderless.Reg(); // Marty  
  cv_video_gamma.Reg();
  cv_fps_limit_sr.Reg(); //Marty

  cv_viewsize.Reg();
  cv_scalestatusbar.Reg();
  cv_fov.Reg();
  cv_splitscreen.Reg();

  cv_screenslink.Reg();
  cv_translucency.Reg();
  cv_fuzzymode.Reg();
  cv_splats.Reg();
  cv_bloodtime.Reg();
  cv_psprites.Reg();


  /// Register OpenGL-specific consvars and commands.
  extern void OGL_AddCommands();
  if (rendermode != render_soft)
    OGL_AddCommands();

  CONS_Printf("\n");
}



// =========================================================================
//                            CLIENT STUFF
// =========================================================================

//  name, color, or skin has changed
//
void SendNameAndColor()
{
  // game.net->server_con->c2sSendClientInfo(number, name, color, skin)
  // better yet, clientinfo struct...
  /*
  struct playerinfo_t
  {
    name, color, skin, autoaim, weaponpref (what if ammo runs out on current weapon...)
  }
  */
}

// splitscreen
void SendNameAndColor2()
{
  // game.net->server_con->c2sSendClientInfo(2)
}


void Got_NameAndcolor(char **cp,int playernum)
{
  /*
  PlayerInfo *p = game.FindPlayer(playernum);

  // color
  p->color=READBYTE(*cp) % MAXSKINCOLORS;

  // a copy of color
  if(p->pawn)
    p->pawn->color = p->color;

  const char *oldname = p->name.c_str();
  char newname[50];
  // name
  if (strcasecmp(oldname, *cp))
    CONS_Printf("%s renamed to %s\n", oldname, *cp);
  READSTRING(*cp, newname);
  p->name = newname;

  // skin
  SetPlayerSkin(playernum,*cp);
  SKIPSTRING(*cp);


  PlayerInfo *p = game.FindPlayer(playernum);
  p->originalweaponswitch = *(*cp)++;
  memcpy(p->favoriteweapon,*cp,NUMWEAPONS);
  *cp+=NUMWEAPONS;
  p->autoaim = *(*cp)++;
  */
}



void GameInfo::CL_Reset()
{
  CONS_Printf("Client reset\n");

  net->CL_Reset();
}



bool GameInfo::CL_SpawnClient()
{
  CONS_Printf("Setting up client...\n");

  if (Read_MAPINFO() <= 0)
    {
      CONS_Printf(" Bad MAPINFO lump.\n");
      return false;
    }

  ReadResourceLumps(); // SNDINFO etc.
  return true;
}


bool GameInfo::CL_StartGame()
{
  CONS_Printf("starting client game...\n");

  // add local players
  int n = 1 + cv_splitscreen.value;
  for (int i=0; i<n; i++)
    if (!LocalPlayers[i].info)
      LocalPlayers[i].info = game.AddPlayer(new PlayerInfo(&LocalPlayers[i]));

  // TODO add bots
  ViewPlayers.clear();
  for (int i=0; i < n; i++)
    ViewPlayers.push_back(LocalPlayers[i].info);

  hud.ST_Start();

  if (paused)
    {
      paused = false;
      S.ResumeMusic();
    }

  G_ReleaseKeys();

  // clear hud messages remains (usually from game startup)
  con.ClearHUD();
  automap.Close();

  //currentcluster = clustermap.begin()->second; // FIXME get cluster from server

  SetState(GS_LEVEL);
  return true;
}
