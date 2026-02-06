// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: sounds.cpp 515 2007-12-22 14:51:46Z jussip $
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
/// \brief High-level audio interface, SNDINFO parser.
///
/// Note: the tables were originally created by a sound utility at Id,
///  kept as a sample, DOOM2 sounds.

#include "doomdef.h"

#include "g_game.h"
#include "g_mapinfo.h"

#include "parser.h"
#include "functors.h"
#include "sounds.h"
#include "s_sound.h"


// Doom/Heretic music names corresponding to musicenum_t
// NOTE: padded with \0 to stringlen=8, to allow dehacked patching

const char* MusicNames[NUMMUSIC] =
{
    NULL,
    // Doom
    "D_E1M1\0\0",
    "D_E1M2\0\0",
    "D_E1M3\0\0",
    "D_E1M4\0\0",
    "D_E1M5\0\0",
    "D_E1M6\0\0",
    "D_E1M7\0\0",
    "D_E1M8\0\0",
    "D_E1M9\0\0",

    "D_E2M1\0\0",
    "D_E2M2\0\0",
    "D_E2M3\0\0",
    "D_E2M4\0\0",
    "D_E2M5\0\0",
    "D_E2M6\0\0",
    "D_E2M7\0\0",
    "D_E2M8\0\0",
    "D_E2M9\0\0",

    "D_E3M1\0\0",
    "D_E3M2\0\0",
    "D_E3M3\0\0",
    "D_E3M4\0\0",
    "D_E3M5\0\0",
    "D_E3M6\0\0",
    "D_E3M7\0\0",
    "D_E3M8\0\0",
    "D_E3M9\0\0",
    // Ultimate Doom Ep. 4 re-used music
    "D_E3M4\0\0",
    "D_E3M2\0\0",
    "D_E3M3\0\0",
    "D_E1M5\0\0",
    "D_E2M7\0\0",
    "D_E2M4\0\0",
    "D_E2M6\0\0",
    "D_E2M5\0\0",
    "D_E1M9\0\0",

    "D_INTER\0" ,
    "D_INTRO\0" ,
    "D_BUNNY\0" ,
    "D_VICTOR"  ,
    "D_INTROA"  ,
    // Doom II
    "D_RUNNIN"  ,
    "D_STALKS"  ,
    "D_COUNTD"  ,
    "D_BETWEE"  ,
    "D_DOOM\0\0",
    "D_THE_DA"  ,
    "D_SHAWN\0" ,
    "D_DDTBLU"  ,
    "D_IN_CIT"  ,
    "D_DEAD\0\0",
    "D_STLKS2"  ,
    "D_THEDA2"  ,
    "D_DOOM2\0" ,
    "D_DDTBL2"  ,
    "D_RUNNI2"  ,
    "D_DEAD2\0" ,
    "D_STLKS3"  ,
    "D_ROMERO"  ,
    "D_SHAWN2"  ,
    "D_MESSAG"  ,
    "D_COUNT2"  ,
    "D_DDTBL3"  ,
    "D_AMPIE\0" ,
    "D_THEDA3"  ,
    "D_ADRIAN"  ,
    "D_MESSG2"  ,
    "D_ROMER2"  ,
    "D_TENSE\0" ,
    "D_SHAWN3"  ,
    "D_OPENIN"  ,
    "D_EVIL\0\0",
    "D_ULTIMA"  ,
    "D_READ_M"  ,
    "D_DM2TTL"  ,
    "D_DM2INT"  ,
    // Heretic
    "MUS_E1M1", // 1-1
    "MUS_E1M2",
    "MUS_E1M3",
    "MUS_E1M4",
    "MUS_E1M5",
    "MUS_E1M6",
    "MUS_E1M7",
    "MUS_E1M8",
    "MUS_E1M9",

    "MUS_E2M1", // 2-1
    "MUS_E2M2",
    "MUS_E2M3",
    "MUS_E2M4",
    "MUS_E1M4",
    "MUS_E2M6",
    "MUS_E2M7",
    "MUS_E2M8",
    "MUS_E2M9",

    "MUS_E1M1", // 3-1
    "MUS_E3M2",
    "MUS_E3M3",
    "MUS_E1M6",
    "MUS_E1M3",
    "MUS_E1M2",
    "MUS_E1M5",
    "MUS_E1M9",
    "MUS_E2M6",

    "MUS_E1M6", // 4-1
    "MUS_E1M2",
    "MUS_E1M3",
    "MUS_E1M4",
    "MUS_E1M5",
    "MUS_E1M1",
    "MUS_E1M7",
    "MUS_E1M8",
    "MUS_E1M9",

    "MUS_E2M1", // 5-1
    "MUS_E2M2",
    "MUS_E2M3",
    "MUS_E2M4",
    "MUS_E1M4",
    "MUS_E2M6",
    "MUS_E2M7",
    "MUS_E2M8",
    "MUS_E2M9",

    "MUS_E3M2", // 6-1
    "MUS_E3M3", // 6-2
    "MUS_E1M6", // 6-3

    "MUS_TITL",
    "MUS_INTR",
    "MUS_CPTD"
};


