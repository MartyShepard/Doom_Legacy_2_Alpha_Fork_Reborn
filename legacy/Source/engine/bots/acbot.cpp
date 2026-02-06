// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: acbot.cpp 533 2009-06-11 08:31:10Z smite-meister $
//
// Copyright (C) 2002-2007 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief ACBot implementation

#include <math.h>
#include <stdlib.h>

#include "command.h"
#include "cvars.h"

#include "d_ticcmd.h"
#include "b_path.h"
#include "acbot.h"

#include "g_game.h"
#include "g_player.h"
#include "g_map.h"
#include "g_blockmap.h"
#include "g_pawn.h"

#include "d_items.h"
#include "p_maputl.h"
#include "r_defs.h"

#include "m_random.h"
#include "tables.h"
//#include "m_dll.h"

#ifndef M_PI
# define M_PI 3.14159265358979323846 // matches value in gcc v2 math.h
#endif

#define ANG5 (ANG90/18)


/*
// TODO eliminate dependence on various externs, convert into a DLL plugin
DATAEXPORT dll_info_t dll_info = {0, 1, "Doom Legacy ACBot plugin"};
*/

const fixed_t MAXSTEP = 24;
const fixed_t MAXWATERSTEP = 37;

static int botforwardmove[2] = {50, 100};
static int botsidemove[2]    = {48, 80};
static int botangleturn[4]   = {500, 1000, 2000, 4000};


/// A target Actor for ACBot
struct ai_target_t
{
  Actor   *a;
  fixed_t  dist;
};

// temp variables
static ai_target_t cMissile; // closest dangerous missile (must be avoided)

static ai_target_t cEnemy;       // closest directly visible enemy
static ai_target_t cUnseenEnemy; // closest nonvisible enemy (node-reachable)

static ai_target_t fTeammate;       // furthest directly reachable teammate
static ai_target_t cUnseenTeammate; // closest not directly reachable teammate (node-reachable)

static ai_target_t bItem; // best directly reachable AND visible item
static float  bItemWeight;
static ai_target_t bUnseenItem; // best non-visible item (node-reachable)
static float  bUnseenItemWeight;

static float bWeaponValue; // value of the best currently usable weapon
static bool  HaveWeaponFor[NUMAMMO]; // does the bot have a weapon for the given ammotype?

static fixed_t JumpHeight; // How high can the bots jump?

//=================================================================
//     Simple straight-line reachability checks
//=================================================================
static Actor	*bot, *goal;
static sector_t *last_sector;
static fixed_t   last_floorz;

/// returns true if the intercepting object can be bypassed
static bool PTR_QuickReachable(intercept_t *in)
{
  if (in->isaline)
    {
      line_t *line = in->line;

      if (!(line->flags & ML_TWOSIDED) || (line->flags & ML_BLOCKING))
	return false; //Cannot continue.
      else
	{
	  // determine the next sector to be traversed
	  sector_t *s = (line->backsector == last_sector) ? line->frontsector : line->backsector;
	  range_t r = s->FindZRange(bot);

	  if ((r.low <= last_floorz + ((last_sector->floortype != FLOOR_WATER) ? (JumpHeight + MAXSTEP) : MAXWATERSTEP)) && // can we jump there?
	      (((r.high == r.low) && line->special) ||
	       (r.high - r.low >= bot->height))) // do we fit?
	    {
	      last_sector = s;
	      last_floorz = r.low; // climb from floor to floor
	      return true;
	    }
	  else
	    return false;
	}
    }
  else
    {
      Actor *thing = in->thing;
      if (thing != bot && thing != goal && (thing->flags & MF_SOLID))
	return false;
    }

  return true;
}


// Checks if the bot can get to the actor 'goal' by simply
// walking towards it, using "doors" and possibly jumping.
bool ACBot::QuickReachable(Actor *g)
{
  bot = pawn;
  goal = g;
  last_sector = pawn->subsector->sector;
  last_floorz = pawn->Feet(); // 3d floors...

  // Bots shouldn't try to get stuff that's on a 3dfloor they can't get to. SSNTails 06-10-2003
  if (last_sector == goal->subsector->sector)
    for (ffloor_t *rover = last_sector->ffloors; rover; rover = rover->next)
      {
        if (!(rover->flags & FF_SOLID) || !(rover->flags & FF_EXISTS))
	  continue;

	if (*rover->topheight <= pawn->Feet() && goal->Top() < *rover->topheight)
	  return false;

	if (*rover->bottomheight >= pawn->Top()
	    && goal->Feet() > *rover->bottomheight)
	  return false;
      }

  return mp->blockmap->PathTraverse(pawn->pos, goal->pos, PT_ADDLINES|PT_ADDTHINGS, PTR_QuickReachable);
}


// Checks if the bot can get to the point (x,y) by simply
// walking towards it, using "doors" and possibly jumping.
bool ACBot::QuickReachable(fixed_t x, fixed_t y)
{
  bot = pawn;
  goal = NULL;
  last_sector = pawn->subsector->sector;
  last_floorz = pawn->Feet(); // 3d floors...
  vec_t<fixed_t> r(x, y, 0);

  return mp->blockmap->PathTraverse(pawn->pos, r, PT_ADDLINES|PT_ADDTHINGS, PTR_QuickReachable);
}





//=================================================================
//   ACBot basics
//=================================================================

ACBot::ACBot(int sk)
{
  skill = sk;
  num_weapons = 0;

  straferight = false;

  avoidtimer = 0;
  blockedcount = 0;
  strafetimer = 0;
  weaponchangetimer = 0;

  lastTarget = NULL;
  destination = NULL;

  // how high can we jump?
  fixed_t g = cv_gravity.Get();
  if (g < 0.1)
    {
      JumpHeight = 0;
      return;
    }
  fixed_t vz = cv_jumpspeed.Get();
  fixed_t temp = (vz/g).floor();
  JumpHeight = -8 +vz*(temp+1) -0.5*g*temp*(temp+1); // some margin
}


