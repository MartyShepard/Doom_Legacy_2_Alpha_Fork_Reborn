// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: wi_stuff.h 352 2006-07-21 00:27:31Z smite-meister $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2004 by DooM Legacy Team.
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
/// \brief Intermission handler.

#ifndef wi_stuff_h
#define wi_stuff_h 1

#include <vector>
#include "doomdef.h"

using namespace std;

/// \brief Intermission handler
///
/// A one-instance class for Winning/Intermission animations etc.
/// could be done using static functions, but I happen to like classes;)
/// NOTE: during the intermission players may enter and leave the game.
/// players who enter do not show up in the stats listing
/// players who leave are not removed until the next level starts, so no problem there.

class Intermission
{
private:
  /// States for the intermission
  enum WIstate_e
  {
    Inactive,
    StatCount,
    ShowNextLoc,
    Wait
  };

  /// specifies current state
  WIstate_e state;

  // player wants to accelerate or skip a stage
  bool acceleratestage;

  /// What animation, if any, do we show? Applicable in Doom1 and Heretic, otherwise zero.
  int  episode;
  bool show_yah;

  const char *interpic;
  const char *intermusic;

  const char *lastlevelname;
  const char *nextlevelname;

  // level numbers for old Doom style graphic levelnames and YAH's
  int next, last; 

  // used for general timing and bg animation timing
  int count, bcount;

  // blinking pointer
  bool pointeron;

  // DM stats
  int nplayers;
  struct fragsort_t *dm_score[4];

  // Coop stats
  int  count_stage;
  bool dofrags;
  struct statcounter_t
  {
    int kills;
    int items;
    int secrets;
    int frags;
  };

  // counters for each player
  vector<statcounter_t> cnt;
  // players that participate in the intermission
  vector<class PlayerInfo *> plrs;

  // level totals
  statcounter_t total;

  int  cnt_time, cnt_par;
  int  time, partime;

  int s_count; /// counting sound id


  void LoadData();
  void UnloadData();

  /// draw background
  void SlamBackground();

  /// background animations (Doom 1 only)
  void UpdateAnimatedBack();

  void InitCoopStats();
  void UpdateCoopStats();
  void DrawCoopStats();

  void InitDMStats();
  void UpdateDMStats();
  void DrawDMStats();

  void DrawYAH();

  void InitWait();

public:
  Intermission();

  /// starts the intermission
  void Start(const class MapInfo *f, const MapInfo *n, int maptic, int kills, int items, int secrets);

  /// the intermission is ended when the server says so
  void End();

  /// accelerate stage?
  bool Responder(struct event_t *ev);

  /// Updates stuff each tick
  void Ticker();

  /// well.
  void Drawer();
};

extern Intermission wi;

#endif
