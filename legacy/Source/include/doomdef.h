// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: doomdef.h 466 2007-05-25 18:55:27Z smite-meister $
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
/// \brief Common defines and utility function prototypes.

#ifndef doomdef_h
#define doomdef_h 1

/// version control
extern const int  LEGACY_VERSION;
extern const int  LEGACY_REVISION;
extern const char LEGACY_VERSIONSTRING[];
extern char LEGACY_VERSION_BANNER[];


//#define RANGECHECK              // Uncheck this to compile debugging code
#define PARANOIA                // do some test that never happens but maybe

#define MAXPLAYERNAME           21
#define MAXSKINCOLORS           11

/// Frame rate, original number of game tics / second.
#define TICRATE 35


/// Max. numbers of local players (human and bot) on a client.
enum
{
  NUM_LOCALHUMANS = 4,
  NUM_LOCALBOTS = 10,
  NUM_LOCALPLAYERS = NUM_LOCALHUMANS + NUM_LOCALBOTS
};



/// development mode (-devparm)
extern bool devparm;


/// commonly used routines - moved here for include convenience
void  I_Error(const char *error, ...);
void  CONS_Printf(const char *fmt, ...);
char *va(const char *format, ...);
char *Z_StrDup(const char *in);
int   I_GetKey();

#endif
