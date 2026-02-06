// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: screen.h 498 2007-10-06 21:30:23Z smite-meister $
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
/// \brief Video subsystem

#ifndef screen_h
#define screen_h 1

#include "doomtype.h"


#define NUMSCREENS 2

// Screen 0 is the screen updated by I_Update screen.
// Screen 1 is an extra buffer.

// we try to re-allocate a minimum of buffers for stability of the memory,
// so all the small-enough tables based on screen size, are allocated once
// and for all at the maximum size.

#define MAXVIDWIDTH     7680 //dont set this too high because actually
#define MAXVIDHEIGHT    4320 // lots of tables are allocated with the MAX size.
#define BASEVIDWIDTH    320  //NEVER CHANGE THIS! this is the original
#define BASEVIDHEIGHT   200  // resolution of the graphics.


/// \brief Video subsystem.
///
/// Takes care of video modes,
/// scaling coefficients for original Doom bitmaps,
/// software mode multiple buffering etc.
/// There's only one global instance in use, called 'vid'.
class Video
{
public:
  int  modenum;       ///< current vidmode num indexes videomodes list
  int  width, height; ///< current resolution in pixels
  int  rowbytes;      ///< bytes per scanline of the current vidmode
  int  BytesPerPixel; ///< color depth: 1=256color, 2=highcolor
  int  BitsPerPixel;  ///< == BytesPerPixel * 8

  // software mode only
public:
  byte  *screens[NUMSCREENS]; ///< Each screen is [vid.width*vid.height];
  byte  *direct;     ///< linear frame buffer, or vga base mem.

  bool windowed;     ///< not fullscreen?

  int   dupx, dupy;    ///< integer scaling value (1,2,3) for menus & overlays
  float fdupx, fdupy;  ///< same as dupx,dupy but exact value when aspect ratio isn't 320/200

  int   centerofs;  ///< centering for the scaled menu gfx
  int   scaledofs;  ///< centering offset for the scaled graphics,

  int   setmodeneeded; ///< video mode change needed if > 0 // (the mode number to set + 1)
  bool  resetpaletteneeded; ///< palette needs to be reset to 0

  RGB_t *palette;        ///< local copy of the current palette
  int    currentpalette; ///< number of the currently active palette

private:
  byte  *buffer;     ///< invisible screens buffer

  /// Recalc screen size dependent stuff
  void Recalc();

public:
  /// constructor
  Video();

  /// Starts up video hardware, loads palette, registers consvars.
  void Startup();

  /// Change video mode, only at the start of a refresh.
  void SetMode();

  /// Loads a set of palettes, applies gamma correction.
  void LoadPalette(const char *lumpname);

  /// Choose the current RGB palette for palettized graphics.
  void SetPalette(int palettenum);

  /// Equivalent to LoadPalette(pal); SetPalette(0);
  void SetPaletteLump(const char *pal);

  /// Returns the currently active palette.
  RGB_t *GetCurrentPalette();

  /// Change to default video mode (called once at startup).
  void CheckDefaultMode();

  /// Set the mode number which is saved in the config.
  void SetDefaultMode();
  
  /* Marty */
  // Neu: öffentliche Methode zum Füllen
  void FillScreen(byte color = 0);

  // Optional: Fill mit Rechteck
  void FillRect(int x, int y, int w, int h, byte color);
  
};


extern Video vid;

#endif
