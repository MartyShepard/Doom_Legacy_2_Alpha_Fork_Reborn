// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2007 by DooM Legacy Team.
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
/// \brief DeHackEd/BEX/DECORATE mnemonics

#include "mnemonics.h"
#include "dstrings.h"
#include "g_actor.h"

//========================================================================
//     BEX mnemonics
//========================================================================


// Original numerical values for the flags from Doom. For old-style DeHackEd flag entries.
// The current counterparts of these flags reside still in the first flags word.
old_flag_t OriginalFlags[26] =
{
  {0x0001, MF_SPECIAL},   // Call TouchSpecialThing when touched.
  {0x0002, MF_SOLID},     // Blocks
  {0x0004, MF_SHOOTABLE}, // Can be hit
  {0x0008, MF_NOSECTOR},  // Don't link to sector (invisible but touchable)
  {0x0010, MF_NOBLOCKMAP},// Don't link to blockmap (inert but visible)
  {0x0020, MF_AMBUSH},    // Not to be activated by sound, deaf monster.
  {0x0040, 0}, // MF_JUSTHIT, almost useless
  {0x0080, 0}, // MF_JUSTATTACKED, even more useless
  {0x0100, MF_SPAWNCEILING}, // Spawned hanging from the ceiling
  {0x0200, MF_NOGRAVITY}, // Does not feel gravity
  {0x0400, MF_DROPOFF},   // Can jump/drop from high places
  {0x0800, MF_PICKUP},    // Can/will pick up items. (players) // useless?
  {0x1000, MF_NOCLIPLINE | MF_NOCLIPTHING}, // Does not clip against lines or Actors.
  {0x2000, 0}, // MF_SLIDE, completely unused
  {0x4000, MF_FLOAT},     // Active floater, can move freely in air (cacodemons etc.)
  {0x8000, 0}, // MF_TELEPORT, not used for any mobjtype
  {0x00010000, MF_MISSILE},   // Missile. Don't hit same species, explode on block.
  {0x00020000, MF_DROPPED},   // Dropped by a monster
  {0x00040000, MF_SHADOW},    // Partial invisibility (spectre). Makes targeting harder.
  {0x00080000, MF_NOBLOOD},   // Does not bleed when shot (furniture)
  {0x00100000, MF_CORPSE},    // Acts like a corpse, falls down stairs etc.
  {0x00200000, 0}, // MF_INFLOAT, useless?
  {0x00400000, MF_COUNTKILL}, // On kill, count towards intermission kill total.
  {0x00800000, MF_COUNTITEM}, // On pickup, count towards intermission item total.
  {0x01000000, 0}, // MF_SKULLFLY, useless?
  {0x02000000, MF_NOTDMATCH}, // Not spawned in DM (keycards etc.)
};


