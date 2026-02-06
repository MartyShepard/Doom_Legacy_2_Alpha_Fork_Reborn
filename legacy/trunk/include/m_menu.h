// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: m_menu.h 449 2007-05-05 20:45:25Z smite-meister $
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
/// \brief Menu system.

#ifndef m_menu_h
#define m_menu_h 1

#include "doomtype.h"


/// \brief Game menu, may contain submenus
class Menu
{
  friend class MsgBox;
private:
  // video and audio resources
  static class font_t *font;

  static short AnimCount;     ///< skull animation counter
  static class Material *pointer[2]; ///< the menu pointer
  static int   which_pointer;
  static int   SkullBaseLump; ///< menu animation base lump for Heretic and Hexen
  static tic_t NowTic;        ///< current time in tics


  static Menu  *currentMenu; ///< currently active menu
  static short  itemOn;      ///< currently highlighted menu item

  typedef void (Menu::* drawfunc_t)();
  typedef bool (* quitfunc_t)();


private:
  const char  *titlepic;    ///< title Texture name
  const char  *title;       ///< title as string for display with bigfont if present
  Menu        *parent;      ///< previous menu
  short        numitems;    ///< # of menu items
  struct menuitem_t *items; ///< array of menu items
  short        lastOn;      ///< last active item
  short        x, y;        ///< menu screen coords
  drawfunc_t   drawroutine; ///< draw routine
  quitfunc_t   quitroutine; ///< called before quit a menu return true if we can

public:
  static bool active; ///< menu is currently open

  /// constructor
  Menu(const char *tpic, const char *t, Menu *up, int nitems, menuitem_t *items,
       short mx, short my, short on = 0, drawfunc_t df = NULL, quitfunc_t qf = NULL);

  /// opens the menu
  static void Open();

  /// closes the menu
  static void Close(bool callexitmenufunc);

  /// tics the menu (skull cursor) animation and video mode testing
  static void Ticker();

  /// eats events
  static bool Responder(struct event_t *ev);

  /// starts up the menu system
  static void Startup();

  /// resets the menu system according to current game.mode
  static void Init();

  /// draws the menus directly into the screen buffer.
  static void Drawer();

  /// changes menu node
  static void SetupNextMenu(Menu *m);

  /// submenu event handler
  bool MenuResponder(int key);

  /// utility
  int GetNumitems() const { return numitems; }

  /// the actual drawing
  void DrawTitle();
  void DrawMenu();

  void HereticMainMenuDrawer();
  void HexenMainMenuDrawer();
  void DrawClass();
  void DrawConnect();
  void DrawSetupPlayer();
  void DrawVideoMode();
  void DrawReadThis1();
  void DrawReadThis2();
  void DrawSave();
  void DrawLoad();
  void DrawControl();
  void DrawOpenGL();
  void OGL_DrawFog();
  void OGL_DrawColor();
};

// opens the menu and creates a message box
void M_StartMessage(const char *str);

// show or hide the setup for player 2 (called at splitscreen change)
void M_SwitchSplitscreen();

// Called by linux_x/i_video_xshm.c
void M_QuitResponse(int ch);

#endif
