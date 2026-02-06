// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: v_video.h 527 2008-01-27 02:25:14Z smite-meister $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2006 by DooM Legacy Team.
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
/// \brief Texture blitting, blitting rectangles between buffers. Font system.

#ifndef v_video_h
#define v_video_h 1

#include <vector>
#include "doomdef.h"
#include "doomtype.h"

/// flags for drawing Textures
enum texture_draw_e
{
  V_SLOC     =  0x10000,   // scale starting location
  V_SSIZE    =  0x20000,   // scale size
  V_FLIPX    =  0x40000,   // mirror the patch in the vertical direction
  V_MAP      =  0x80000,   // use a colormap
  V_WHITEMAP = 0x100000,   // white colormap (for V_DrawString)
  V_TL       = 0x200000,   // translucency using transmaps

  V_SCALE = V_SLOC | V_SSIZE,

  V_FLAGMASK = 0xFFFF0000,
  V_SCREENMASK = 0xF
};



/// \brief ABC for fonts
class font_t
{
#define TABWIDTH 32.0

public:
  float lineskip, advance; ///< Basically world dimensions of the character '0' in the font.

public:
  virtual ~font_t();

  /// Draw a single UCS-4 character, return width.
  virtual float DrawCharacter(float x, float y, int c, int flags) = 0;
  /// Draw an unkerned UTF-8 string, return string width.
  float DrawString(float x, float y, const char *str, int flags);

  /// Returns the width of the unkerned UTF-8 string in world units.
  virtual float StringWidth(const char *str) = 0;
  /// Returns the width of the first n characters of the unkerned UTF-8 string in world units.
  virtual float StringWidth(const char *str, int n) = 0;

  /// Returns the lineskip of the font in world units.
  inline float Height() const { return lineskip; }
  /// Returns the advance of the font in world units.
  inline float Width() const { return advance; }

  /// Advanced composition is available (kerning etc.).
  virtual bool CanCompose() const { return false; }
  /// Renders the UFT-8 string using kerning into a Material.
  virtual class Material *ComposeString(const char *str) { return NULL; }

  /// Initialize the font system.
  static void Init();
};

extern font_t *hud_font;
extern font_t *big_font;


/// color translation
extern byte    translationtables[MAXSKINCOLORS][256];
extern byte   *current_colormap; // for applying colormaps to Drawn Textures


void VID_BlitLinearScreen(byte *srcptr, byte *destptr, int width,
			  int height, int srcrowbytes, int destrowbytes);

void V_CopyRect(float srcx,  float srcy,  int srcscrn,
		float width, float height,
		float destx, float desty, int destscrn);


// Draw a linear block of pixels into the view buffer.
void V_DrawBlock(int x, int y, int scrn, int width, int height, byte* src);
// Reads a linear block of pixels into the view buffer.
void V_GetBlock( int x, int y, int scrn, int width, int height, byte* dest);


//added:05-02-98: fill a box with a single color
void V_DrawFill(int x, int y, int w, int h, int c);

//added:10-02-98: fade down the screen buffer before drawing the menu over
void V_DrawFadeScreen();
//added:20-03-98: test console
void V_DrawFadeConsBack(float x1, float y1, float x2, float y2);


//added:12-02-98:
void V_DrawTiltView(byte *viewbuffer);

//added:05-04-98: test persp. correction !!
void V_DrawPerspView(byte *viewbuffer, int aiming);

#endif