/// Clears the current path
void ACBot::ClearPath()
{
#ifdef SHOWBOTPATH
  while (!path.empty())
    {
      if (path.front()->marker)
	path.front()->marker->Remove();

      path.pop_front();
    }
#endif
  path.clear();
}




//=================================================================
//  ACBot pickup AI
//=================================================================

enum ai_item_e
{
  F_WEAPON,
  F_AMMO,
  F_HEAL,
  F_ARMOR,
  F_KEY,
  F_POWERUP,
};

/// defines pickup values for the AI
struct ai_item_t
{
  mobjtype_t mtype; ///< item type
  float weight;     ///< value
  int type;         ///< classification
  int misc; ///< different enum/int values depending on type
};


static ai_item_t item_ai[] =
{
  // weapon (if not in possession, x-2 if having adequate weapon and ammo for it, x if not.
  // if already have it and need ammo and teammates don't need it, 3
  // good: chaingun, missile, plasma, supershotgun
  {MT_SHAINSAW,     3, F_WEAPON, wp_chainsaw},
  {MT_SHOTGUN,      6, F_WEAPON, wp_shotgun},
  {MT_SUPERSHOTGUN, 7, F_WEAPON, wp_supershotgun},
  {MT_CHAINGUN,     6, F_WEAPON, wp_chaingun}, // 6 5 3 ???
  {MT_ROCKETLAUNCH, 7, F_WEAPON, wp_missile},
  {MT_PLASMAGUN,    7, F_WEAPON, wp_plasma},
  {MT_BFG9000,      7, F_WEAPON, wp_bfg},
  {MT_WGAUNTLETS,   3, F_WEAPON, wp_gauntlets},
  {MT_WCROSSBOW,    6, F_WEAPON, wp_crossbow},
  {MT_WBLASTER,     6, F_WEAPON, wp_blaster},
  {MT_WPHOENIXROD,  7, F_WEAPON, wp_phoenixrod},
  {MT_WSKULLROD,    7, F_WEAPON, wp_skullrod},
  {MT_WMACE,        7, F_WEAPON, wp_mace},
  {MT_FW_AXE,       6, F_WEAPON, wp_timons_axe},
  {MT_CW_SERPSTAFF, 6, F_WEAPON, wp_serpent_staff},
  {MT_MW_CONE,      6, F_WEAPON, wp_cone_of_shards},
  {MT_FW_HAMMER,    7, F_WEAPON, wp_hammer_of_retribution},
  {MT_CW_FLAME,     7, F_WEAPON, wp_firestorm},
  {MT_MW_LIGHTNING, 7, F_WEAPON, wp_arc_of_death},
  {MT_FW_SWORD1,    8, F_WEAPON, wp_quietus},
  {MT_FW_SWORD2,    8, F_WEAPON, wp_quietus},
  {MT_FW_SWORD3,    8, F_WEAPON, wp_quietus},
  {MT_CW_HOLY1,     8, F_WEAPON, wp_wraithverge},
  {MT_CW_HOLY2,     8, F_WEAPON, wp_wraithverge},
  {MT_CW_HOLY3,     8, F_WEAPON, wp_wraithverge},
  {MT_MW_STAFF1,    8, F_WEAPON, wp_bloodscourge},
  {MT_MW_STAFF2,    8, F_WEAPON, wp_bloodscourge},
  {MT_MW_STAFF3,    8, F_WEAPON, wp_bloodscourge},

  // ammo (if dry on this type and have a weapon for it and using an inferior weapon, x, otherwise if room to take it, x-3
  {MT_CLIP,       6,   F_AMMO, am_clip},
  {MT_AMMOBOX,    6.5, F_AMMO, am_clip},
  {MT_SHELL,      6,   F_AMMO, am_shell},
  {MT_SHELLBOX,   6.5, F_AMMO, am_shell},
  {MT_CELL,       6,   F_AMMO, am_cell},
  {MT_CELLPACK,   6.5, F_AMMO, am_cell},
  {MT_ROCKETAMMO, 6,   F_AMMO, am_misl},
  {MT_ROCKETBOX,  6.5, F_AMMO, am_misl},
  {MT_BACKPACK,   7,   F_AMMO, am_clip},
  {MT_AMGWNDWIMPY, 6,   F_AMMO, am_goldwand},
  {MT_AMGWNDHEFTY, 6.5, F_AMMO, am_goldwand},
  {MT_AMCBOWWIMPY, 6,   F_AMMO, am_crossbow},
  {MT_AMCBOWHEFTY, 6.5, F_AMMO, am_crossbow},
  {MT_AMBLSRWIMPY, 6,   F_AMMO, am_blaster},
  {MT_AMBLSRHEFTY, 6.5, F_AMMO, am_blaster},
  {MT_AMSKRDWIMPY, 6,   F_AMMO, am_skullrod},
  {MT_AMSKRDHEFTY, 6.5, F_AMMO, am_skullrod},
  {MT_AMPHRDWIMPY, 6,   F_AMMO, am_phoenixrod},
  {MT_AMPHRDHEFTY, 6.5, F_AMMO, am_phoenixrod},
  {MT_AMMACEWIMPY, 6,   F_AMMO, am_mace},
  {MT_AMMACEHEFTY, 6.5, F_AMMO, am_mace},
  {MT_BAGOFHOLDING, 7,  F_AMMO, am_goldwand},
  {MT_MANA1, 6,   F_AMMO, am_mana1},
  {MT_MANA2, 6,   F_AMMO, am_mana2},
  {MT_MANA3, 6.5, F_AMMO, am_mana1},

#define MAXSOUL  200
#define MAXARMOR 200

  // medication, take if you need or are selfish, low health => more interest, low skill => less interest
  {MT_MEGA,          9, F_HEAL, MAXSOUL}, // MAXARMOR too...
  {MT_BERSERKPACK,   9, F_HEAL, MAXSOUL}, // pw_strength too..., also, more skill => less interest?
  {MT_SOULSPHERE,    8, F_HEAL, MAXSOUL},
  {MT_MEDI,          6, F_HEAL, MAXSOUL/2},
  {MT_STIM,          6, F_HEAL, MAXSOUL/2},
  {MT_HEALTHBONUS,   1, F_HEAL, MAXSOUL},
  {MT_ARTISUPERHEAL, 8, F_HEAL, MAXSOUL},
  {MT_HEALINGBOTTLE, 1, F_HEAL, MAXSOUL/2},

  // armor, take if you can (can => need), low skill => less interest
  {MT_BLUEARMOR,   8, F_ARMOR, MAXARMOR},
  {MT_GREENARMOR,  5, F_ARMOR, MAXARMOR/2},
  {MT_ARMORBONUS,  1, F_ARMOR, MAXARMOR},
  {MT_ITEMSHIELD2, 8, F_ARMOR, MAXARMOR},
  {MT_ITEMSHIELD1, 5, F_ARMOR, MAXARMOR/2},
  {MT_ARMOR_1,     5, F_ARMOR, 15}, // roughly
  {MT_ARMOR_2,     6, F_ARMOR, 15},
  {MT_ARMOR_3,     5, F_ARMOR, 15},
  {MT_ARMOR_4,     6, F_ARMOR, 15},

  // keys (take it if you don't already have it)
  {MT_BLUECARD,    5, F_KEY, it_bluecard},
  {MT_YELLOWCARD,  5, F_KEY, it_yellowcard},
  {MT_REDCARD,     5, F_KEY, it_redcard},
  {MT_BLUESKULL,   5, F_KEY, it_blueskull},
  {MT_YELLOWSKULL, 5, F_KEY, it_yellowskull},
  {MT_REDSKULL,    5, F_KEY, it_redskull},
  {MT_BKEY,        5, F_KEY, it_bluecard},
  {MT_CKEY,        5, F_KEY, it_yellowcard},
  {MT_AKEY,        5, F_KEY, it_redcard},
  {MT_KEY1,        5, F_KEY, it_key_1},
  {MT_KEY2,        5, F_KEY, it_key_2},
  {MT_KEY3,        5, F_KEY, it_key_3},
  {MT_KEY4,        5, F_KEY, it_key_4},
  {MT_KEY5,        5, F_KEY, it_key_5},
  {MT_KEY6,        5, F_KEY, it_key_6},
  {MT_KEY7,        5, F_KEY, it_key_7},
  {MT_KEY8,        5, F_KEY, it_key_8},
  {MT_KEY9,        5, F_KEY, it_key_9},
  {MT_KEYA,        5, F_KEY, it_key_A},
  {MT_KEYB,        5, F_KEY, it_key_B},

  // doom powerups, take if you need or are selfish
  {MT_INV, 10, F_POWERUP, pw_invulnerability},
  {MT_INS,  9, F_POWERUP, pw_invisibility},
  {MT_ARTIINVULNERABILITY, 10, F_POWERUP, pw_invulnerability},
  {MT_ARTIINVISIBILITY,     9, F_POWERUP, pw_invisibility},
  {MT_RADSUIT, 5, F_POWERUP, pw_ironfeet},

  // TODO inventory items (acbot must also be made to use them!)

  {MT_NONE}
};







