// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_video.h 498 2007-10-06 21:30:23Z smite-meister $
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
/// \brief Video system interface.


#ifndef i_video_h
#define i_video_h 1

#include "doomtype.h"

enum rendermode_t
{
  render_soft   = 1,
  render_opengl = 2
};

extern rendermode_t rendermode;


bool I_StartupGraphics();
void I_ShutdownGraphics();

void I_SetPalette(RGB_t* palette); // Takes full 8 bit values.
void I_SetGamma(float r, float g, float b); // Set display gamma exponents.

int  I_SetVideoMode(int modenum);
int  I_NumVideoModes();
const char *I_GetVideoModeName(unsigned modenum);
int  I_GetVideoModeForSize(int w, int h);

void I_FinishUpdate();

#endif