flag_mnemonic_t BEX_FlagMnemonics[] =
{
  {"NOSECTOR",     MF_NOSECTOR,     1}, // Don't link to sector (invisible but touchable)
  {"NOBLOCKMAP",   MF_NOBLOCKMAP,   1}, // Don't link to blockmap (inert but visible)
  {"SOLID",        MF_SOLID,        1}, // Blocks
  {"SHOOTABLE",    MF_SHOOTABLE,    1}, // Can be hit
  {"NOCLIP",       MF_NOCLIPLINE | MF_NOCLIPTHING, 1}, // Does not clip against lines or Actors.
  {"NOGRAVITY",    MF_NOGRAVITY,    1}, // Does not feel gravity
  {"PICKUP",       MF_PICKUP,       1}, // Can/will pick up items. (players)
  {"FLOAT",        MF_FLOAT,        1}, // Active floater, can move freely in air (cacodemons etc.)
  {"DROPOFF",      MF_DROPOFF,      1}, // Can jump/drop from high places
  {"AMBUSH",       MF_AMBUSH,       1}, // Not to be activated by sound, deaf monster.
  {"DONTSPLASH",   MF_NOSPLASH,     1}, // Does not cause splashes in liquid.
  {"SHADOW",       MF_SHADOW,       1}, // Partial invisibility (spectre). Makes targeting harder.
  {"NOBLOOD",      MF_NOBLOOD,      1}, // Does not bleed when shot (furniture)
  {"SPAWNCEILING", MF_SPAWNCEILING, 1}, // Spawned hanging from the ceiling
  {"SPAWNFLOAT",   MF_SPAWNFLOAT,   1}, // Spawned at random height
  {"NOTDMATCH",    MF_NOTDMATCH,    1}, // Not spawned in DM (keycards etc.)
  {"COUNTKILL",    MF_COUNTKILL,    1}, // On kill, count towards intermission kill total.
  {"COUNTITEM",    MF_COUNTITEM,    1}, // On pickup, count towards intermission item total.
  {"SPECIAL",      MF_SPECIAL,      1}, // Call TouchSpecialThing when touched.
  {"DROPPED",      MF_DROPPED,      1}, // Dropped by a monster
  {"MISSILE",      MF_MISSILE,      1}, // Missile. Don't hit same species, explode on block.
  {"CORPSE",       MF_CORPSE,       1}, // Acts like a corpse, falls down stairs etc.
  {"ISMONSTER",    MF_MONSTER,      1},

  // Heretic/Hexen/ZDoom additions
  {"LOWGRAVITY",     MF2_LOGRAV,        2}, // Experiences only 1/8 gravity
  {"WINDTHRUST",     MF2_WINDTHRUST,    2}, // Is affected by wind
  {"HERETICBOUNCE",  MF2_FLOORBOUNCE,   2}, // Bounces off the floor
  {"HEXENBOUNCE",    MF2_FULLBOUNCE,    2}, // Bounces off walls and floor
  {"SLIDESONWALLS",  MF2_SLIDE,         2}, // Slides against walls
  {"PUSHABLE",       MF2_PUSHABLE,      2}, // Can be pushed by other moving actors
  {"CANNOTPUSH",     MF2_CANNOTPUSH,    2}, // Cannot push other pushable actors
  {"FLOORHUGGER",    MF2_FLOORHUGGER,   2},
  {"CEILINGHUGGER",  MF2_CEILINGHUGGER, 2},
  {"DONTBLAST",      MF2_NONBLASTABLE,  2},
  {"QUICKTORETALIATE", MF2_QUICKTORETALIATE, 2},
  {"NOTARGET",       MF2_NOTARGET,      2}, // Will not be targeted by other monsters of same team (like Arch-Vile)
  {"FLOATBOB",       MF2_FLOATBOB,      2}, // Bobs up and down in the air (item)
  {"THRUGHOST",      MF2_THRUGHOST,     2}, // Will pass through ghosts (missile)
  {"RIPPER",         MF2_RIP,           2}, // Rips through solid targets (missile)
  {"CANPASS",        0,                 2}, // TODO inverted!  Can move over/under other Actors
  {"NOTELEPORT",     MF2_NOTELEPORT,    2}, // Does not teleport
  {"NONSHOOTABLE",   MF2_NONSHOOTABLE,  2}, // Transparent to MF_MISSILEs
  {"INVULNERABLE",   MF2_INVULNERABLE,  2}, // Does not take damage
  {"DORMANT",        MF2_DORMANT,       2}, // Cannot be damaged, is not noticed by seekers
  {"CANTLEAVEFLOORPIC", MF2_CANTLEAVEFLOORPIC, 2}, // Stays within a certain floor texture
  {"BOSS",           MF2_BOSS,          2}, // Is a major boss, not as easy to kill
  {"SEEKERMISSILE",  MF2_SEEKERMISSILE, 2}, // Is a seeker (for reflection)
  {"REFLECTIVE",     MF2_REFLECTIVE,    2}, // Reflects missiles
  {"FLOORCLIP",      MF2_FOOTCLIP,      2}, // Feet may be be clipped
  {"DONTDRAW",       MF2_DONTDRAW,      2}, // Invisible (does not generate a vissprite)
  {"NODAMAGETHRUST", MF2_NODMGTHRUST,   2}, // Does not thrust target when damaging        
  {"TELESTOMP",      MF2_TELESTOMP,     2}, // Can telefrag another Actor
  {"ACTIVATEIMPACT", MF2_IMPACT,        2}, // Can activate SPAC_IMPACT
  {"CANPUSHWALLS",   MF2_PUSHWALL,      2}, // Can activate SPAC_PUSH
  {"ACTIVATEMCROSS", MF2_MCROSS,        2}, // Can activate SPAC_MCROSS
  {"ACTIVATEPCROSS", MF2_PCROSS,        2}, // Can activate SPAC_PCROSS
  {NULL, 0, 0} // terminator
};


