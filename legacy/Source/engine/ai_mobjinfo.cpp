// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: ai_mobjinfo.cpp 570 2009-11-27 23:57:47Z smite-meister $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1993-1996 by Raven Software, Corp.
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
/// \brief The mobjinfo array.
/// Defines the basic properties of different types of mapthings.
/// Holds all information needed to spawn a DActor of a specific type.

#include "info.h"
#include "g_actor.h"  // flag definitions
#include "sounds.h"
#include "p_fab.h" // hacks

// often-used flag combos
#define FLAGS_missile      (MF_MISSILE | MF_DROPOFF | MF_NOBLOCKMAP | MF_NOGRAVITY)
#define FLAGS_gravmissile  (MF_MISSILE | MF_DROPOFF | MF_NOBLOCKMAP)
#define FLAGS_monster_nocount (MF_MONSTER | MF_SOLID | MF_SHOOTABLE)
#define FLAGS_monster      (FLAGS_monster_nocount | MF_COUNTKILL)

#define FLAGS_floater      (MF_FLOAT | MF_DROPOFF | MF_NOGRAVITY)
#define FLAGS_breakable    (MF_SOLID | MF_SHOOTABLE | MF_NOBLOOD)

#define FLAGS2_missile     (MF2_NOTELEPORT | MF2_IMPACT | MF2_PCROSS)
#define FLAGS2_missile_nt  (MF2_NOTELEPORT)
#define FLAGS2_monster     (MF2_PUSHWALL | MF2_MCROSS)

#define INF_MASS 1e10