void ACBot::LookForThings()
{
  cMissile.a = NULL;
  cMissile.dist = fixed_t::FMAX;

  cEnemy.a = NULL;
  cEnemy.dist = fixed_t::FMAX;
  cUnseenEnemy.a = NULL;
  cUnseenEnemy.dist = fixed_t::FMAX;

  fTeammate.a = NULL;
  fTeammate.dist = 0;
  cUnseenTeammate.a = NULL;
  cUnseenTeammate.dist = fixed_t::FMAX;

  bItem.a = NULL;
  bItem.dist = fixed_t::FMAX;
  bItemWeight = 0.0;
  bUnseenItem.a = NULL;
  bUnseenItem.dist = fixed_t::FMAX;
  bUnseenItemWeight = 0.0;

  // search through the list of all thinkers

  for (Thinker *th = mp->thinkercap.Next(); th != &mp->thinkercap; th = th->Next())
    {
      Actor *actor = th->Inherits<Actor>();
      if (!actor)
	continue; // keep looking

      DActor *da = actor->Inherits<DActor>();
      mobjtype_t type = da ? da->type : MT_NONE;
      fixed_t dist = P_XYdist(pawn->pos, actor->pos);
      bool enemyFound = false;
      SearchNode_t *node;

      if ((actor->flags & MF_MONSTER || type == MT_BARREL) &&
	  (actor->flags & MF_SOLID))
	enemyFound = true; // a live monster
      else if (actor->flags & MF_PLAYER && actor->flags & MF_SOLID && actor != pawn)
	{
	  // a playerpawn or equivalent, but not ours
	  if (actor->team != subject->team)
	    enemyFound = true;
	  else
	    {
	      // a teammate (TODO prefer "unseen" humans to "seen" bots)
	      if (QuickReachable(actor)) // is there a direct route to the teammate?
		{
		  if ((dist > fTeammate.dist))
		    {
		      fTeammate.dist = dist;
		      fTeammate.a = actor;
		    }
		}
	      else
		{
		  node = mp->botnodes->GetNodeAt(actor->pos);
		  if (node && dist < cUnseenTeammate.dist)
		    {
		      cUnseenTeammate.dist = dist;
		      cUnseenTeammate.a = actor;
		    }
		}
	    }
	}
      else if ((actor->flags & MF_MISSILE) && actor->owner != pawn) // a threatening missile
	{
	  // see if the missile is heading my way
	  vec_t<fixed_t> dpos = actor->pos - pawn->pos;
	  vec_t<fixed_t> dv   = actor->vel - pawn->vel;
	  if (dot(dpos, dv) < 0)
	    {
	      //if its the closest missile and its reasonably close I should try and avoid it
	      if (dist != 0 && (dist < cMissile.dist) && (dist <= 300))
		{
		  cMissile.dist = dist;
		  cMissile.a = actor;
		}
	    }
	}
      else if (actor->flags & MF_SPECIAL) // most likely a pickup
	{
	  float weight = 0.0;
	  bool selfish = cv_deathmatch.value;
	  for (ai_item_t *t = item_ai; t->mtype != MT_NONE; t++)
	    if (t->mtype == type)
	      {
		switch (t->type)
		  {
		  case F_WEAPON:
		    if (!pawn->weaponowned[t->misc])		
		      {
			weight = t->weight;
			if (bWeaponValue >= 50)
			  weight -= 2;
		      }
		    else if (selfish || (actor->flags & MF_DROPPED))
		      {
			int atype = wpnlev1info[t->misc].ammo;
			if (pawn->ammo[atype] < pawn->maxammo[atype])
			  weight = 3;
		      }
		    break;

		  case F_AMMO:
		    if (!pawn->ammo[t->misc] && HaveWeaponFor[t->misc] && bWeaponValue < 10) // fist, chainsaw
		      weight = t->weight;
		    else if (pawn->ammo[t->misc] < pawn->maxammo[t->misc])
		      weight = t->weight - 3;
		    break;

		  case F_HEAL:
		    if (selfish)
		      weight = t->weight;
		    else if (pawn->health < t->misc)
		      {
			weight = t->weight + (skill - sk_nightmare); // harder skill => health is valued higher
			if (pawn->health >= 80) weight -= 3;
			else if (pawn->health >= 60) weight -= 2;
			else if (pawn->health >= 50) weight -= 1;
		
			if (weight < 1)
			  weight = 1;
		      }
		    break;

		  case F_ARMOR:
		    if (selfish || (pawn->armorpoints[0] < t->misc))
		      weight = t->weight;
		    break;

		  case F_KEY:
		    if (!(pawn->keycards & t->misc))
		      weight = t->weight;
		    break;

		  case F_POWERUP:
		    if (selfish || !pawn->powers[t->misc])
		      weight = t->weight;
		    break;
		  }
		break;
	      }

	  if (mp->CheckSight(pawn, actor) && QuickReachable(actor))
	    {
	      if (weight > bItemWeight || (weight == bItemWeight && dist < bItem.dist))
		{
		  bItem.a = actor;
		  bItem.dist = dist;
		  bItemWeight = weight;
		}
	    }
	  else
	    {
	      // item is not gettable atm, may use a search later to find a path to it
	      node = mp->botnodes->GetNodeAt(actor->pos);
	      if (node  //&& P_AproxDistance(posX2x(node->x) - actor->x, posY2y(node->y) - actor->y) < (BOTNODEGRIDSIZE << 1)
		  && (weight > bUnseenItemWeight || (weight == bUnseenItemWeight && dist < bUnseenItem.dist)))
		{
		  bUnseenItem.a = actor;
		  bUnseenItem.dist = dist;
		  bUnseenItemWeight = weight;
		  //CONS_Printf("best item set to x:%d y:%d for type:%d\n", actor->pos.x.floor(), actor->pos.y.floor(), actor->type);
		}

	      //if (!node)
	      // CONS_Printf("could not find a node here x:%d y:%d for type:%d\n", actor->pos.x.floor(), actor->pos.y.floor(), actor->type);
	    }
	}

      if (enemyFound)
	{
	  if (mp->CheckSight(pawn, actor))
	    {
	      // TODO prefer player enemies to monster enemies
	      // 
	      if (dist < cEnemy.dist || (actor->flags & MF_PLAYER && !(cEnemy.a->flags & MF_PLAYER)))
		{
		  cEnemy.dist = dist;
		  cEnemy.a = actor;
		}
	    }
	  else
	    {
	      node = mp->botnodes->GetNodeAt(actor->pos);
	      if (node &&
		  (dist < cUnseenEnemy.dist ||
		   (actor->flags & MF_PLAYER && !(cEnemy.a->flags & MF_PLAYER))))
		{
		  cUnseenEnemy.dist = dist;
		  cUnseenEnemy.a = actor;
		}
	    }
	}
    }
}




