// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: d_items.h 447 2007-04-06 15:31:11Z smite-meister $
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
/// \brief Game items: keys, armor, artifacts, weapons, ammo.

#ifndef d_items_h
#define d_items_h 1

#include "doomdef.h"
#include "doomtype.h"
#include "info.h"


/// Key types
enum keycard_t
{
  // Hexen
  it_key_1 = 0x0001,
  it_key_2 = 0x0002,
  it_key_3 = 0x0004,
  it_key_4 = 0x0008,
  it_key_5 = 0x0010,
  it_key_6 = 0x0020,
  it_key_7 = 0x0040,
  it_key_8 = 0x0080,
  it_key_9 = 0x0100,
  it_key_A = 0x0200,
  it_key_B = 0x0400,

  // Doom and Heretic keys are merged
  it_bluecard   = 0x0800,
  it_yellowcard = 0x1000,
  it_redcard    = 0x2000,
  it_blueskull  = 0x4000,
  it_yellowskull= 0x8000,
  it_redskull   = 0x10000,

  NUMKEYS = 17,
  it_allkeys = 0x1ffff,
};


/// Armor locations
enum armortype_t
{
  armor_field, // old doom style armor
  armor_armor,
  armor_shield,
  armor_helmet,
  armor_amulet,
  NUMARMOR
};


/// Inventory artifact types
enum artitype_t
{
  arti_none,
  arti_invulnerability,
  arti_invisibility,
  arti_health,
  arti_superhealth,
  arti_tomeofpower,
  arti_torch,
  arti_firebomb,
  arti_egg,
  arti_fly,
  arti_teleport,

  // hexen
  arti_healingradius,
  arti_summon,
  arti_pork,
  arti_blastradius,
  arti_poisonbag,
  arti_teleportother,
  arti_speed,
  arti_boostmana,
  arti_boostarmor,
  arti_xinvulnerability,

  // Puzzle artifacts
  arti_firstpuzzitem,
  arti_puzzskull = arti_firstpuzzitem,
  arti_puzzgembig,
  arti_puzzgemred,
  arti_puzzgemgreen1,
  arti_puzzgemgreen2,
  arti_puzzgemblue1,
  arti_puzzgemblue2,
  arti_puzzbook1,
  arti_puzzbook2,
  arti_puzzskull2,
  arti_puzzfweapon,
  arti_puzzcweapon,
  arti_puzzmweapon,
  arti_puzzgear1,
  arti_puzzgear2,
  arti_puzzgear3,
  arti_puzzgear4,

  // weapon pieces
  arti_fsword1,
  arti_fsword2,
  arti_fsword3,
  arti_choly1,
  arti_choly2,
  arti_choly3,
  arti_mstaff1,
  arti_mstaff2,
  arti_mstaff3,

  NUMARTIFACTS
};


/// PlayerPawn inventory entry
struct inventory_t
{
  byte type;
  byte count;

  inventory_t();
  inventory_t(byte t, byte c);
};

#define NUMINVENTORYSLOTS  14
#define MAXARTECONT        16 


/// Powerups
enum powertype_t
{
  pw_invulnerability,
  pw_strength,
  pw_invisibility,
  pw_ironfeet,
  pw_allmap,
  pw_infrared,

  // heretic
  pw_weaponlevel2,
  pw_flight,
  
  // hexen
  pw_shield,
  pw_health2,
  pw_speed,
  pw_minotaur,

  NUMPOWERS
};


/// Powerup durations,
enum powerduration_t
{
  INVULNTICS  = (30*TICRATE),
  INVISTICS   = (60*TICRATE),
  INFRATICS   = (120*TICRATE),
  IRONTICS    = (60*TICRATE),
  WPNLEV2TICS = (40*TICRATE),
  FLIGHTTICS  = (60*TICRATE),
  MAULATORTICS = (25*TICRATE),
  SPEEDTICS   = (45*TICRATE)
};



/// Weapons
enum weapontype_t
{
  wp_none = -1,

  wp_fist = 0, wp_doom = wp_fist,
  wp_chainsaw,
  wp_pistol,
  wp_shotgun,
  wp_supershotgun,
  wp_chaingun,
  wp_missile,
  wp_plasma,
  wp_bfg,

  wp_staff, wp_heretic = wp_staff,
  wp_gauntlets,
  wp_goldwand,
  wp_crossbow,
  wp_blaster,
  wp_phoenixrod,
  wp_skullrod,
  wp_mace,
  wp_beak,

  wp_fpunch, wp_hexen = wp_fpunch,
  wp_cmace,
  wp_mwand,
  wp_timons_axe,
  wp_serpent_staff,
  wp_cone_of_shards,
  wp_hammer_of_retribution,
  wp_firestorm,
  wp_arc_of_death,
  wp_quietus,
  wp_wraithverge,
  wp_bloodscourge,
  wp_snout,

  NUMWEAPONS,
  wp_barrel    // barrel explosion
};


/// Ammunition types
enum ammotype_t
{
  am_noammo = -1, // unlimited

  am_clip = 0, am_doom = am_clip, // Pistol / chaingun ammo.
  am_shell,   // Shotgun / double barreled shotgun.
  am_cell,    // Plasma rifle, BFG.
  am_misl,    // Missile launcher.

  am_goldwand, am_heretic = am_goldwand,
  am_crossbow,
  am_blaster,
  am_skullrod,
  am_phoenixrod,
  am_mace,

  // Hexen
  am_mana1, am_hexen = am_mana1,
  am_mana2,

  NUMAMMO,
  am_manaboth  // both mana types
};

/// Ammo limits for players
extern int maxammo1[NUMAMMO]; ///< without backpack
extern int maxammo2[NUMAMMO]; ///< with backpack


/// \brief Volatile weapon info (changes with tome of power etc...)
struct weaponinfo_t
{
  ammotype_t ammo;               ///< Type of ammo it uses.
  int        ammopershoot;       ///< How much ammo it uses per shot.
  weaponstatenum_t upstate;      ///< Being drawn.
  weaponstatenum_t downstate;    ///< Being put away.
  weaponstatenum_t readystate;   ///< Ready to be fired.
  weaponstatenum_t atkstate;     ///< Fire.
  weaponstatenum_t holdatkstate; ///< Continuous fire.
  weaponstatenum_t flashstate;   ///< Muzzle flash.
};

extern weaponinfo_t wpnlev1info[NUMWEAPONS];
extern weaponinfo_t wpnlev2info[NUMWEAPONS];


/// Weapon grouping
struct weapon_group_t
{
  int group;   // in which group it resides
  weapontype_t next; // next weapon in group
};

extern weapon_group_t weapongroup[NUMWEAPONS];

#endif
