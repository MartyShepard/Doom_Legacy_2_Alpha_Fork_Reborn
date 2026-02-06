// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: g_input.h 508 2007-12-18 21:00:41Z jussip $
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
/// \brief Mouse/keyboard/joystick inputs, game controls (forward, use, fire...)

#ifndef g_input_h
#define g_input_h 1

#include "doomtype.h"
#include "keys.h"

enum gamecontrols_e
{
  gc_null = 0,        //a key/button mapped to gc_null has no effect
  gc_forward,
  gc_backward,
  gc_strafe,
  gc_straferight,
  gc_strafeleft,
  gc_speed,
  gc_turnleft,
  gc_turnright,
  gc_lookup,
  gc_lookdown,
  gc_centerview,
  gc_mouseaiming,     // mouse aiming is momentary (toggleable in the menu)
  gc_fire,
  gc_use,
  gc_jump,
  gc_flydown, // flyup is jump
  gc_impulse, // controls after this need not be held down continuously
  gc_weapon1 = gc_impulse,
  gc_weapon2,
  gc_weapon3,
  gc_weapon4,
  gc_weapon5,
  gc_weapon6,
  gc_weapon7,
  gc_weapon8,
  gc_nextweapon,
  gc_prevweapon,
  gc_bestweapon,
  gc_invnext,
  gc_invprev,
  gc_invuse,
  num_gamecontrols,

  // common controls
  gk_console = 0,
  gk_talk,
  gk_scores,
  num_commoncontrols
};

//! All possible actions joystick axes can be bound to.
enum joyactions_e
{
  ja_pitch,  //!< Set up/down looking angle.
  ja_move,   //!< Moving front and back.
  ja_turn,   //!< Turn left or right.
  ja_strafe, //!< Strafing.
  num_joyactions
};

//! Contains the mappings from joystick axises to game actions.
struct joybinding_t
{
  int playnum;         //!< What player is controlled.
  int joynum;          //!< Which joystick to use.
  int axisnum;         //!< Which axis is the important one.
  joyactions_e action; //!< What should be done.
  float scale;         //!< A scaling factor. Set negative to flip axis.
};


// per-player keys (two (virtual) key codes per game control)
extern short gamecontrol[NUM_LOCALHUMANS][num_gamecontrols][2];

// shared control keys
extern short commoncontrols[num_commoncontrols][2];

// clears the pressed-down status of all gamekeys
void G_ReleaseKeys();

// remaps the input event to a game control.
bool  G_MapEventsToControls(struct event_t *ev);

// returns the name of a key
char *G_KeynumToString(int keynum);
int   G_KeyStringtoNum(const char *keystr);

// detach any keys associated to the given game control
void  G_ClearControlKeys(short (*setupcontrols)[2], int control);
void  Command_Setcontrol_f();
void  G_Controldefault();
void  G_CheckDoubleUsage(int keynum);


#endif
