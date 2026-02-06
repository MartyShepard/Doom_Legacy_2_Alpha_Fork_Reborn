// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: am_map.h 449 2007-05-05 20:45:25Z smite-meister $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2003 by DooM Legacy Team.
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
/// \brief AutoMap class

#ifndef am_map_h
#define am_map_h 1

#include "doomtype.h"
#include "m_fixed.h"


struct fpoint_t
{
  int x, y;
};

struct fline_t
{
  fpoint_t a, b;
};

struct mpoint_t
{
  fixed_t x,y;
};

/// \brief The in-game overhead map.
///
/// There is only one global instance in use, called "automap".
class AutoMap
{
private:
  int  amclock; // tic counter

  bool bigstate;  // toggle between user view and large view (full map view)
  bool followplayer; // specifies whether to follow the player around
  bool grid;

  const class Map *mp;     // currently seen Map
  const class PlayerPawn *mpawn; // the pawn represented by an arrow

  bool am_recalc;   ///< screen size has changed...

  class Material *mapback; ///< possible automap background

  Material *marknums[10]; // numbers used for marking by the automap

  static const int AM_NUMMARKPOINTS = 10;
  mpoint_t markpoints[AM_NUMMARKPOINTS];   // where the points are
  int markpointnum;                    // next point to be assigned

public:
  bool active;      ///< is the AutoMap currently open?
  bool translucent; // draw overlaid on 3D view?

  int  am_cheating; // AutoMap cheats

  AutoMap();

  void Startup();

  void Ticker();
  bool Responder(struct event_t *ev); ///< eats control events

  void Resize();        ///< called after a vidmode change
  void InitVariables();

  void Open(const PlayerPawn *p);
  void Close();

  // called after a map change
  //void Reset(const PlayerPawn *p);



  void addMark();
  void clearMarks();
  void restoreScaleAndLoc();
  void changeWindowLoc();

  // drawing
  void Drawer();
  void clearFB(int color);
  void drawGrid(int color);
  void drawWalls();
  void drawPlayers();
  void drawThings(int colors, int colorrange);
  void drawMarks();
};

extern AutoMap automap;

#endif
