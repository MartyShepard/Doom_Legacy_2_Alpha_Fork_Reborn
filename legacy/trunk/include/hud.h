// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: hud.h 464 2007-05-23 23:53:00Z smite-meister $
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
/// \brief Heads up display. Everything that is rendered on the 3D view.
/// Includes status bar, crosshair, icons...

#ifndef hud_h
#define hud_h 1

#include <vector>
#include <list>
#include <string>

using namespace std;


/// \brief Struct for scoreboard listings
struct fragsort_t
{
  int  count;
  int  num;
  int  color;
  const char *name;
};

void HU_DrawRanking(const char *title, int x, int y, fragsort_t *fragtable,
		    int scorelines, bool large, int white);



/// \brief Heads Up Display
///
/// This class handles everything that is drawn over the 3D view during the game,
/// such as the status bar. There is only one global instance in use, called "hud".
class HUD
{
protected:
  // statusbar data
  bool st_active;
  int  st_x, st_y;

  // what should we draw?
  bool statusbar_on;
  bool mainbar_on;
  bool invopen;
  bool drawscore; ///< should we draw frags instead of statusbar overlay?

  bool st_refresh;  ///< the statusbar needs to be redrawn

  vector<class HudWidget *> statusbar; ///< status bar
  vector<HudWidget *> mainbar; ///< part of the status bar that is hidden by open inventory
  vector<HudWidget *> keybar;  ///< part of the status bar that is shown under automap
  vector<HudWidget *> always;  ///< part of the status bar overlapping the 3D viewport, must be drawn always
  vector<HudWidget *> overlay; ///< HUD overlay


  void UpdateWidgets(class PlayerInfo *p, int vp);
  void PaletteFlash(PlayerInfo *p);

  void ST_RefreshBackground();

  void ST_CreateWidgets();
  void CreateDoomWidgets();
  void CreateHereticWidgets();
  void CreateHexenWidgets();

public:
  void CreateOverlayWidgets();

  int  stbarheight; ///< status bar height in pixels (with current drawing options)
  bool overlay_on;  ///< draw overlay instead of statusbar?

  int st_palette;  ///< current palette

  bool    chat_on; ///< player is currently typing a chat msg
protected:
  string  chat_msg;
  void SendChat();

  list<class HudTip*> tips; ///< scriptable HUD messages
  void DrawTips();

  vector<class HudPic*> pics; ///< scriptable HUD pictures
  void DrawPics();

public:
  HUD();

  void Startup();  // register HUD commands and consvars
  void Init();     // cache HUD data
  bool Responder(struct event_t* ev);
  void Ticker();
  void Draw(PlayerInfo *player, int vp);
  void DrawCommon();

  void ST_Drawer(int vp);
  void ST_Start();
  void ST_Stop();

  void ST_Recalc(); // recalculates the status bar coordinates
  // (after changing the resolution or scaling, for example)

  void HU_Erase();
  inline void RefreshStatusbar() { st_refresh = true; }

  int  GetFSPic(int lumpnum, int xpos, int ypos);
  bool DeleteFSPic(int handle);
  bool ModifyFSPic(int handle, int lumpnum, int xpos, int ypos);
  bool DisplayFSPic(int handle, bool newval);
};


extern HUD hud;

#endif