/*
// Information about all the sfx. Kept here to see if the priority info is needed (see resources/SNDINFO.lmp)
sfxinfo_t S_sfx[NUMSFX] =
{
  // S_sfx[0] needs to be a dummy for odd reasons.
//              multiplicity
//   tagname             |  priority(U)
//     |    lumpname     |    |
  { NULL, "NONE"       , 0,   0},
  { NULL, "DSPISTOL"   , 0,  64},
  { NULL, "DSSHOTGN"   , 0,  64},
  { NULL, "DSSGCOCK"   , 0,  64},
  { NULL, "DSDSHTGN"   , 0,  64},
  { NULL, "DSDBOPN\0"  , 0,  64},
  { NULL, "DSDBCLS\0"  , 0,  64},
  { NULL, "DSDBLOAD"   , 0,  64},
  { NULL, "DSPLASMA"   , 0,  64},
  { NULL, "DSBFG\0\0\0", 0,  64},

  { NULL, "DSSAWUP\0"  , 0,  64},
  { NULL, "DSSAWIDL"   , 0, 118},
  { NULL, "DSSAWFUL"   , 0,  64},
  { NULL, "DSSAWHIT"   , 0,  64},
  { NULL, "DSRLAUNC"   , 0,  64},
  { NULL, "DSRXPLOD"   , 0,  70},
  { NULL, "DSFIRSHT"   , 0,  70},
  { NULL, "DSFIRXPL"   , 0,  70},
  { NULL, "DSPSTART"   , 0, 100},
  { NULL, "DSPSTOP\0"  , 0, 100},

  { NULL, "DSDOROPN"   , 0, 100},
  { NULL, "DSDORCLS"   , 0, 100},
  { NULL, "DSSTNMOV"   , 0, 119},
  { NULL, "DSSWTCHN"   , 0,  78},
  { NULL, "DSSWTCHX"   , 0,  78},
  { NULL, "DSPLPAIN"   , 0,  96},   //SKSPLPAIN},
  { NULL, "DSDMPAIN"   , 0,  96},
  { NULL, "DSPOPAIN"   , 0,  96},
  { NULL, "DSVIPAIN"   , 0,  96},
  { NULL, "DSMNPAIN"   , 0,  96},

  { NULL, "DSPEPAIN"   , 0,  96},
  { NULL, "DSSLOP\0\0" , 0,  78},    //SKSSLOP},
  { NULL, "DSITEMUP"   ,  1,  78},
  { NULL, "DSWPNUP"    ,  1,  78},
  { NULL, "DSOOF\0\0\0", 0,  96},    //SKSOOF},
  { NULL, "DSTELEPT"   , 0,  32},
  { NULL, "DSPOSIT1"   ,  1,  98},
  { NULL, "DSPOSIT2"   ,  1,  98},
  { NULL, "DSPOSIT3"   ,  1,  98},
  { NULL, "DSBGSIT1"   ,  1,  98},

  { NULL, "DSBGSIT2"   ,  1,  98},
  { NULL, "DSSGTSIT"   ,  1,  98},
  { NULL, "DSCACSIT"   ,  1,  98},
  { NULL, "DSBRSSIT"   ,  1,  94},
  { NULL, "DSCYBSIT"   ,  1,  92},
  { NULL, "DSSPISIT"   ,  1,  90},
  { NULL, "DSBSPSIT"   ,  1,  90},
  { NULL, "DSKNTSIT"   ,  1,  90},
  { NULL, "DSVILSIT"   ,  1,  90},
  { NULL, "DSMANSIT"   ,  1,  90},

  { NULL, "DSPESIT\0"  ,  1,  90},
  { NULL, "DSSKLATK"   , 0,  70},
  { NULL, "DSSGTATK"   , 0,  70},
  { NULL, "DSSKEPCH"   , 0,  70},
  { NULL, "DSVILATK"   , 0,  70},
  { NULL, "DSCLAW\0\0" , 0,  70},
  { NULL, "DSSKESWG"   , 0,  70},
  { NULL, "DSPLDETH"   , 0,  32}, //SKSPLDETH},
  { NULL, "DSPDIEHI"   , 0,  32}, //SKSPDIEHI},
  { NULL, "DSPODTH1"   , 0,  70},

  { NULL, "DSPODTH2"   , 0,  70},
  { NULL, "DSPODTH3"   , 0,  70},
  { NULL, "DSBGDTH1"   , 0,  70},
  { NULL, "DSBGDTH2"   , 0,  70},
  { NULL, "DSSGTDTH"   , 0,  70},
  { NULL, "DSCACDTH"   , 0,  70},
  { NULL, "DSSKLDTH"   , 0,  70},
  { NULL, "DSBRSDTH"   , 0,  32},
  { NULL, "DSCYBDTH"   , 0,  32},
  { NULL, "DSSPIDTH"   , 0,  32},

  { NULL, "DSBSPDTH"   , 0,  32},
  { NULL, "DSVILDTH"   , 0,  32},
  { NULL, "DSKNTDTH"   , 0,  32},
  { NULL, "DSPEDTH\0"  , 0,  32},
  { NULL, "DSSKEDTH"   , 0,  32},
  { NULL, "DSPOSACT"   ,  1, 120},
  { NULL, "DSBGACT\0"  ,  1, 120},
  { NULL, "DSDMACT\0"  ,  1, 120},
  { NULL, "DSBSPACT"   ,  1, 100},
  { NULL, "DSBSPWLK"   ,  1, 100},

  { NULL, "DSVILACT"   ,  1, 100},
  { NULL, "DSNOWAY\0"  , 0,  78}, //SKSNOWAY},
  { NULL, "DSBAREXP"   , 0,  60},
  { NULL, "DSPUNCH\0"  , 0,  64}, //SKSPUNCH},
  { NULL, "DSHOOF\0\0" , 0,  70},
  { NULL, "DSMETAL\0"  , 0,  70},
  { NULL, "DSPISTOL"  , 0,  64}, // FIXME wrong pitch for chaingun... "dschgun\0", &S_sfx[sfx_pistol], 150, 0},
  { NULL, "DSTINK\0\0" , 0,  60},
  { NULL, "DSBDOPN\0"  , 0, 100},
  { NULL, "DSBDCLS\0"  , 0, 100},

  { NULL, "DSITMBK\0"  , 0, 100},
  { NULL, "DSFLAME\0"  , 0,  32},
  { NULL, "DSFLAMST"   , 0,  32},
  { NULL, "DSGETPOW"   , 0,  60},
  { NULL, "DSBOSPIT"   , 0,  70},
  { NULL, "DSBOSCUB"   , 0,  70},
  { NULL, "DSBOSSIT"   , 0,  70},
  { NULL, "DSBOSPN\0"  , 0,  70},
  { NULL, "DSBOSDTH"   , 0,  70},
  { NULL, "DSMANATK"   , 0,  70},

  { NULL, "DSMANDTH"   , 0,  70},
  { NULL, "DSSSSIT\0"  , 0,  70},
  { NULL, "DSSSDTH\0"  , 0,  70},
  { NULL, "DSKEENPN"   , 0,  70},
  { NULL, "DSKEENDT"   , 0,  70},
  { NULL, "DSSKEACT"   , 0,  70},
  { NULL, "DSSKESIT"   , 0,  70},
  { NULL, "DSSKEATK"   , 0,  70},
  { NULL, "DSRADIO\0"  , 0,  60}, //SKSRADIO},

  // legacy.wad extra sounds
  // sound when the player avatar jumps in air 'hmpf!'
  { NULL, "DSJUMP\0\0" , 0,  60}, //SKSJUMP},

  { NULL, "DSOUCH\0\0" , 0,  64}, //SKSOUCH}, // 110
  // water sounds
  { NULL, "DSGLOOP\0"  , 0,  60},
  { NULL, "DSSPLASH"   , 0,  64},
  { NULL, "DSFLOUSH"   , 0,  64},

  // Heretic
  { NULL, "GLDHIT",  0, 32},
  { NULL, "GNTFUL",  0, 32},
  { NULL, "GNTHIT",  0, 32},
  { NULL, "GNTPOW",  0, 32},
  { NULL, "GNTACT",  0, 32},
  { NULL, "GNTUSE",  0, 32},

  { NULL, "PHOSHT",  0, 32},
  { NULL, "PHOHIT",  0, 32},
  { NULL, "HEDAT1", 0, 32}, // "-phopow", &S_sfx[sfx_hedat1], -1, -1},
  { NULL, "LOBSHT",  0, 20},
  { NULL, "LOBHIT",  0, 20},
  { NULL, "LOBPOW",  0, 20},
  { NULL, "HRNSHT",  0, 32},
  { NULL, "HRNHIT",  0, 32},
  { NULL, "HRNPOW",  0, 32},
  { NULL, "RAMPHIT", 0, 32},

  { NULL, "RAMRAIN", 0, 10},
  { NULL, "BOWSHT",  0, 32},
  { NULL, "STFHIT",  0, 32},
  { NULL, "STFPOW",  0, 32},
  { NULL, "STFCRK",  0, 32},
  { NULL, "IMPSIT",  0, 32},
  { NULL, "IMPAT1",  0, 32},
  { NULL, "IMPAT2",  0, 32},
  { NULL, "IMPDTH",  0, 80},
  { NULL, "IMPSIT", 0, 20}, //"-impact", &S_sfx[sfx_impsit], -1, -1},

  { NULL, "IMPPAI",  0, 32},
  { NULL, "MUMSIT",  0, 32},
  { NULL, "MUMAT1",  0, 32},
  { NULL, "MUMAT2",  0, 32},
  { NULL, "MUMDTH",  0, 80},
  { NULL, "MUMSIT", 0, 20}, //"-mumact" &S_sfx[sfx_mumsit], -1, -1},
  { NULL, "MUMPAI",  0, 32},
  { NULL, "MUMHED",  0, 32},
  { "beast/sight", "BSTSIT",  0, 32},
  { NULL, "BSTATK",  0, 32},

  { NULL, "BSTDTH",  0, 80},
  { NULL, "BSTACT",  0, 20},
  { NULL, "BSTPAI",  0, 32},
  { NULL, "CLKSIT",  0, 32},
  { NULL, "CLKATK",  0, 32},
  { NULL, "CLKDTH",  0, 80},
  { NULL, "CLKACT",  0, 20},
  { NULL, "CLKPAI",  0, 32},
  { NULL, "SNKSIT",  0, 32},
  { NULL, "SNKATK",  0, 32},

  { NULL, "SNKDTH",  0, 80},
  { NULL, "SNKACT",  0, 20},
  { NULL, "SNKPAI",  0, 32},
  { NULL, "KGTSIT",  0, 32},
  { NULL, "KGTATK",  0, 32},
  { NULL, "KGTAT2",  0, 32},
  { NULL, "KGTDTH",  0, 80},
  { NULL, "KGTSIT", 0, 20}, // "-kgtact", &S_sfx[sfx_kgtsit], -1, -1},
  { NULL, "KGTPAI",  0, 32},
  { NULL, "WIZSIT",  0, 32},

  { NULL, "WIZATK",  0, 32},
  { NULL, "WIZDTH",  0, 80},
  { NULL, "WIZACT",  0, 20},
  { NULL, "WIZPAI",  0, 32},
  { NULL, "MINSIT",  0, 32},
  { NULL, "MINAT1",  0, 32},
  { NULL, "MINAT2",  0, 32},
  { NULL, "MINAT3",  0, 32},
  { NULL, "MINDTH",  0, 80},
  { NULL, "MINACT",  0, 20},

  { NULL, "MINPAI",  0, 32},
  { NULL, "HEDSIT",  0, 32},
  { NULL, "HEDAT1",  0, 32},
  { NULL, "HEDAT2",  0, 32},
  { NULL, "HEDAT3",  0, 32},
  { NULL, "HEDDTH",  0, 80},
  { NULL, "HEDACT",  0, 20},
  { NULL, "HEDPAI",  0, 32},
  { NULL, "SORZAP",  0, 32},
  { NULL, "SORRISE", 0, 32},

  { NULL, "SORSIT",  0, 200},
  { NULL, "SORATK",  0, 32},
  { NULL, "SORACT",  0, 200},
  { NULL, "SORPAI",  0, 200},
  { NULL, "SORDSPH", 0, 200},
  { NULL, "SORDEXP", 0, 200},
  { NULL, "SORDBON", 0, 200},
  { NULL, "BSTSIT", 0, 32}, // "-sbtsit", &S_sfx[sfx_bstsit], -1, -1},
  { NULL, "BSTATK", 0, 32}, // "-sbtatk", &S_sfx[sfx_bstatk], -1, -1},
  { NULL, "SBTDTH",  0, 80},

  { NULL, "SBTACT",  0, 20},
  { NULL, "SBTPAI",  0, 32},
  { NULL, "PLROOF",  0, 32},
  { NULL, "PLRPAI",  0, 32},
  { NULL, "PLRDTH",  0, 80},
  { NULL, "GIBDTH",  0, 100},
  { NULL, "PLRWDTH", 0, 80},
  { NULL, "PLRCDTH", 0, 100},
  { NULL, "ITEMUP",  0, 32},
  { NULL, "WPNUP",   0, 32},

  { NULL, "TELEPT",  0, 50},
  { NULL, "DOROPN",  0, 40},
  { NULL, "DORCLS",  0, 40},
  { NULL, "DORMOV",  0, 40},
  { NULL, "ARTIUP",  0, 32},
  { NULL, "SWITCH",  0, 40},
  { NULL, "PSTART",  0, 40},
  { NULL, "PSTOP",   0, 40},
  { NULL, "STNMOV",  0, 40},
  { NULL, "CHICPAI", 0, 32},

  { NULL, "CHICATK", 0, 32},
  { NULL, "CHICDTH", 0, 40},
  { NULL, "CHICACT", 0, 32},
  { NULL, "CHICPK1", 0, 32},
  { NULL, "CHICPK2", 0, 32},
  { NULL, "CHICPK3", 0, 32},
  { NULL, "KEYUP"  , 0, 50},
  { NULL, "RIPSLOP", 0, 16},
  { NULL, "NEWPOD" , 0, 16},
  { NULL, "PODEXP" , 0, 40},

  { NULL, "BOUNCE" , 0, 16},
  { NULL, "BSTATK", 0, 16}, // "-volsht", &S_sfx[sfx_bstatk], -1, -1},
  { NULL, "LOBHIT", 0, 16}, // "-volhit", &S_sfx[sfx_lobhit], -1, -1},
  { NULL, "BURN"   , 0, 10},
  { NULL, "SPLASH" , 0, 10},
  { NULL, "GLOOP"  , 0, 10},
  { NULL, "RESPAWN", 0, 10},
  { NULL, "BLSSHT" , 0, 32},
  { NULL, "BLSHIT" , 0, 32},
  { NULL, "CHAT"   , 0, 100},

  { NULL, "ARTIUSE", 0, 32},
  { NULL, "GFRAG"  , 0, 100},
  { NULL, "WATERFL", 0, 16},

  // Monophonic sounds
  { NULL, "wind"   , 0, 16},
  { "ambient/scream", "AMB1"   , 0,  1},
  { "ambient/squish", "AMB2"   , 0,  1},
  { "ambient/drops1", "AMB3"   , 0,  1},
  { "ambient/step1",  "AMB4"   , 0,  1},
  { "ambient/heartbeat", "AMB5"   , 0,  1},
  { "ambient/bells",  "AMB6"   , 0,  1},

  { "ambient/drops2", "AMB7"   , 0,  1},
  { "ambient/magic",  "AMB8"   , 0,  1},
  { "ambient/laugh1", "AMB9"   , 0,  1},
  { "ambient/laugh2", "AMB10"  , 0,  1},
  { "ambient/step2",  "AMB11"  , 0,  1},


  // Hexen
  // tagname, lumpname, multiplicity, priority
  { "PlayerFighterNormalDeath", "\0", 2, 256},
  { "PlayerFighterCrazyDeath", "\0", 2, 256},
  { "PlayerFighterExtreme1Death", "\0", 2, 256},
  { "PlayerFighterExtreme2Death", "\0", 2, 256},
  { "PlayerFighterExtreme3Death", "\0", 2, 256},

  { "PlayerFighterBurnDeath", "\0", 2, 256},
  { "PlayerClericNormalDeath", "\0", 2, 256},
  { "PlayerClericCrazyDeath", "\0", 2, 256},
  { "PlayerClericExtreme1Death", "\0", 2, 256},
  { "PlayerClericExtreme2Death", "\0", 2, 256},
  { "PlayerClericExtreme3Death", "\0", 2, 256},
  { "PlayerClericBurnDeath", "\0", 2, 256},
  { "PlayerMageNormalDeath", "\0", 2, 256},
  { "PlayerMageCrazyDeath", "\0", 2, 256},
  { "PlayerMageExtreme1Death", "\0", 2, 256},

  { "PlayerMageExtreme2Death", "\0", 2, 256},
  { "PlayerMageExtreme3Death", "\0", 2, 256},
  { "PlayerMageBurnDeath", "\0", 2, 256},
  { "PlayerFighterPain", "\0", 2, 256},
  { "PlayerClericPain", "\0", 2, 256},
  { "PlayerMagePain", "\0", 2, 256},
  { "PlayerFighterGrunt", "\0", 2, 256},
  { "PlayerClericGrunt", "\0", 2, 256},
  { "PlayerMageGrunt", "\0", 2, 256},
  { "PlayerLand", "\0", 2, 32},

  { "PlayerPoisonCough", "\0", 2, 256},
  { "PlayerFighterFallingScream", "\0", 2, 256},
  { "PlayerClericFallingScream", "\0", 2, 256},
  { "PlayerMageFallingScream", "\0", 2, 256},
  { "PlayerFallingSplat", "\0", 2, 256},
  { "PlayerFighterFailedUse", "\0", 1, 256},
  { "PlayerClericFailedUse", "\0", 1, 256},
  { "PlayerMageFailedUse", "\0", 1, 256},
  { "PlatformStart", "\0", 2, 36},
  { "PlatformStartMetal", "\0", 2, 36},

  { "PlatformStop", "\0", 2, 40},
  { "StoneMove", "\0", 2, 32},
  { "MetalMove", "\0", 2, 32},
  { "DoorOpen", "\0", 2, 36},
  { "DoorLocked", "\0", 2, 36},
  { "DoorOpenMetal", "\0", 2, 36},
  { "DoorCloseMetal", "\0", 2, 36},
  { "DoorCloseLight", "\0", 2, 36},
  { "DoorCloseHeavy", "\0", 2, 36},
  { "DoorCreak", "\0", 2, 36},

  { "PickupWeapon", "\0", 2, 36},
  { "PickupArtifact", "\0", 2, 36},
  { "PickupKey", "\0", 2, 36},
  { "PickupItem", "\0", 2, 36},
  { "PickupPiece", "\0", 2, 36},
  { "WeaponBuild", "\0", 2, 36},
  { "UseArtifact", "\0", 2, 36},
  { "BlastRadius", "\0", 2, 36},
  { "Teleport", "\0", 2, 256},
  { "ThunderCrash", "\0", 2, 30},

  { "FighterPunchMiss", "\0", 2, 80},
  { "FighterPunchHitThing", "\0", 2, 80},
  { "FighterPunchHitWall", "\0", 2, 80},
  { "FighterGrunt", "\0", 2, 80},
  { "FighterAxeHitThing", "\0", 2, 80},
  { "FighterHammerMiss", "\0", 2, 80},
  { "FighterHammerHitThing", "\0", 2, 80},
  { "FighterHammerHitWall", "\0", 2, 80},
  { "FighterHammerContinuous", "\0", 2, 32},
  { "FighterHammerExplode", "\0", 2, 80},

  { "FighterSwordFire", "\0", 2, 80},
  { "FighterSwordExplode", "\0", 2, 80},
  { "ClericCStaffFire", "\0", 2, 80},
  { "ClericCStaffExplode", "\0", 2, 40},
  { "ClericCStaffHitThing", "\0", 2, 80},
  { "ClericFlameFire", "\0", 2, 80},
  { "ClericFlameExplode", "\0", 2, 80},
  { "ClericFlameCircle", "\0", 2, 80},
  { "MageWandFire", "\0", 2, 80},
  { "MageLightningFire", "\0", 2, 80},

  { "MageLightningZap", "\0", 2, 32},
  { "MageLightningContinuous", "\0", 2, 32},
  { "MageLightningReady", "\0", 2, 30},
  { "MageShardsFire","\0", 2, 80},
  { "MageShardsExplode","\0", 2, 36},
  { "MageStaffFire","\0", 2, 80},
  { "MageStaffExplode","\0", 2, 40},
  { "Switch1", "\0", 2, 32},
  { "Switch2", "\0", 2, 32},
  { "SerpentSight", "\0", 2, 32},

  { "SerpentActive", "\0", 2, 32},
  { "SerpentPain", "\0", 2, 32},
  { "SerpentAttack", "\0", 2, 32},
  { "SerpentMeleeHit", "\0", 2, 32},
  { "SerpentDeath", "\0", 2, 40},
  { "SerpentBirth", "\0", 2, 32},
  { "SerpentFXContinuous", "\0", 2, 32},
  { "SerpentFXHit", "\0", 2, 32},
  { "PotteryExplode", "\0", 2, 32},
  { "Drip", "\0", 2, 32},

  { "CentaurSight", "\0", 2, 32},
  { "CentaurActive", "\0", 2, 32},
  { "CentaurPain", "\0", 2, 32},
  { "CentaurAttack", "\0", 2, 32},
  { "CentaurDeath", "\0", 2, 40},
  { "CentaurLeaderAttack", "\0", 2, 32},
  { "CentaurMissileExplode", "\0", 2, 32},
  { "Wind", "\0", 2, 1},
  { "BishopSight", "\0", 2, 32},
  { "BishopActive", "\0", 2, 32},

  { "BishopPain", "\0", 2, 32},
  { "BishopAttack", "\0", 2, 32},
  { "BishopDeath", "\0", 2, 40},
  { "BishopMissileExplode", "\0", 2, 32},
  { "BishopBlur", "\0", 2, 32},
  { "DemonSight", "\0", 2, 32},
  { "DemonActive", "\0", 2, 32},
  { "DemonPain", "\0", 2, 32},
  { "DemonAttack", "\0", 2, 32},
  { "DemonMissileFire", "\0", 2, 32},

  { "DemonMissileExplode", "\0", 2, 32},
  { "DemonDeath", "\0", 2, 40},
  { "WraithSight", "\0", 2, 32},
  { "WraithActive", "\0", 2, 32},
  { "WraithPain", "\0", 2, 32},
  { "WraithAttack", "\0", 2, 32},
  { "WraithMissileFire", "\0", 2, 32},
  { "WraithMissileExplode", "\0", 2, 32},
  { "WraithDeath", "\0", 2, 40},
  { "PigActive1", "\0", 2, 32},

  { "PigActive2", "\0", 2, 32},
  { "PigPain", "\0", 2, 32},
  { "PigAttack", "\0", 2, 32},
  { "PigDeath", "\0", 2, 40},
  { "MaulatorSight", "\0", 2, 32},
  { "MaulatorActive", "\0", 2, 32},
  { "MaulatorPain", "\0", 2, 32},
  { "MaulatorHamSwing", "\0", 2, 32},
  { "MaulatorHamHit", "\0", 2, 32},
  { "MaulatorMissileHit", "\0", 2, 32},

  { "MaulatorDeath", "\0", 2, 40},
  { "FreezeDeath", "\0", 2, 40},
  { "FreezeShatter", "\0", 2, 40},
  { "EttinSight", "\0", 2, 32},
  { "EttinActive", "\0", 2, 32},
  { "EttinPain", "\0", 2, 32},
  { "EttinAttack", "\0", 2, 32},
  { "EttinDeath", "\0", 2, 40},
  { "FireDemonSpawn", "\0", 2, 32},
  { "FireDemonActive", "\0", 2, 32},

  { "FireDemonPain", "\0", 2, 32},
  { "FireDemonAttack", "\0", 2, 32},
  { "FireDemonMissileHit", "\0", 2, 32},
  { "FireDemonDeath", "\0", 2, 40},
  { "IceGuySight", "\0", 2, 32},
  { "IceGuyActive", "\0", 2, 32},
  { "IceGuyAttack", "\0", 2, 32},
  { "IceGuyMissileExplode", "\0", 2, 32},
  { "SorcererSight", "\0", 2, 256},
  { "SorcererActive", "\0", 2, 256},

  { "SorcererPain", "\0", 2, 256},
  { "SorcererSpellCast", "\0", 2, 256},
  { "SorcererBallWoosh", "\0", 4, 256},
  { "SorcererDeathScream", "\0", 2, 256},
  { "SorcererBishopSpawn", "\0", 2, 80},
  { "SorcererBallPop", "\0", 2, 80},
  { "SorcererBallBounce", "\0", 3, 80},
  { "SorcererBallExplode", "\0", 3, 80},
  { "SorcererBigBallExplode", "\0", 3, 80},
  { "SorcererHeadScream", "\0", 2, 256},

  { "DragonSight", "\0", 2, 64},
  { "DragonActive", "\0", 2, 64},
  { "DragonWingflap", "\0", 2, 64},
  { "DragonAttack", "\0", 2, 64},
  { "DragonPain", "\0", 2, 64},
  { "DragonDeath", "\0", 2, 64},
  { "DragonFireballExplode", "\0", 2, 32},
  { "KoraxSight", "\0", 2, 256},
  { "KoraxActive", "\0", 2, 256},
  { "KoraxPain", "\0", 2, 256},

  { "KoraxAttack", "\0", 2, 256},
  { "KoraxCommand", "\0", 2, 256},
  { "KoraxDeath", "\0", 2, 256},
  { "KoraxStep", "\0", 2, 128},
  { "ThrustSpikeRaise", "\0", 2, 32},
  { "ThrustSpikeLower", "\0", 2, 32},
  { "GlassShatter", "\0", 2, 32},
  { "FlechetteBounce", "\0", 2, 32},
  { "FlechetteExplode", "\0", 2, 32},
  { "LavaMove", "\0", 2, 36},

  { "WaterMove", "\0", 2, 36},
  { "IceStartMove", "\0", 2, 36},
  { "EarthStartMove", "\0", 2, 36},
  { "WaterSplash", "\0", 2, 32},
  { "LavaSizzle", "\0", 2, 32},
  { "SludgeGloop", "\0", 2, 32},
  { "HolySymbolFire", "\0", 2, 64},
  { "SpiritActive", "\0", 2, 32},
  { "SpiritAttack", "\0", 2, 32},
  { "SpiritDie", "\0", 2, 32},

  { "ValveTurn", "\0", 2, 36},
  { "RopePull", "\0", 2, 36},
  { "FlyBuzz", "\0", 2, 20},
  { "Ignite", "\0", 2, 32},
  { "PuzzleSuccess", "\0", 2, 256},
  { "PuzzleFailFighter", "\0", 2, 256},
  { "PuzzleFailCleric", "\0", 2, 256},
  { "PuzzleFailMage", "\0", 2, 256},
  { "Earthquake", "\0", 2, 32},
  { "BellRing", "\0", 2, 32},

  { "TreeBreak", "\0", 2, 32},
  { "TreeExplode", "\0", 2, 32},
  { "SuitofArmorBreak", "\0", 2, 32},
  { "PoisonShroomPain", "\0", 2, 20},
  { "PoisonShroomDeath", "\0", 2, 32},
  { "Ambient1", "\0", 1, 1},
  { "Ambient2", "\0", 1, 1},
  { "Ambient3", "\0", 1, 1},
  { "Ambient4", "\0", 1, 1},
  { "Ambient5", "\0", 1, 1},

  { "Ambient6", "\0", 1, 1},
  { "Ambient7", "\0", 1, 1},
  { "Ambient8", "\0", 1, 1},
  { "Ambient9", "\0", 1, 1},
  { "Ambient10", "\0", 1, 1},
  { "Ambient11", "\0", 1, 1},
  { "Ambient12", "\0", 1, 1},
  { "Ambient13", "\0", 1, 1},
  { "Ambient14", "\0", 1, 1},
  { "Ambient15", "\0", 1, 1},

  { "StartupTick", "\0", 2, 32},
  { "SwitchOtherLevel", "\0", 2, 32},
  { "Respawn", "\0", 2, 32},
  { "KoraxVoiceGreetings", "\0", 1, 512}, // originally Korax speeches had multiplicity 2
  { "KoraxVoiceReady", "\0", 1, 512},
  { "KoraxVoiceBlood", "\0", 1, 512},
  { "KoraxVoiceGame", "\0", 1, 512},
  { "KoraxVoiceBoard", "\0", 1, 512},
  { "KoraxVoiceWorship", "\0", 1, 512},
  { "KoraxVoiceMaybe", "\0", 1, 512},

  { "KoraxVoiceStrong", "\0", 1, 512},
  { "KoraxVoiceFace", "\0", 1, 512},
  { "BatScream", "\0", 2, 32},
  { "Chat", "\0", 2, 512},
  { "MenuMove", "\0", 2, 32},
  { "ClockTick", "\0", 2, 32},
  { "Fireball", "\0", 2, 32},
  { "PuppyBeat", "\0", 2, 30},
  { "MysticIncant", "\0", 4, 32} // 498
};
*/


