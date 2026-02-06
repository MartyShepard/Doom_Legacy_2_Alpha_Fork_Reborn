// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: hwr_render.h 449 2007-05-05 20:45:25Z smite-meister $
//
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

#ifndef hwr_render_h
#define hwr_render_h 1

/* OpenGL Init --------------------
 * liegt in oglinit.h 
		#define GL_GLEXT_PROTOTYPES
		#if defined(__APPLE_CC__) || defined(__MACOS__)
			#include <OpenGL/gl.h>
			#include <OpenGL/glu.h>
		#else
			#include <GL/gl.h>
			#include <GL/glu.h>	
		#endif
*/
#include "ogl_init.h"
/* OpenGL Init End ------------- */



#include "../r_render.h"

class HWBsp;


/**
  \brief Handles the new hardware rendering code.
  Note: This is early development and all that can completely change in the future.
        It's here only until the new renderer will be at least as good as the old one.
*/
class HWRend
{
private:

  GLfloat model_matrix[16];
  GLfloat projection_matrix[16];
  GLint   viewport[4];

  HWBsp *bsp;

public:
  HWRend();
  ~HWRend();

  /// Do all the rendering depending on the player state.
  /// That means player view and player sprites but not the console and the menu.
  void RenderPlayerView(int viewnumber, PlayerInfo *player);

  /// Get the size of memory allocated for texture (in bytes).
  int GetTextureUsed();

  /// add all hardware related commands.
  void AddCommands();

  /// Clear the automap
  void ClearAutomap();

  /// Grab the frame buffer and write it to a file
  bool Screenshot(char *lbmname);

  /// Change doom palette
  void SetPalette(RGB_t *palette);

  /// Fade part of the screen buffer, so that the menu and the console is more readable
  void FadeScreenMenuBack(unsigned long color, int height);

  /// Fills a box of pixels with a single color, NOTE: scaled to screen size
  void DrawFill(int x, int y, int w, int h, int color);

  /// Fills a box of pixels using a flat texture as a pattern
  void DrawFill(int x, int y, int w, int h, class Material *m);

  /// Draw the border of the screen.
  void DrawViewBorder();

  /// Change the size of the player view
  void SetViewSize(int blocks);

  /// Prepare hardware related structures for the current map.
  void Setup(int bspnum);

  /// Intialize OpenGL (windows, states,...).
  void Startup();

  /// Draw a line of the automap (it's a callback)
  static void DrawAMline(struct fline_t* fl, int color);
};

extern HWRend HWR;

#endif