//=================================================================
//    Bots using special linedefs
//=================================================================

// This function looks through the sector the bot is in and, one sector level outwards,
// very inefficient cause searches of sectors are done multiple times
// when a sector has many linedefs between a single sector-sector boundary
//  must fix this, perhaps use the visited bool
// maybe should do search through thes switches array instead
bool ACBot::LookForSpecialLine(fixed_t *x, fixed_t *y)
{
  for (msecnode_t *secnode = pawn->touching_sectorlist; secnode; secnode = secnode->m_tnext)
    {
      sector_t *sec = secnode->m_sector;
      for (int i = 0; i < sec->linecount; i++)
	{
	  line_t *edge = sec->lines[i];
	  int spac = GET_SPAC(edge->flags);
	  int special = edge->special;
	  //sector_t *specialsector = (sec == edge->frontsector) ? edge->backsector : edge->frontsector;
	  if (!(edge->flags & ML_REPEAT_SPECIAL) && (spac == SPAC_USE || spac == SPAC_PASSUSE))
	    // && (special == 11 || special == 21 || special == 200))
	    //(!specialsector || !specialsector->ceilingdata))
	    //!(line->flags & ML_TWOSIDED) || (line->flags & ML_BLOCKING))
	    //(edge->frontsector == sec) //if its a pressable switch
	    {
	      *x = (edge->v1->x + edge->v2->x) / 2;
	      *y = (edge->v1->y + edge->v2->y) / 2;

	      return true;
	    }
	  else if (edge->sideptr[1]) //if its a double sided sector
	    {
	      sector_t *sector = (sec == edge->frontsector) ? edge->backsector : edge->frontsector;

	      for (int j = 0; j < sector->linecount; j++)
		{
		  edge = sector->lines[j];
		  spac = GET_SPAC(edge->flags);
		  special = edge->special;

		  if (!(edge->flags & ML_REPEAT_SPECIAL) && (spac == SPAC_USE || spac == SPAC_PASSUSE))
		    //&& (special == 11 || special == 21 || special == 200))
		    {
		      *x = (edge->v1->x + edge->v2->x) / 2;
		      *y = (edge->v1->y + edge->v2->y) / 2;

		      return true;
		    }
		}
	    }
	}
    }

  return false;
}