//=========================================================
//  Sound utilities
//=========================================================

static vector<sfxinfo_t*> SoundStore;  // keeps track of allocated sfx for deleting

static int  soundnumber = 1;
static byte def_multi = 0;
static byte def_priority = 64;
static int  def_pitch = 128;

// Sound mappings
map<const char*, sfxinfo_t*, less_cstring> SoundInfo; // tag => sound
map<int, sfxinfo_t*> SoundID;  // ID-number => sound

typedef map<const char*, sfxinfo_t*, less_cstring>::iterator sound_iter_t;
typedef map<int, sfxinfo_t*>::iterator soundID_iter_t;


sfxinfo_t::sfxinfo_t(const char *t, int n)
{
  strncpy(tag, t, S_TAGLEN);
  number = n;
  lumpname[0] = lumpname[8] = '\0';

  multiplicity = def_multi;
  priority = def_priority;
  pitch = def_pitch;
}


// deletes all SNDINFO/SNDSEQ data
void S_ClearSounds()
{
  // There is no reason to stop currently playing sounds,
  // because they should be independent of the definitions.

  extern map<int, struct sndseq_t*> SoundSeqs;
  map<int, sndseq_t*>::iterator t;
  for (t = SoundSeqs.begin(); t != SoundSeqs.end(); t++)
    delete t->second;
  SoundSeqs.clear();

  int n = SoundStore.size();
  for (int i=0; i<n; i++)
    delete SoundStore[i];

  SoundStore.clear();
  SoundInfo.clear();
  SoundID.clear();

  soundnumber = 1;
  def_multi = 0;
  def_priority = 64;
  def_pitch = 128;
}


