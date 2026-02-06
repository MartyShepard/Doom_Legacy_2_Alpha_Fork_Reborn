// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
// DESCRIPTION:
// Just debug stuff here anymore.
//
//-----------------------------------------------------------------------------


#ifndef d_debug_h
#define d_debug_h 1

#include <stdio.h>


// ===========================
// Internal parameters, fixed.
// ===========================
// These are set by the engine, and not changed
//  according to user inputs. Partly load from
//  WAD, partly set at startup time.

#define localgametic  cmap.leveltic


#ifdef __MACOS__
# define DEBFILE(msg) I_OutputMsg(msg)
extern  FILE*           debugfile;
#else
# define DEBUGFILE
# ifdef DEBUGFILE
#  define DEBFILE(msg) { if(debugfile) fputs(msg,debugfile); }
extern  FILE*           debugfile;
# else
#  define DEBFILE(msg) {}
extern  FILE*           debugfile;
# endif
#endif //__MACOS__


#endif // d_debug_h
