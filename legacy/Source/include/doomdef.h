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

  // Hardware 3DSound
  // #define HW3SOUND
     
	// Add Borderless
     #define BORDERLESS_WIN32
     
	// Release/DeRelease Mouse in Window Mode
     #define GRAB_MIDDLEMOUSE

	// Drag Wad/Zip over the Doom Legacy Icon
     #define DRAGFILE
     
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

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// Latest Original Code Branch Sourceforge
#define SVN_REVISION_ORG 705
#ifndef SVN_REV
#define SVN_REV STR(SVN_REVISION_ORG)
#endif

// Versioning: Fork Versioning
#define DOOMLEGACY2_MAJOR  2
#define DOOMLEGACY2_MINOR  0
#define DOOMLEGACY2_PATCH  1
#define DOOMLEGACY2_BUILD  02

#ifndef DOOMLEGACY2_VERSION
#define DOOMLEGACY2_VERSION STR(DOOMLEGACY2_MAJOR) "." STR(DOOMLEGACY2_MINOR) "." STR(DOOMLEGACY2_PATCH) "." STR(DOOMLEGACY2_BUILD)
#endif

#define BUILD_DATE __DATE__ 
#define BUILD_TIME __TIME__

#define FILEDESC "Doom Legacy 2.0 Alpha: Reborn " STR(DOOMLEGACY2_PATCH) "." STR(DOOMLEGACY2_BUILD) " (32Bit/SDL1)"

/* 
 * Weiterletung and die RC datei. 
 * 
*/

		#define VALUE_COMMENT          "DOOM LEGACY 2.0 ALPHA BACK FROM THE GRAVE LAWLESS DAJORMAS RULEZ FOREVER WHO READS THIS SHIT ANYWAY? BALDUR'S GATE IS FOR NERDS FRAG.COM/DOOMLEGACY OR GTFO"
		#define VALUE_COMPANY          "Intel Outside" 
		#define VALUE_FILEDESCRIPTION  FILEDESC
// Fork Versioning
		#define VALUE_VERSION_COMMA     DOOMLEGACY2_MAJOR,DOOMLEGACY2_MINOR,DOOMLEGACY2_PATCH,DOOMLEGACY2_BUILD
		#define VALUE_VERSION_STRING    STR(DOOMLEGACY2_MAJOR) ", " STR(DOOMLEGACY2_MINOR) ", " STR(DOOMLEGACY2_PATCH) ", " STR(DOOMLEGACY2_BUILD)
    
		#define VALUE_INTERNAL_NAME    "DoomLegacy2"
		#define VALUE_LEGALCOPYRIGHT   "Copyright (C) 1998-2024 by DooM Legacy Team"
		#define VALUE_LEGALTRADEMARKS  "Compiled from Marty (" __DATE__ ")"
		#define VALUE_ORIGINALFILENAME "DoomLegacy2.exe"
		#define VALUE_PRODUCTNAME      "Doom Legacy 2 Forked: Based on svn705"
		#define VALUE_PRIVATEBUILD     ""
		#define VALUE_SPECIALBUILD     ""
// Latest Original Code Branch Sourceforge    
		#define VALUE_PRODUCT_COMMA   DOOMLEGACY2_MAJOR,DOOMLEGACY2_MINOR,DOOMLEGACY2_MINOR,SVN_REVISION_ORG
		#define VALUE_PRODUCTVSTRING   STR(DOOMLEGACY2_MAJOR) ", " STR(DOOMLEGACY2_MINOR) ", " STR(DL_VER_REV)    
#endif