mobjinfo_t mobjinfo[NUMMOBJTYPES] =
{
  //============================================================================
  // Doom mapthings
  //============================================================================

  // MT_PLAYER
  {
    -1, 100, 0, 255, 8.33, 16, 56, 100, 0,
    MF_SOLID|MF_SHOOTABLE|MF_DROPOFF|MF_PICKUP|MF_NOTDMATCH,
    MF2_WINDTHRUST | MF2_FOOTCLIP | MF2_SLIDE | MF2_TELESTOMP,
    sfx_None, sfx_None, sfx_plpain, sfx_pldeth, sfx_None,
    &states[S_PLAY], &states[S_PLAY_RUN1], &states[S_PLAY_ATK1], &states[S_PLAY_ATK2], &states[S_PLAY_PAIN],
    &states[S_PLAY_DIE1], &states[S_PLAY_XDIE1], NULL, NULL,
    NULL,
    "DoomPlayer"
  },

  // MT_POSSESSED
  {
    3004, 20, 8, 200, 8, 20, 56, 100, 0,
    FLAGS_monster,
    FLAGS2_monster,
    sfx_posit1, sfx_pistol, sfx_popain, sfx_podth1, sfx_posact,
    &states[S_POSS_STND], &states[S_POSS_RUN1], NULL, &states[S_POSS_ATK1], &states[S_POSS_PAIN],
    &states[S_POSS_DIE1], &states[S_POSS_XDIE1], NULL, &states[S_POSS_RAISE1],
    NULL,
    "ZombieMan"
  },

  // MT_SHOTGUY
  {
    9, 30, 8, 170, 8, 20, 56, 100, 0,
    FLAGS_monster,
    FLAGS2_monster,
    sfx_posit2, 0, sfx_popain, sfx_podth2, sfx_posact,
    &states[S_SPOS_STND], &states[S_SPOS_RUN1], NULL, &states[S_SPOS_ATK1], &states[S_SPOS_PAIN],
    &states[S_SPOS_DIE1], &states[S_SPOS_XDIE1], NULL, &states[S_SPOS_RAISE1],
    NULL,
    "ShotgunGuy"
  },

  // MT_VILE
  {
    64, 700, 8, 10, 15, 20, 56, 500, 0,
    FLAGS_monster,
    FLAGS2_monster | MF2_NOTARGET,
    sfx_vilsit, 0, sfx_vipain, sfx_vildth, sfx_vilact,
    &states[S_VILE_STND], &states[S_VILE_RUN1], NULL, &states[S_VILE_ATK1], &states[S_VILE_PAIN],
    &states[S_VILE_DIE1], NULL, NULL, NULL,
    NULL,
    "Archvile"
  },

  // MT_FIRE
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_FIRE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "ArchvileFire"
  },

  // MT_UNDEAD
  {
    66, 300, 8, 100, 10, 20, 56, 500, 0,
    FLAGS_monster,
    FLAGS2_monster,
    sfx_skesit, 0, sfx_popain, sfx_skedth, sfx_skeact,
    &states[S_SKEL_STND], &states[S_SKEL_RUN1], &states[S_SKEL_FIST1], &states[S_SKEL_MISS1], &states[S_SKEL_PAIN],
    &states[S_SKEL_DIE1], NULL, NULL, &states[S_SKEL_RAISE1],
    NULL,
    "Revenant"
  },

  // MT_TRACER
  {
    -1, 1000, 8, 0, 10, 11, 8, 100, 10,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_skeatk, sfx_None, sfx_None, sfx_barexp, sfx_None,
    &states[S_TRACER], NULL, NULL, NULL, NULL,
    &states[S_TRACEEXP1], NULL, NULL, NULL,
    NULL,
    "RevenantTracer"
  },

  // MT_SMOKE
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SMOKE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "RevenantTracerSmoke"
  },

  // MT_FATSO
  {
    67, 600, 8, 80, 8, 48, 64, 1000, 0,
    FLAGS_monster,
    FLAGS2_monster,
    sfx_mansit, 0, sfx_mnpain, sfx_mandth, sfx_posact,
    &states[S_FATT_STND], &states[S_FATT_RUN1], NULL, &states[S_FATT_ATK1], &states[S_FATT_PAIN],
    &states[S_FATT_DIE1], NULL, NULL, &states[S_FATT_RAISE1],
    NULL,
    "Fatso"
  },

  // MT_FATSHOT
  {
    -1, 1000, 8, 0, 20, 6, 8, 100, 8 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_firsht, sfx_None, sfx_None, sfx_firxpl, sfx_None,
    &states[S_FATSHOT1], NULL, NULL, NULL, NULL,
    &states[S_FATSHOTX1], NULL, NULL, NULL,
    NULL,
    "FatShot"
  },

  // MT_CHAINGUY
  {
    65, 70, 8, 170, 8, 20, 56, 100, 0,
    FLAGS_monster,
    FLAGS2_monster,
    sfx_posit2, 0, sfx_popain, sfx_podth2, sfx_posact,
    &states[S_CPOS_STND], &states[S_CPOS_RUN1], NULL, &states[S_CPOS_ATK1], &states[S_CPOS_PAIN],
    &states[S_CPOS_DIE1], &states[S_CPOS_XDIE1], NULL, &states[S_CPOS_RAISE1],
    NULL,
    "ChaingunGuy"
  },

  // MT_TROOP
  {
    3001, 60, 8, 200, 8, 20, 56, 100, 0,
    FLAGS_monster,
    FLAGS2_monster,
    sfx_bgsit1, 0, sfx_popain, sfx_bgdth1, sfx_bgact,
    &states[S_TROO_STND], &states[S_TROO_RUN1], &states[S_TROO_ATK1], &states[S_TROO_ATK1], &states[S_TROO_PAIN],
    &states[S_TROO_DIE1], &states[S_TROO_XDIE1], NULL, &states[S_TROO_RAISE1],
    NULL,
    "DoomImp"
  },

  // MT_SERGEANT
  {
    3002, 150, 8, 180, 10, 30, 56, 400, 0,
    FLAGS_monster,
    FLAGS2_monster,
    sfx_sgtsit, sfx_sgtatk, sfx_dmpain, sfx_sgtdth, sfx_dmact,
    &states[S_SARG_STND], &states[S_SARG_RUN1], &states[S_SARG_ATK1], NULL, &states[S_SARG_PAIN],
    &states[S_SARG_DIE1], NULL, NULL, &states[S_SARG_RAISE1],
    NULL,
    "Demon"
  },

  // MT_SHADOWS
  {
    58, 150, 8, 180, 10, 30, 56, 400, 0,
    FLAGS_monster | MF_SHADOW,
    FLAGS2_monster,
    sfx_sgtsit, sfx_sgtatk, sfx_dmpain, sfx_sgtdth, sfx_dmact,
    &states[S_SARG_STND], &states[S_SARG_RUN1], &states[S_SARG_ATK1], NULL, &states[S_SARG_PAIN],
    &states[S_SARG_DIE1], NULL, NULL, &states[S_SARG_RAISE1],
    NULL,
    "Spectre"
  },

  // MT_HEAD
  {
    3005, 400, 8, 128, 8, 31, 56, 400, 0,
    FLAGS_monster | FLAGS_floater,
    FLAGS2_monster,
    sfx_cacsit, 0, sfx_dmpain, sfx_cacdth, sfx_dmact,
    &states[S_HEAD_STND], &states[S_HEAD_RUN1], NULL, &states[S_HEAD_ATK1], &states[S_HEAD_PAIN],
    &states[S_HEAD_DIE1], NULL, NULL, &states[S_HEAD_RAISE1],
    NULL,
    "Cacodemon"
  },

  // MT_BRUISER
  {
    3003, 1000, 8, 50, 8, 24, 64, 1000, 0,
    FLAGS_monster,
    FLAGS2_monster,
    sfx_brssit, 0, sfx_dmpain, sfx_brsdth, sfx_dmact,
    &states[S_BOSS_STND], &states[S_BOSS_RUN1], &states[S_BOSS_ATK1], &states[S_BOSS_ATK1], &states[S_BOSS_PAIN],
    &states[S_BOSS_DIE1], NULL, NULL, &states[S_BOSS_RAISE1],
    NULL,
    "BaronOfHell"
  },

  // MT_BRUISERSHOT
  {
    -1, 1000, 8, 0, 15, 6, 8, 100, 8 | dt_magic,
    FLAGS_missile,
    FLAGS2_missile_nt,
    sfx_firsht, sfx_None, sfx_None, sfx_firxpl, sfx_None,
    &states[S_BRBALL1], NULL, NULL, NULL, NULL,
    &states[S_BRBALLX1], NULL, NULL, NULL,
    NULL,
    "BaronBall"
  },

  // MT_KNIGHT
  {
    69, 500, 8, 50, 8, 24, 64, 1000, 0,
    FLAGS_monster,
    FLAGS2_monster,
    sfx_kntsit, 0, sfx_dmpain, sfx_kntdth, sfx_dmact,
    &states[S_BOS2_STND], &states[S_BOS2_RUN1], &states[S_BOS2_ATK1], &states[S_BOS2_ATK1], &states[S_BOS2_PAIN],
    &states[S_BOS2_DIE1], NULL, NULL, &states[S_BOS2_RAISE1],
    NULL,
    "HellKnight"
  },

  // MT_SKULL
  {
    3006, 100, 8, 256, 8, 16, 56, 50, 3,
    FLAGS_monster_nocount | FLAGS_floater,
    FLAGS2_monster,
    0, sfx_sklatk, sfx_dmpain, sfx_firxpl, sfx_dmact,
    &states[S_SKULL_STND], &states[S_SKULL_RUN1], NULL, &states[S_SKULL_ATK1], &states[S_SKULL_PAIN],
    &states[S_SKULL_DIE1], NULL, NULL, NULL,
    NULL,
    "LostSoul"
  },

  // MT_SPIDER
  {
    7, 3000, 8, 40, 12, 128, 100, 1000, 0,
    FLAGS_monster,
    FLAGS2_monster | MF2_BOSS,
    sfx_spisit, sfx_shotgn, sfx_dmpain, sfx_spidth, sfx_dmact,
    &states[S_SPID_STND], &states[S_SPID_RUN1], NULL, &states[S_SPID_ATK1], &states[S_SPID_PAIN],
    &states[S_SPID_DIE1], NULL, NULL, NULL,
    NULL,
    "SpiderMastermind"
  },

  // MT_BABY
  {
    68, 500, 8, 128, 12, 64, 64, 600, 0,
    FLAGS_monster,
    FLAGS2_monster,
    sfx_bspsit, 0, sfx_dmpain, sfx_bspdth, sfx_bspact,
    &states[S_BSPI_STND], &states[S_BSPI_SIGHT], NULL, &states[S_BSPI_ATK1], &states[S_BSPI_PAIN],
    &states[S_BSPI_DIE1], NULL, NULL, &states[S_BSPI_RAISE1],
    NULL,
    "Arachnotron"
  },

  // MT_CYBORG
  {
    16, 4000, 8, 20, 16, 40, 110, 1000, 0,
    FLAGS_monster,
    FLAGS2_monster | MF2_BOSS,
    sfx_cybsit, 0, sfx_dmpain, sfx_cybdth, sfx_dmact,
    &states[S_CYBER_STND], &states[S_CYBER_RUN1], NULL, &states[S_CYBER_ATK1], &states[S_CYBER_PAIN],
    &states[S_CYBER_DIE1], NULL, NULL, NULL,
    NULL,
    "Cyberdemon"
  },

  // MT_PAIN
  {
    71, 400, 8, 128, 8, 31, 56, 400, 0,
    FLAGS_monster | FLAGS_floater,
    FLAGS2_monster,
    sfx_pesit, 0, sfx_pepain, sfx_pedth, sfx_dmact,
    &states[S_PAIN_STND], &states[S_PAIN_RUN1], NULL, &states[S_PAIN_ATK1], &states[S_PAIN_PAIN],
    &states[S_PAIN_DIE1], NULL, NULL, &states[S_PAIN_RAISE1],
    NULL,
    "PainElemental"
  },

  // MT_WOLFSS
  {
    84, 50, 8, 170, 8, 20, 56, 100, 0,
    FLAGS_monster,
    FLAGS2_monster,
    sfx_sssit, 0, sfx_popain, sfx_ssdth, sfx_posact,
    &states[S_SSWV_STND], &states[S_SSWV_RUN1], NULL, &states[S_SSWV_ATK1], &states[S_SSWV_PAIN],
    &states[S_SSWV_DIE1], &states[S_SSWV_XDIE1], NULL, &states[S_SSWV_RAISE1],
    NULL,
    "WolfensteinSS"
  },

  // MT_KEEN
  {
    72, 100, 8, 256, 0, 16, 72, 10000000, 0,
    FLAGS_monster | MF_SPAWNCEILING | MF_NOGRAVITY,
    FLAGS2_monster,
    sfx_None, sfx_None, sfx_keenpn, sfx_keendt, sfx_None,
    &states[S_KEENSTND], NULL, NULL, NULL, &states[S_KEENPAIN],
    &states[S_COMMKEEN], NULL, NULL, NULL,
    NULL,
    "CommanderKeen"
  },

  // MT_BOSSBRAIN
  {
    88, 250, 8, 255, 0, 16, 64, 10000000, 0,
    FLAGS_monster_nocount,
    0,
    sfx_None, sfx_None, sfx_bospn, sfx_bosdth, sfx_None,
    &states[S_BRAIN], NULL, NULL, NULL, &states[S_BRAIN_PAIN],
    &states[S_BRAIN_DIE1], NULL, NULL, NULL,
    NULL,
    "BossBrain"
  },

  // MT_BOSSSPIT
  {
    89, 1000, 8, 0, 0, 20, 32, 100, 0,
    MF_NOBLOCKMAP|MF_NOSECTOR,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BRAINEYE], &states[S_BRAINEYESEE], NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "BossEye"
  },

  // MT_BOSSTARGET
  {
    87, 1000, 8, 0, 0, 20, 32, 100, 0,
    MF_NOBLOCKMAP|MF_NOSECTOR,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_NULL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "BossTarget"
  },

  // MT_SPAWNSHOT
  {
    -1, 1000, 8, 0, 10, 6, 32, 100, 3,
    FLAGS_missile|MF_NOCLIPLINE|MF_NOCLIPTHING,
    FLAGS2_missile,
    sfx_bospit, sfx_None, sfx_None, sfx_firxpl, sfx_None,
    &states[S_SPAWN1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "SpawnShot"
  },

  // MT_SPAWNFIRE
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SPAWNFIRE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "SpawnFire"
  },

  // MT_BARREL
  {
    2035, 20, 8, 0, 0, 10, 32, 100, 0,
    FLAGS_breakable,
    0,
    sfx_None, sfx_None, sfx_None, sfx_barexp, sfx_None,
    &states[S_BAR1], NULL, NULL, NULL, NULL,
    &states[S_BEXP], NULL, NULL, NULL,
    NULL,
    "ExplosiveBarrel"
  },

  // MT_TROOPSHOT
  {
    -1, 1000, 8, 0, 10, 6, 8, 100, 3 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile_nt,
    sfx_firsht, sfx_None, sfx_None, sfx_firxpl, sfx_None,
    &states[S_TBALL1], NULL, NULL, NULL, NULL,
    &states[S_TBALLX1], NULL, NULL, NULL,
    NULL,
    "DoomImpBall"
  },

  // MT_HEADSHOT
  {
    -1, 1000, 8, 0, 10, 6, 8, 100, 5 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile_nt,
    sfx_firsht, sfx_None, sfx_None, sfx_firxpl, sfx_None,
    &states[S_RBALL1], NULL, NULL, NULL, NULL,
    &states[S_RBALLX1], NULL, NULL, NULL,
    NULL,
    "CacodemonBall"
  },

  // MT_ROCKET
  {
    -1, 1000, 8, 0, 20, 11, 8, 100, 20 | dt_concussion,
    FLAGS_missile,
    FLAGS2_missile_nt,
    sfx_rlaunc, sfx_None, sfx_None, sfx_barexp, sfx_None,
    &states[S_ROCKET], NULL, NULL, NULL, NULL,
    &states[S_EXPLODE1], NULL, NULL, NULL,
    NULL,
    "Rocket"
  },

  // MT_PLASMA
  {
    -1, 1000, 8, 0, 25, 13, 8, 100, 5 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile_nt,
    sfx_plasma, sfx_None, sfx_None, sfx_firxpl, sfx_None,
    &states[S_PLASBALL], NULL, NULL, NULL, NULL,
    &states[S_PLASEXP], NULL, NULL, NULL,
    NULL,
    "PlasmaBall"
  },

  // MT_BFG
  {
    -1, 1000, 8, 0, 25, 13, 8, 100, 100 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile_nt,
    sfx_None, sfx_None, sfx_None, sfx_rxplod, sfx_None,
    &states[S_BFGSHOT], NULL, NULL, NULL, NULL,
    &states[S_BFGLAND], NULL, NULL, NULL,
    NULL,
    "BFGBall"
  },

  // MT_ARACHPLAZ
  {
    -1, 1000, 8, 0, 25, 13, 8, 100, 5 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_plasma, sfx_None, sfx_None, sfx_firxpl, sfx_None,
    &states[S_ARACH_PLAZ], NULL, NULL, NULL, NULL,
    &states[S_ARACH_PLEX], NULL, NULL, NULL,
    NULL,
    "ArachnotronPlasma"
  },

  // MT_PUFF
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPLINE|MF_NOCLIPTHING,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_PUFF1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "BulletPuff"
  },

  //WARNING: this mobj is hacked in g_downgrade (g_game.c)
  // MT_BLOOD
  {
    -1, 1000, 8, 0, 0, 3, 0, 100, 0,
    MF_NOSPLASH,  // | MF_NOBLOCKMAP
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BLOOD1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "Blood"
  },

  // MT_TFOG
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TFOG], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "TeleportFog"
  },

  // MT_IFOG
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_IFOG], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "ItemFog"
  },

  // MT_TELEPORTMAN
  {
    14, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    MF2_DONTDRAW,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_NULL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "TeleportDest"
  },

  // MT_EXTRABFG
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BFGEXP], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "BFGExtra"
  },

  // MT_MISC0
  {
    2018, 100, 8, 0, 0.333, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARM1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC1
  {
    2019, 200, 8, 0, 0.5, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARM2], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC2
  {
    2014, 1, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_COUNTITEM,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BON1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC3
  {
    2015, 1, 8, 0, 0.333, 20, 16, 100, 0,
    MF_SPECIAL|MF_COUNTITEM,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BON2], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC4
  {
    5, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOTDMATCH,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BKEY], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC5
  {
    13, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOTDMATCH,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_RKEY], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC6
  {
    6, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOTDMATCH,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_YKEY], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC7
  {
    39, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOTDMATCH,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_YSKULL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC8
  {
    38, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOTDMATCH,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_RSKULL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC9
  {
    40, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOTDMATCH,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BSKULL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC10
  {
    2011, 10, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_STIM], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC11
  {
    2012, 25, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MEDI], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC12
  {
    2013, 100, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_COUNTITEM,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SOUL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_INV
  {
    2022, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_COUNTITEM | MF_NORESPAWN,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_PINV], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC13
  {
    2023, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_COUNTITEM,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_PSTR], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_INS
  {
    2024, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_COUNTITEM | MF_NORESPAWN,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_PINS], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC14
  {
    2025, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SUIT], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC15
  {
    2026, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_COUNTITEM,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_PMAP], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC16
  {
    2045, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_COUNTITEM,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_PVIS], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MEGA
  {
    83, 200, 8, 0, 0.5, 20, 16, 100, 0,
    MF_SPECIAL|MF_COUNTITEM,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MEGA], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_CLIP
  {
    2007, 10, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_CLIP], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "Clip"
  },

  // MT_AMMOBOX
  {
    2048, 50, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_AMMO], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "ClipBox"
  },

  // MT_ROCKETAMMO
  {
    2010, 1, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ROCK], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "RocketAmmo"
  },

  // MT_ROCKETBOX
  {
    2046, 5, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BROK], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "RocketBox"
  },

  // MT_CELL
  {
    2047, 20, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_CELL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "Cell"
  },

  // MT_CELLPACK
  {
    17, 100, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_CELP], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "CellPack"
  },

  // MT_SHELL
  {
    2008, 4, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SHEL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "Shell"
  },

  // MT_SHELLBOX
  {
    2049, 20, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SBOX], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "ShellBox"
  },

  // MT_BACKPACK
  {
    8, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BPAK], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "Backpack"
  },

  // MT_BFG9000
  {
    2006, 40, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BFUG], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "BFG9000"
  },

  // MT_CHAINGUN
  {
    2002, 20, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MGUN], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "Chaingun"
  },

  // MT_SHAINSAW
  {
    2005, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_CSAW], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "Chainsaw"
  },

  // MT_ROCKETLAUNCH
  {
    2003, 2, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_LAUN], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "RocketLauncher"
  },

  // MT_PLASMAGUN
  {
    2004, 40, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_PLAS], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "PlasmaRifle"
  },

  // MT_SHOTGUN
  {
    2001, 8, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SHOT], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "Shotgun"
  },

  // MT_SUPERSHOTGUN
  {
    82, 8, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SHOT2], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "SuperShotgun"
  },

  // MT_MISC29
  {
    85, 1000, 8, 0, 0, 16, 72, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TECHLAMP], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC30
  {
    86, 1000, 8, 0, 0, 16, 56, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TECH2LAMP], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC31
  {
    2028, 1000, 8, 0, 0, 16, 48, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_COLU], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC32
  {
    30, 1000, 8, 0, 0, 16, 52, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TALLGRNCOL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC33
  {
    31, 1000, 8, 0, 0, 16, 40, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SHRTGRNCOL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC34
  {
    32, 1000, 8, 0, 0, 16, 52, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TALLREDCOL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC35
  {
    33, 1000, 8, 0, 0, 16, 40, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SHRTREDCOL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC36
  {
    37, 1000, 8, 0, 0, 16, 40, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SKULLCOL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC37
  {
    36, 1000, 8, 0, 0, 16, 40, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HEARTCOL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC38
  {
    41, 1000, 8, 0, 0, 16, 16, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_EVILEYE], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC39
  {
    42, 1000, 8, 0, 0, 16, 48, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_FLOATSKULL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC40
  {
    43, 1000, 8, 0, 0, 16, 64, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TORCHTREE], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC41
  {
    44, 1000, 8, 0, 0, 16, 64, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BLUETORCH], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC42
  {
    45, 1000, 8, 0, 0, 16, 64, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_GREENTORCH], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC43
  {
    46, 1000, 8, 0, 0, 16, 64, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_REDTORCH], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC44
  {
    55, 1000, 8, 0, 0, 16, 40, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BTORCHSHRT], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC45
  {
    56, 1000, 8, 0, 0, 16, 40, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_GTORCHSHRT], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC46
  {
    57, 1000, 8, 0, 0, 16, 40, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_RTORCHSHRT], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC47
  {
    47, 1000, 8, 0, 0, 16, 40, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_STALAGTITE], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC48
  {
    48, 1000, 8, 0, 0, 16, 120, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TECHPILLAR], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC49
  {
    34, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_CANDLESTIK], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC50
  {
    35, 1000, 8, 0, 0, 16, 56, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_CANDELABRA], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC51
  {
    49, 1000, 8, 0, 0, 16, 68, 100, 0,
    MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BLOODYTWITCH], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC52
  {
    50, 1000, 8, 0, 0, 16, 84, 100, 0,
    MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MEAT2], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC53
  {
    51, 1000, 8, 0, 0, 16, 84, 100, 0,
    MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MEAT3], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC54
  {
    52, 1000, 8, 0, 0, 16, 68, 100, 0,
    MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MEAT4], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC55
  {
    53, 1000, 8, 0, 0, 16, 52, 100, 0,
    MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MEAT5], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC56
  {
    59, 1000, 8, 0, 0, 20, 84, 100, 0,
    MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MEAT2], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC57
  {
    60, 1000, 8, 0, 0, 20, 68, 100, 0,
    MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MEAT4], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC58
  {
    61, 1000, 8, 0, 0, 20, 52, 100, 0,
    MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MEAT3], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC59
  {
    62, 1000, 8, 0, 0, 20, 52, 100, 0,
    MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MEAT5], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC60
  {
    63, 1000, 8, 0, 0, 20, 68, 100, 0,
    MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BLOODYTWITCH], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC61
  {
    22, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HEAD_DIE6], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC62
  {
    15, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_PLAY_DIE7], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC63
  {
    18, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_POSS_DIE5], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC64
  {
    21, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SARG_DIE6], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC65
  {
    23, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SKULL_DIE6], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC66
  {
    20, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TROO_DIE5], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC67
  {
    19, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SPOS_DIE5], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC68
  {
    10, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_PLAY_XDIE9], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC69
  {
    12, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_PLAY_XDIE9], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC70
  {
    28, 1000, 8, 0, 0, 16, 64, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HEADSONSTICK], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC71
  {
    24, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_GIBS], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC72
  {
    27, 1000, 8, 0, 0, 16, 56, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HEADONASTICK], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC73
  {
    29, 1000, 8, 0, 0, 16, 48, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HEADCANDLES], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC74
  {
    25, 1000, 8, 0, 0, 16, 64, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_DEADSTICK], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC75
  {
    26, 1000, 8, 0, 0, 16, 64, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_LIVESTICK], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC76
  {
    54, 1000, 8, 0, 0, 32, 96, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BIGTREE], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC77
  {
    70, 1000, 8, 0, 0, 16, 30, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BBAR1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC78
  {
    73, 1000, 8, 0, 0, 16, 88, 100, 0,
    MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HANGNOGUTS], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC79
  {
    74, 1000, 8, 0, 0, 16, 88, 100, 0,
    MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HANGBNOBRAIN], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC80
  {
    75, 1000, 8, 0, 0, 16, 64, 100, 0,
    MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HANGTLOOKDN], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC81
  {
    76, 1000, 8, 0, 0, 16, 64, 100, 0,
    MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HANGTSKULL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC82
  {
    77, 1000, 8, 0, 0, 16, 64, 100, 0,
    MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HANGTLOOKUP], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC83
  {
    78, 1000, 8, 0, 0, 16, 64, 100, 0,
    MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HANGTNOBRAIN], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC84
  {
    79, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_COLONGIBS], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC85
  {
    80, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SMALLPOOL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC86
  {
    81, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BRAINSTEM], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  //============================================================================
  // Legacy specific mapthings
  //============================================================================

  // MT_DEFAULT_THING
  {
    -1, 0, 0, 0, 0, 0, 0, 0, 0,
    MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY,
    MF2_SLIDE,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_NULL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  //added:26-02-98: chase camera
  // MT_CHASECAM
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOSECTOR|FLAGS_floater|MF_NOCLIPTHING,
    MF2_SLIDE,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_NULL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  //added:9-06-98: spirit for movement prediction
  // MT_SPIRIT
  {
    -1, 1000, 0, 0, 0, 16, 56, 100, 0,
    MF_DROPOFF | MF_NOSPLASH,
    MF2_SLIDE,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_NULL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  //test v1.25 smoke from lava/slime damage,
  // MT_SMOK
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPLINE|MF_NOCLIPTHING,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SMOK1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_SPLASH
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPLINE|MF_NOCLIPTHING | MF_NOSPLASH,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SPLASH1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // For use with wind and current effects // jff 5/11/98 deconflict with DOSDoom
  // MT_PUSH
  {
    5001, 1000, 8, 0, 0, 8, 8, 10, 0,
    MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TNT1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // For use with wind and current effects // jff 5/11/98 deconflict with DOSDoom
  // MT_PULL
  {
    5002, 1000, 8, 0, 0, 8, 8, 10, 0,
    MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TNT1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },
  //SoM: Note that the above is thing type # 138

  //SoM: Dogs, and BetaBFG are NOT implemented! These things are here to hold spaces.
  // MT_DOGS
  {
    888, 500, 8, 180, 10, 12, 28, 100, 0,
    MF_NOSECTOR|MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TNT1], &states[S_TNT1], &states[S_TNT1], NULL, &states[S_TNT1],
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_PLASMA1
  {
    -1, 1000, 8, 0, 25, 13, 8, 100, 4,
    MF_NOSECTOR|MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TNT1], NULL, NULL, NULL, NULL,
    &states[S_TNT1], NULL, NULL, NULL,
    NULL
  },

  // MT_PLASMA2
  {
    -1, 1000, 8, 0, 25, 6, 8, 100, 4,
    MF_NOSECTOR|MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TNT1], NULL, NULL, NULL, NULL,
    &states[S_TNT1], NULL, NULL, NULL,
    NULL
  },

  //script camera location
  // MT_CAMERA
  {
    5003, 1000, 8, 0, 0, 8, 8, 10, 0,
    MF_NOBLOCKMAP|MF_NOCLIPLINE|MF_NOCLIPTHING|MF_NOGRAVITY|MF_NOSECTOR,
    MF2_SLIDE,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TNT1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // SoM: Nodes for things like camera movement, and (eventually) monster trails...
  // MT_NODE
  {
    5004, 1000, 8, 0, 0, 8, 8, 10, 0,
    MF_NOBLOCKMAP|MF_NOCLIPLINE|MF_NOCLIPTHING|MF_NOSECTOR|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TNT1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  //============================================================================
  // Heretic data ! 
  //============================================================================

  // MT_HPLAYER
  {
    -1, 100, 0, 255, 8.33, 16, 56, 100, 0,
    MF_SOLID | MF_SHOOTABLE | MF_DROPOFF | MF_PICKUP | MF_NOTDMATCH,
    MF2_WINDTHRUST | MF2_FOOTCLIP | MF2_SLIDE | MF2_TELESTOMP,
    sfx_None, sfx_None, sfx_plrpai, sfx_plrdth, sfx_None,
    &states[S_HPLAY], &states[S_HPLAY_RUN1], &states[S_HPLAY_ATK2], &states[S_HPLAY_ATK2], &states[S_HPLAY_PAIN],
    &states[S_HPLAY_DIE1], &states[S_HPLAY_XDIE1], NULL, NULL,
    NULL
  },

  // MT_HMISC0
  {
    81, 10, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ITEM_PTN1_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ITEMSHIELD1
  {
    85, 100, 8, 0, 0.5, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ITEM_SHLD1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ITEMSHIELD2
  {
    31, 200, 8, 0, 0.75, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ITEM_SHD2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HMISC1
  {
    8, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL | MF_COUNTITEM,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ITEM_BAGH1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HMISC2
  {
    35, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL | MF_COUNTITEM,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ITEM_SPMP1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIINVISIBILITY
  {
    75, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL | MF_SHADOW | MF_COUNTITEM,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTI_INVS1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HMISC3
  {
    82, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL | MF_COUNTITEM,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTI_PTN2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIFLY
  {
    83, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL | MF_COUNTITEM,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTI_SOAR1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIINVULNERABILITY
  {
    84, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL | MF_COUNTITEM,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTI_INVU1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTITOMEOFPOWER
  {
    86, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL | MF_COUNTITEM,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTI_PWBK1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIEGG
  {
    30, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL | MF_COUNTITEM,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTI_EGGC1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_EGGFX
  {
    -1, 1000, 8, 0, 18, 8, 8, 100, 1,
    FLAGS_missile | MF_NOSCORCH,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_EGGFX1], NULL, NULL, NULL, NULL,
    &states[S_EGGFXI1_1], NULL, NULL, NULL,
    NULL
  },

  // MT_ARTISUPERHEAL
  {
    32, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL | MF_COUNTITEM,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTI_SPHL1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HMISC4
  {
    33, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL | MF_COUNTITEM,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTI_TRCH1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HMISC5
  {
    34, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL | MF_COUNTITEM,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTI_FBMB1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_FIREBOMB
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOGRAVITY | MF_SHADOW,
    0,
    sfx_None, sfx_None, sfx_None, sfx_phohit, sfx_None,
    &states[S_FIREBOMB1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTITELEPORT
  {
    36, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL | MF_COUNTITEM,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTI_ATLP1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_POD
  {
    2035, 45, 8, 255, 0, 16, 54, 100, 0,
    MF_SOLID | MF_NOBLOOD | MF_SHOOTABLE | MF_DROPOFF,
    MF2_WINDTHRUST | MF2_PUSHABLE | MF2_SLIDE | MF2_TELESTOMP,
    sfx_None, sfx_None, sfx_None, sfx_podexp, sfx_None,
    &states[S_POD_WAIT1], NULL, NULL, NULL, &states[S_POD_PAIN1],
    &states[S_POD_DIE1], NULL, NULL, NULL,
    NULL
  },

  // MT_PODGOO
  {
    -1, 1000, 8, 0, 0, 2, 4, 100, 0,
    FLAGS_gravmissile | MF_NOSCORCH,
    MF2_NOTELEPORT | MF2_LOGRAV | MF2_CANNOTPUSH,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_PODGOO1], NULL, NULL, NULL, NULL,
    &states[S_PODGOOX], NULL, NULL, NULL,
    NULL
  },

  // MT_PODGENERATOR
  {
    43, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOSECTOR,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_PODGENERATOR], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HSPLASH
  {
    -1, 1000, 8, 0, 0, 2, 4, 100, 0,
    FLAGS_gravmissile | MF_NOSPLASH | MF_NOSCORCH,
    MF2_NOTELEPORT | MF2_LOGRAV | MF2_CANNOTPUSH,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HSPLASH1], NULL, NULL, NULL, NULL,
    &states[S_SPLASHX], NULL, NULL, NULL,
    NULL
  },

  // MT_SPLASHBASE
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOSPLASH,
    MF2_NONBLASTABLE,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SPLASHBASE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_LAVASPLASH
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_LAVASPLASH1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_LAVASMOKE
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOGRAVITY | MF_SHADOW,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_LAVASMOKE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_SLUDGECHUNK
  {
    -1, 1000, 8, 0, 0, 2, 4, 100, 0,
    FLAGS_gravmissile | MF_NOSCORCH,
    MF2_NOTELEPORT | MF2_LOGRAV | MF2_CANNOTPUSH,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SLUDGECHUNK1], NULL, NULL, NULL, NULL,
    &states[S_SLUDGECHUNKX], NULL, NULL, NULL,
    NULL
  },

  // MT_SLUDGESPLASH
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SLUDGESPLASH1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_SKULLHANG70
  {
    17, 1000, 8, 0, 0, 20, 70, 100, 0,
    MF_SPAWNCEILING | MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SKULLHANG70_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_SKULLHANG60
  {
    24, 1000, 8, 0, 0, 20, 60, 100, 0,
    MF_SPAWNCEILING | MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SKULLHANG60_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_SKULLHANG45
  {
    25, 1000, 8, 0, 0, 20, 45, 100, 0,
    MF_SPAWNCEILING | MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SKULLHANG45_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_SKULLHANG35
  {
    26, 1000, 8, 0, 0, 20, 35, 100, 0,
    MF_SPAWNCEILING | MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SKULLHANG35_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_CHANDELIER
  {
    28, 1000, 8, 0, 0, 20, 60, 100, 0,
    MF_SPAWNCEILING | MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_CHANDELIER1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_SERPTORCH
  {
    27, 1000, 8, 0, 0, 12, 54, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SERPTORCH1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_SMALLPILLAR
  {
    29, 1000, 8, 0, 0, 16, 34, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SMALLPILLAR], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_STALAGMITESMALL
  {
    37, 1000, 8, 0, 0, 8, 32, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_STALAGMITESMALL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_STALAGMITELARGE
  {
    38, 1000, 8, 0, 0, 12, 64, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_STALAGMITELARGE], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_STALACTITESMALL
  {
    39, 1000, 8, 0, 0, 8, 36, 100, 0,
    MF_SOLID | MF_SPAWNCEILING | MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_STALACTITESMALL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_STALACTITELARGE
  {
    40, 1000, 8, 0, 0, 12, 68, 100, 0,
    MF_SOLID | MF_SPAWNCEILING | MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_STALACTITELARGE], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC6
  {
    76, 1000, 8, 0, 0, 16, 44, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_FIREBRAZIER1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HBARREL
  {
    44, 1000, 8, 0, 0, 12, 32, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BARREL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HMISC7
  {
    47, 1000, 8, 0, 0, 14, 128, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BRPILLAR], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HMISC8
  {
    48, 1000, 8, 0, 0, 20, 23, 100, 0,
    MF_SPAWNCEILING | MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MOSS1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HMISC9
  {
    49, 1000, 8, 0, 0, 20, 27, 100, 0,
    MF_SPAWNCEILING | MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MOSS2], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HMISC10
  {
    50, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_WALLTORCH1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HMISC11
  {
    51, 1000, 8, 0, 0, 8, 104, 100, 0,
    MF_SOLID | MF_SPAWNCEILING | MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HANGINGCORPSE], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_KEYGIZMOBLUE
  {
    94, 1000, 8, 0, 0, 16, 50, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_KEYGIZMO1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_KEYGIZMOGREEN
  {
    95, 1000, 8, 0, 0, 16, 50, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_KEYGIZMO1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_KEYGIZMOYELLOW
  {
    96, 1000, 8, 0, 0, 16, 50, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_KEYGIZMO1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_KEYGIZMOFLOAT
  {
    -1, 1000, 8, 0, 0, 16, 16, 100, 0,
    MF_SOLID | MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_KGZ_START], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HMISC12
  {
    87, 1000, 8, 0, 0, 12, 20, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_VOLCANO1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_VOLCANOBLAST
  {
    -1, 1000, 8, 0, 2, 8, 8, 100, 2 | dt_heat,
    FLAGS_gravmissile,
    MF2_LOGRAV | MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_volhit, sfx_None,
    &states[S_VOLCANOBALL1], NULL, NULL, NULL, NULL,
    &states[S_VOLCANOBALLX1], NULL, NULL, NULL,
    NULL
  },

  // MT_VOLCANOTBLAST
  {
    -1, 1000, 8, 0, 2, 8, 6, 100, 1 | dt_heat,
    FLAGS_gravmissile,
    MF2_LOGRAV | MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_VOLCANOTBALL1], NULL, NULL, NULL, NULL,
    &states[S_VOLCANOTBALLX1], NULL, NULL, NULL,
    NULL
  },

  // MT_TELEGLITGEN
  {
    74, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOGRAVITY | MF_NOSECTOR,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TELEGLITGEN1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_TELEGLITGEN2
  {
    52, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOGRAVITY | MF_NOSECTOR,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TELEGLITGEN2], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_TELEGLITTER
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOGRAVITY | MF_MISSILE | MF_NOSCORCH,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TELEGLITTER1_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_TELEGLITTER2
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOGRAVITY | MF_MISSILE | MF_NOSCORCH,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TELEGLITTER2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HTFOG
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HTFOG1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HTELEPORTMAN
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOSECTOR,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_NULL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_STAFFPUFF
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOGRAVITY,
    0,
    sfx_None, sfx_stfhit, sfx_None, sfx_None, sfx_None,
    &states[S_STAFFPUFF1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_STAFFPUFF2
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOGRAVITY,
    0,
    sfx_None, sfx_stfpow, sfx_None, sfx_None, sfx_None,
    &states[S_STAFFPUFF2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_BEAKPUFF
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOGRAVITY,
    0,
    sfx_None, sfx_chicatk, sfx_None, sfx_None, sfx_None,
    &states[S_STAFFPUFF1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HMISC13
  {
    2005, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_WGNT], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_GAUNTLETPUFF1
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOGRAVITY | MF_SHADOW,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_GAUNTLETPUFF1_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_GAUNTLETPUFF2
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOGRAVITY | MF_SHADOW,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_GAUNTLETPUFF2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_WBLASTER
  {
    53, 30, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BLSR], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_BLASTERFX1
  {
    -1, 1000, 8, 0, 184, 12, 8, 100, 2 | dt_magic,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_blshit, sfx_None,
    &states[S_BLASTERFX1_1], NULL, NULL, NULL, NULL,
    &states[S_BLASTERFXI1_1], NULL, NULL, NULL,
    NULL
  },

  // MT_BLASTERSMOKE
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOGRAVITY | MF_SHADOW,
    MF2_NOTELEPORT | MF2_CANNOTPUSH,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BLASTERSMOKE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_RIPPER
  {
    -1, 1000, 8, 0, 14, 8, 6, 100, 1 | dt_magic,
    FLAGS_missile,
    FLAGS2_missile | MF2_RIP,
    sfx_None, sfx_None, sfx_None, sfx_hrnhit, sfx_None,
    &states[S_RIPPER1], NULL, NULL, NULL, NULL,
    &states[S_RIPPERX1], NULL, NULL, NULL,
    NULL
  },

  // MT_BLASTERPUFF1
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BLASTERPUFF1_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_BLASTERPUFF2
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOGRAVITY,
    0,
    sfx_None, sfx_blshit, sfx_None, sfx_None, sfx_None,
    &states[S_BLASTERPUFF2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_WMACE
  {
    2002, 50, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_WMCE], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MACEFX1
  {
    -1, 1000, 8, 0, 20, 8, 6, 100, 2 | dt_concussion,
    FLAGS_missile,
    MF2_FLOORBOUNCE | MF2_THRUGHOST | FLAGS2_missile,
    sfx_lobsht, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MACEFX1_1], NULL, NULL, NULL, NULL,
    &states[S_MACEFXI1_1], NULL, NULL, NULL,
    NULL
  },

  // MT_MACEFX2
  {
    -1, 1000, 8, 0, 10, 8, 6, 100, 6 | dt_concussion,
    FLAGS_gravmissile,
    MF2_LOGRAV | MF2_FLOORBOUNCE | MF2_THRUGHOST | MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MACEFX2_1], NULL, NULL, NULL, NULL,
    &states[S_MACEFXI2_1], NULL, NULL, NULL,
    NULL
  },

  // MT_MACEFX3
  {
    -1, 1000, 8, 0, 7, 8, 6, 100, 4 | dt_concussion,
    FLAGS_gravmissile,
    MF2_LOGRAV | MF2_FLOORBOUNCE | MF2_THRUGHOST | MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MACEFX3_1], NULL, NULL, NULL, NULL,
    &states[S_MACEFXI1_1], NULL, NULL, NULL,
    NULL
  },

  // MT_MACEFX4
  {
    -1, 1000, 8, 0, 7, 8, 6, 100, 18 | dt_concussion,
    FLAGS_gravmissile,
    MF2_LOGRAV | MF2_FLOORBOUNCE | MF2_THRUGHOST | MF2_TELESTOMP,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MACEFX4_1], NULL, NULL, NULL, NULL,
    &states[S_MACEFXI4_1], NULL, NULL, NULL,
    NULL
  },

  // MT_WSKULLROD
  {
    2004, 50, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_WSKL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HORNRODFX1
  {
    -1, 1000, 8, 0, 22, 12, 8, 100, 3 | dt_magic,
    FLAGS_missile,
    MF2_WINDTHRUST | FLAGS2_missile,
    sfx_hrnsht, sfx_None, sfx_None, sfx_hrnhit, sfx_None,
    &states[S_HRODFX1_1], NULL, NULL, NULL, NULL,
    &states[S_HRODFXI1_1], NULL, NULL, NULL,
    NULL
  },

  // MT_HORNRODFX2
  {
    -1, 4 * 35, 8, 0, 22, 12, 8, 100, 10 | dt_magic,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_hrnsht, sfx_None, sfx_None, sfx_ramphit, sfx_None,
    &states[S_HRODFX2_1], NULL, NULL, NULL, NULL,
    &states[S_HRODFXI2_1], NULL, NULL, NULL,
    NULL
  },

  // MT_RAINPLR1
  {
    -1, 1000, 8, 0, 12, 5, 12, 100, 5 | dt_magic,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_RAINPLR1_1], NULL, NULL, NULL, NULL,
    &states[S_RAINPLR1X_1], NULL, NULL, NULL,
    NULL
  },

  // MT_RAINPLR2
  {
    -1, 1000, 8, 0, 12, 5, 12, 100, 5 | dt_magic,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_RAINPLR2_1], NULL, NULL, NULL, NULL,
    &states[S_RAINPLR2X_1], NULL, NULL, NULL,
    NULL
  },

  // MT_RAINPLR3
  {
    -1, 1000, 8, 0, 12, 5, 12, 100, 5 | dt_magic,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_RAINPLR3_1], NULL, NULL, NULL, NULL,
    &states[S_RAINPLR3X_1], NULL, NULL, NULL,
    NULL
  },

  // MT_RAINPLR4
  {
    -1, 1000, 8, 0, 12, 5, 12, 100, 5 | dt_magic,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_RAINPLR4_1], NULL, NULL, NULL, NULL,
    &states[S_RAINPLR4X_1], NULL, NULL, NULL,
    NULL
  },

  // MT_GOLDWANDFX1
  {
    -1, 1000, 8, 0, 22, 10, 6, 100, 2 | dt_magic,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_gldhit, sfx_None,
    &states[S_GWANDFX1_1], NULL, NULL, NULL, NULL,
    &states[S_GWANDFXI1_1], NULL, NULL, NULL,
    NULL
  },

  // MT_GOLDWANDFX2
  {
    -1, 1000, 8, 0, 18, 10, 6, 100, 1 | dt_magic,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_GWANDFX2_1], NULL, NULL, NULL, NULL,
    &states[S_GWANDFXI1_1], NULL, NULL, NULL,
    NULL
  },

  // MT_GOLDWANDPUFF1
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_GWANDPUFF1_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_GOLDWANDPUFF2
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_GWANDFXI1_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_WPHOENIXROD
  {
    2003, 2, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_WPHX], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_PHOENIXFX1
  {
    -1, 1000, 8, 0, 20, 11, 8, 100, 20 | dt_magic,
    FLAGS_missile,
    MF2_THRUGHOST | FLAGS2_missile,
    sfx_phosht, sfx_None, sfx_None, sfx_phohit, sfx_None,
    &states[S_PHOENIXFX1_1], NULL, NULL, NULL, NULL,
    &states[S_PHOENIXFXI1_1], NULL, NULL, NULL,
    NULL
  },

  // MT_PHOENIXPUFF
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOGRAVITY | MF_SHADOW,
    MF2_NOTELEPORT | MF2_CANNOTPUSH,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_PHOENIXPUFF1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_PHOENIXFX2
  {
    -1, 1000, 8, 0, 10, 6, 8, 100, 2 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_PHOENIXFX2_1], NULL, NULL, NULL, NULL,
    &states[S_PHOENIXFXI2_1], NULL, NULL, NULL,
    NULL
  },

  // MT_WCROSSBOW
  {
    2001, 10, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_WBOW], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_CRBOWFX1
  {
    -1, 1000, 8, 0, 30, 11, 8, 100, 10 | dt_magic,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_bowsht, sfx_None, sfx_None, sfx_hrnhit, sfx_None,
    &states[S_CRBOWFX1], NULL, NULL, NULL, NULL,
    &states[S_CRBOWFXI1_1], NULL, NULL, NULL,
    NULL
  },

  // MT_CRBOWFX2
  {
    -1, 1000, 8, 0, 32, 11, 8, 100, 6 | dt_magic,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_bowsht, sfx_None, sfx_None, sfx_hrnhit, sfx_None,
    &states[S_CRBOWFX2], NULL, NULL, NULL, NULL,
    &states[S_CRBOWFXI1_1], NULL, NULL, NULL,
    NULL
  },

  // MT_CRBOWFX3
  {
    -1, 1000, 8, 0, 20, 11, 8, 100, 2 | dt_magic,
    FLAGS_missile,
    MF2_WINDTHRUST | MF2_THRUGHOST | FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_hrnhit, sfx_None,
    &states[S_CRBOWFX3], NULL, NULL, NULL, NULL,
    &states[S_CRBOWFXI3_1], NULL, NULL, NULL,
    NULL
  },

  // MT_CRBOWFX4
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    MF2_LOGRAV,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_CRBOWFX4_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HBLOOD
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HBLOOD1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_BLOODSPLATTER
  {
    -1, 1000, 8, 0, 0, 2, 4, 100, 0,
    FLAGS_gravmissile | MF_NOSPLASH | MF_NOSCORCH,
    MF2_NOTELEPORT | MF2_CANNOTPUSH,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BLOODSPLATTER1], NULL, NULL, NULL, NULL,
    &states[S_BLOODSPLATTERX], NULL, NULL, NULL,
    NULL
  },

  // MT_BLOODYSKULL
  {
    -1, 1000, 8, 0, 0, 4, 4, 100, 0,
    MF_NOBLOCKMAP | MF_DROPOFF,
    MF2_LOGRAV | MF2_CANNOTPUSH,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BLOODYSKULL1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_CHICPLAYER
  {
    -1, 100, 0, 255, 10.2, 16, 24, 100, 0,
    MF_SOLID | MF_SHOOTABLE | MF_DROPOFF | MF_NOTDMATCH,
    MF2_WINDTHRUST | MF2_SLIDE | MF2_FOOTCLIP | MF2_LOGRAV | MF2_TELESTOMP,
    sfx_None, sfx_None, sfx_chicpai, sfx_chicdth, sfx_None,
    &states[S_CHICPLAY], &states[S_CHICPLAY_RUN1], NULL, &states[S_CHICPLAY_ATK1], &states[S_CHICPLAY_PAIN],
    &states[S_CHICKEN_DIE1], NULL, NULL, NULL,
    NULL
  },

  // MT_CHICKEN
  {
    -1, 10, 8, 200, 4, 9, 22, 40, 0,
    FLAGS_monster | MF_DROPOFF,
    FLAGS2_monster | MF2_WINDTHRUST | MF2_FOOTCLIP,
    sfx_chicpai, sfx_chicatk, sfx_chicpai, sfx_chicdth, sfx_chicact,
    &states[S_CHICKEN_LOOK1], &states[S_CHICKEN_WALK1], &states[S_CHICKEN_ATK1], NULL, &states[S_CHICKEN_PAIN1],
    &states[S_CHICKEN_DIE1], NULL, NULL, NULL,
    NULL,
    "Chicken"
  },

  // MT_FEATHER
  {
    -1, 1000, 8, 0, 0, 2, 4, 100, 0,
    FLAGS_gravmissile | MF_NOSCORCH,
    MF2_NOTELEPORT | MF2_LOGRAV | MF2_CANNOTPUSH | MF2_WINDTHRUST,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_FEATHER1], NULL, NULL, NULL, NULL,
    &states[S_FEATHERX], NULL, NULL, NULL,
    NULL,
    "Feather"
  },

  // MT_MUMMY
  {
    68, 80, 8, 128, 12, 22, 62, 75, 0,
    FLAGS_monster,
    FLAGS2_monster | MF2_FOOTCLIP,
    sfx_mumsit, sfx_mumat1, sfx_mumpai, sfx_mumdth, sfx_mumact,
    &states[S_MUMMY_LOOK1], &states[S_MUMMY_WALK1], &states[S_MUMMY_ATK1], NULL, &states[S_MUMMY_PAIN1],
    &states[S_MUMMY_DIE1], NULL, NULL, NULL,
    NULL,
    "Mummy"
  },

  // MT_MUMMYLEADER
  {
    45, 100, 8, 64, 12, 22, 62, 75, 0,
    FLAGS_monster,
    FLAGS2_monster | MF2_FOOTCLIP,
    sfx_mumsit, sfx_mumat1, sfx_mumpai, sfx_mumdth, sfx_mumact,
    &states[S_MUMMY_LOOK1], &states[S_MUMMY_WALK1], &states[S_MUMMY_ATK1], &states[S_MUMMYL_ATK1], &states[S_MUMMY_PAIN1],
    &states[S_MUMMY_DIE1], NULL, NULL, NULL,
    NULL,
    "MummyLeader"
  },

  // MT_MUMMYGHOST
  {
    69, 80, 8, 128, 12, 22, 62, 75, 0,
    FLAGS_monster | MF_SHADOW,
    FLAGS2_monster | MF2_FOOTCLIP,
    sfx_mumsit, sfx_mumat1, sfx_mumpai, sfx_mumdth, sfx_mumact,
    &states[S_MUMMY_LOOK1], &states[S_MUMMY_WALK1], &states[S_MUMMY_ATK1], NULL, &states[S_MUMMY_PAIN1],
    &states[S_MUMMY_DIE1], NULL, NULL, NULL,
    NULL,
    "MummyGhost"
  },

  // MT_MUMMYLEADERGHOST
  {
    46, 100, 8, 64, 12, 22, 62, 75, 0,
    FLAGS_monster | MF_SHADOW,
    FLAGS2_monster | MF2_FOOTCLIP,
    sfx_mumsit, sfx_mumat1, sfx_mumpai, sfx_mumdth, sfx_mumact,
    &states[S_MUMMY_LOOK1], &states[S_MUMMY_WALK1], &states[S_MUMMY_ATK1], &states[S_MUMMYL_ATK1], &states[S_MUMMY_PAIN1],
    &states[S_MUMMY_DIE1], NULL, NULL, NULL,
    NULL,
    "MummyLeaderGhost"
  },

  // MT_MUMMYSOUL
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MUMMY_SOUL1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "MummySoul"
  },

  // MT_MUMMYFX1
  {
    -1, 1000, 8, 0, 9, 8, 14, 100, 4,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_mumhed,
    &states[S_MUMMYFX1_1], NULL, NULL, NULL, NULL,
    &states[S_MUMMYFXI1_1], NULL, NULL, NULL,
    NULL
  },

  // MT_BEAST
  {
    70, 220, 8, 100, 14, 32, 74, 200, 0,
    FLAGS_monster,
    FLAGS2_monster | MF2_FOOTCLIP,
    sfx_bstsit, sfx_bstatk, sfx_bstpai, sfx_bstdth, sfx_bstact,
    &states[S_BEAST_LOOK1], &states[S_BEAST_WALK1], NULL, &states[S_BEAST_ATK1], &states[S_BEAST_PAIN1],
    &states[S_BEAST_DIE1], &states[S_BEAST_XDIE1], NULL, NULL,
    NULL,
    "Beast"
  },

  // MT_BEASTBALL
  {
    -1, 1000, 8, 0, 12, 9, 8, 100, 4 | dt_heat,
    FLAGS_missile,
    MF2_WINDTHRUST | FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BEASTBALL1], NULL, NULL, NULL, NULL,
    &states[S_BEASTBALLX1], NULL, NULL, NULL,
    NULL
  },

  // MT_BURNBALL
  {
    -1, 1000, 8, 0, 10, 6, 8, 100, 2 | dt_heat,
    MF_NOBLOCKMAP | MF_NOGRAVITY | MF_MISSILE,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BURNBALL1], NULL, NULL, NULL, NULL,
    &states[S_BEASTBALLX1], NULL, NULL, NULL,
    NULL
  },

  // MT_BURNBALLFB
  {
    -1, 1000, 8, 0, 10, 6, 8, 100, 2 | dt_heat,
    MF_NOBLOCKMAP | MF_NOGRAVITY | MF_MISSILE,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BURNBALLFB1], NULL, NULL, NULL, NULL,
    &states[S_BEASTBALLX1], NULL, NULL, NULL,
    NULL
  },

  // MT_PUFFY
  {
    -1, 1000, 8, 0, 10, 6, 8, 100, 2 | dt_heat,
    MF_NOBLOCKMAP | MF_NOGRAVITY | MF_MISSILE,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_PUFFY1], NULL, NULL, NULL, NULL,
    &states[S_PUFFY1], NULL, NULL, NULL,
    NULL
  },

  // MT_SNAKE
  {
    92, 280, 8, 48, 10, 22, 70, 100, 0,
    FLAGS_monster,
    FLAGS2_monster | MF2_FOOTCLIP,
    sfx_snksit, sfx_snkatk, sfx_snkpai, sfx_snkdth, sfx_snkact,
    &states[S_SNAKE_LOOK1], &states[S_SNAKE_WALK1], NULL, &states[S_SNAKE_ATK1], &states[S_SNAKE_PAIN1],
    &states[S_SNAKE_DIE1], NULL, NULL, NULL,
    NULL,
    "Snake"
  },

  // MT_SNAKEPRO_A
  {
    -1, 1000, 8, 0, 14, 12, 8, 100, 1 | dt_heat,
    FLAGS_missile,
    MF2_WINDTHRUST | FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SNAKEPRO_A1], NULL, NULL, NULL, NULL,
    &states[S_SNAKEPRO_AX1], NULL, NULL, NULL,
    NULL
  },

  // MT_SNAKEPRO_B
  {
    -1, 1000, 8, 0, 14, 12, 8, 100, 3 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SNAKEPRO_B1], NULL, NULL, NULL, NULL,
    &states[S_SNAKEPRO_BX1], NULL, NULL, NULL,
    NULL
  },

  // MT_HHEAD
  {
    6, 700, 8, 32, 6, 40, 72, 325, 0,
    FLAGS_monster | MF_NOBLOOD,
    FLAGS2_monster,
    sfx_hedsit, sfx_hedat1, sfx_hedpai, sfx_heddth, sfx_hedact,
    &states[S_HHEAD_LOOK], &states[S_HHEAD_FLOAT], NULL, &states[S_HHEAD_ATK1], &states[S_HHEAD_PAIN1],
    &states[S_HHEAD_DIE1], NULL, NULL, NULL,
    NULL,
    "Ironlich"
  },

  // MT_HEADFX1
  {
    -1, 1000, 8, 0, 13, 12, 6, 100, 1 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile | MF2_THRUGHOST,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HHEADFX1_1], NULL, NULL, NULL, NULL,
    &states[S_HHEADFXI1_1], NULL, NULL, NULL,
    NULL
  },

  // MT_HEADFX2
  {
    -1, 1000, 8, 0, 8, 12, 6, 100, 3 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HHEADFX2_1], NULL, NULL, NULL, NULL,
    &states[S_HHEADFXI2_1], NULL, NULL, NULL,
    NULL
  },

  // MT_HEADFX3
  {
    -1, 1000, 8, 0, 10, 14, 12, 100, 5 | dt_heat,
    FLAGS_missile,
    MF2_WINDTHRUST | FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HHEADFX3_1], NULL, NULL, NULL, NULL,
    &states[S_HHEADFXI3_1], NULL, NULL, NULL,
    NULL
  },

  // MT_WHIRLWIND
  {
    -1, 1000, 8, 0, 10, 16, 74, 100, 1 | dt_crushing,
    FLAGS_missile | MF_SHADOW | MF_NOSCORCH,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HHEADFX4_1], NULL, NULL, NULL, NULL,
    &states[S_HHEADFXI4_1], NULL, NULL, NULL,
    NULL
  },

  // MT_CLINK
  {
    90, 150, 8, 32, 14, 20, 64, 75, 0,
    FLAGS_monster | MF_NOBLOOD,
    FLAGS2_monster | MF2_FOOTCLIP,
    sfx_clksit, sfx_clkatk, sfx_clkpai, sfx_clkdth, sfx_clkact,
    &states[S_CLINK_LOOK1], &states[S_CLINK_WALK1], &states[S_CLINK_ATK1], NULL, &states[S_CLINK_PAIN1],
    &states[S_CLINK_DIE1], NULL, NULL, NULL,
    NULL,
    "Clink"
  },

  // MT_WIZARD
  {
    15, 180, 8, 64, 12, 16, 68, 100, 0,
    FLAGS_monster | FLAGS_floater,
    FLAGS2_monster | MF2_NOPASSMOBJ,
    sfx_wizsit, sfx_wizatk, sfx_wizpai, sfx_wizdth, sfx_wizact,
    &states[S_WIZARD_LOOK1], &states[S_WIZARD_WALK1], NULL, &states[S_WIZARD_ATK1], &states[S_WIZARD_PAIN1],
    &states[S_WIZARD_DIE1], NULL, NULL, NULL,
    NULL,
    "Wizard"
  },

  // MT_WIZFX1
  {
    -1, 1000, 8, 0, 18, 10, 6, 100, 3 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_WIZFX1_1], NULL, NULL, NULL, NULL,
    &states[S_WIZFXI1_1], NULL, NULL, NULL,
    NULL
  },

  // MT_IMP
  {
    66, 40, 8, 200, 10, 16, 36, 50, 0,
    FLAGS_monster | FLAGS_floater | MF_SPAWNFLOAT,
    FLAGS2_monster | MF2_NOPASSMOBJ,
    sfx_impsit, sfx_impat1, sfx_imppai, sfx_impdth, sfx_impact,
    &states[S_IMP_LOOK1], &states[S_IMP_FLY1], &states[S_IMP_MEATK1], &states[S_IMP_MSATK1_1], &states[S_IMP_PAIN1],
    &states[S_IMP_DIE1], &states[S_IMP_XDIE1], &states[S_IMP_CRASH1], NULL,
    NULL,
    "HereticImp"
  },

  // MT_IMPLEADER
  {
    5, 80, 8, 200, 10, 16, 36, 50, 0,
    FLAGS_monster | FLAGS_floater | MF_SPAWNFLOAT,
    FLAGS2_monster,
    sfx_impsit, sfx_impat2, sfx_imppai, sfx_impdth, sfx_impact,
    &states[S_IMP_LOOK1], &states[S_IMP_FLY1], NULL, &states[S_IMP_MSATK2_1], &states[S_IMP_PAIN1],
    &states[S_IMP_DIE1], &states[S_IMP_XDIE1], &states[S_IMP_CRASH1], NULL,
    NULL,
    "HereticImpLeader"
  },

  // MT_IMPCHUNK1
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_IMP_CHUNKA1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "HereticImpChunk1"
  },

  // MT_IMPCHUNK2
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_IMP_CHUNKB1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "HereticImpChunk2"
  },

  // MT_IMPBALL
  {
    -1, 1000, 8, 0, 10, 8, 8, 100, 1 | dt_heat,
    FLAGS_missile,
    MF2_WINDTHRUST | FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_IMPFX1], NULL, NULL, NULL, NULL,
    &states[S_IMPFXI1], NULL, NULL, NULL,
    NULL
  },

  // MT_HKNIGHT
  {
    64, 200, 8, 100, 12, 24, 78, 150, 0,
    FLAGS_monster,
    FLAGS2_monster | MF2_FOOTCLIP,
    sfx_kgtsit, sfx_kgtatk, sfx_kgtpai, sfx_kgtdth, sfx_kgtact,
    &states[S_KNIGHT_STND1], &states[S_KNIGHT_WALK1], &states[S_KNIGHT_ATK1], &states[S_KNIGHT_ATK1], &states[S_KNIGHT_PAIN1],
    &states[S_KNIGHT_DIE1], NULL, NULL, NULL,
    NULL,
    "Knight"
  },

  // MT_KNIGHTGHOST
  {
    65, 200, 8, 100, 12, 24, 78, 150, 0,
    FLAGS_monster | MF_SHADOW,
    FLAGS2_monster | MF2_FOOTCLIP,
    sfx_kgtsit, sfx_kgtatk, sfx_kgtpai, sfx_kgtdth, sfx_kgtact,
    &states[S_KNIGHT_STND1], &states[S_KNIGHT_WALK1], &states[S_KNIGHT_ATK1], &states[S_KNIGHT_ATK1], &states[S_KNIGHT_PAIN1],
    &states[S_KNIGHT_DIE1], NULL, NULL, NULL,
    NULL,
    "KnightGhost"
  },

  // MT_KNIGHTAXE
  {
    -1, 1000, 8, 0, 9, 10, 8, 100, 2 | dt_cutting,
    FLAGS_missile,
    MF2_WINDTHRUST | FLAGS2_missile | MF2_THRUGHOST,
    sfx_None, sfx_None, sfx_None, sfx_hrnhit, sfx_kgtatk,
    &states[S_SPINAXE1], NULL, NULL, NULL, NULL,
    &states[S_SPINAXEX1], NULL, NULL, NULL,
    NULL
  },

  // MT_REDAXE
  {
    -1, 1000, 8, 0, 9, 10, 8, 100, 7 | dt_cutting,
    FLAGS_missile,
    FLAGS2_missile | MF2_THRUGHOST,
    sfx_None, sfx_None, sfx_None, sfx_hrnhit, sfx_None,
    &states[S_REDAXE1], NULL, NULL, NULL, NULL,
    &states[S_REDAXEX1], NULL, NULL, NULL,
    NULL
  },

  // MT_SORCERER1
  {
    7, 2000, 8, 56, 16, 28, 100, 800, 0,
    FLAGS_monster,
    FLAGS2_monster | MF2_FOOTCLIP | MF2_BOSS | MF2_NOTARGET,
    sfx_sbtsit, sfx_sbtatk, sfx_sbtpai, sfx_sbtdth, sfx_sbtact,
    &states[S_SRCR1_LOOK1], &states[S_SRCR1_WALK1], NULL, &states[S_SRCR1_ATK1], &states[S_SRCR1_PAIN1],
    &states[S_SRCR1_DIE1], NULL, NULL, NULL,
    NULL,
    "Sorcerer1"
  },

  // MT_SRCRFX1
  {
    -1, 1000, 8, 0, 20, 10, 10, 100, 10 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SRCRFX1_1], NULL, NULL, NULL, NULL,
    &states[S_SRCRFXI1_1], NULL, NULL, NULL,
    NULL,
    "SorcererFX1"
  },

  // MT_SORCERER2
  {
    -1, 3500, 8, 32, 14, 16, 70, 300, 0,
    FLAGS_monster | MF_DROPOFF,
    FLAGS2_monster | MF2_FOOTCLIP | MF2_BOSS | MF2_NOTARGET,
    sfx_sorsit, sfx_soratk, sfx_sorpai, sfx_None, sfx_soract,
    &states[S_SOR2_LOOK1], &states[S_SOR2_WALK1], NULL, &states[S_SOR2_ATK1], &states[S_SOR2_PAIN1],
    &states[S_SOR2_DIE1], NULL, NULL, NULL,
    NULL,
    "Sorcerer2"
  },

  // MT_SOR2FX1
  {
    -1, 1000, 8, 0, 20, 10, 6, 100, 1 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SOR2FX1_1], NULL, NULL, NULL, NULL,
    &states[S_SOR2FXI1_1], NULL, NULL, NULL,
    NULL,
    "Sorcerer2FX1"
  },

  // MT_SOR2FXSPARK
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOGRAVITY,
    MF2_NOTELEPORT | MF2_CANNOTPUSH,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SOR2FXSPARK1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "Sorcerer2FXSpark"
  },

  // MT_SOR2FX2
  {
    -1, 1000, 8, 0, 6, 10, 6, 100, 10 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SOR2FX2_1], NULL, NULL, NULL, NULL,
    &states[S_SOR2FXI2_1], NULL, NULL, NULL,
    NULL,
    "Sorcerer2FX2"
  },

  // MT_SOR2TELEFADE
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SOR2TELEFADE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "Sorcerer2Telefade"
  },

  // MT_MINOTAUR
  {
    9, 3000, 8, 25, 16, 28, 100, 800, 7,
    FLAGS_monster | MF_DROPOFF,
    FLAGS2_monster | MF2_FOOTCLIP | MF2_BOSS | MF2_NOTARGET,
    sfx_minsit, sfx_minat1, sfx_minpai, sfx_mindth, sfx_minact,
    &states[S_MNTR_LOOK1], &states[S_MNTR_WALK1], &states[S_MNTR_ATK1_1], &states[S_MNTR_ATK2_1], &states[S_MNTR_PAIN1],
    &states[S_MNTR_DIE1], NULL, NULL, NULL,
    NULL,
    "Minotaur"
  },

  // MT_MNTRFX1
  {
    -1, 1000, 8, 0, 20, 10, 6, 100, 3 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MNTRFX1_1], NULL, NULL, NULL, NULL,
    &states[S_MNTRFXI1_1], NULL, NULL, NULL,
    NULL
  },

  // MT_MNTRFX2
  {
    -1, 1000, 8, 0, 14, 5, 12, 100, 4 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile | MF2_FLOORHUGGER,
    sfx_None, sfx_None, sfx_None, sfx_phohit, sfx_None,
    &states[S_MNTRFX2_1], NULL, NULL, NULL, NULL,
    &states[S_MNTRFXI2_1], NULL, NULL, NULL,
    NULL
  },

  // MT_MNTRFX3
  {
    -1, 1000, 8, 0, 0, 8, 16, 100, 4 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_phohit, sfx_None,
    &states[S_MNTRFX3_1], NULL, NULL, NULL, NULL,
    &states[S_MNTRFXI2_1], NULL, NULL, NULL,
    NULL
  },

  // MT_AKYY
  {
    73, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL | MF_NOTDMATCH,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_AKYY1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_BKYY
  {
    79, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL | MF_NOTDMATCH,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BKYY1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_CKEY
  {
    80, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL | MF_NOTDMATCH,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_CKYY1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_AMGWNDWIMPY
  {
    10, 10, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_AMG1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_AMGWNDHEFTY
  {
    12, 50, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_AMG2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_AMMACEWIMPY
  {
    13, 20, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_AMM1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_AMMACEHEFTY
  {
    16, 100, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_AMM2], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_AMCBOWWIMPY
  {
    18, 5, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_AMC1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_AMCBOWHEFTY
  {
    19, 20, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_AMC2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_AMSKRDWIMPY
  {
    20, 20, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_AMS1_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_AMSKRDHEFTY
  {
    21, 100, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_AMS2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_AMPHRDWIMPY
  {
    22, 1, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_AMP1_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_AMPHRDHEFTY
  {
    23, 10, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_AMP2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_AMBLSRWIMPY
  {
    54, 10, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_AMB1_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_AMBLSRHEFTY
  {
    55, 25, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_AMB2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_SOUNDWIND
  {
    42, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOSECTOR,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_wind,
    &states[S_SND_WIND], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_SOUNDWATERFALL
  {
    41, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOSECTOR,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_waterfall,
    &states[S_SND_WATERFALL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  //============================================================================
  // Hexen mobjtypes
  //============================================================================

  // MT_MAPSPOT
  {
    9001, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MAPSPOT], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "MapSpot"
  },

  // MT_MAPSPOTGRAVITY
  {
    9013, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    MF2_DONTDRAW,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MAPSPOT], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "MapSpotGravity"
  },

  // MT_FIREBALL1
  {
    -1, 1000, 8, 0, 2, 8, 8, 100, 4 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, SFX_FIREBALL, sfx_None,
    &states[S_FIREBALL1_1], NULL, NULL, NULL, NULL,
    &states[S_FIREBALL1_X1], NULL, NULL, NULL,
    NULL
  },

  // MT_ARROW
  {
    -1, 1000, 8, 0, 6, 8, 4, 100, 4,
    FLAGS_missile | MF_NOSCORCH,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARROW_1], NULL, NULL, NULL, NULL,
    &states[S_ARROW_X1], NULL, NULL, NULL,
    NULL
  },

  // MT_DART
  {
    -1, 1000, 8, 0, 6, 8, 4, 100, 2,
    FLAGS_missile | MF_NOSCORCH,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_DART_1], NULL, NULL, NULL, NULL,
    &states[S_DART_X1], NULL, NULL, NULL,
    NULL
  },

  // MT_POISONDART
  {
    -1, 1000, 8, 0, 6, 8, 4, 100, 2 | dt_poison,
    FLAGS_missile | MF_NOSCORCH,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_POISONDART_1], NULL, NULL, NULL, NULL,
    &states[S_POISONDART_X1], NULL, NULL, NULL,
    NULL
  },

  // MT_RIPPERBALL
  {
    -1, 1000, 8, 0, 6, 8, 16, 100, 2,
    FLAGS_missile,
    FLAGS2_missile|MF2_RIP,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_RIPPERBALL_1], NULL, NULL, NULL, NULL,
    &states[S_RIPPERBALL_X1], NULL, NULL, NULL,
    NULL
  },

  // MT_PROJECTILE_BLADE
  {
    -1, 1000, 8, 0, 6, 6, 6, 100, 3 | dt_cutting,
    FLAGS_missile | MF_NOSCORCH,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_PRJ_BLADE1], NULL, NULL, NULL, NULL,
    &states[S_PRJ_BLADE_X1], NULL, NULL, NULL,
    NULL
  },

  // MT_ICESHARD
  {
    -1, 1000, 8, 0, 25, 13, 8, 100, 1 | dt_cold,
    FLAGS_missile | MF_NOSCORCH,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, SFX_MAGE_SHARDS_EXPLODE, sfx_None,
    &states[S_ICESHARD1], NULL, NULL, NULL, NULL,
    &states[S_SHARDFXE1_1], NULL, NULL, NULL,
    NULL
  },

  // MT_FLAME_SMALL_TEMP
  {
    10500, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_FLAME_TSMALL1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_FLAME_LARGE_TEMP
  {
    10502, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_FLAME_TLARGE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_FLAME_SMALL
  {
    10501, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    MF2_NOTELEPORT|MF2_DONTDRAW,
    sfx_None, SFX_IGNITE, sfx_None, sfx_None, sfx_None,
    &states[S_FLAME_SMALL1], &states[S_FLAME_SDORM1], &states[S_FLAME_SMALL1], NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_FLAME_LARGE
  {
    10503, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    MF2_NOTELEPORT|MF2_DONTDRAW,
    sfx_None, SFX_IGNITE, sfx_None, sfx_None, sfx_None,
    &states[S_FLAME_LARGE1], &states[S_FLAME_LDORM1], &states[S_FLAME_LARGE1], NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HEALINGBOTTLE
  {
    81, 10, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_XITEM_PTN1_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HEALTHFLASK
  {
    82, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_XARTI_PTN2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIFLY
  {
    83, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_XARTI_SOAR1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIINVULNERABILITY
  {
    84, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_XARTI_INVU1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_SUMMONMAULATOR
  {
    86, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTI_SUMMON], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_SUMMON_FX
  {
    -1, 1000, 8, 0, 20, 20, 16, 100, 0,
    FLAGS_gravmissile | MF_NOSCORCH,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SUMMON_FX1_1], NULL, NULL, NULL, NULL,
    &states[S_SUMMON_FX2_1], NULL, NULL, NULL,
    NULL
  },

  // MT_THRUSTFLOOR_UP
  {
    10091, 1000, 8, 0, 0, 20, 128, 100, 0,
    MF_SOLID,
    MF2_NOTELEPORT|MF2_FOOTCLIP,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_THRUSTINIT2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_THRUSTFLOOR_DOWN
  {
    10090, 1000, 8, 0, 0, 20, 128, 100, 0,
    0,
    MF2_NOTELEPORT|MF2_FOOTCLIP|MF2_DONTDRAW,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_THRUSTINIT1_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_TELEPORTOTHER
  {
    10040, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTI_TELOTHER1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_TELOTHER_FX1
  {
    -1, 1000, 8, 0, 20, 16, 16, 100, 10001,
    FLAGS_missile | MF_NOSCORCH,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TELO_FX1], NULL, NULL, NULL, NULL,
    &states[S_TELO_FX9], NULL, NULL, NULL,
    NULL
  },

  // MT_TELOTHER_FX2
  {
    -1, 1000, 8, 0, 16, 16, 16, 100, 10001,
    FLAGS_missile | MF_NOSCORCH,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TELO_FX2_1], NULL, NULL, NULL, NULL,
    &states[S_TELO_FX9], NULL, NULL, NULL,
    NULL
  },

  // MT_TELOTHER_FX3
  {
    -1, 1000, 8, 0, 16, 16, 16, 100, 10001,
    FLAGS_missile | MF_NOSCORCH,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TELO_FX3_1], NULL, NULL, NULL, NULL,
    &states[S_TELO_FX9], NULL, NULL, NULL,
    NULL
  },

  // MT_TELOTHER_FX4
  {
    -1, 1000, 8, 0, 16, 16, 16, 100, 10001,
    FLAGS_missile | MF_NOSCORCH,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TELO_FX4_1], NULL, NULL, NULL, NULL,
    &states[S_TELO_FX9], NULL, NULL, NULL,
    NULL
  },

  // MT_TELOTHER_FX5
  {
    -1, 1000, 8, 0, 16, 16, 16, 100, 10001,
    FLAGS_missile | MF_NOSCORCH,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TELO_FX5_1], NULL, NULL, NULL, NULL,
    &states[S_TELO_FX9], NULL, NULL, NULL,
    NULL
  },

  // MT_DIRT1
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    FLAGS_gravmissile | MF_NOSCORCH,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_DIRT1_1], NULL, NULL, NULL, NULL,
    &states[S_DIRT1_D], NULL, NULL, NULL,
    NULL
  },

  // MT_DIRT2
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    FLAGS_gravmissile | MF_NOSCORCH,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_DIRT2_1], NULL, NULL, NULL, NULL,
    &states[S_DIRT2_D], NULL, NULL, NULL,
    NULL
  },

  // MT_DIRT3
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    FLAGS_gravmissile | MF_NOSCORCH,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_DIRT3_1], NULL, NULL, NULL, NULL,
    &states[S_DIRT3_D], NULL, NULL, NULL,
    NULL
  },

  // MT_DIRT4
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    FLAGS_gravmissile | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_LOGRAV,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_DIRT4_1], NULL, NULL, NULL, NULL,
    &states[S_DIRT4_D], NULL, NULL, NULL,
    NULL
  },

  // MT_DIRT5
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    FLAGS_gravmissile | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_LOGRAV,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_DIRT5_1], NULL, NULL, NULL, NULL,
    &states[S_DIRT5_D], NULL, NULL, NULL,
    NULL
  },

  // MT_DIRT6
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    FLAGS_gravmissile | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_LOGRAV,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_DIRT6_1], NULL, NULL, NULL, NULL,
    &states[S_DIRT6_D], NULL, NULL, NULL,
    NULL
  },

  // MT_DIRTCLUMP
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_DIRTCLUMP1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ROCK1
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    FLAGS_gravmissile | MF_NOSCORCH,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ROCK1_1], NULL, NULL, NULL, NULL,
    &states[S_ROCK1_D], NULL, NULL, NULL,
    NULL
  },

  // MT_ROCK2
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    FLAGS_gravmissile | MF_NOSCORCH,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ROCK2_1], NULL, NULL, NULL, NULL,
    &states[S_ROCK2_D], NULL, NULL, NULL,
    NULL
  },

  // MT_ROCK3
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    FLAGS_gravmissile | MF_NOSCORCH,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ROCK3_1], NULL, NULL, NULL, NULL,
    &states[S_ROCK3_D], NULL, NULL, NULL,
    NULL
  },

  // MT_FOGSPAWNER
  {
    10000, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOSECTOR,
    MF2_DONTDRAW|MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SPAWNFOG1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_FOGPATCHS
  {
    10001, 1000, 8, 0, 1, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_FLOAT|MF_NOGRAVITY|MF_SHADOW|MF_NOCLIPLINE|MF_NOCLIPTHING,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_FOGPATCHS1], NULL, NULL, NULL, NULL,
    &states[S_FOGPATCHS0], NULL, NULL, NULL,
    NULL
  },

  // MT_FOGPATCHM
  {
    10002, 1000, 8, 0, 1, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_FLOAT|MF_NOGRAVITY|MF_SHADOW|MF_NOCLIPLINE|MF_NOCLIPTHING,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_FOGPATCHM1], NULL, NULL, NULL, NULL,
    &states[S_FOGPATCHM0], NULL, NULL, NULL,
    NULL
  },

  // MT_FOGPATCHL
  {
    10003, 1000, 8, 0, 1, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_FLOAT|MF_NOGRAVITY|MF_SHADOW|MF_NOCLIPLINE|MF_NOCLIPTHING,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_FOGPATCHL1], NULL, NULL, NULL, NULL,
    &states[S_FOGPATCHL0], NULL, NULL, NULL,
    NULL
  },

  // MT_QUAKE_FOCUS
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOSECTOR,
    MF2_DONTDRAW,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_quake,
    &states[S_QUAKE_ACTIVE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_SGSHARD1
  {
    -1, 1000, 8, 0, 0, 5, 16, 5, 0,
    FLAGS_missile | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_FULLBOUNCE,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SGSHARD1_1], NULL, NULL, NULL, NULL,
    &states[S_SGSHARD1_D], NULL, NULL, NULL,
    NULL
  },

  // MT_SGSHARD2
  {
    -1, 1000, 8, 0, 0, 5, 16, 5, 0,
    FLAGS_missile | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_FULLBOUNCE,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SGSHARD2_1], NULL, NULL, NULL, NULL,
    &states[S_SGSHARD2_D], NULL, NULL, NULL,
    NULL
  },

  // MT_SGSHARD3
  {
    -1, 1000, 8, 0, 0, 5, 16, 5, 0,
    FLAGS_missile | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_FULLBOUNCE,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SGSHARD3_1], NULL, NULL, NULL, NULL,
    &states[S_SGSHARD3_D], NULL, NULL, NULL,
    NULL
  },

  // MT_SGSHARD4
  {
    -1, 1000, 8, 0, 0, 5, 16, 5, 0,
    FLAGS_missile | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_FULLBOUNCE,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SGSHARD4_1], NULL, NULL, NULL, NULL,
    &states[S_SGSHARD4_D], NULL, NULL, NULL,
    NULL
  },

  // MT_SGSHARD5
  {
    -1, 1000, 8, 0, 0, 5, 16, 5, 0,
    FLAGS_missile | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_FULLBOUNCE,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SGSHARD5_1], NULL, NULL, NULL, NULL,
    &states[S_SGSHARD5_D], NULL, NULL, NULL,
    NULL
  },

  // MT_SGSHARD6
  {
    -1, 1000, 8, 0, 0, 5, 16, 5, 0,
    FLAGS_missile | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_FULLBOUNCE,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SGSHARD6_1], NULL, NULL, NULL, NULL,
    &states[S_SGSHARD6_D], NULL, NULL, NULL,
    NULL
  },

  // MT_SGSHARD7
  {
    -1, 1000, 8, 0, 0, 5, 16, 5, 0,
    FLAGS_missile | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_FULLBOUNCE,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SGSHARD7_1], NULL, NULL, NULL, NULL,
    &states[S_SGSHARD7_D], NULL, NULL, NULL,
    NULL
  },

  // MT_SGSHARD8
  {
    -1, 1000, 8, 0, 0, 5, 16, 5, 0,
    FLAGS_missile | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_FULLBOUNCE,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SGSHARD8_1], NULL, NULL, NULL, NULL,
    &states[S_SGSHARD8_D], NULL, NULL, NULL,
    NULL
  },

  // MT_SGSHARD9
  {
    -1, 1000, 8, 0, 0, 5, 16, 5, 0,
    FLAGS_missile | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_FULLBOUNCE,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SGSHARD9_1], NULL, NULL, NULL, NULL,
    &states[S_SGSHARD9_D], NULL, NULL, NULL,
    NULL
  },

  // MT_SGSHARD0
  {
    -1, 1000, 8, 0, 0, 5, 16, 5, 0,
    FLAGS_missile | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_FULLBOUNCE,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SGSHARD0_1], NULL, NULL, NULL, NULL,
    &states[S_SGSHARD0_D], NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIEGG
  {
    30, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTI_EGGC1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_XEGGFX
  {
    -1, 1000, 8, 0, 18, 8, 8, 100, 1,
    FLAGS_missile | MF_NOSCORCH,
    FLAGS2_missile,
    0, sfx_None, sfx_None, 0, sfx_None,
    &states[S_XEGGFX1], NULL, NULL, NULL, NULL,
    &states[S_XEGGFXI1_1], NULL, NULL, NULL,
    NULL
  },

  // MT_ARTISUPERHEAL
  {
    32, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_XARTI_SPHL1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ZWINGEDSTATUENOSKULL
  {
    9011, 1000, 8, 0, 0, 10, 62, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZWINGEDSTATUENOSKULL], NULL, &states[S_ZWINGEDSTATUENOSKULL2], NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ZGEMPEDESTAL
  {
    9012, 1000, 8, 0, 0, 10, 40, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZGEMPEDESTAL1], NULL, &states[S_ZGEMPEDESTAL2], NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIPUZZSKULL
  {
    9002, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTIPUZZSKULL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIPUZZGEMBIG
  {
    9003, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTIPUZZGEMBIG], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIPUZZGEMRED
  {
    9004, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTIPUZZGEMRED], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIPUZZGEMGREEN1
  {
    9005, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTIPUZZGEMGREEN1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIPUZZGEMGREEN2
  {
    9009, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTIPUZZGEMGREEN2], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIPUZZGEMBLUE1
  {
    9006, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTIPUZZGEMBLUE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIPUZZGEMBLUE2
  {
    9010, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTIPUZZGEMBLUE2], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIPUZZBOOK1
  {
    9007, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTIPUZZBOOK1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIPUZZBOOK2
  {
    9008, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTIPUZZBOOK2], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIPUZZSKULL2
  {
    9014, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTIPUZZSKULL2], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIPUZZFWEAPON
  {
    9015, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTIPUZZFWEAPON], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIPUZZCWEAPON
  {
    9016, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTIPUZZCWEAPON], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIPUZZMWEAPON
  {
    9017, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTIPUZZMWEAPON], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIPUZZGEAR
  {
    9018, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTIPUZZGEAR_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIPUZZGEAR2
  {
    9019, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTIPUZZGEAR2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIPUZZGEAR3
  {
    9020, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTIPUZZGEAR3_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIPUZZGEAR4
  {
    9021, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTIPUZZGEAR4_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTITORCH
  {
    33, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_XARTI_TRCH1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_FIREBOMB
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0 | dt_heat,
    MF_NOGRAVITY|MF_ALTSHADOW,
    0,
    sfx_None, sfx_None, sfx_None, SFX_FLECHETTE_EXPLODE, sfx_None,
    &states[S_XFIREBOMB1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTITELEPORT
  {
    36, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_XARTI_ATLP1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARTIPOISONBAG
  {
    8000, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTI_PSBG1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_POISONBAG
  {
    -1, 1000, 8, 0, 0, 5, 5, 100, 0,
    MF_NOGRAVITY|MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_POISONBAG1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_POISONCLOUD
  {
    -1, 1000, 8, 0, 0, 1, 1, INF_MASS, 0 | dt_poison,
    MF_NOGRAVITY|MF_NOBLOCKMAP|MF_SHADOW|MF_NOCLIPLINE|MF_NOCLIPTHING|MF_DROPOFF,
    MF2_NODMGTHRUST,
    sfx_None, sfx_None, sfx_None, SFX_POISONSHROOM_DEATH, sfx_None,
    &states[S_POISONCLOUD1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_THROWINGBOMB
  {
    -1, 48, 8, 0, 12, 8, 10, 100, 0 | dt_heat,
    FLAGS_gravmissile,
    MF2_FULLBOUNCE,
    SFX_FLECHETTE_BOUNCE, sfx_None, SFX_FLECHETTE_BOUNCE, SFX_FLECHETTE_EXPLODE, sfx_None,
    &states[S_THROWINGBOMB1], NULL, NULL, NULL, NULL,
    &states[S_THROWINGBOMB_X1], NULL, NULL, NULL,
    NULL
  },

  // MT_SPEEDBOOTS
  {
    8002, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTI_BOOTS1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_BOOSTMANA
  {
    8003, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTI_MANA], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_BOOSTARMOR
  {
    8041, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTI_ARMOR1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_BLASTRADIUS
  {
    10110, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTI_BLAST1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HEALRADIUS
  {
    10120, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARTI_HEALRAD1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_XSPLASH
  {
    -1, 1000, 8, 0, 0, 2, 4, 100, 0,
    FLAGS_gravmissile | MF_NOSPLASH | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_LOGRAV|MF2_CANNOTPUSH,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_XSPLASH1], NULL, NULL, NULL, NULL,
    &states[S_XSPLASHX], NULL, NULL, NULL,
    NULL
  },

  // MT_XSPLASHBASE
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOSPLASH,
    MF2_NONBLASTABLE,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_XSPLASHBASE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_XLAVASPLASH
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOSPLASH,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_XLAVASPLASH1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_XLAVASMOKE
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOGRAVITY | MF_NOSPLASH | MF_SHADOW,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_XLAVASMOKE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_XSLUDGECHUNK
  {
    -1, 1000, 8, 0, 0, 2, 4, 100, 0,
    FLAGS_gravmissile | MF_NOSPLASH | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_LOGRAV|MF2_CANNOTPUSH,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_XSLUDGECHUNK1], NULL, NULL, NULL, NULL,
    &states[S_XSLUDGECHUNKX], NULL, NULL, NULL,
    NULL
  },

  // MT_XSLUDGESPLASH
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP | MF_NOSPLASH,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_XSLUDGESPLASH1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC0
  {
    5, 1000, 8, 0, 0, 10, 62, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZWINGEDSTATUE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC1
  {
    6, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZROCK1_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC2
  {
    7, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZROCK2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC3
  {
    9, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZROCK3_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC4
  {
    15, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZROCK4_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC5
  {
    17, 1000, 8, 0, 0, 20, 60, 100, 0,
    MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZCHANDELIER1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC6
  {
    8063, 1000, 8, 0, 0, 20, 60, 100, 0,
    MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZCHANDELIER_U], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC7
  {
    24, 1000, 8, 0, 0, 10, 96, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZTREEDEAD1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC8
  {
    25, 1000, 8, 0, 0, 15, 128, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZTREE], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_TREEDESTRUCTIBLE
  {
    8062, 70, 8, 0, 0, 15, 180, INF_MASS, 0,
    FLAGS_breakable,
    0,
    sfx_None, sfx_None, sfx_None, SFX_TREE_BREAK, sfx_None,
    &states[S_ZTREEDESTRUCTIBLE1], NULL, NULL, NULL, NULL,
    &states[S_ZTREEDES_D1], NULL, NULL, NULL,
    NULL
  },

  // MT_MISC9
  {
    26, 1000, 8, 0, 0, 10, 150, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZTREESWAMP182_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC10
  {
    27, 1000, 8, 0, 0, 10, 120, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZTREESWAMP172_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC11
  {
    28, 1000, 8, 0, 0, 12, 20, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTUMPBURNED1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC12
  {
    29, 1000, 8, 0, 0, 12, 20, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTUMPBARE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC13
  {
    37, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTUMPSWAMP1_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC14
  {
    38, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTUMPSWAMP2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC15
  {
    39, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSHROOMLARGE1_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC16
  {
    40, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSHROOMLARGE2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC17
  {
    41, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSHROOMLARGE3_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC18
  {
    42, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSHROOMSMALL1_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC19
  {
    44, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSHROOMSMALL2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC20
  {
    45, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSHROOMSMALL3_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC21
  {
    46, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSHROOMSMALL4_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC22
  {
    47, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSHROOMSMALL5_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC23
  {
    48, 1000, 8, 0, 0, 8, 138, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTALAGMITEPILLAR1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC24
  {
    49, 1000, 8, 0, 0, 8, 48, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTALAGMITELARGE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC25
  {
    50, 1000, 8, 0, 0, 6, 40, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTALAGMITEMEDIUM1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC26
  {
    51, 1000, 8, 0, 0, 8, 36, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTALAGMITESMALL1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC27
  {
    52, 1000, 8, 0, 0, 8, 66, 100, 0,
    MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTALACTITELARGE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC28
  {
    56, 1000, 8, 0, 0, 6, 50, 100, 0,
    MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTALACTITEMEDIUM1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC29
  {
    57, 1000, 8, 0, 0, 8, 40, 100, 0,
    MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTALACTITESMALL1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC30
  {
    58, 1000, 8, 0, 0, 20, 20, 100, 0,
    MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZMOSSCEILING1_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC31
  {
    59, 1000, 8, 0, 0, 20, 24, 100, 0,
    MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZMOSSCEILING2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC32
  {
    60, 1000, 8, 0, 0, 8, 52, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSWAMPVINE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC33
  {
    61, 1000, 8, 0, 0, 10, 92, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZCORPSEKABOB1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC34
  {
    62, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZCORPSESLEEPING1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC35
  {
    63, 1000, 8, 0, 0, 10, 46, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZTOMBSTONERIP1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC36
  {
    64, 1000, 8, 0, 0, 10, 46, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZTOMBSTONESHANE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC37
  {
    65, 1000, 8, 0, 0, 10, 46, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZTOMBSTONEBIGCROSS1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC38
  {
    66, 1000, 8, 0, 0, 10, 52, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZTOMBSTONEBRIANR1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC39
  {
    67, 1000, 8, 0, 0, 10, 52, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZTOMBSTONECROSSCIRCLE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC40
  {
    68, 1000, 8, 0, 0, 8, 46, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZTOMBSTONESMALLCROSS1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC41
  {
    69, 1000, 8, 0, 0, 8, 46, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZTOMBSTONEBRIANP1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC42
  {
    71, 1000, 8, 0, 0, 6, 75, 100, 0,
    MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_CORPSEHANGING_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC43
  {
    72, 1000, 8, 0, 0, 14, 108, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTATUEGARGOYLEGREENTALL_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC44
  {
    73, 1000, 8, 0, 0, 14, 108, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTATUEGARGOYLEBLUETALL_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC45
  {
    74, 1000, 8, 0, 0, 14, 62, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTATUEGARGOYLEGREENSHORT_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC46
  {
    76, 1000, 8, 0, 0, 14, 62, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTATUEGARGOYLEBLUESHORT_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC47
  {
    8044, 1000, 8, 0, 0, 14, 108, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTATUEGARGOYLESTRIPETALL_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC48
  {
    8045, 1000, 8, 0, 0, 14, 108, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTATUEGARGOYLEDARKREDTALL_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC49
  {
    8046, 1000, 8, 0, 0, 14, 108, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTATUEGARGOYLEREDTALL_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC50
  {
    8047, 1000, 8, 0, 0, 14, 108, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTATUEGARGOYLETANTALL_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC51
  {
    8048, 1000, 8, 0, 0, 14, 108, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTATUEGARGOYLERUSTTALL_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC52
  {
    8049, 1000, 8, 0, 0, 14, 62, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTATUEGARGOYLEDARKREDSHORT_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC53
  {
    8050, 1000, 8, 0, 0, 14, 62, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTATUEGARGOYLEREDSHORT_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC54
  {
    8051, 1000, 8, 0, 0, 14, 62, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTATUEGARGOYLETANSHORT_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC55
  {
    8052, 1000, 8, 0, 0, 14, 62, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTATUEGARGOYLERUSTSHORT_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC56
  {
    77, 1000, 8, 0, 0, 8, 120, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZBANNERTATTERED_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC57
  {
    78, 1000, 8, 0, 0, 15, 180, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZTREELARGE1], NULL, NULL, NULL, NULL,
    &states[S_ZTREELARGE1], NULL, NULL, NULL,
    NULL
  },

  // MT_MISC58
  {
    79, 1000, 8, 0, 0, 15, 180, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZTREELARGE2], NULL, NULL, NULL, NULL,
    &states[S_ZTREELARGE2], NULL, NULL, NULL,
    NULL
  },

  // MT_MISC59
  {
    80, 1000, 8, 0, 0, 22, 100, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZTREEGNARLED1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC60
  {
    87, 1000, 8, 0, 0, 22, 100, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZTREEGNARLED2], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC61
  {
    88, 1000, 8, 0, 0, 20, 25, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZLOG], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC62
  {
    89, 1000, 8, 0, 0, 8, 66, 100, 0,
    MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTALACTITEICELARGE], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC63
  {
    90, 1000, 8, 0, 0, 5, 50, 100, 0,
    MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTALACTITEICEMEDIUM], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC64
  {
    91, 1000, 8, 0, 0, 4, 32, 100, 0,
    MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTALACTITEICESMALL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC65
  {
    92, 1000, 8, 0, 0, 4, 8, 100, 0,
    MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTALACTITEICETINY], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC66
  {
    93, 1000, 8, 0, 0, 8, 66, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTALAGMITEICELARGE], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC67
  {
    94, 1000, 8, 0, 0, 5, 50, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTALAGMITEICEMEDIUM], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC68
  {
    95, 1000, 8, 0, 0, 4, 32, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTALAGMITEICESMALL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC69
  {
    96, 1000, 8, 0, 0, 4, 8, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZSTALAGMITEICETINY], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC70
  {
    97, 1000, 8, 0, 0, 17, 72, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZROCKBROWN1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC71
  {
    98, 1000, 8, 0, 0, 15, 50, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZROCKBROWN2], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC72
  {
    99, 1000, 8, 0, 0, 20, 40, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZROCKBLACK], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC73
  {
    100, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZRUBBLE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC74
  {
    101, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZRUBBLE2], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC75
  {
    102, 1000, 8, 0, 0, 20, 16, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZRUBBLE3], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC76
  {
    103, 1000, 8, 0, 0, 12, 54, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZVASEPILLAR], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_POTTERY1
  {
    104, 15, 8, 0, 0, 10, 32, 100, 0,
    FLAGS_breakable|MF_DROPOFF,
    MF2_SLIDE|MF2_PUSHABLE|MF2_TELESTOMP,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZPOTTERY1], NULL, NULL, NULL, NULL,
    &states[S_ZPOTTERY_EXPLODE], NULL, NULL, NULL,
    NULL
  },

  // MT_POTTERY2
  {
    105, 15, 8, 0, 0, 10, 25, 100, 0,
    FLAGS_breakable|MF_DROPOFF,
    MF2_SLIDE|MF2_PUSHABLE|MF2_TELESTOMP,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZPOTTERY2], NULL, NULL, NULL, NULL,
    &states[S_ZPOTTERY_EXPLODE], NULL, NULL, NULL,
    NULL
  },

  // MT_POTTERY3
  {
    106, 15, 8, 0, 0, 15, 25, 100, 0,
    FLAGS_breakable|MF_DROPOFF,
    MF2_SLIDE|MF2_PUSHABLE|MF2_TELESTOMP,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZPOTTERY3], NULL, NULL, NULL, NULL,
    &states[S_ZPOTTERY_EXPLODE], NULL, NULL, NULL,
    NULL
  },

  // MT_POTTERYBIT1
  {
    -1, 1000, 8, 0, 0, 5, 5, 100, 0,
    MF_MISSILE | MF_NOSCORCH,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_POTTERYBIT_1], NULL, NULL, NULL, NULL,
    &states[S_POTTERYBIT_EX0], NULL, NULL, NULL,
    NULL
  },

  // MT_MISC77
  {
    108, 1000, 8, 0, 0, 11, 95, 100, 0,
    MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZCORPSELYNCHED1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ZLYNCHED_NOHEART
  {
    109, 1000, 8, 0, 0, 10, 100, 100, 0,
    MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZCORPSELYNCHED2], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC78
  {
    110, 30, 8, 0, 0, 15, 35, 100, 0,
    FLAGS_breakable,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZCORPSESITTING], NULL, NULL, NULL, NULL,
    &states[S_ZCORPSESITTING_X], NULL, NULL, NULL,
    NULL
  },

  // MT_CORPSEBIT
  {
    -1, 1000, 8, 0, 0, 5, 5, 100, 0,
    MF_NOBLOCKMAP,
    MF2_TELESTOMP,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_CORPSEBIT_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_CORPSEBLOODDRIP
  {
    -1, 1000, 8, 0, 0, 1, 4, 100, 0,
    MF_MISSILE | MF_NOSCORCH,
    MF2_LOGRAV,
    sfx_None, sfx_None, sfx_None, SFX_DRIP, sfx_None,
    &states[S_CORPSEBLOODDRIP], NULL, NULL, NULL, NULL,
    &states[S_CORPSEBLOODDRIP_X1], NULL, NULL, NULL,
    NULL
  },

  // MT_BLOODPOOL
  {
    111, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BLOODPOOL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC79
  {
    119, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZCANDLE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC80
  {
    113, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOSECTOR,
    MF2_DONTDRAW,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZLEAFSPAWNER], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_LEAF1
  {
    -1, 1000, 8, 0, 0, 2, 4, 100, 0,
    MF_NOBLOCKMAP | MF_MISSILE | MF_NOSPLASH | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_LOGRAV,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_LEAF1_1], NULL, NULL, NULL, NULL,
    &states[S_LEAF_X1], NULL, NULL, NULL,
    NULL
  },

  // MT_LEAF2
  {
    -1, 1000, 8, 0, 0, 2, 4, 100, 0,
    MF_NOBLOCKMAP |MF_MISSILE | MF_NOSPLASH | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_LOGRAV,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_LEAF2_1], NULL, NULL, NULL, NULL,
    &states[S_LEAF_X1], NULL, NULL, NULL,
    NULL
  },

  // MT_ZTWINEDTORCH
  {
    116, 1000, 8, 0, 0, 10, 64, 100, 0,
    MF_SOLID,
    0,
    sfx_None, SFX_IGNITE, sfx_None, sfx_None, sfx_None,
    &states[S_ZTWINEDTORCH_1], &states[S_ZTWINEDTORCH_UNLIT], &states[S_ZTWINEDTORCH_1], NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ZTWINEDTORCH_UNLIT
  {
    117, 1000, 8, 0, 0, 10, 64, 100, 0,
    MF_SOLID,
    0,
    sfx_None, SFX_IGNITE, sfx_None, sfx_None, sfx_None,
    &states[S_ZTWINEDTORCH_UNLIT], &states[S_ZTWINEDTORCH_UNLIT], &states[S_ZTWINEDTORCH_1], NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_BRIDGE
  {
    118, 1000, 8, 0, 0, 32, 2, 100, 0,
    MF_SOLID|MF_NOGRAVITY,
    MF2_DONTDRAW,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BRIDGE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_BRIDGEBALL
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BBALL1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ZWALLTORCH
  {
    54, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY,
    0,
    sfx_None, SFX_IGNITE, sfx_None, sfx_None, sfx_None,
    &states[S_ZWALLTORCH1], &states[S_ZWALLTORCH_U], &states[S_ZWALLTORCH1], NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ZWALLTORCH_UNLIT
  {
    55, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY,
    0,
    sfx_None, SFX_IGNITE, sfx_None, sfx_None, sfx_None,
    &states[S_ZWALLTORCH_U], &states[S_ZWALLTORCH_U], &states[S_ZWALLTORCH1], NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ZBARREL
  {
    8100, 1000, 8, 0, 0, 15, 32, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZBARREL1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ZSHRUB1
  {
    8101, 20, 8, 0, 0, 8, 24, INF_MASS, 0,
    FLAGS_breakable,
    0,
    sfx_None, sfx_None, sfx_None, SFX_TREE_EXPLODE, sfx_None,
    &states[S_ZSHRUB1], NULL, &states[S_ZSHRUB1_X1], NULL, NULL,
    &states[S_ZSHRUB1_DIE], NULL, NULL, NULL,
    NULL
  },

  // MT_ZSHRUB2
  {
    8102, 10, 8, 0, 0, 16, 40, INF_MASS, 0,
    FLAGS_breakable,
    0,
    sfx_None, sfx_None, sfx_None, SFX_TREE_EXPLODE, sfx_None,
    &states[S_ZSHRUB2], NULL, &states[S_ZSHRUB2_X1], NULL, NULL,
    &states[S_ZSHRUB2_DIE], NULL, NULL, NULL,
    NULL
  },

  // MT_ZBUCKET
  {
    8103, 1000, 8, 0, 0, 8, 72, 100, 0,
    MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZBUCKET1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ZPOISONSHROOM
  {
    8104, 30, 8, 255, 0, 6, 20, INF_MASS, 0,
    FLAGS_breakable,
    0,
    sfx_None, sfx_None, SFX_POISONSHROOM_PAIN, SFX_POISONSHROOM_DEATH, sfx_None,
    &states[S_ZPOISONSHROOM1], NULL, NULL, NULL, &states[S_ZPOISONSHROOM_P1],
    &states[S_ZPOISONSHROOM_X1], NULL, NULL, NULL,
    NULL
  },

  // MT_ZFIREBULL
  {
    8042, 1000, 8, 0, 0, 20, 80, 100, 0,
    MF_SOLID,
    0,
    sfx_None, SFX_IGNITE, sfx_None, sfx_None, sfx_None,
    &states[S_ZFIREBULL1], &states[S_ZFIREBULL_DEATH], &states[S_ZFIREBULL_BIRTH], NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ZFIREBULL_UNLIT
  {
    8043, 1000, 8, 0, 0, 20, 80, 100, 0,
    MF_SOLID,
    0,
    sfx_None, SFX_IGNITE, sfx_None, sfx_None, sfx_None,
    &states[S_ZFIREBULL_U], &states[S_ZFIREBULL_DEATH], &states[S_ZFIREBULL_BIRTH], NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_FIRETHING
  {
    8060, 1000, 8, 0, 0, 5, 10, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZFIRETHING1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_BRASSTORCH
  {
    8061, 1000, 8, 0, 0, 6, 35, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZBRASSTORCH1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ZSUITOFARMOR
  {
    8064, 60, 8, 0, 0, 16, 72, INF_MASS, 0,
    FLAGS_breakable,
    0,
    sfx_None, sfx_None, sfx_None, SFX_SUITOFARMOR_BREAK, sfx_None,
    &states[S_ZSUITOFARMOR], NULL, NULL, NULL, NULL,
    &states[S_ZSUITOFARMOR_X1], NULL, NULL, NULL,
    NULL
  },

  // MT_ZARMORCHUNK
  {
    -1, 1000, 8, 0, 0, 4, 8, 100, 0,
    0,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZARMORCHUNK1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ZBELL
  {
    8065, 5, 8, 0, 0, 56, 120, INF_MASS, 0,
    FLAGS_breakable|MF_NOGRAVITY|MF_SPAWNCEILING,
    0,
    sfx_None, sfx_None, sfx_None, SFX_BELLRING, sfx_None,
    &states[S_ZBELL], NULL, NULL, NULL, NULL,
    &states[S_ZBELL_X1], NULL, NULL, NULL,
    NULL
  },

  // MT_ZBLUE_CANDLE
  {
    8066, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZBLUE_CANDLE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ZIRON_MAIDEN
  {
    8067, 1000, 8, 0, 0, 12, 60, 100, 0,
    MF_SOLID,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZIRON_MAIDEN], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ZXMAS_TREE
  {
    8068, 20, 8, 0, 0, 11, 130, INF_MASS, 0,
    FLAGS_breakable,
    0,
    sfx_None, sfx_None, sfx_None, SFX_TREE_EXPLODE, sfx_None,
    &states[S_ZXMAS_TREE], NULL, &states[S_ZXMAS_TREE_X1], NULL, NULL,
    &states[S_ZXMAS_TREE_DIE], NULL, NULL, NULL,
    NULL
  },

  // MT_ZCAULDRON
  {
    8069, 1000, 8, 0, 0, 12, 26, 100, 0,
    MF_SOLID,
    0,
    sfx_None, SFX_IGNITE, sfx_None, sfx_None, sfx_None,
    &states[S_ZCAULDRON1], &states[S_ZCAULDRON_U], &states[S_ZCAULDRON1], NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ZCAULDRON_UNLIT
  {
    8070, 1000, 8, 0, 0, 12, 26, 100, 0,
    MF_SOLID,
    0,
    sfx_None, SFX_IGNITE, sfx_None, sfx_None, sfx_None,
    &states[S_ZCAULDRON_U], &states[S_ZCAULDRON_U], &states[S_ZCAULDRON1], NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ZCHAINBIT32
  {
    8071, 1000, 8, 0, 0, 4, 32, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SPAWNCEILING,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZCHAINBIT32], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ZCHAINBIT64
  {
    8072, 1000, 8, 0, 0, 4, 64, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SPAWNCEILING,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZCHAINBIT64], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ZCHAINEND_HEART
  {
    8073, 1000, 8, 0, 0, 4, 32, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SPAWNCEILING,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZCHAINEND_HEART], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ZCHAINEND_HOOK1
  {
    8074, 1000, 8, 0, 0, 4, 32, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SPAWNCEILING,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZCHAINEND_HOOK1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ZCHAINEND_HOOK2
  {
    8075, 1000, 8, 0, 0, 4, 32, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SPAWNCEILING,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZCHAINEND_HOOK2], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ZCHAINEND_SPIKE
  {
    8076, 1000, 8, 0, 0, 4, 32, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SPAWNCEILING,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZCHAINEND_SPIKE], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ZCHAINEND_SKULL
  {
    8077, 1000, 8, 0, 0, 4, 32, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SPAWNCEILING,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ZCHAINEND_SKULL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_TABLE_SHIT1
  {
    8500, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TABLE_SHIT1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_TABLE_SHIT2
  {
    8501, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TABLE_SHIT2], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_TABLE_SHIT3
  {
    8502, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TABLE_SHIT3], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_TABLE_SHIT4
  {
    8503, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TABLE_SHIT4], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_TABLE_SHIT5
  {
    8504, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TABLE_SHIT5], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_TABLE_SHIT6
  {
    8505, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TABLE_SHIT6], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_TABLE_SHIT7
  {
    8506, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TABLE_SHIT7], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_TABLE_SHIT8
  {
    8507, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TABLE_SHIT8], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_TABLE_SHIT9
  {
    8508, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TABLE_SHIT9], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_TABLE_SHIT10
  {
    8509, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TABLE_SHIT10], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_XTFOG
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_XTFOG1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MISC81
  {
    140, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_TELESMOKE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_XTELEPORTMAN
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOSECTOR,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_NULL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_PUNCHPUFF
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SHADOW,
    0,
    SFX_FIGHTER_PUNCH_HITTHING, SFX_FIGHTER_PUNCH_HITWALL, sfx_None, sfx_None, sfx_None,
    &states[S_PUNCHPUFF1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_FW_AXE
  {
    8010, 25, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_AXE], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_AXEPUFF
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SHADOW,
    0,
    SFX_FIGHTER_AXE_HITTHING, SFX_FIGHTER_HAMMER_HITWALL, sfx_None, sfx_None, sfx_None,
    &states[S_HAMMERPUFF1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_AXEPUFF_GLOW
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY,
    0,
    SFX_FIGHTER_AXE_HITTHING, SFX_FIGHTER_HAMMER_HITWALL, sfx_None, sfx_None, sfx_None,
    &states[S_AXEPUFF_GLOW1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_AXEBLOOD
  {
    -1, 1000, 8, 0, 0, 2, 4, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_DROPOFF,
    MF2_NOTELEPORT|MF2_CANNOTPUSH,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_AXEBLOOD1], NULL, NULL, NULL, NULL,
    &states[S_AXEBLOOD6], NULL, NULL, NULL,
    NULL
  },

  // MT_FW_HAMMER
  {
    123, 25, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HAMM], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HAMMER_MISSILE
  {
    -1, 1000, 8, 0, 25, 14, 20, 100, 10 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, SFX_FIGHTER_HAMMER_EXPLODE, SFX_FIGHTER_HAMMER_CONTINUOUS,
    &states[S_HAMMER_MISSILE_1], NULL, NULL, NULL, NULL,
    &states[S_HAMMER_MISSILE_X1], NULL, NULL, NULL,
    NULL
  },

  // MT_HAMMERPUFF
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SHADOW,
    0,
    SFX_FIGHTER_HAMMER_HITTHING, SFX_FIGHTER_HAMMER_HITWALL, sfx_None, sfx_None, sfx_None,
    &states[S_HAMMERPUFF1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_FSWORD_MISSILE
  {
    -1, 1000, 8, 0, 30, 16, 8, 100, 8 | dt_magic,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, SFX_FIGHTER_SWORD_EXPLODE, sfx_None,
    &states[S_FSWORD_MISSILE1], NULL, NULL, NULL, NULL,
    &states[S_FSWORD_MISSILE_X1], NULL, NULL, NULL,
    NULL
  },

  // MT_FSWORD_FLAME
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SHADOW,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_FSWORD_FLAME1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_CW_SERPSTAFF
  {
    10, 25, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_CSTAFF], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_CSTAFF_MISSILE
  {
    -1, 1000, 8, 0, 22, 12, 10, 100, 5 | dt_poison,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, SFX_CLERIC_CSTAFF_EXPLODE, sfx_None,
    &states[S_CSTAFF_MISSILE1], NULL, NULL, NULL, NULL,
    &states[S_CSTAFF_MISSILE_X1], NULL, NULL, NULL,
    NULL
  },

  // MT_CSTAFFPUFF
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SHADOW,
    0,
    SFX_CLERIC_CSTAFF_HITTHING, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_CSTAFFPUFF1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_CW_FLAME
  {
    8009, 25, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_CFLAME1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_CFLAMEFLOOR
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_CFLAMEFLOOR1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_FLAMEPUFF
  {
    -1, 1000, 8, 0, 0, 1, 1, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY,
    0,
    SFX_CLERIC_FLAME_EXPLODE, SFX_CLERIC_FLAME_EXPLODE, sfx_None, sfx_None, sfx_None,
    &states[S_FLAMEPUFF1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_FLAMEPUFF2
  {
    -1, 1000, 8, 0, 0, 1, 1, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY,
    0,
    SFX_CLERIC_FLAME_EXPLODE, SFX_CLERIC_FLAME_EXPLODE, sfx_None, sfx_None, sfx_None,
    &states[S_FLAMEPUFF2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_CIRCLEFLAME
  {
    -1, 1000, 8, 0, 0, 6, 16, 100, 2 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, SFX_CLERIC_FLAME_CIRCLE, sfx_None,
    &states[S_CIRCLE_FLAME1], NULL, NULL, NULL, NULL,
    &states[S_CIRCLE_FLAME_X1], NULL, NULL, NULL,
    NULL
  },

  // MT_CFLAME_MISSILE
  {
    -1, 1000, 8, 0, 200, 14, 8, 100, 8 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile|MF2_DONTDRAW,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_CFLAME_MISSILE1], NULL, NULL, NULL, NULL,
    &states[S_CFLAME_MISSILE_X], NULL, NULL, NULL,
    NULL
  },

  // MT_HOLY_FX
  {
    -1, 105, 8, 0, 12, 10, 6, 100, 3 | dt_magic,
    FLAGS_missile | MF_ALTSHADOW | MF_TOUCHFUNC,
    FLAGS2_missile|MF2_SEEKERMISSILE|MF2_RIP,
    sfx_None, sfx_None, sfx_None, SFX_SPIRIT_DIE, sfx_None,
    &states[S_HOLY_FX1], NULL, NULL, NULL, NULL,
    &states[S_HOLY_FX_X1], NULL, NULL, NULL,
    MT_HOLY_FX_touchfunc,
    "HolySpirit"
  },

  // MT_HOLY_TAIL
  {
    -1, 1000, 8, 0, 0, 1, 1, 100, 0,
    MF_NOBLOCKMAP|MF_DROPOFF|MF_NOGRAVITY|MF_NOCLIPLINE|MF_NOCLIPTHING|MF_ALTSHADOW,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HOLY_TAIL1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HOLY_PUFF
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SHADOW,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HOLY_PUFF1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_HOLY_MISSILE
  {
    -1, 1000, 8, 0, 30, 15, 8, 100, 4 | dt_magic,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HOLY_MISSILE1], NULL, NULL, NULL, NULL,
    &states[S_HOLY_MISSILE_X], NULL, NULL, NULL,
    NULL
  },

  // MT_HOLY_MISSILE_PUFF
  {
    -1, 1000, 8, 0, 0, 4, 8, 100, 0,
    MF_NOBLOCKMAP|MF_DROPOFF|MF_NOGRAVITY|MF_ALTSHADOW,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_HOLY_MISSILE_P1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MWANDPUFF
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY,
    MF2_NOTELEPORT|MF2_CANNOTPUSH|MF2_NODMGTHRUST,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MWANDPUFF1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MWANDSMOKE
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SHADOW,
    MF2_NOTELEPORT|MF2_CANNOTPUSH|MF2_NODMGTHRUST,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MWANDSMOKE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MWAND_MISSILE
  {
    -1, 1000, 8, 0, 184, 12, 8, 100, 2 | dt_magic,
    FLAGS_missile,
    FLAGS2_missile|MF2_RIP|MF2_NODMGTHRUST|MF2_CANNOTPUSH,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MWAND_MISSILE1], NULL, NULL, NULL, NULL,
    &states[S_MWANDPUFF1], NULL, NULL, NULL,
    NULL
  },

  // MT_MW_LIGHTNING
  {
    8040, 25, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MW_LIGHTNING1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_LIGHTNING_CEILING
  {
    -1, 144, 8, 0, 25, 16, 40, 100, 8 | dt_shock,
    FLAGS_missile | MF_TOUCHFUNC,
    FLAGS2_missile | MF2_CEILINGHUGGER,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_LIGHTNING_CEILING1], NULL, NULL, NULL, NULL,
    &states[S_LIGHTNING_C_X1], NULL, NULL, NULL,
    MT_LIGHTNING_touchfunc,
    "LightningCeiling"
  },

  // MT_LIGHTNING_FLOOR
  {
    -1, 144, 8, 0, 25, 16, 40, 100, 8 | dt_shock,
    FLAGS_missile | MF_TOUCHFUNC,
    FLAGS2_missile | MF2_FLOORHUGGER,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_LIGHTNING_FLOOR1], NULL, NULL, NULL, NULL,
    &states[S_LIGHTNING_F_X1], NULL, NULL, NULL,
    MT_LIGHTNING_touchfunc,
    "LightningFloor"
  },

  // MT_LIGHTNING_ZAP
  {
    -1, 1000, 8, 0, 0, 15, 35, 100, 2 | dt_shock,
    FLAGS_missile | MF_TOUCHFUNC,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_LIGHTNING_ZAP1], NULL, NULL, NULL, NULL,
    &states[S_LIGHTNING_ZAP_X8], NULL, NULL, NULL,
    MT_LIGHTNING_ZAP_touchfunc,
    "LightningZap"
  },

  // MT_MSTAFF_FX
  {
    -1, 1000, 8, 0, 20, 16, 8, 100, 6 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile|MF2_RIP,
    sfx_None, sfx_None, sfx_None, SFX_MAGE_STAFF_EXPLODE, sfx_None,
    &states[S_MSTAFF_FX1_1], NULL, NULL, NULL, NULL,
    &states[S_MSTAFF_FX_X1], NULL, NULL, NULL,
    NULL
  },

  // MT_MSTAFF_FX2
  {
    -1, 1000, 8, 0, 17, 20, 8, 100, 4 | dt_heat,
    FLAGS_missile | MF_TOUCHFUNC,
    FLAGS2_missile | MF2_SEEKERMISSILE,
    sfx_None, sfx_None, sfx_None, SFX_MAGE_STAFF_EXPLODE, sfx_None,
    &states[S_MSTAFF_FX2_1], NULL, NULL, NULL, NULL,
    &states[S_MSTAFF_FX2_X1], NULL, NULL, NULL,
    MT_MSTAFF_FX2_touchfunc,
    "MageStaffFX2"
  },

  // MT_FW_SWORD1
  {
    12, 25, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_FSWORD1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_FW_SWORD2
  {
    13, 25, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_FSWORD2], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_FW_SWORD3
  {
    16, 25, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_FSWORD3], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_CW_HOLY1
  {
    18, 25, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_CHOLY1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_CW_HOLY2
  {
    19, 25, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_CHOLY2], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_CW_HOLY3
  {
    20, 25, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_CHOLY3], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MW_STAFF1
  {
    21, 25, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MSTAFF1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MW_STAFF2
  {
    22, 25, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MSTAFF2], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MW_STAFF3
  {
    23, 25, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MSTAFF3], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_SNOUTPUFF
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SHADOW,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_PUNCHPUFF1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MW_CONE
  {
    53, 25, 8, 0, 0, 20, 16, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_COS1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_SHARDFX1
  {
    -1, 1000, 8, 0, 25, 13, 8, 100, 8 | dt_cold,
    FLAGS_missile | MF_NOSCORCH,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, SFX_MAGE_SHARDS_EXPLODE, sfx_None,
    &states[S_SHARDFX1_1], NULL, NULL, NULL, NULL,
    &states[S_SHARDFXE1_1], NULL, NULL, NULL,
    NULL
  },

  // MT_XBLOOD
  {
    -1, 1000, 8, 0, 0, 20, 16, 5, 0,
    MF_NOBLOCKMAP | MF_NOSPLASH,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_XBLOOD1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_XBLOODSPLATTER
  {
    -1, 1000, 8, 0, 0, 2, 4, 5, 0,
    FLAGS_gravmissile | MF_NOSPLASH | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_CANNOTPUSH,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_XBLOODSPLATTER1], NULL, NULL, NULL, NULL,
    &states[S_XBLOODSPLATTERX], NULL, NULL, NULL,
    NULL
  },

  // MT_GIBS
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_DROPOFF|MF_CORPSE,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_GIBS1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_PLAYER_FIGHTER
  {
    -1, 100, 0, 255, 10, 16, 64, 100, 0,
    MF_SOLID|MF_SHOOTABLE|MF_DROPOFF|MF_PICKUP|MF_NOTDMATCH,
    MF2_WINDTHRUST|MF2_FOOTCLIP|MF2_SLIDE|MF2_TELESTOMP|MF2_PUSHWALL,
    sfx_None, sfx_None, SFX_PLAYER_FIGHTER_PAIN, sfx_None, sfx_None,
    &states[S_FPLAY], &states[S_FPLAY_RUN1], &states[S_FPLAY_ATK1], &states[S_FPLAY_ATK1], &states[S_FPLAY_PAIN],
    &states[S_FPLAY_DIE1], &states[S_FPLAY_XDIE1], NULL, NULL,
    NULL
  },

  // MT_XBLOODYSKULL
  {
    -1, 1000, 8, 0, 0, 4, 4, 100, 0,
    MF_NOBLOCKMAP|MF_DROPOFF,
    MF2_LOGRAV|MF2_CANNOTPUSH,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_XBLOODYSKULL1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_PLAYER_SPEED
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_ALTSHADOW,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_PLAYER_SPEED1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ICECHUNK
  {
    -1, 1000, 8, 0, 0, 3, 4, 100, 0,
    MF_NOBLOCKMAP|MF_DROPOFF,
    MF2_LOGRAV|MF2_CANNOTPUSH|MF2_FOOTCLIP,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ICECHUNK1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_PLAYER_CLERIC
  {
    -1, 100, 0, 255, 8.33, 16, 64, 100, 0,
    MF_SOLID|MF_SHOOTABLE|MF_DROPOFF|MF_PICKUP|MF_NOTDMATCH,
    MF2_WINDTHRUST|MF2_FOOTCLIP|MF2_SLIDE|MF2_TELESTOMP|MF2_PUSHWALL,
    sfx_None, sfx_None, SFX_PLAYER_CLERIC_PAIN, sfx_None, sfx_None,
    &states[S_CPLAY], &states[S_CPLAY_RUN1], &states[S_CPLAY_ATK1], &states[S_CPLAY_ATK1], &states[S_CPLAY_PAIN],
    &states[S_CPLAY_DIE1], &states[S_CPLAY_XDIE1], NULL, NULL,
    NULL
  },

  // MT_PLAYER_MAGE
  {
    -1, 100, 0, 255, 7.5, 16, 64, 100, 0,
    MF_SOLID|MF_SHOOTABLE|MF_DROPOFF|MF_PICKUP|MF_NOTDMATCH,
    MF2_WINDTHRUST|MF2_FOOTCLIP|MF2_SLIDE|MF2_TELESTOMP|MF2_PUSHWALL,
    sfx_None, sfx_None, SFX_PLAYER_MAGE_PAIN, sfx_None, sfx_None,
    &states[S_MPLAY], &states[S_MPLAY_RUN1], &states[S_MPLAY_ATK1], &states[S_MPLAY_ATK1], &states[S_MPLAY_PAIN],
    &states[S_MPLAY_DIE1], &states[S_MPLAY_XDIE1], NULL, NULL,
    NULL
  },

  // MT_PIGPLAYER
  {
    -1, 100, 0, 255, 8.163, 16, 24, 100, 0,
    MF_SOLID|MF_SHOOTABLE|MF_DROPOFF|MF_NOTDMATCH,
    MF2_WINDTHRUST|MF2_SLIDE|MF2_FOOTCLIP|MF2_TELESTOMP|MF2_PUSHWALL,
    sfx_None, sfx_None, SFX_PIG_PAIN, SFX_PIG_DEATH, sfx_None,
    &states[S_PIGPLAY], &states[S_PIGPLAY_RUN1], &states[S_PIGPLAY_ATK1], &states[S_PIGPLAY_ATK1], &states[S_PIGPLAY_PAIN],
    &states[S_PIG_DIE1], NULL, NULL, NULL,
    NULL
  },

  // MT_PIG
  {
    -1, 25, 8, 128, 10, 12, 22, 60, 0,
    FLAGS_monster,
    MF2_WINDTHRUST|MF2_FOOTCLIP|MF2_PUSHWALL|MF2_TELESTOMP,
    SFX_PIG_ACTIVE1, sfx_None, SFX_PIG_PAIN, SFX_PIG_DEATH, SFX_PIG_ACTIVE1,
    &states[S_PIG_LOOK1], &states[S_PIG_WALK1], &states[S_PIG_ATK1], NULL, &states[S_PIG_PAIN],
    &states[S_PIG_DIE1], NULL, NULL, NULL,
    NULL,
    "Pig"
  },

  // MT_CENTAUR
  {
    107, 200, 8, 135, 13, 20, 64, 120, 0,
    FLAGS_monster,
    FLAGS2_monster | MF2_FOOTCLIP |MF2_TELESTOMP,
    SFX_CENTAUR_SIGHT, SFX_CENTAUR_ATTACK, SFX_CENTAUR_PAIN, SFX_CENTAUR_DEATH, SFX_CENTAUR_ACTIVE,
    &states[S_CENTAUR_LOOK1], &states[S_CENTAUR_WALK1], &states[S_CENTAUR_ATK1], NULL, &states[S_CENTAUR_PAIN1],
    &states[S_CENTAUR_DEATH1], &states[S_CENTAUR_DEATH_X1], NULL, NULL,
    NULL,
    "Centaur"
  },

  // MT_CENTAURLEADER
  {
    115, 250, 8, 96, 10, 20, 64, 120, 0,
    FLAGS_monster,
    FLAGS2_monster | MF2_FOOTCLIP |MF2_TELESTOMP,
    SFX_CENTAUR_SIGHT, SFX_CENTAUR_ATTACK, SFX_CENTAUR_PAIN, SFX_CENTAUR_DEATH, SFX_CENTAUR_ACTIVE,
    &states[S_CENTAUR_LOOK1], &states[S_CENTAUR_WALK1], &states[S_CENTAUR_ATK1], &states[S_CENTAUR_MISSILE1], &states[S_CENTAUR_PAIN1],
    &states[S_CENTAUR_DEATH1], &states[S_CENTAUR_DEATH_X1], NULL, NULL,
    NULL,
    "CentaurLeader"
  },

  // MT_CENTAUR_FX
  {
    -1, 1000, 8, 0, 20, 20, 16, 100, 4 | dt_magic,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, SFX_CENTAUR_MISSILE_EXPLODE, sfx_None,
    &states[S_CENTAUR_FX1], NULL, NULL, NULL, NULL,
    &states[S_CENTAUR_FX_X1], NULL, NULL, NULL,
    NULL,
    "CentaurFX"
  },

  // MT_CENTAUR_SHIELD
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_DROPOFF|MF_CORPSE,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_CENTAUR_SHIELD1], NULL, NULL, NULL, NULL,
    &states[S_CENTAUR_SHIELD_X1], NULL, NULL, NULL,
    NULL,
    "CentaurShield"
  },

  // MT_CENTAUR_SWORD
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_DROPOFF|MF_CORPSE,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_CENTAUR_SWORD1], NULL, NULL, NULL, NULL,
    &states[S_CENTAUR_SWORD_X1], NULL, NULL, NULL,
    NULL,
    "CentaurSword"
  },

  // MT_DEMON
  {
    31, 250, 8, 50, 13, 32, 64, 220, 0,
    FLAGS_monster,
    MF2_FOOTCLIP|MF2_MCROSS|MF2_TELESTOMP,
    SFX_DEMON_SIGHT, SFX_DEMON_ATTACK, SFX_DEMON_PAIN, SFX_DEMON_DEATH, SFX_DEMON_ACTIVE,
    &states[S_DEMN_LOOK1], &states[S_DEMN_CHASE1], &states[S_DEMN_ATK1_1], &states[S_DEMN_ATK2_1], &states[S_DEMN_PAIN1],
    &states[S_DEMN_DEATH1], &states[S_DEMN_XDEATH1], NULL, NULL,
    NULL,
    "Demon1"
  },

  // MT_DEMONCHUNK1
  {
    -1, 1000, 8, 0, 0, 5, 5, 100, 0,
    FLAGS_gravmissile | MF_CORPSE | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_FOOTCLIP,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_DEMONCHUNK1_1], NULL, NULL, NULL, NULL,
    &states[S_DEMONCHUNK1_4], NULL, NULL, NULL,
    NULL,
    "Demon1Chunk1"
  },

  // MT_DEMONCHUNK2
  {
    -1, 1000, 8, 0, 0, 5, 5, 100, 0,
    FLAGS_gravmissile | MF_CORPSE | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_FOOTCLIP,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_DEMONCHUNK2_1], NULL, NULL, NULL, NULL,
    &states[S_DEMONCHUNK2_4], NULL, NULL, NULL,
    NULL,
    "Demon1Chunk2"
  },

  // MT_DEMONCHUNK3
  {
    -1, 1000, 8, 0, 0, 5, 5, 100, 0,
    FLAGS_gravmissile | MF_CORPSE | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_FOOTCLIP,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_DEMONCHUNK3_1], NULL, NULL, NULL, NULL,
    &states[S_DEMONCHUNK3_4], NULL, NULL, NULL,
    NULL,
    "Demon1Chunk3"
  },

  // MT_DEMONCHUNK4
  {
    -1, 1000, 8, 0, 0, 5, 5, 100, 0,
    FLAGS_gravmissile | MF_CORPSE | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_FOOTCLIP,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_DEMONCHUNK4_1], NULL, NULL, NULL, NULL,
    &states[S_DEMONCHUNK4_4], NULL, NULL, NULL,
    NULL,
    "Demon1Chunk4"
  },

  // MT_DEMONCHUNK5
  {
    -1, 1000, 8, 0, 0, 5, 5, 100, 0,
    FLAGS_gravmissile | MF_CORPSE | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_FOOTCLIP,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_DEMONCHUNK5_1], NULL, NULL, NULL, NULL,
    &states[S_DEMONCHUNK5_4], NULL, NULL, NULL,
    NULL,
    "Demon1Chunk5"
  },

  // MT_DEMONFX1
  {
    -1, 1000, 8, 0, 15, 10, 6, 100, 5 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, SFX_DEMON_MISSILE_EXPLODE, sfx_None,
    &states[S_DEMONFX_MOVE1], NULL, NULL, NULL, NULL,
    &states[S_DEMONFX_BOOM1], NULL, NULL, NULL,
    NULL,
    "Demon1FX1"
  },

  // MT_DEMON2
  {
    8080, 250, 8, 50, 13, 32, 64, 220, 0,
    FLAGS_monster,
    MF2_FOOTCLIP|MF2_MCROSS|MF2_TELESTOMP,
    SFX_DEMON_SIGHT, SFX_DEMON_ATTACK, SFX_DEMON_PAIN, SFX_DEMON_DEATH, SFX_DEMON_ACTIVE,
    &states[S_DEMN2_LOOK1], &states[S_DEMN2_CHASE1], &states[S_DEMN2_ATK1_1], &states[S_DEMN2_ATK2_1], &states[S_DEMN2_PAIN1],
    &states[S_DEMN2_DEATH1], &states[S_DEMN2_XDEATH1], NULL, NULL,
    NULL,
    "Demon2"
  },

  // MT_DEMON2CHUNK1
  {
    -1, 1000, 8, 0, 0, 5, 5, 100, 0,
    FLAGS_gravmissile | MF_CORPSE | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_FOOTCLIP,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_DEMON2CHUNK1_1], NULL, NULL, NULL, NULL,
    &states[S_DEMON2CHUNK1_4], NULL, NULL, NULL,
    NULL,
    "Demon2Chunk1"
  },

  // MT_DEMON2CHUNK2
  {
    -1, 1000, 8, 0, 0, 5, 5, 100, 0,
    FLAGS_gravmissile | MF_CORPSE | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_FOOTCLIP,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_DEMON2CHUNK2_1], NULL, NULL, NULL, NULL,
    &states[S_DEMON2CHUNK2_4], NULL, NULL, NULL,
    NULL,
    "Demon2Chunk2"
  },

  // MT_DEMON2CHUNK3
  {
    -1, 1000, 8, 0, 0, 5, 5, 100, 0,
    FLAGS_gravmissile | MF_CORPSE | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_FOOTCLIP,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_DEMON2CHUNK3_1], NULL, NULL, NULL, NULL,
    &states[S_DEMON2CHUNK3_4], NULL, NULL, NULL,
    NULL,
    "Demon2Chunk3"
  },

  // MT_DEMON2CHUNK4
  {
    -1, 1000, 8, 0, 0, 5, 5, 100, 0,
    FLAGS_gravmissile | MF_CORPSE | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_FOOTCLIP,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_DEMON2CHUNK4_1], NULL, NULL, NULL, NULL,
    &states[S_DEMON2CHUNK4_4], NULL, NULL, NULL,
    NULL,
    "Demon2Chunk4"
  },

  // MT_DEMON2CHUNK5
  {
    -1, 1000, 8, 0, 0, 5, 5, 100, 0,
    FLAGS_gravmissile | MF_CORPSE | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_FOOTCLIP,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_DEMON2CHUNK5_1], NULL, NULL, NULL, NULL,
    &states[S_DEMON2CHUNK5_4], NULL, NULL, NULL,
    NULL,
    "Demon2Chunk5"
  },

  // MT_DEMON2FX1
  {
    -1, 1000, 8, 0, 15, 10, 6, 100, 5 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, SFX_DEMON_MISSILE_EXPLODE, sfx_None,
    &states[S_DEMON2FX_MOVE1], NULL, NULL, NULL, NULL,
    &states[S_DEMON2FX_BOOM1], NULL, NULL, NULL,
    NULL,
    "Demon2FX1"
  },

  // MT_WRAITHB
  {
    10011, 150, 8, 25, 11, 20, 68, 75, 10,
    FLAGS_floater |MF_COUNTKILL,
    MF2_FOOTCLIP|MF2_PUSHWALL|MF2_TELESTOMP|MF2_DONTDRAW | MF2_NONBLASTABLE,
    SFX_WRAITH_SIGHT, SFX_WRAITH_ATTACK, SFX_WRAITH_PAIN, SFX_WRAITH_DEATH, SFX_WRAITH_ACTIVE,
    &states[S_WRAITH_LOOK1], &states[S_WRAITH_RAISE1], &states[S_WRAITH_ATK1_1], &states[S_WRAITH_ATK2_1], &states[S_WRAITH_PAIN1],
    &states[S_WRAITH_DEATH1_1], &states[S_WRAITH_DEATH2_1], NULL, NULL,
    NULL,
    "WraithBuried"
  },

  // MT_WRAITH
  {
    34, 150, 8, 25, 11, 20, 55, 75, 10,
    FLAGS_monster | FLAGS_floater,
    MF2_FOOTCLIP|MF2_PUSHWALL|MF2_TELESTOMP,
    SFX_WRAITH_SIGHT, SFX_WRAITH_ATTACK, SFX_WRAITH_PAIN, SFX_WRAITH_DEATH, SFX_WRAITH_ACTIVE,
    &states[S_WRAITH_INIT1], &states[S_WRAITH_CHASE1], &states[S_WRAITH_ATK1_1], &states[S_WRAITH_ATK2_1], &states[S_WRAITH_PAIN1],
    &states[S_WRAITH_DEATH1_1], &states[S_WRAITH_DEATH2_1], NULL, NULL,
    NULL,
    "Wraith"
  },

  // MT_WRAITHFX1
  {
    -1, 1000, 8, 0, 14, 10, 6, 5, 5 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile|MF2_FOOTCLIP,
    sfx_None, sfx_None, sfx_None, SFX_WRAITH_MISSILE_EXPLODE, sfx_None,
    &states[S_WRTHFX_MOVE1], NULL, NULL, NULL, NULL,
    &states[S_WRTHFX_BOOM1], NULL, NULL, NULL,
    NULL,
    "WraithFX1"
  },

  // MT_WRAITHFX2
  {
    -1, 1000, 8, 0, 0, 2, 5, 5, 0,
    MF_NOBLOCKMAP|MF_DROPOFF,
    MF2_NOTELEPORT|MF2_FOOTCLIP,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_WRTHFX_SIZZLE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "WraithFX2"
  },

  // MT_WRAITHFX3
  {
    -1, 1000, 8, 0, 0, 2, 5, 5, 0,
    FLAGS_gravmissile,
    MF2_NOTELEPORT|MF2_FOOTCLIP,
    sfx_None, sfx_None, sfx_None, SFX_DRIP, sfx_None,
    &states[S_WRTHFX_DROP1], NULL, NULL, NULL, NULL,
    &states[S_WRTHFX_DEAD1], NULL, NULL, NULL,
    NULL,
    "WraithFX3"
  },

  // MT_WRAITHFX4
  {
    -1, 1000, 8, 0, 0, 2, 5, 5, 0,
    FLAGS_gravmissile,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, SFX_DRIP, sfx_None,
    &states[S_WRTHFX_ADROP1], NULL, NULL, NULL, NULL,
    &states[S_WRTHFX_ADEAD1], NULL, NULL, NULL,
    NULL,
    "WraithFX4"
  },

  // MT_WRAITHFX5
  {
    -1, 1000, 8, 0, 0, 2, 5, 5, 0,
    FLAGS_gravmissile,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, SFX_DRIP, sfx_None,
    &states[S_WRTHFX_BDROP1], NULL, NULL, NULL, NULL,
    &states[S_WRTHFX_BDEAD1], NULL, NULL, NULL,
    NULL,
    "WraithFX5"
  },

  // MT_XMINOTAUR
  {
    9, 2500, 8, 25, 16, 28, 100, 800, 7,
    FLAGS_monster|MF_SHADOW,
    MF2_FOOTCLIP|MF2_PUSHWALL|MF2_TELESTOMP,
    SFX_MAULATOR_SIGHT, SFX_MAULATOR_HAMMER_SWING, SFX_MAULATOR_PAIN, SFX_MAULATOR_DEATH, SFX_MAULATOR_ACTIVE,
    &states[S_XMNTR_SPAWN1], &states[S_XMNTR_WALK1], &states[S_XMNTR_ATK1_1], &states[S_XMNTR_ATK2_1], &states[S_XMNTR_PAIN1],
    &states[S_XMNTR_DIE1], NULL, NULL, NULL,
    NULL
  },

  // MT_XMNTRFX1
  {
    -1, 1000, 8, 0, 20, 10, 6, 100, 3 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_XMNTRFX1_1], NULL, NULL, NULL, NULL,
    &states[S_XMNTRFXI1_1], NULL, NULL, NULL,
    NULL
  },

  // MT_XMNTRFX2
  {
    -1, 1000, 8, 0, 14, 5, 12, 100, 4 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile | MF2_FLOORHUGGER,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_XMNTRFX2_1], NULL, NULL, NULL, NULL,
    &states[S_XMNTRFXI2_1], NULL, NULL, NULL,
    NULL
  },

  // MT_XMNTRFX3
  {
    -1, 1000, 8, 0, 0, 8, 16, 100, 4 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile,
    0, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_XMNTRFX3_1], NULL, NULL, NULL, NULL,
    &states[S_XMNTRFXI2_1], NULL, NULL, NULL,
    NULL
  },

  // MT_MNTRSMOKE
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SHADOW,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MINOSMOKE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MNTRSMOKEEXIT
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SHADOW,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MINOSMOKEX1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_SERPENT
  {
    121, 90, 8, 96, 12, 32, 70, INF_MASS, 0,
    MF_SOLID|MF_COUNTKILL|MF_NOBLOOD,
    MF2_DONTDRAW|MF2_CANTLEAVEFLOORPIC|MF2_NONSHOOTABLE|MF2_MCROSS | MF2_NONBLASTABLE,
    SFX_SERPENT_SIGHT, SFX_SERPENT_ATTACK, SFX_SERPENT_PAIN, SFX_SERPENT_DEATH, sfx_None,
    &states[S_SERPENT_LOOK1], &states[S_SERPENT_SWIM1], &states[S_SERPENT_SURFACE1], NULL, &states[S_SERPENT_PAIN1],
    &states[S_SERPENT_DIE1], &states[S_SERPENT_XDIE1], NULL, NULL,
    NULL,
    "Serpent"
  },

  // MT_SERPENTLEADER
  {
    120, 90, 8, 96, 12, 32, 70, 200, 0,
    MF_SOLID|MF_COUNTKILL|MF_NOBLOOD,
    MF2_DONTDRAW|MF2_CANTLEAVEFLOORPIC|MF2_NONSHOOTABLE|MF2_MCROSS | MF2_NONBLASTABLE,
    SFX_SERPENT_SIGHT, SFX_SERPENT_ATTACK, SFX_SERPENT_PAIN, SFX_SERPENT_DEATH, sfx_None,
    &states[S_SERPENT_LOOK1], &states[S_SERPENT_SWIM1], &states[S_SERPENT_SURFACE1], NULL, &states[S_SERPENT_PAIN1],
    &states[S_SERPENT_DIE1], &states[S_SERPENT_XDIE1], NULL, NULL,
    NULL,
    "SerpentLeader"
  },

  // MT_SERPENTFX
  {
    -1, 1000, 8, 0, 15, 8, 10, 100, 4,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, SFX_SERPENTFX_HIT, SFX_SERPENTFX_CONTINUOUS,
    &states[S_SERPENT_FX1], NULL, NULL, NULL, NULL,
    &states[S_SERPENT_FX_X1], NULL, NULL, NULL,
    NULL,
    "SerpentFX"
  },

  // MT_SERPENT_HEAD
  {
    -1, 1000, 8, 0, 0, 5, 10, 100, 0,
    MF_NOBLOCKMAP,
    MF2_LOGRAV,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SERPENT_HEAD1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "SerpentHead"
  },

  // MT_SERPENT_GIB1
  {
    -1, 1000, 8, 0, 0, 3, 3, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SERPENT_GIB1_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "SerpentGib1"
  },

  // MT_SERPENT_GIB2
  {
    -1, 1000, 8, 0, 0, 3, 3, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SERPENT_GIB2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "SerpentGib2"
  },

  // MT_SERPENT_GIB3
  {
    -1, 1000, 8, 0, 0, 3, 3, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SERPENT_GIB3_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "SerpentGib3"
  },

  // MT_BISHOP
  {
    114, 130, 8, 110, 10, 22, 65, 100, 0,
    FLAGS_monster | FLAGS_floater | MF_NOBLOOD,
    MF2_NOPASSMOBJ | MF2_PUSHWALL | MF2_TELESTOMP,
    SFX_BISHOP_SIGHT, SFX_BISHOP_ATTACK, SFX_BISHOP_PAIN, SFX_BISHOP_DEATH, SFX_BISHOP_ACTIVE,
    &states[S_BISHOP_LOOK1], &states[S_BISHOP_WALK1], NULL, &states[S_BISHOP_ATK1], &states[S_BISHOP_PAIN1],
    &states[S_BISHOP_DEATH1], NULL, NULL, NULL,
    NULL,
    "Bishop"
  },

  // MT_BISHOP_PUFF
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_SHADOW|MF_NOBLOCKMAP|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BISHOP_PUFF1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "BishopPuff"
  },

  // MT_BISHOPBLUR
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SHADOW,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BISHOPBLUR1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "BishopBlur"
  },

  // MT_BISHOPPAINBLUR
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SHADOW,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BISHOPPAINBLUR1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "BishopPainBlur"
  },

  // MT_BISH_FX
  {
    -1, 1000, 8, 0, 10, 10, 6, 100, 1 | dt_magic,
    FLAGS_missile,
    FLAGS2_missile|MF2_SEEKERMISSILE,
    sfx_None, sfx_None, sfx_None, SFX_BISHOP_MISSILE_EXPLODE, sfx_None,
    &states[S_BISHFX1_1], NULL, NULL, NULL, NULL,
    &states[S_BISHFXI1_1], NULL, NULL, NULL,
    NULL
  },

  // MT_DRAGON
  {
    254, 640, 8, 128, 10, 20, 65, INF_MASS, 0,
    FLAGS_monster | FLAGS_floater | MF_NOBLOOD,
    MF2_BOSS | MF2_NOTARGET,
    SFX_DRAGON_SIGHT, SFX_DRAGON_ATTACK, SFX_DRAGON_PAIN, SFX_DRAGON_DEATH, SFX_DRAGON_ACTIVE,
    &states[S_DRAGON_LOOK1], &states[S_DRAGON_INIT], NULL, &states[S_DRAGON_ATK1], &states[S_DRAGON_PAIN1],
    &states[S_DRAGON_DEATH1], NULL, NULL, NULL,
    NULL,
    "Dragon"
  },

  // MT_DRAGON_FX
  {
    -1, 1000, 8, 0, 24, 12, 10, 100, 6 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, SFX_DRAGON_FIREBALL_EXPLODE, sfx_None,
    &states[S_DRAGON_FX1_1], NULL, NULL, NULL, NULL,
    &states[S_DRAGON_FX1_X1], NULL, NULL, NULL,
    NULL,
    "DragonFireball"
  },

  // MT_DRAGON_FX2
  {
    -1, 1000, 8, 0, 0, 8, 8, 100, 0 | dt_heat,
    MF_NOBLOCKMAP,
    MF2_NOTELEPORT|MF2_DONTDRAW,
    sfx_None, sfx_None, sfx_None, SFX_DRAGON_FIREBALL_EXPLODE, sfx_None,
    &states[S_DRAGON_FX2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "DragonExplosion"
  },

  // MT_ARMOR_1
  {
    8005, 1000, 8, 0, 3, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARMOR_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARMOR_2
  {
    8006, 1000, 8, 0, 3, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARMOR_2], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARMOR_3
  {
    8007, 1000, 8, 0, 3, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARMOR_3], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ARMOR_4
  {
    8008, 1000, 8, 0, 3, 20, 16, 100, 0,
    MF_SPECIAL|MF_NOGRAVITY,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ARMOR_4], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MANA1
  {
    122, 15, 8, 0, 0, 8, 8, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MANA1_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MANA2
  {
    124, 15, 8, 0, 0, 8, 8, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MANA2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_MANA3
  {
    8004, 20, 8, 0, 0, 8, 8, 100, 0,
    MF_SPECIAL,
    MF2_FLOATBOB,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_MANA3_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_KEY1
  {
    8030, 1000, 8, 0, 0, 8, 20, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_KEY1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_KEY2
  {
    8031, 1000, 8, 0, 0, 8, 20, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_KEY2], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_KEY3
  {
    8032, 1000, 8, 0, 0, 8, 20, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_KEY3], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_KEY4
  {
    8033, 1000, 8, 0, 0, 8, 20, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_KEY4], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_KEY5
  {
    8034, 1000, 8, 0, 0, 8, 20, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_KEY5], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_KEY6
  {
    8035, 1000, 8, 0, 0, 8, 20, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_KEY6], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_KEY7
  {
    8036, 1000, 8, 0, 0, 8, 20, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_KEY7], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_KEY8
  {
    8037, 1000, 8, 0, 0, 8, 20, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_KEY8], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_KEY9
  {
    8038, 1000, 8, 0, 0, 8, 20, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_KEY9], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_KEYA
  {
    8039, 1000, 8, 0, 0, 8, 20, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_KEYA], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_KEYB
  {
    8200, 1000, 8, 0, 0, 8, 20, 100, 0,
    MF_SPECIAL,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_KEYB], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_XSOUNDWIND
  {
    1410, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOSECTOR,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, SFX_WIND,
    &states[S_SND_WIND1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_XSOUNDWATERFALL
  {
    41, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOSECTOR,
    0,
    sfx_None, sfx_None, sfx_None, sfx_None, SFX_WATER_MOVE,
    &states[S_XSND_WATERFALL], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ETTIN
  {
    10030, 175, 8, 60, 13, 25, 68, 175, 3,
    FLAGS_monster,
    MF2_FOOTCLIP|MF2_PUSHWALL|MF2_MCROSS|MF2_TELESTOMP,
    SFX_ETTIN_SIGHT, SFX_ETTIN_ATTACK, SFX_ETTIN_PAIN, SFX_ETTIN_DEATH, SFX_ETTIN_ACTIVE,
    &states[S_ETTIN_LOOK1], &states[S_ETTIN_CHASE1], &states[S_ETTIN_ATK1_1], NULL, &states[S_ETTIN_PAIN1],
    &states[S_ETTIN_DEATH1_1], &states[S_ETTIN_DEATH2_1], NULL, NULL,
    NULL,
    "Ettin"
  },

  // MT_ETTIN_MACE
  {
    -1, 1000, 8, 0, 0, 5, 5, 100, 0,
    MF_DROPOFF|MF_CORPSE,
    MF2_NOTELEPORT|MF2_FOOTCLIP,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ETTIN_MACE1], NULL, NULL, NULL, NULL,
    &states[S_ETTIN_MACE5], NULL, NULL, NULL,
    NULL,
    "EttinMace"
  },

  // MT_FIREDEMON
  {
    10060, 80, 8, 1, 13, 20, 68, 75, 1,
    FLAGS_monster | FLAGS_floater,
    FLAGS2_monster | MF2_FOOTCLIP|MF2_INVULNERABLE|MF2_TELESTOMP,
    SFX_FIRED_SPAWN, sfx_None, SFX_FIRED_PAIN, SFX_FIRED_DEATH, SFX_FIRED_ACTIVE,
    &states[S_FIRED_SPAWN1], &states[S_FIRED_LOOK4], NULL, &states[S_FIRED_ATTACK1], &states[S_FIRED_PAIN1],
    &states[S_FIRED_DEATH1], &states[S_FIRED_XDEATH1], &states[S_FIRED_XDEATH1], NULL,
    NULL,
    "FireDemon"
  },

  // MT_FIREDEMON_SPLOTCH1
  {
    -1, 1000, 8, 0, 0, 3, 16, 100, 0,
    MF_DROPOFF|MF_CORPSE,
    MF2_NOTELEPORT|MF2_FOOTCLIP,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_FIRED_CORPSE1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "FireDemonSplotch1"
  },

  // MT_FIREDEMON_SPLOTCH2
  {
    -1, 1000, 8, 0, 0, 3, 16, 100, 0,
    MF_DROPOFF|MF_CORPSE,
    MF2_NOTELEPORT|MF2_FOOTCLIP,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_FIRED_CORPSE4], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "FireDemonSplotch2"
  },

  // MT_FIREDEMON_FX1
  {
    -1, 1000, 8, 0, 0, 3, 5, 16, 0,
    FLAGS_gravmissile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_FIRED_RDROP1], NULL, NULL, NULL, NULL,
    &states[S_FIRED_RDEAD1_1], &states[S_FIRED_RDEAD1_2], NULL, NULL,
    NULL,
    "FireDemonRock1"
  },

  // MT_FIREDEMON_FX2
  {
    -1, 1000, 8, 0, 0, 3, 5, 16, 0,
    FLAGS_gravmissile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_FIRED_RDROP2], NULL, NULL, NULL, NULL,
    &states[S_FIRED_RDEAD2_1], &states[S_FIRED_RDEAD2_2], NULL, NULL,
    NULL,
    "FireDemonRock2"
  },

  // MT_FIREDEMON_FX3
  {
    -1, 1000, 8, 0, 0, 3, 5, 16, 0,
    FLAGS_gravmissile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_FIRED_RDROP3], NULL, NULL, NULL, NULL,
    &states[S_FIRED_RDEAD3_1], &states[S_FIRED_RDEAD3_2], NULL, NULL,
    NULL,
    "FireDemonRock3"
  },

  // MT_FIREDEMON_FX4
  {
    -1, 1000, 8, 0, 0, 3, 5, 16, 0,
    FLAGS_gravmissile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_FIRED_RDROP4], NULL, NULL, NULL, NULL,
    &states[S_FIRED_RDEAD4_1], &states[S_FIRED_RDEAD4_2], NULL, NULL,
    NULL,
    "FireDemonRock4"
  },

  // MT_FIREDEMON_FX5
  {
    -1, 1000, 8, 0, 0, 3, 5, 16, 0,
    FLAGS_gravmissile,
    FLAGS2_missile,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_FIRED_RDROP5], NULL, NULL, NULL, NULL,
    &states[S_FIRED_RDEAD5_1], &states[S_FIRED_RDEAD5_2], NULL, NULL,
    NULL,
    "FireDemonRock5"
  },

  // MT_FIREDEMON_FX6
  {
    -1, 1000, 8, 0, 10, 10, 6, 15, 1 | dt_heat,
    FLAGS_missile,
    FLAGS2_missile|MF2_FOOTCLIP,
    sfx_None, sfx_None, sfx_None, SFX_FIRED_MISSILE_HIT, sfx_None,
    &states[S_FIRED_FX6_1], NULL, NULL, NULL, NULL,
    &states[S_FIRED_FX6_2], NULL, NULL, NULL,
    NULL,
    "FireDemonMissile"
  },

  // MT_ICEGUY
  {
    8020, 120, 8, 144, 14, 22, 75, 150, 0 | dt_cold,
    FLAGS_monster|MF_NOBLOOD,
    FLAGS2_monster | MF2_TELESTOMP,
    SFX_ICEGUY_SIGHT, SFX_ICEGUY_ATTACK, sfx_None, sfx_None, SFX_ICEGUY_ACTIVE,
    &states[S_ICEGUY_LOOK], &states[S_ICEGUY_WALK1], NULL, &states[S_ICEGUY_ATK1], &states[S_ICEGUY_PAIN1],
    &states[S_ICEGUY_DEATH], NULL, NULL, NULL,
    NULL,
    "IceGuy"
  },

  // MT_ICEGUY_FX
  {
    -1, 1000, 8, 0, 14, 8, 10, 100, 1 | dt_cold,
    FLAGS_missile | MF_NOSCORCH,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, SFX_ICEGUY_FX_EXPLODE, sfx_None,
    &states[S_ICEGUY_FX1], NULL, NULL, NULL, NULL,
    &states[S_ICEGUY_FX_X1], NULL, NULL, NULL,
    NULL,
    "IceGuyFX"
  },

  // MT_ICEFX_PUFF
  {
    -1, 1000, 8, 0, 0, 1, 1, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SHADOW|MF_DROPOFF,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ICEFX_PUFF1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_ICEGUY_FX2
  {
    -1, 1000, 8, 0, 10, 4, 4, 100, 1 | dt_cold,
    FLAGS_gravmissile | MF_NOSCORCH,
    MF2_NOTELEPORT|MF2_LOGRAV,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ICEGUY_FX2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "IceGuyFX2"
  },

  // MT_ICEGUY_BIT
  {
    -1, 1000, 8, 0, 0, 1, 1, 100, 0,
    MF_NOBLOCKMAP|MF_DROPOFF,
    MF2_NOTELEPORT|MF2_LOGRAV,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ICEGUY_BIT1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "IceGuyBit"
  },

  // MT_ICEGUY_WISP1
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    FLAGS_missile | MF_ALTSHADOW | MF_NOSCORCH,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ICEGUY_WISP1_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "IceGuyWisp1"
  },

  // MT_ICEGUY_WISP2
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    FLAGS_missile | MF_ALTSHADOW | MF_NOSCORCH,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_ICEGUY_WISP2_1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "IceGuyWisp2"
  },

  // MT_FIGHTER_BOSS
  {
    10100, 800, 8, 50, 25, 16, 64, 100, 0,
    FLAGS_monster,
    FLAGS2_monster | MF2_FOOTCLIP | MF2_TELESTOMP,
    sfx_None, sfx_None, SFX_PLAYER_FIGHTER_PAIN, SFX_PLAYER_FIGHTER_CRAZY_DEATH, sfx_None,
    &states[S_FIGHTER], &states[S_FIGHTER_RUN1], &states[S_FIGHTER_ATK1], &states[S_FIGHTER_ATK1], &states[S_FIGHTER_PAIN],
    &states[S_FIGHTER_DIE1], &states[S_FIGHTER_XDIE1], NULL, NULL,
    NULL,
    "FighterBoss"
  },

  // MT_CLERIC_BOSS
  {
    10101, 800, 8, 50, 25, 16, 64, 100, 0,
    FLAGS_monster,
    FLAGS2_monster | MF2_FOOTCLIP|MF2_TELESTOMP,
    sfx_None, sfx_None, SFX_PLAYER_CLERIC_PAIN, SFX_PLAYER_CLERIC_CRAZY_DEATH, sfx_None,
    &states[S_CLERIC], &states[S_CLERIC_RUN1], &states[S_CLERIC_ATK1], &states[S_CLERIC_ATK1], &states[S_CLERIC_PAIN],
    &states[S_CLERIC_DIE1], &states[S_CLERIC_XDIE1], NULL, NULL,
    NULL,
    "ClericBoss"
  },

  // MT_MAGE_BOSS
  {
    10102, 800, 8, 50, 25, 16, 64, 100, 0,
    FLAGS_monster,
    FLAGS2_monster | MF2_FOOTCLIP|MF2_TELESTOMP,
    sfx_None, sfx_None, SFX_PLAYER_MAGE_PAIN, SFX_PLAYER_MAGE_CRAZY_DEATH, sfx_None,
    &states[S_MAGE], &states[S_MAGE_RUN1], &states[S_MAGE_ATK1], &states[S_MAGE_ATK1], &states[S_MAGE_PAIN],
    &states[S_MAGE_DIE1], &states[S_MAGE_XDIE1], NULL, NULL,
    NULL,
    "MageBoss"
  },

  // MT_SORCBOSS
  {
    10080, 5000, 8, 10, 16, 40, 110, 500, 9,
    FLAGS_monster|MF_NOBLOOD,
    FLAGS2_monster | MF2_FOOTCLIP | MF2_BOSS | MF2_NOTARGET,
    SFX_SORCERER_SIGHT, sfx_None, SFX_SORCERER_PAIN, SFX_SORCERER_DEATHSCREAM, SFX_SORCERER_ACTIVE,
    &states[S_SORC_SPAWN1], &states[S_SORC_WALK1], NULL, &states[S_SORC_ATK2_1], &states[S_SORC_PAIN1],
    &states[S_SORC_DIE1], NULL, NULL, NULL,
    NULL,
    "Heresiarch"
  },

  // MT_SORCBALL1
  {
    -1, 1000, 8, 0, 10, 5, 5, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_MISSILE,
    MF2_NOTELEPORT|MF2_FULLBOUNCE | MF2_NONBLASTABLE,
    SFX_SORCERER_BALLBOUNCE, sfx_None, SFX_SORCERER_BALLBOUNCE, SFX_SORCERER_BIGBALLEXPLODE, sfx_None,
    &states[S_SORCBALL1_1], NULL, NULL, NULL, &states[S_SORCBALL1_D1],
    &states[S_SORCBALL1_D5], NULL, NULL, NULL,
    NULL,
    "SorcBall1"
  },

  // MT_SORCBALL2
  {
    -1, 1000, 8, 0, 10, 5, 5, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_MISSILE,
    MF2_NOTELEPORT|MF2_FULLBOUNCE | MF2_NONBLASTABLE,
    SFX_SORCERER_BALLBOUNCE, sfx_None, SFX_SORCERER_BALLBOUNCE, SFX_SORCERER_BIGBALLEXPLODE, sfx_None,
    &states[S_SORCBALL2_1], NULL, NULL, NULL, &states[S_SORCBALL2_D1],
    &states[S_SORCBALL2_D5], NULL, NULL, NULL,
    NULL,
    "SorcBall2"
  },

  // MT_SORCBALL3
  {
    -1, 1000, 8, 0, 10, 5, 5, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_MISSILE,
    MF2_NOTELEPORT|MF2_FULLBOUNCE | MF2_NONBLASTABLE,
    SFX_SORCERER_BALLBOUNCE, sfx_None, SFX_SORCERER_BALLBOUNCE, SFX_SORCERER_BIGBALLEXPLODE, sfx_None,
    &states[S_SORCBALL3_1], NULL, NULL, NULL, &states[S_SORCBALL3_D1],
    &states[S_SORCBALL3_D5], NULL, NULL, NULL,
    NULL,
    "SorcBall3"
  },

  // MT_SORCFX1
  {
    -1, 1000, 8, 0, 7, 5, 5, 100, 0,
    MF_NOBLOCKMAP|MF_MISSILE,
    MF2_NOTELEPORT|MF2_FULLBOUNCE,
    SFX_SORCERER_BALLBOUNCE, sfx_None, SFX_SORCERER_BALLBOUNCE, SFX_SORCERER_HEADSCREAM, sfx_None,
    &states[S_SORCFX1_1], NULL, NULL, NULL, NULL,
    &states[S_SORCFX1_D1], &states[S_SORCFX1_D1], NULL, NULL,
    NULL,
    "SorcFX1"
  },

  // MT_SORCFX2
  {
    -1, 1000, 8, 0, 15, 5, 5, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SORCFX2_SPLIT1], NULL, NULL, NULL, NULL,
    &states[S_SORCFX2T1], NULL, NULL, NULL,
    NULL,
    "SorcFX2"
  },

  // MT_SORCFX2_T1
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_ALTSHADOW,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SORCFX2T1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "SorcFX2T1"
  },

  // MT_SORCFX3
  {
    -1, 1000, 8, 0, 15, 22, 65, 100, 0,
    MF_NOBLOCKMAP|MF_MISSILE,
    MF2_NOTELEPORT,
    SFX_SORCERER_BISHOPSPAWN, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SORCFX3_1], NULL, NULL, NULL, NULL,
    &states[S_BISHMORPH1], NULL, NULL, NULL,
    NULL,
    "SorcFX3"
  },

  // MT_SORCFX3_EXPLOSION
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_ALTSHADOW,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SORCFX3_EXP1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "SorcFX3Explosion"
  },

  // MT_SORCFX4
  {
    -1, 1000, 8, 0, 12, 10, 10, 100, 0,
    MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, SFX_SORCERER_BALLEXPLODE, sfx_None,
    &states[S_SORCFX4_1], NULL, NULL, NULL, NULL,
    &states[S_SORCFX4_D1], NULL, NULL, NULL,
    NULL,
    "SorcFX4"
  },

  // MT_SORCSPARK1
  {
    -1, 1000, 8, 0, 0, 5, 5, 100, 0,
    MF_NOBLOCKMAP|MF_DROPOFF,
    MF2_NOTELEPORT|MF2_LOGRAV,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SORCSPARK1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "SorcSpark1"
  },

  // MT_BLASTEFFECT
  {
    -1, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPLINE|MF_NOCLIPTHING|MF_ALTSHADOW,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BLASTEFFECT1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_WATER_DRIP
  {
    -1, 1000, 8, 0, 0, 20, 16, 1, 0,
    MF_MISSILE | MF_NOSCORCH,
    MF2_LOGRAV|MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, SFX_DRIP, sfx_None,
    &states[S_WATERDRIP1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_KORAX
  {
    10200, 5000, 8, 20, 10, 65, 115, 2000, 15,
    FLAGS_monster,
    FLAGS2_monster | MF2_FOOTCLIP | MF2_TELESTOMP | MF2_BOSS | MF2_NOTARGET,
    SFX_KORAX_SIGHT, SFX_KORAX_ATTACK, SFX_KORAX_PAIN, SFX_KORAX_DEATH, SFX_KORAX_ACTIVE,
    &states[S_KORAX_LOOK1], &states[S_KORAX_CHASE2], NULL, &states[S_KORAX_ATTACK1], &states[S_KORAX_PAIN1],
    &states[S_KORAX_DEATH1], NULL, NULL, NULL,
    NULL,
    "Korax"
  },

  // MT_KORAX_SPIRIT1
  {
    -1, 1000, 8, 0, 8, 20, 16, 100, 0,
    FLAGS_missile | MF_ALTSHADOW|MF_NOCLIPLINE|MF_NOCLIPTHING,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_KSPIRIT_ROAM1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "KoraxSpirit"
  },

  // MT_KORAX_SPIRIT2
  {
    -1, 1000, 8, 0, 8, 20, 16, 100, 0,
    FLAGS_missile | MF_ALTSHADOW|MF_NOCLIPLINE|MF_NOCLIPTHING,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_KSPIRIT_ROAM1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_KORAX_SPIRIT3
  {
    -1, 1000, 8, 0, 8, 20, 16, 100, 0,
    FLAGS_missile | MF_ALTSHADOW|MF_NOCLIPLINE|MF_NOCLIPTHING,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_KSPIRIT_ROAM1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_KORAX_SPIRIT4
  {
    -1, 1000, 8, 0, 8, 20, 16, 100, 0,
    FLAGS_missile | MF_ALTSHADOW|MF_NOCLIPLINE|MF_NOCLIPTHING,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_KSPIRIT_ROAM1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_KORAX_SPIRIT5
  {
    -1, 1000, 8, 0, 8, 20, 16, 100, 0,
    FLAGS_missile | MF_ALTSHADOW|MF_NOCLIPLINE|MF_NOCLIPTHING,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_KSPIRIT_ROAM1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_KORAX_SPIRIT6
  {
    -1, 1000, 8, 0, 8, 20, 16, 100, 0,
    FLAGS_missile | MF_ALTSHADOW|MF_NOCLIPLINE|MF_NOCLIPTHING,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_KSPIRIT_ROAM1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  },

  // MT_DEMON_MASH
  {
    -1, 250, 8, 50, 13, 32, 64, 220, 0,
    FLAGS_monster|MF_ALTSHADOW|MF_NOBLOOD,
    FLAGS2_monster | MF2_FOOTCLIP, //|MF2_BLASTED
    SFX_DEMON_SIGHT, SFX_DEMON_ATTACK, SFX_DEMON_PAIN, SFX_DEMON_DEATH, SFX_DEMON_ACTIVE,
    &states[S_DEMN_LOOK1], &states[S_DEMN_CHASE1], &states[S_DEMN_ATK1_1], &states[S_DEMN_ATK2_1], &states[S_DEMN_PAIN1],
    NULL, NULL, NULL, NULL,
    NULL,
    "Demon1Mash"
  },

  // MT_DEMON2_MASH
  {
    -1, 250, 8, 50, 13, 32, 64, 220, 0,
    FLAGS_monster|MF_ALTSHADOW|MF_NOBLOOD,
    FLAGS2_monster | MF2_FOOTCLIP, //|MF2_BLASTED
    SFX_DEMON_SIGHT, SFX_DEMON_ATTACK, SFX_DEMON_PAIN, SFX_DEMON_DEATH, SFX_DEMON_ACTIVE,
    &states[S_DEMN2_LOOK1], &states[S_DEMN2_CHASE1], &states[S_DEMN2_ATK1_1], &states[S_DEMN2_ATK2_1], &states[S_DEMN2_PAIN1],
    NULL, NULL, NULL, NULL,
    NULL,
    "Demon2Mash"
  },

  // MT_ETTIN_MASH
  {
    -1, 175, 8, 60, 13, 25, 68, 175, 3,
    FLAGS_monster|MF_ALTSHADOW|MF_NOBLOOD,
    FLAGS2_monster | MF2_FOOTCLIP, //|MF2_BLASTED
    SFX_ETTIN_SIGHT, SFX_ETTIN_ATTACK, SFX_ETTIN_PAIN, SFX_ETTIN_DEATH, SFX_ETTIN_ACTIVE,
    &states[S_ETTIN_LOOK1], &states[S_ETTIN_CHASE1], &states[S_ETTIN_ATK1_1], NULL, &states[S_ETTIN_PAIN1],
    NULL, NULL, NULL, NULL,
    NULL,
    "EttinMash"
  },

  // MT_CENTAUR_MASH
  {
    -1, 200, 8, 135, 13, 20, 64, 120, 0,
    FLAGS_monster|MF_ALTSHADOW|MF_NOBLOOD,
    FLAGS2_monster | MF2_FOOTCLIP, //|MF2_BLASTED
    SFX_CENTAUR_SIGHT, SFX_CENTAUR_ATTACK, SFX_CENTAUR_PAIN, SFX_CENTAUR_DEATH, SFX_CENTAUR_ACTIVE,
    &states[S_CENTAUR_LOOK1], &states[S_CENTAUR_WALK1], &states[S_CENTAUR_ATK1], NULL, &states[S_CENTAUR_PAIN1],
    NULL, NULL, NULL, NULL,
    NULL,
    "CentaurMash"
  },

  // MT_KORAX_BOLT
  {
    -1, 1000, 8, 0, 0, 15, 35, 100, 0,
    FLAGS_missile,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_KBOLT1], NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "KoraxBolt"
  },

  // MT_BAT_SPAWNER
  {
    10225, 1000, 8, 0, 0, 20, 16, 100, 0,
    MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY,
    MF2_DONTDRAW,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_SPAWNBATS1], &states[S_SPAWNBATS_OFF], &states[S_SPAWNBATS1], NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "BatSpawner"
  },

  // MT_BAT
  {
    -1, 1000, 8, 0, 5, 3, 3, 100, 0,
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_MISSILE | MF_NOSCORCH,
    MF2_NOTELEPORT,
    sfx_None, sfx_None, sfx_None, sfx_None, sfx_None,
    &states[S_BAT1], NULL, NULL, NULL, NULL,
    &states[S_BAT_DEATH], NULL, NULL, NULL,
    NULL,
    "Bat"
  }
};