int S_GetSoundID(const char *tag)
{
  sound_iter_t i = SoundInfo.find(tag);
  if (i == SoundInfo.end())
    {
      CONS_Printf("S_GetSoundID: Tag '%s' not found.\n", tag);
      return -1;
    }

  return i->second->number;
}


const char *S_GetSoundTag(int sfx_id)
{
  soundID_iter_t i = SoundID.find(sfx_id);
  if (i == SoundID.end())
    {
      CONS_Printf("S_GetSoundTag: ID %d not found!\n", sfx_id);
      return NULL;
    }

  return i->second->tag;
}



//======================================
//  SNDINFO parser
//======================================

enum SNDINFO_cmd_t
{
  SI_Map,
  SI_IFDoom,
  SI_IFHeretic,
  SI_IFHexen,
  SI_Multi,
  SI_Priority,
  SI_Pitch,
  SI_NUM
};

static const char *SNDINFO_cmds[SI_NUM + 1] =
{
  "$map",
  "$ifdoom",
  "$ifheretic",
  "$ifhexen",
  "$multiplicity",
  "$priority",
  "$pitch",
  NULL
};

// parses the SNDINFO lump
int S_Read_SNDINFO(int lump)
{
  Parser p;
  int i, n;
  char c;

  if (!p.Open(lump))
    return -1;

  CONS_Printf("Reading SNDINFO...\n");

  p.RemoveComments(';');
  while (p.NewLine())
    {
      char tag[50], lname[9];
      
      if (!p.GetString(tag, S_TAGLEN))
	{
	  CONS_Printf("Crap in the SNDINFO lump!\n");
	  continue;
	}

      if (tag[0] == '$')
	{
	  // it's a command.
	  switch (P_MatchString(tag, SNDINFO_cmds))
	    {
	    case SI_Map:
	      n = p.GetInt();
	      i = p.GetString(lname, 8);
	      if (i >= 1  && n >= 1 && n <= 99)
		{
		  // store the map music
		  strupr(lname);
		  MapInfo *t = game.FindMapInfo(n);
		  if (t)
		    t->musiclump = lname;
		}
	      break;
	    case SI_IFDoom:
	      if (game.mode == gm_heretic || game.mode == gm_hexen)
		p.GoToNext("$endif");
	      break;
	    case SI_IFHeretic:
	      if (game.mode != gm_heretic)
		p.GoToNext("$endif");
	      break;
	    case SI_IFHexen:
	      if (game.mode != gm_hexen)
		p.GoToNext("$endif");
	      break;
	    case SI_Multi:
	      def_multi = p.GetInt();
	      break;
	    case SI_Priority:
	      def_priority = p.GetInt();
	      break;
	    case SI_Pitch:
	      def_pitch = 128 + p.GetInt();
	      break;

	    default:
	      // $ARCHIVEPATH ignored
	      break;
	    }
	}
      else
	{
	  // must be a tagname mapping
	  // if tag is found, update, if not, create
	  sfxinfo_t *info;
	  sound_iter_t t = SoundInfo.find(tag);
	  if (t == SoundInfo.end())
	    {
	      info = new sfxinfo_t(tag, soundnumber++);
	      SoundStore.push_back(info); // to make deleting easy (the maps are many-to-one!)
	      SoundInfo[info->tag] = info; // NOTE that we need a static copy of the tag c-string for the map to work!
	      SoundID[info->number] = info;
	    }
	  else
	    info = t->second;

	  //CONS_Printf("  sound '%s'", tag);

	  if (!(n = p.GetString(tag, 30)))
	    {
	      CONS_Printf("SNDINFO: bad lumpname\n");
	      continue;
	    }

	  // TODO random sounds
	  if (n == 1 && tag[0] == '=')
	    {
	      // alias (hence no sound lumpname may be a plain "=")
	      p.GetString(tag, S_TAGLEN);
	      //CONS_Printf(" alias '%s', #%d\n", tag, info->number);

	      sfxinfo_t *al;
	      t = SoundInfo.find(tag); // is the alias already there?
	      if (t == SoundInfo.end())
		{
		  al = new sfxinfo_t(tag, -1); // 'al' only exists to provide static storage for the tag of the alias
		  SoundStore.push_back(al); // to make deleting easy
		}
	      else
		{
		  CONS_Printf("Warning: Alias sound '%s' already defined!\n", tag);
		  // it loses its tag mapping, but the number mapping remains.
		  al = t->second;
		}

	      SoundInfo[al->tag] = info; // info contains all the real data
	      continue;
	    }
	  else
	    {
	      // lumpname
	      if (n > 8)
		CONS_Printf("Warning: Long lumpname '%s'\n", tag);
	      //CONS_Printf(" => '%s', #%d\n", tag, info->number);
	      //strupr(tag);
	      strncpy(info->lumpname, tag, 8);
	    }

	  // read attributes
	  while (p.GetChar(&c))
	    switch (c)
	      {
	      case 'm':
		info->multiplicity = p.GetInt();
		break;
	      case 'r':
		info->priority = p.GetInt();
		break;
	      case 'p':
		info->pitch = 128 + p.GetInt();
		break;
	      default:
		CONS_Printf("Unknown sound attribute '%c'\n", c);
	      }
	}
    }

  CONS_Printf(" %d sounds found.\n", soundnumber - 1);
  return (soundnumber - 1);
}