//=================================================================
//   ACBot combat AI
//=================================================================

/// Weapon information for the AI system
struct ai_weapon_t
{
  byte       type;       // flags
  float      value;      // preference when changing weapon (not exactly the original ones!)
  mobjtype_t missile;    // or MT_NONE
  fixed_t    dangerdist; // blast radius
};

enum ai_weapon_e
{
  MELEE   = 0,
  INSTANT = 1,
  MISSILE = 2,
  TYPEMASK = 0x3,
  AF  = 0x4 // use area fire (shoot in the general direction of the enemy)
};


static ai_weapon_t ai_weapon_data[NUMWEAPONS] =
{
  /*
  wp_fist, wp_chainsaw, wp_pistol,
  wp_shotgun, wp_supershotgun, wp_chaingun,
  wp_missile,  wp_plasma,  wp_bfg,
  */
  {MELEE, 0.1, MT_NONE, 0}, {MELEE, 1, MT_NONE, 0}, {INSTANT|AF, 0.5, MT_NONE, 0},
  {INSTANT, 30, MT_NONE, 0}, {INSTANT, 55, MT_NONE, 0}, {INSTANT|AF, 50, MT_NONE, 0},
  {MISSILE, 50, MT_ROCKET, 100}, {MISSILE|AF, 50, MT_PLASMA, 0}, {MISSILE, 20, MT_BFG, 0},
  
  /*
  wp_staff, wp_gauntlets, wp_goldwand,
  wp_crossbow, wp_blaster, wp_phoenixrod,
  wp_skullrod, wp_mace, wp_beak,
  */
  {MELEE, 0.1, MT_NONE, 0}, {MELEE, 1, MT_NONE, 0}, {INSTANT|AF, 0.5, MT_NONE, 0},
  {MISSILE, 35, MT_CRBOWFX1, 0}, {INSTANT|AF, 50, MT_NONE, 0}, {MISSILE, 50, MT_PHOENIXFX1, 100},
  {MISSILE|AF, 55, MT_HORNRODFX1, 0}, {MISSILE|AF, 50, MT_MACEFX1, 0}, {MELEE, 0.1, MT_NONE, 0},

  /*
  wp_fpunch, wp_cmace, wp_mwand,
  wp_timons_axe, wp_serpent_staff, wp_cone_of_shards,
  wp_hammer_of_retribution, wp_firestorm, wp_arc_of_death,
  wp_quietus, wp_wraithverge, wp_bloodscourge, wp_snout
  */
  {MELEE, 10, MT_NONE, 0}, {MELEE, 1, MT_NONE, 0}, {INSTANT|AF, 20, MT_NONE, 0},
  {MELEE, 30, MT_NONE, 0}, {MISSILE, 30, MT_CSTAFF_MISSILE, 0}, {MISSILE, 30, MT_SHARDFX1, 0},
  {MISSILE, 40, MT_HAMMER_MISSILE, 0}, {INSTANT, 40, MT_NONE, 0}, {MISSILE, 40, MT_LIGHTNING_FLOOR, 0},
  {MISSILE, 50, MT_FSWORD_MISSILE, 0}, {MISSILE, 50, MT_HOLY_MISSILE, 0}, {MISSILE, 50, MT_MSTAFF_FX2, 0}, {MELEE, 0.1, MT_NONE, 0}
};



void ACBot::AvoidMissile(const Actor *missile)
{
  int delta = pawn->yaw - R_PointToAngle2(pawn->pos, missile->pos);

  if (delta >= 0)
    cmd->side = -botsidemove[1];
  else
    cmd->side = botsidemove[1];
}