#define BEX_STR(x) {#x, TXT_ ## x},
string_mnemonic_t BEX_StringMnemonics[] =
{
  BEX_STR(GOTARMOR)
  BEX_STR(GOTMEGA)
  BEX_STR(GOTHTHBONUS)
  BEX_STR(GOTARMBONUS)
  BEX_STR(GOTSTIM)
  BEX_STR(GOTMEDINEED)
  BEX_STR(GOTMEDIKIT)
  BEX_STR(GOTSUPER)
  BEX_STR(GOTINVUL)
  BEX_STR(GOTBERSERK)
  BEX_STR(GOTINVIS)
  BEX_STR(GOTSUIT)
  BEX_STR(GOTMAP)
  BEX_STR(GOTVISOR)
  BEX_STR(GOTMSPHERE)
  BEX_STR(GOTCLIP)
  BEX_STR(GOTCLIPBOX)
  BEX_STR(GOTROCKET)
  BEX_STR(GOTROCKBOX)
  BEX_STR(GOTCELL)
  BEX_STR(GOTCELLBOX)
  BEX_STR(GOTSHELLS)
  BEX_STR(GOTSHELLBOX)
  BEX_STR(GOTBACKPACK)
  BEX_STR(GOTBFG9000)
  BEX_STR(GOTCHAINGUN)
  BEX_STR(GOTCHAINSAW)
  BEX_STR(GOTLAUNCHER)
  BEX_STR(GOTPLASMA)
  BEX_STR(GOTSHOTGUN)
  BEX_STR(GOTSHOTGUN2)

  BEX_STR(PD_BLUEO)
  BEX_STR(PD_YELLOWO)
  BEX_STR(PD_REDO)
  BEX_STR(PD_BLUEK)
  BEX_STR(PD_YELLOWK)
  BEX_STR(PD_REDK)

  BEX_STR(CC_ZOMBIE)
  BEX_STR(CC_SHOTGUN)
  BEX_STR(CC_HEAVY)
  BEX_STR(CC_IMP)
  BEX_STR(CC_DEMON)
  BEX_STR(CC_LOST)
  BEX_STR(CC_CACO)
  BEX_STR(CC_HELL)
  BEX_STR(CC_BARON)
  BEX_STR(CC_ARACH)
  BEX_STR(CC_PAIN)
  BEX_STR(CC_REVEN)
  BEX_STR(CC_MANCU)
  BEX_STR(CC_ARCH)
  BEX_STR(CC_SPIDER)
  BEX_STR(CC_CYBER)
  BEX_STR(CC_HERO)

  // Boom messages.
  BEX_STR(PD_BLUEC)
  BEX_STR(PD_YELLOWC)
  BEX_STR(PD_REDC)
  BEX_STR(PD_BLUES)
  BEX_STR(PD_YELLOWS)
  BEX_STR(PD_REDS)
  BEX_STR(PD_ANY)
  BEX_STR(PD_ALL3)
  BEX_STR(PD_ALL6)

  {NULL, -1}
};
#undef BEX_STR
