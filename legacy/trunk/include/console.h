// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: console.h 500 2007-11-09 00:37:13Z smite-meister $
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
/// \brief Console interface

#ifndef console_h
#define console_h 1

#include <deque>
#include <string>

#include "doomtype.h"
#include "keys.h"

using namespace std;

/// \brief Console
///
/// Console serves as an user interface to the command buffer.
/// It gathers input, keeps track of command history and of course
/// draws the messages on screen.
/// There is only one global instance in use, called "con".

class Console
{
public:
  bool refresh; ///< explicitly refresh screen after each CONS_Printf (game is not yet in display loop)
  bool recalc;  ///< set true when screen size has changed. on next tick, console will conform

  char *bindtable[NUMINPUTS]; ///< console key bindings

protected:
  bool graphic; ///< console can be drawn
  bool active;  ///< console is active (accepting input)

  int con_tick; ///< console ticker for anim of blinking prompt cursor


  // console input
#define  CON_MAXINPUT 256
#define  CON_HISTORY 32
  deque<string> input_history; ///< input history (current line is always input_history.front())
  int           input_browse;  ///< input history line number being browsed/edited


  // console output
#define CON_MAXLINECHARS 120 // nobody wants to read lines longer than this
#define CON_LINES 100 // reasonable output history
  char  con_buffer[CON_LINES][CON_MAXLINECHARS+1]; ///< wrapping output buffer (ASCII or UTF-8)
  char *con_line;   ///< pointer to current output line

  int con_cx;       ///< cursor position in current line (column number)
  int con_cy;       ///< cursor line number in con_buffer, wraps around using modulo
  int con_scrollup; ///< how many rows of text to scroll up (pgup/pgdn)


  // hud lines, used when console is closed
#define  CON_HUDLINES 5
  int con_lineowner[CON_HUDLINES]; ///< In splitscreen, which player gets this line of text
  int con_hudtime[CON_HUDLINES];   ///< remaining time of display for hud msg lines

  // graphics
  class Material *con_backpic; ///< console background picture, loaded static
  Material *con_lborder, *con_rborder; ///< console borders in translucent mode

  int con_destheight; ///< destination height in pixels
  int con_height;     ///< current console height in pixels

  float con_width; ///< printing area width in pixels
  float con_lineheight; ///< height of a line in pixels


protected:
  /// called after vidmode has changed
  void RecalcSize();

  void DrawConsole();
  void DrawHudlines();

  void Print(char *msg);
  void Linefeed(int player);

public:
  Console();
  ~Console();

  /// client init
  void Init();

  /// change console state (on/off)
  void Toggle(bool forceoff = false);

  /// clear the output buffer
  void Clear();

  /// clears HUD messages
  void ClearHUD();

  /// event responder
  bool Responder(struct event_t *ev);

  /// animation, message timers
  void Ticker();

  /// draws the console
  void Drawer();

  /// wrapper
  friend void CONS_Printf(const char *fmt, ...);
};


void CONS_Error(char *msg); // print out error msg, wait for keypress


extern Console con;


extern int con_keymap;

// character input
extern char *shiftxform; // SHIFT-keycode to ASCII mapping
char KeyTranslation(unsigned char ch); // keycode to ASCII mapping


// top clip value for view render: do not draw part of view hidden by console
extern int  con_clipviewtop;
extern int  con_clearlines;  // lines of top of screen to refresh
extern bool con_hudupdate;   // hud messages have changed, need refresh



// globally used colormaps
extern byte *whitemap;
extern byte *greenmap;
extern byte *graymap;


#endif