void ACBot::ChangeWeapon()
{
  bool weapon_usable[NUMWEAPONS]; // good for at least one shot
  int i, n = 0;

  bWeaponValue = 0;

  ai_weapon_data[wp_fist].value = pawn->powers[pw_strength] ? 5 : 0.1;

  for (i=0; i<NUMAMMO; i++)
    HaveWeaponFor[i] = false;

  for (i=0; i<NUMWEAPONS; i++)
    {
      ammotype_t at = pawn->weaponinfo[i].ammo;
      if (pawn->weaponowned[i])
	{
	  if (at == am_manaboth)
	    HaveWeaponFor[am_mana1] = HaveWeaponFor[am_mana2] = true;
	  else if (at == am_noammo)
	    ;
	  else 
	    HaveWeaponFor[at] = true;
	}
	  
      weapon_usable[i] = pawn->weaponowned[i] &&
	(at == am_noammo ||
	 ((at == am_manaboth) ? min(pawn->ammo[am_mana1], pawn->ammo[am_mana2]) : pawn->ammo[at]) >= pawn->weaponinfo[i].ammopershoot);

      if (weapon_usable[i])
	{
	  n++;
	  if (ai_weapon_data[i].value > bWeaponValue)
	    bWeaponValue = ai_weapon_data[i].value;
	}
    }

  // should we change?
  if (!weaponchangetimer || !weapon_usable[pawn->readyweapon] || n != num_weapons)
    {
      weapon_usable[pawn->readyweapon] = false; // must change

      float sum = 0;
      for (i=0; i<NUMWEAPONS; i++)
	if (weapon_usable[i])
	  sum += ai_weapon_data[i].value;

      if (sum > 0)
	{
	  float r = Random() * sum;

	  for (i=0; i<NUMWEAPONS; i++)
	    if (weapon_usable[i] && r < ai_weapon_data[i].value)
	      break;
	    else
	      r -= ai_weapon_data[i].value;

	  cmd->buttons &= ~ticcmd_t::BT_ATTACK; // stop rocket from jamming;
	  cmd->buttons |= (i + 1) << ticcmd_t::WEAPONSHIFT;
	}
      else
	; // no usable weapons! Should not happen.

      weaponchangetimer = Random()*2048 + 1000; // how long until I next change my weapon
    }
  else
    weaponchangetimer--;

  if (n != num_weapons)
    cmd->buttons &= ~ticcmd_t::BT_ATTACK; //stop rocket from jamming;
  num_weapons = n;

  //CONS_Printf("weaponchangetimer is %d\n", weaponchangetimer);
}



void ACBot::TurnTowardsPoint(fixed_t x, fixed_t y)
{
  angle_t ang = R_PointToAngle2(pawn->pos.x, pawn->pos.y, x, y);
  int delta = ang - pawn->yaw;

  int botspeed;

  if (unsigned(abs(delta)) < (ANG45 >> 2))
    botspeed = 0;
  else if (unsigned(abs(delta)) < ANG45)
    botspeed = 2; // 1
  else
    botspeed = 3; // 1

  if (unsigned(abs(delta)) < ANG5)
    cmd->yaw = ang >> 16; //perfect aim
  else if (delta > 0)
    cmd->yaw += botangleturn[botspeed];
  else
    cmd->yaw -= botangleturn[botspeed];
}



void ACBot::AimWeapon()
{
  ai_weapon_t *wdata = &ai_weapon_data[pawn->readyweapon];

  mobjtype_t miss = wdata->missile;

  float m_speed = 0;
  if (miss != MT_NONE)
    m_speed = mobjinfo[miss].speed;

  if (skill < sk_hard)
    m_speed = 0; // no leading

  Actor *dest = cEnemy.a;
  fixed_t dist = P_XYdist(dest->pos, pawn->pos);

  if (dist > wdata->dangerdist)
    {
      vec_t<fixed_t> temp;

      if (m_speed > 0)
	{
	  float t = dist.Float() / m_speed;
	  temp = dest->pos + dest->vel * t;
	  t = (P_XYdist(temp, pawn->pos).Float() / m_speed) + 4;

	  do
	    {
	      t -= 4;
	      if (t < 0)
		t = 0;
	      temp = dest->pos + dest->vel * t;
          //} while (!mp->CheckSight2(pawn, dest, nx, ny, nz) && (t > 0));
	    } while (false); // FIXME

	  subsector_t *sec = mp->GetSubsector(temp.x, temp.y);
	  if (!sec)
	    sec = dest->subsector;

	  if (temp.z < sec->sector->floorheight)
	    temp.z = sec->sector->floorheight;
	  else if (temp.z > sec->sector->ceilingheight)
	    temp.z = sec->sector->ceilingheight - dest->height;
	}
      else
	{
	  temp = dest->pos;
	}

      angle_t angle, realAngle;
      realAngle = angle = R_PointToAngle2(pawn->pos, temp);

      cmd->pitch = int(atan(((temp.z - pawn->pos.z + (dest->height - pawn->height)/2) / dist).Float()) * (ANG180 / M_PI)) >> 16;

      int spread = RandomS()*ANG45; // was P_SignedRandom()<<21;

      if (P_AproxDistance(dest->vel.x, dest->vel.y) > 8) //enemy is moving reasonably fast, so not perfectly acurate
	{
	  if (dest->flags & MF_SHADOW)
	    angle += spread << 2;
	  else if (!m_speed)
	    angle += spread << 1;
	}
      else
	{
	  if (dest->flags & MF_SHADOW)
	    angle += spread << 1;
	  else if (!m_speed)
	    angle += spread;
	}


      int botspeed;

      int delta = angle - pawn->yaw;
      if (unsigned(abs(delta)) < (ANG45 >> 1))
	botspeed = 0;
      else if (unsigned(abs(delta)) < ANG45)
	botspeed = 1;
      else
	botspeed = 3;

      if (unsigned(abs(delta)) < ANG45)
	{
	  if (wdata->type & AF)
	    cmd->buttons |= ticcmd_t::BT_ATTACK;

	  if (unsigned(abs(delta)) <= ANG5)
	    {
	      if (skill <= sk_medium)  // check skill, if anything but nightmare bot aim is imperfect
		cmd->yaw = angle >> 16;	// not so perfect aim
	      else
		cmd->yaw = realAngle >> 16; // perfect aim

	      delta = 0;
	      /*if (p->readyweapon == wp_missile)
		{
		Actor*     tempRocket = Z_Malloc (sizeof(*tempRocket), PU_LEVEL, NULL);
		mobjinfo_t* info;

		info = &mobjinfo[MT_ROCKET];
		tempRocket->type = MT_ROCKET;
		tempRocket->info = info;
		tempRocket->x = pawn->x;
		tempRocket->y = pawn->y;
		tempRocket->z = pawn->z + 1835008;
		tempRocket->radius = info->radius;
		tempRocket->height = info->height;
		if  (B_ReachablePoint(p, GetSubsector(nx, ny)->sector, nx, ny))
		cmd->buttons |= ticcmd_t::BT_ATTACK;
 
		//B_ChangeWeapon(p);
		Z_Free(tempRocket);
		}
		else*/
	      cmd->buttons |= ticcmd_t::BT_ATTACK;
	    }
	}

      if (delta > 0)
	cmd->yaw += botangleturn[botspeed]; //turn right
      else if (delta < 0)
	cmd->yaw -= botangleturn[botspeed]; //turn left
    }
}