//=========================================================
// wrappers
//=========================================================

// Starts some music with the music id found in sounds.h.
bool S_StartMusic(int m_id, bool loop)
{
  if (m_id > mus_None && m_id < NUMMUSIC)
    return S.StartMusic(MusicNames[m_id], loop);
  else
    I_Error("Bad music id: %d\n", m_id);

  return false;
}


int S_StartAmbSound(PlayerInfo *p, int sfx_id, float volume)
{
  // TEMPorary
  return S_StartLocalAmbSound(sfx_id, volume);
}

int S_StartLocalAmbSound(int sfx_id, float volume)
{
  if (!sfx_id)
    return -1; // no sound

#ifdef PARANOIA
  // check for bogus sound #
  if (sfx_id < 0 || sfx_id >= NUMSFX)
    {
      CONS_Printf("Bad sfx number: %d\n", sfx_id);
      return -1;
    }
#endif

#ifdef HW3SOUND
  if (hws_mode != HWS_DEFAULT_MODE)
    {
      volume += 0.1;
      if (volume > 1)
	volume = 1;
      HW3S_StartSoundTypeAtVolume(NULL, sfx_id, CT_AMBIENT, volume);
      return -1;
    }
#endif

  if (volume < 0.0)
    return -1;

  soundID_iter_t i = SoundID.find(sfx_id);
  if (i == SoundID.end())
    {
      CONS_Printf("Sound ID %d not found!\n", sfx_id);
      return -1;
    }

  return S.StartAmbSound(i->second, volume);
}


