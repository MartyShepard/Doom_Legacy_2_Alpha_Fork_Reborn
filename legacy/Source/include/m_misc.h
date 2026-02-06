// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: m_misc.h 516 2007-12-22 15:57:58Z jussip $
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
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Default configfile, screenshots, file I/O

#ifndef m_misc_h
#define m_misc_h 1

#include "doomtype.h"

//===========================================================================

bool  FIL_WriteFile(const char *name, void *source, int length);
int   FIL_ReadFile(const char *name, byte **buffer);
void  FIL_DefaultExtension(char *path, const char *extension);
void  FIL_ExtractFileBase(char *path, char *dest);
bool  FIL_CheckExtension(const char *in);
const char *FIL_StripPath(const char *s);

//===========================================================================

void M_ScreenShot();

//===========================================================================

extern char configfile[128];
extern char savegamename[256];
extern char hubsavename[256];

void Command_SaveConfig_f();
void Command_LoadConfig_f();
void Command_ChangeConfig_f();

void M_FirstLoadConfig();
//Fab:26-04-98: save game config : cvars, aliases..
void M_SaveConfig(char *filename);

//===========================================================================

// s1=s2+s3+s1 (1024 lenghtmax)
void strcatbf(char *s1,char *s2,char *s3);

string string_to_upper(const char *c);
#endif