//=================================================================
// Main ACBot AI
//=================================================================

void ACBot::BuildInput(PlayerInfo *p, int elapsed)
{
  subject = p;
  pawn = p->pawn;
  mp = p->mp;
  cmd = &p->cmd;

  bool use_down = (cmd->buttons & ticcmd_t::BT_USE);
  // needed so bot doesn't hold down use before reaching a switch

  cmd->Clear();

  if (!subject->pawn || (subject->pawn->flags & MF_CORPSE))
    {
      cmd->buttons |= ticcmd_t::BT_USE; // I want to respawn
      return;
    }

  int botspeed = 1;
  int forwardmove = 0, sidemove = 0;

  cmd->yaw = pawn->yaw >> 16;
  cmd->pitch = 0; //pawn->pitch >> 16;

  ChangeWeapon();
  LookForThings();


  if (avoidtimer)
    {
      avoidtimer--;
      if (pawn->eflags & MFE_UNDERWATER)
	{
	  forwardmove = botforwardmove[1];
	  cmd->buttons |= ticcmd_t::BT_JUMP;
	}
      else
	{
	  if (cmd->forward > 0)
	    forwardmove = -botforwardmove[1];
	  else
	    forwardmove = botforwardmove[1];
	  sidemove = botsidemove[1];
	}
    }
  else
    {
      int dist; // how far away (in FRACUNITs) is the enemy or the wanted thing

      if (bItem.a && (bItemWeight > 5 || !cEnemy.a))
	{
	  // If a nearby item has a good weight, get it no matter what. Else only if we have no target/enemy.
	  dist = P_XYdist(pawn->pos, bItem.a->pos).floor();
	  if (dist > 64)
	    botspeed = 1;
	  else
	    botspeed = 0;
	  TurnTowardsPoint(bItem.a->pos.x, bItem.a->pos.y);
	  forwardmove = botforwardmove[botspeed];
	  if ((bItem.a->floorz - pawn->Feet()).floor() > MAXSTEP && dist <= 100)
	    cmd->buttons |= ticcmd_t::BT_JUMP;
	}
      else if (cEnemy.a)
	{
	  // attack the enemy
	  weapontype_t w = pawn->readyweapon;

	  //CONS_Printf("heading for an enemy\n");
	  dist = P_XYdist(pawn->pos, cEnemy.a->pos).floor();
	  if (dist > 300 || (ai_weapon_data[w].type & TYPEMASK) == MELEE)
	    forwardmove = botforwardmove[botspeed];
	  if (ai_weapon_data[w].dangerdist > 50 && dist < 400)
	    forwardmove = -botforwardmove[botspeed];

	  PlayerPawn *pp = cEnemy.a->Inherits<PlayerPawn>();

	  // if we are at close range or player enemy has a long range weapon, better strafe
	  // skill setting determines when bot will start strafing
	  int ai_strafedist[5] = { 32, 150, 150, 350, 650 };

	  if (dist <= ai_strafedist[skill] ||
	      (pp && skill >= sk_medium && (ai_weapon_data[pp->readyweapon].type & TYPEMASK) == INSTANT))
	    sidemove = botsidemove[botspeed];

	  AimWeapon();
	  lastTarget = cEnemy.a;
	  lastTargetX = cEnemy.a->pos.x;
	  lastTargetY = cEnemy.a->pos.y;
	}
      else
	{
	  // nothing urgent to do

	  fixed_t x, y;
	  if (LookForSpecialLine(&x, &y) && QuickReachable(x, y))
	    {
	      // look for an unactivated switch/door
	      //CONS_Printf("found a special line\n");
	      TurnTowardsPoint(x, y);
	      if (P_AproxDistance(pawn->pos.x - x, pawn->pos.y - y) <= USERANGE)
		{
		  if (!use_down)
		    cmd->buttons |= ticcmd_t::BT_USE;
		}
	      else
		forwardmove = botforwardmove[1];
	    }
	  else if (fTeammate.a)
	    {
	      // keep together with team (go to furthest visible teammate)
	      dist = P_AproxDistance(pawn->pos.x - fTeammate.a->pos.x, pawn->pos.y - fTeammate.a->pos.y).floor();
	      if (dist > 100)
		{
		  TurnTowardsPoint(fTeammate.a->pos.x, fTeammate.a->pos.y);
		  forwardmove = botforwardmove[botspeed];
		}

	      lastTarget = fTeammate.a;
	      lastTargetX = fTeammate.a->pos.x;
	      lastTargetY = fTeammate.a->pos.y;
	    }
	  else if (lastTarget && (lastTarget->flags & MF_SOLID))
	    {
	      // nothing else to do, go where the last enemy/teamate was seen
	      if ((pawn->vel.x == 0 && pawn->vel.y == 0) ||
		  !mp->botnodes->DirectlyReachable(NULL, pawn->pos.x, pawn->pos.y, lastTargetX, lastTargetY))
		lastTarget = NULL; //just went through teleporter
	      else
		{
		  //CONS_Printf("heading towards last mobj\n");
		  TurnTowardsPoint(lastTargetX, lastTargetY);
		  forwardmove = botforwardmove[botspeed];
		}
	    }
	  else
	    {
	      // we see nothing interesting and have nothing urgent to do... lets do some pathing!
	      lastTarget = NULL;
	      SearchNode_t *newdest;
	      if (bUnseenItem.a)
		{
		  newdest = mp->botnodes->GetNodeAt(bUnseenItem.a->pos);
		  //CONS_Printf("found a best item at x:%d, y:%d\n", bUnseenItem->x>>FRACBITS, bUnseenItem.a->y>>FRACBITS);
		}
	      else if (cUnseenTeammate.a)
		newdest = mp->botnodes->GetNodeAt(cUnseenTeammate.a->pos);
	      else if (cUnseenEnemy.a)
		newdest = mp->botnodes->GetNodeAt(cUnseenEnemy.a->pos);
	      else 
		newdest = NULL;

	      if (newdest != destination)
		ClearPath();
	      destination = newdest;

	      if (destination)
		{
		  // we have a valid destination
		  if (!path.empty() &&
		      P_AproxDistance(pawn->pos.x - path.front()->mx, pawn->pos.y - path.front()->my) < (BotNodes::GRIDSIZE << 1))
		    {
		      // next path node reached!
#ifdef SHOWBOTPATH
		      SearchNode_t* temp = path.front();
		      temp->marker->Remove();
#endif
		      path.pop_front();
		    }

		  // do we need to rebuild the path?
		  if (path.empty() ||
		      !mp->botnodes->DirectlyReachable(NULL, pawn->pos.x, pawn->pos.y, path.front()->mx, path.front()->my))
		    if (!mp->botnodes->FindPath(path, mp->botnodes->GetNodeAt(pawn->pos), destination))
		      {
			//CONS_Printf("Bot stuck at x:%d y:%d could not find a path to x:%d y:%d\n",pawn->x>>FRACBITS, pawn->y>>FRACBITS, posX2x(destination->x)>>FRACBITS, posY2y(destination->y)>>FRACBITS);
			destination = NULL; // can't get there
		      }

		  // follow the path
		  if (!path.empty())
		    {
		      //CONS_Printf("turning towards node at x%d, y%d\n", (nextItemNode->x>>FRACBITS), (nextItemNode->y>>FRACBITS));
		      //CONS_Printf("it has a distance %d\n", (P_AproxDistance(pawn->x - nextItemNode->x, pawn->y - nextItemNode->y)>>FRACBITS));
		      TurnTowardsPoint(path.front()->mx, path.front()->my);
		      forwardmove = botforwardmove[1];//botspeed];
		    }
		}
	    }
	}

      int forwardAngle = pawn->yaw >> ANGLETOFINESHIFT;
      int sideAngle = (pawn->yaw - ANG90) >> ANGLETOFINESHIFT;
      fixed_t cpx = (forwardmove * finecosine[forwardAngle] + sidemove * finecosine[sideAngle]) >> 5;
      fixed_t cpy = (forwardmove * finesine[forwardAngle] + sidemove * finesine[sideAngle]) >> 5;
      vec_t<fixed_t> npos = pawn->pos + pawn->vel + vec_t<fixed_t>(cpx, cpy, 0);

      position_check_t *ccc = pawn->CheckPosition(npos, Actor::PC_FIT);

      bool blocked = !ccc->xy_move_ok // XY no-fit
	|| ccc->op.bottom > npos.z + MAXSTEP // max climb is not enough
	|| ccc->op.Range() < pawn->height; // Z no-fit

      // if its time to change strafe directions, 
      if (sidemove && ((pawn->eflags & MFE_JUSTHIT) || blocked))
	{
	  straferight = !straferight;
	  pawn->eflags &= ~MFE_JUSTHIT;
	}

      if (blocked)
	{
	  if (++blockedcount > 20 &&
	      (P_AproxDistance(pawn->vel.x, pawn->vel.y) < 4 ||
	       (ccc->block_thing && (ccc->block_thing->flags & MF_SOLID))))
	    avoidtimer = 20;

	  if (ccc->op.bottom - pawn->Feet() > MAXSTEP &&
	      (ccc->op.bottom - pawn->Feet() <= MAXWATERSTEP ||
	       (ccc->op.bottom - pawn->Feet() <= 45 && pawn->subsector->sector->floortype != FLOOR_WATER))) // FIXME cv_jumpspeed
	    cmd->buttons |= ticcmd_t::BT_JUMP;

	  for (unsigned i=0; i < ccc->spechit.size(); i++)
	    if (ccc->spechit[i]->backsector)
	      {
		if (!ccc->spechit[i]->backsector->ceilingdata && !ccc->spechit[i]->backsector->floordata)
		  cmd->buttons |= ticcmd_t::BT_USE;
	      }
	}
      else
	blockedcount = 0;
    }

  if (sidemove)
    {
      if (strafetimer)
	strafetimer--;
      else
	{
	  straferight = !straferight;
	  strafetimer = Random()*85;
	}
    }

  if (weaponchangetimer)
    weaponchangetimer--;

  cmd->forward = forwardmove;
  cmd->side = straferight ? sidemove : -sidemove;
  if (cMissile.a)
    AvoidMissile(cMissile.a);
}