int S_StartSound(mappoint_t *m, int sfx_id, float vol)
{
  if (!sfx_id)
    return -1; // no sound

  // check for bogus sound #
  if (sfx_id < 0 || sfx_id >= NUMSFX)
    {
      CONS_Printf("Bad sfx number: %d\n", sfx_id);
      return -1;
    }

  soundsource_t s;
  s.isactor = false;
  s.mpt = m;

#ifdef HW3SOUND
  if (hws_mode != HWS_DEFAULT_MODE)
    HW3S_StartSound(NULL, sfx_id);
  else;
#endif

  soundID_iter_t i = SoundID.find(sfx_id);
  if (i == SoundID.end())
    {
      CONS_Printf("Sound ID %d not found!\n", sfx_id);
      return -1;
    }

  return S.Start3DSound(i->second, &s, vol);
}


int S_StartSound(Actor *a, int sfx_id, float vol)
{
  if (!sfx_id)
    return -1; // no sound

  // check for bogus sound #
  if (sfx_id < 0 || sfx_id >= NUMSFX)
    {
      CONS_Printf("Bad sfx number: %d\n", sfx_id);
      return -1;
    }

  soundsource_t s;
  s.isactor = true;
  s.act = a;

  /* FIXME skins are temporarily removed until a better system is made
    if (sfx->skinsound!=-1 && origin && origin->skin)
    {
    // it redirect player sound to the sound in the skin table
    sfx_id = ((skin_t *)origin->skin)->soundsid[sfx->skinsound];
    sfx    = &S_sfx[sfx_id];
    }
  */

#ifdef HW3SOUND
  if (hws_mode != HWS_DEFAULT_MODE)
    HW3S_StartSound(a, sfx_id);
  else;
#endif

  soundID_iter_t i = SoundID.find(sfx_id);
  if (i == SoundID.end())
    {
      CONS_Printf("Sound ID %d not found!\n", sfx_id);
      return -1;
    }

  return S.Start3DSound(i->second, &s, vol);
}



bool S_PlayingSound(Actor *origin, int sfx_id)
{
  return S.PlayingSound(origin);
}
