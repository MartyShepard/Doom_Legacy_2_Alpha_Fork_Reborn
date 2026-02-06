// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: p_enemy.cpp 531 2008-11-02 09:03:56Z smite-meister $
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
/// \brief Enemy thinking, AI.
/// Action Functions that are associated with states/frames.

#include "doomdef.h"
#include "doomtype.h"
#include "command.h"
#include "cvars.h"

#include "p_spec.h"
#include "p_enemy.h"

#include "g_game.h"
#include "g_map.h"
#include "g_blockmap.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "g_player.h"
#include "g_decorate.h"

#include "r_presentation.h"
#include "sounds.h"
#include "m_random.h"
#include "p_maputl.h"
#include "tables.h"



//
// P_NewChaseDir related LUT.
//
static dirtype_t opposite[] =
{
  DI_WEST, DI_SOUTHWEST, DI_SOUTH, DI_SOUTHEAST,
  DI_EAST, DI_NORTHEAST, DI_NORTH, DI_NORTHWEST, DI_NODIR
};

static dirtype_t diags[] =
{
  DI_NORTHWEST, DI_NORTHEAST, DI_SOUTHWEST, DI_SOUTHEAST
};


void FastMonster_OnChange()
{
  static bool fast=false;
  static const struct {
    mobjtype_t type;
    float speed[2];
  } MonsterMissileInfo[] =
    {
      // doom
      { MT_BRUISERSHOT, {15, 20}},
      { MT_HEADSHOT,    {10, 20}},
      { MT_TROOPSHOT,   {10, 20}},
        
      // heretic
      { MT_IMPBALL,     {10, 20}},
      { MT_MUMMYFX1,    { 9, 18}},
      { MT_KNIGHTAXE,   { 9, 18}},
      { MT_REDAXE,      { 9, 18}},
      { MT_BEASTBALL,   {12, 20}},
      { MT_WIZFX1,      {18, 24}},
      { MT_SNAKEPRO_A,  {14, 20}},
      { MT_SNAKEPRO_B,  {14, 20}},
      { MT_HEADFX1,     {13, 20}},
      { MT_HEADFX3,     {10, 18}},
      { MT_MNTRFX1,     {20, 26}},
      { MT_MNTRFX2,     {14, 20}},
      { MT_SRCRFX1,     {20, 28}},
      { MT_SOR2FX1,     {20, 28}},
      
      { mobjtype_t(-1), {-1, -1} } // Terminator
    };

  int i;
  if (cv_fastmonsters.value && !fast)
    {
      for (i=S_SARG_RUN1 ; i<=S_SARG_PAIN2 ; i++)
	states[i].tics >>= 1;
      fast=true;
    }
  else if(!cv_fastmonsters.value && fast)
    {
      for (i=S_SARG_RUN1 ; i<=S_SARG_PAIN2 ; i++)
	states[i].tics <<= 1;
      fast=false;
    }

  for(i = 0; MonsterMissileInfo[i].type != -1; i++)
    {
      mobjinfo[MonsterMissileInfo[i].type].speed
	= MonsterMissileInfo[i].speed[cv_fastmonsters.value];
    }
}



bool P_CheckSpecialDeath(DActor *m, int dtype)
{
  bool ret = false;
      
  // Check for flame death
  if ((dtype & dt_TYPEMASK) == dt_heat)
    {
      ret = true;
      switch (m->type)
	{
	case MT_FIGHTER_BOSS:
	  S_StartSound(m, SFX_PLAYER_FIGHTER_BURN_DEATH);
	  m->SetState(S_PLAY_F_FDTH1);
	  break;
	case MT_CLERIC_BOSS:
	  S_StartSound(m, SFX_PLAYER_CLERIC_BURN_DEATH);
	  m->SetState(S_PLAY_C_FDTH1);
	  break;
	case MT_MAGE_BOSS:
	  S_StartSound(m, SFX_PLAYER_MAGE_BURN_DEATH);
	  m->SetState(S_PLAY_M_FDTH1);
	  break;
	case MT_TREEDESTRUCTIBLE:
	  m->SetState(S_ZTREEDES_X1);
	  m->height = 24;
	  S_StartSound(m, SFX_TREE_EXPLODE);
	  break;
	default:
	  ret = false;
	  break;
	}

      if (ret)
	return true;
    }

  if ((dtype & dt_TYPEMASK) == dt_cold)
    {
      ret = true;
      //flags |= MF_ICECORPSE; // TODO
      switch (m->type)
	{
	case MT_BISHOP:
	  m->SetState(S_BISHOP_ICE);
	  break;		
	case MT_CENTAUR:
	case MT_CENTAURLEADER:
	  m->SetState(S_CENTAUR_ICE);
	  break;		
	case MT_DEMON:
	case MT_DEMON2:
	  m->SetState(S_DEMON_ICE);
	  break;		
	case MT_SERPENT:
	case MT_SERPENTLEADER:
	  m->SetState(S_SERPENT_ICE);
	  break;		
	case MT_WRAITH:
	case MT_WRAITHB:
	  m->SetState(S_WRAITH_ICE);
	  break;
	case MT_ETTIN:
	  m->SetState(S_ETTIN_ICE1);
	  break;
	case MT_FIREDEMON:
	  m->SetState(S_FIRED_ICE1);
	  break;
	case MT_FIGHTER_BOSS:
	  m->SetState(S_FIGHTER_ICE);
	  break;
	case MT_CLERIC_BOSS:
	  m->SetState(S_CLERIC_ICE);
	  break;
	case MT_MAGE_BOSS:
	  m->SetState(S_MAGE_ICE);
	  break;
	case MT_PIG:
	  m->SetState(S_PIG_ICE);
	  break;
	default:
	  //flags &= ~MF_ICECORPSE;
	  ret = false;
	  break;
	}
    }

  return ret;
}


//
// ENEMY THINKING
// Enemies are always spawned
// with targetplayer = -1, threshold = 0
// Most monsters are spawned unaware of all players,
// but some can be made preaware
//


//
// Called by P_NoiseAlert.
// Recursively traverse adjacent sectors,
// sound blocking lines cut off traversal.
//

static Actor   *soundtarget;

static void P_RecursiveSound(sector_t *sec, int soundblocks)
{
  // wake up all monsters in this sector
  if (sec->validcount == validcount && sec->soundtraversed <= soundblocks)
    return; // already flooded with louder sound

  sec->validcount = validcount;
  sec->soundtraversed = soundblocks;
  sec->soundtarget = soundtarget;

  for (int i=0; i < sec->linecount; i++)
    {
      line_t *check = sec->lines[i];
      if (!(check->flags & ML_TWOSIDED))
	continue;

      line_opening_t *open = line_opening_t::Get(check, soundtarget); // TODO not accurate, but...

      if (open->Range() <= 0)
	continue;   // closed door

      sector_t *other;

      if (check->sideptr[0]->sector == sec)
	other = check->sideptr[1]->sector;
      else
	other = check->sideptr[0]->sector;

      // sound can cross one ML_SOUNDBLOCK line, but no more
      if (check->flags & ML_SOUNDBLOCK)
        {
	  if (soundblocks <= 0)
	    P_RecursiveSound(other, soundblocks+1);
        }
      else
	P_RecursiveSound(other, soundblocks);
    }
}



// If a monster yells at a player,
// it will alert other monsters to the player.
void P_NoiseAlert(Actor *target, Actor *emitter)
{
  soundtarget = target;
  validcount++;
  P_RecursiveSound(emitter->subsector->sector, 0);
}




bool DActor::CheckMeleeRange()
{
  Actor *pl = target;

  if (pl == NULL)
    return false;

  fixed_t dist = P_XYdist(pl->pos, pos);

  if (dist >= MELEERANGE - 20 + pl->radius)
    return false;

  //added:19-03-98: check height now, so that damn imps cant attack
  //                you if you stand on a higher ledge.
  if (pl->Feet() > Top() || Feet() > pl->Top())
    return false;

  if (!mp->CheckSight(this, target))
    return false;

  return true;
}

//
// was P_CheckMissileRange
//
bool DActor::CheckMissileRange()
{
  if (!mp->CheckSight(this, target))
    return false;

  if (eflags & MFE_JUSTHIT)
    {
      // the target just hit the enemy, so fight back!
      eflags &= ~MFE_JUSTHIT;
      return true;
    }

  if (reactiontime)
    return false;   // do not attack yet

  Actor *t = target;

  // OPTIMIZE: get this from a global checksight
  int dist = P_XYdist(pos, t->pos).floor() - 64;

  if (!info->meleestate)
    dist -= 128;   // no melee attack, so fire more

  if (type == MT_VILE)
    {
      if (dist > 14*64)
	return false;       // too far away
    }

  if (type == MT_UNDEAD)
    {
      if (dist < 196)
	return false;       // close for fist attack
      dist >>= 1;
    }


  if (type == MT_CYBORG
      || type == MT_SPIDER
      || type == MT_SKULL
      || type == MT_IMP) // heretic monster
    {
      dist >>= 1;
    }

  if (dist > 200)
    dist = 200;

  if (type == MT_CYBORG && dist > 160)
    dist = 160;

  if (P_Random () < dist)
    return false;

  return true;
}


//
// P_Move
// Move non-player  in the current direction,
// returns false if the move is blocked.
//
static const float fff = 47000.0 / 65536.0; // roughly 1/sqrt(2)
static const fixed_t xspeed[8] = {1, fff, 0, -fff, -1, -fff, 0, fff};
static const fixed_t yspeed[8] = {0, fff, 1, fff, 0, -fff, -1, -fff};

bool DActor::P_Move()
{
  if (eflags & MFE_BLASTED)
    return true;
  if (movedir == DI_NODIR)
    return false;

#ifdef PARANOIA
  if (movedir >= 8)
    I_Error ("Weird movedir!");
#endif

  fixed_t tryx = pos.x + info->speed * xspeed[movedir];
  fixed_t tryy = pos.y + info->speed * yspeed[movedir];

  pair<bool, position_check_t*> ppp = TryMove(tryx, tryy, false);
  position_check_t* ccc = ppp.second;

  if (!ppp.first)
    {
      // open any specials
      if (flags & MF_FLOAT && ccc->floatok)
        {
	  // must adjust height
	  if (pos.z < ccc->op.bottom)
	    pos.z += FLOATSPEED;
	  else
	    pos.z -= FLOATSPEED;

	  eflags |= MFE_INFLOAT;
	  return true;
        }

      if (!ccc->spechit.size())
	return false;

      movedir = DI_NODIR;

      bool good = false;
      while (ccc->spechit.size())
        {	  
	  line_t *ld = ccc->spechit.back();
	  // if the special is not a door
	  // that can be opened,
	  // return false
	  if (mp->ActivateLine(ld, this, 0, SPAC_USE))
	    good = true;
	  // Old version before use/cross/impact specials were combined
	  //if (mp->UseSpecialLine(this, ld, 0))
	  ccc->spechit.pop_back();
        }
      return good;
    }
  else
    {
      eflags &= ~MFE_INFLOAT;
    }


  if (!(flags & MF_FLOAT))
    {
      if (pos.z > floorz)
	HitFloor();
      pos.z = floorz;
    }
  return true;
}


//
// Attempts to move actor on
// in its current (ob->moveangle) direction.
// If blocked by either a wall or an actor
// returns FALSE
// If move is either clear or blocked only by a door,
// returns TRUE and sets...
// If a door is in the way,
// an OpenDoor call is made to start it opening.
//
bool DActor::P_TryWalk()
{
  if (!P_Move())
    return false;

  movecount = P_Random() & 15;
  return true;
}



void DActor::P_NewChaseDir()
{
  fixed_t   deltax, deltay;
  dirtype_t d[3];

  if (!target)
    I_Error("P_NewChaseDir: called with no target");

  int olddir = movedir;
  dirtype_t turnaround = opposite[olddir];

  deltax = target->pos.x - pos.x;
  deltay = target->pos.y - pos.y;

  if (deltax > 10)
    d[1]= DI_EAST;
  else if (deltax < -10)
    d[1]= DI_WEST;
  else
    d[1]= DI_NODIR;

  if (deltay < -10)
    d[2]= DI_SOUTH;
  else if (deltay > 10)
    d[2]= DI_NORTH;
  else
    d[2]= DI_NODIR;

  // try direct route
  if (d[1] != DI_NODIR && d[2] != DI_NODIR)
    {
      movedir = diags[((deltay<0)<<1)+(deltax>0)];
      if (movedir != turnaround && P_TryWalk())
	return;
    }

  // try other directions
  if (P_Random() > 200 || abs(deltay) > abs(deltax))
    {
      dirtype_t temp = d[1];
      d[1] = d[2];
      d[2] = temp;
    }

  if (d[1] == turnaround)
    d[1] = DI_NODIR;
  if (d[2] == turnaround)
    d[2] = DI_NODIR;

  if (d[1] != DI_NODIR)
    {
      movedir = d[1];
      if (P_TryWalk())
        {
	  // either moved forward or attacked
	  return;
        }
    }

  if (d[2] != DI_NODIR)
    {
      movedir = d[2];
      if (P_TryWalk())
	return;
    }

  // there is no direct path to the player,
  // so pick another direction.
  if (olddir!=DI_NODIR)
    {
      movedir = olddir;

      if (P_TryWalk())
	return;
    }

  int tdir;
  // randomly determine direction of search
  if (P_Random()&1)
    {
      for (tdir = DI_EAST; tdir <= DI_SOUTHEAST; tdir++)
        {
	  if (tdir != turnaround)
            {
	      movedir = tdir;

	      if (P_TryWalk())
		return;
            }
        }
    }
  else
    {
      for (tdir = DI_SOUTHEAST; tdir >= DI_EAST; tdir--)
        {
	  if (tdir != turnaround)
            {
	      movedir = tdir;

	      if (P_TryWalk())
		return;
            }
        }
    }

  if (turnaround !=  DI_NODIR)
    {
      movedir = turnaround;
      if (P_TryWalk())
	return;
    }

  movedir = DI_NODIR;  // can not move
}



static Actor *looker;
static int search_count;

// TODO save CPU, add realism, use blockmap or BSP to only seek surroundings
static bool IT_FindEnemies(Thinker *th)
{
  const int MONS_LOOK_RANGE = 20*64;
  const int MONS_LOOK_LIMIT = 64;

  Actor *a = th->Inherits<Actor>();

  if (!a)
    return true; // not an Actor
	
  if (!(a->flags & MF_VALIDTARGET) ||
      !(a->flags & MF_SHOOTABLE) ||
      (a->flags & MF_CORPSE))
    {
      return true; // not a valid target
    }

  if (P_XYdist(looker->pos, a->pos) > MONS_LOOK_RANGE)
    return true; // out of range

  if (P_Random() < 16)
    return true; // skip on a whim

  if (search_count++ > MONS_LOOK_LIMIT)
    return false; // stop searching

  if ((looker->team && looker->team == a->team) || // I pity the foo' who's not on my team!
      a == looker || a == looker->owner)
    return true;

  if (!looker->mp->CheckSight(looker, a))
    return true; // out of sight

  looker->target = a; // found a target
  return false;
}


// If allaround is false, only look 180 degrees in front.
// Returns true if a new target is found
//
// First looks for enemy players, then for any Actor who does not belong to the same team!
// Armies of monsters fighting against each other!
bool DActor::LookForEnemies(bool allaround)
{
  int n = mp->players.size();
  if (n == 0)
    goto noplayerfound;

  // BP: first time init, this allow minimum lastlook changes
  if (lastlook < 0)
    lastlook = P_Random () % n;

  for (int c = 0; c < n; c++, lastlook++)
    {
      if (lastlook >= n)
	lastlook = 0;

      // check max. 2 players/turn
      if (c >= 2)
	break;

      PlayerInfo *k = mp->players[lastlook];

      if (!k->pawn || (k->pawn->flags & MF_CORPSE) || k->spectator)
	continue;

      PlayerPawn *p = k->pawn;

      if (!mp->CheckSight(this, p))
	continue; // out of sight

      if ((team && team == p->team) ||
	  p == owner)
	continue; // don't target master or teammate

      if (!allaround)
        {
	  angle_t an = R_PointToAngle2(pos, p->pos) - yaw;

	  if (an > ANG90 && an < ANG270)
            {
	      fixed_t dist = P_XYdist(p->pos, pos);
	      // if real close, react anyway
	      if (dist > MELEERANGE)
		continue;   // behind back
            }
        }

      if (p->flags & MF_SHADOW)
        {
	  // Player is invisible
	  if (P_XYdist(p->pos, pos) > 2*MELEERANGE && P_AproxDistance(p->vel.x, p->vel.y) < 5)
	    continue; // Player is sneaking - can't detect

	  if (P_Random() < 225)
	    continue; // Player isn't sneaking, but still didn't detect
        }

      target = p;
      return true;
    }

 noplayerfound:
  if (false)  // TEST FIXME
    {
      looker = this;
      search_count = 0;
      return (!mp->IterateThinkers(IT_FindEnemies) && target);
    }

  return false;
}



//=========================================
//  COMMON ACTION ROUTINES
//=========================================

//
// A_Look
// Stay in state until a player is sighted.
//
void A_Look(DActor *actor)
{
  actor->threshold = 0;       // any shot will wake up
  Actor *targ = actor->subsector->sector->soundtarget;

  if (targ && (targ->flags & MF_SHOOTABLE))
    {
      actor->target = targ;

      if (actor->flags & MF_AMBUSH)
        {
	  if (actor->mp->CheckSight(actor, actor->target))
	    goto seeyou;
        }
      else
	goto seeyou;
    }


  if (!actor->LookForEnemies(false))
    return;

  // go into chase state
 seeyou:
  if (actor->info->seesound)
    {
      int             sound;

      switch (actor->info->seesound)
        {
	case sfx_posit1:
	case sfx_posit2:
	case sfx_posit3:
	  sound = sfx_posit1+P_Random()%3;
	  break;

	case sfx_bgsit1:
	case sfx_bgsit2:
	  sound = sfx_bgsit1+P_Random()%2;
	  break;

	default:
	  sound = actor->info->seesound;
	  break;
        }

      if (actor->flags2 & MF2_BOSS)
        {
	  // full volume
	  S_StartAmbSound(NULL, sound);
        }
      else
	S_StartScreamSound(actor, sound);
    }

  actor->SetState(actor->info->seestate);
  actor->pres->SetAnim(presentation_t::Run);
}


//
// A_Chase
// Actor has a melee attack,
// so it tries to close as fast as possible
//
void A_Chase(DActor *actor)
{
  int         delta;

  if (actor->reactiontime)
    actor->reactiontime--;

  // modify target threshold
  if (actor->threshold)
    {
      // FIXME is the heretic (and hexen) special behavior necessary?
      if (game.mode != gm_heretic &&
	  (!actor->target || actor->target->health <= 0 || (actor->target->flags & MF_CORPSE)))
        {
	  actor->threshold = 0;
        }
      else
	actor->threshold--;
    }

  if (cv_fastmonsters.value &&
      (game.mode == gm_heretic || game.mode == gm_hexen))
    { // Monsters move faster in nightmare mode
      actor->tics -= actor->tics/2;
      if (actor->tics < 3)
	actor->tics = 3;
    }

  // turn towards movement direction if not there yet
  if (actor->movedir < 8)
    {
      actor->yaw &= (7<<29);
      delta = actor->yaw - (actor->movedir << 29);

      if (delta > 0)
	actor->yaw -= ANG90/2;
      else if (delta < 0)
	actor->yaw += ANG90/2;
    }

  if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
    {
      // look for a new target
      if (actor->LookForEnemies(true))
	return;     // got a new target

      actor->SetState(actor->info->spawnstate);
      actor->pres->SetAnim(presentation_t::Idle);
      return;
    }

  // do not attack twice in a row
  if (actor->eflags & MFE_JUSTATTACKED)
    {
      actor->eflags &= ~MFE_JUSTATTACKED;
      if (!cv_fastmonsters.value)
	actor->P_NewChaseDir();
      return;
    }

  // check for melee attack
  if (actor->info->meleestate && actor->CheckMeleeRange())
    {
      if (actor->info->attacksound)
	S_StartAttackSound(actor, actor->info->attacksound);

      actor->SetState(actor->info->meleestate);
      actor->pres->SetAnim(presentation_t::Melee);
      return;
    }

  // check for missile attack
  if (actor->info->missilestate)
    {
      if (!cv_fastmonsters.value && actor->movecount)
	goto nomissile;

      if (!actor->CheckMissileRange())
	goto nomissile;

      actor->SetState(actor->info->missilestate);
      actor->pres->SetAnim(presentation_t::Shoot);
      actor->eflags |= MFE_JUSTATTACKED;
      return;
    }

 nomissile:
  // possibly choose another target
  if (game.multiplayer && !actor->threshold
      && !actor->mp->CheckSight(actor, actor->target) )
    {
      if (actor->LookForEnemies(true))
	return;     // got a new target
    }

  // chase towards player
  if (--actor->movecount < 0 || !actor->P_Move())
    actor->P_NewChaseDir();

  // make active sound
  if (actor->info->activesound && P_Random () < 3)
    {
      if ((actor->type == MT_WIZARD || actor->type == MT_BISHOP)&& P_Random() < 128)
	S_StartScreamSound(actor, actor->info->seesound);
      else if(actor->flags2 & MF2_BOSS)
	S_StartAmbSound(NULL, actor->info->activesound);
      //FIXME else if (actor->type == MT_PIG) S_StartScreamSound(actor, SFX_PIG_ACTIVE1+(P_Random()&1));
      else
	S_StartScreamSound(actor, actor->info->activesound);
    }
}


//
// A_FaceTarget
//
void A_FaceTarget(DActor *actor)
{
  if (!actor->target)
    return;

  Actor *t = actor->target;

  actor->flags &= ~MF_AMBUSH;

  actor->yaw = R_PointToAngle2(actor->pos.x, actor->pos.y, t->pos.x, t->pos.y);

  if (t->flags & MF_SHADOW)
    actor->yaw += P_SignedRandom()<<21;
}

//
// A_Pain
//
void A_Pain(DActor *actor)
{
  if (actor->info->painsound)
    S_StartScreamSound(actor, actor->info->painsound);
}




static void P_DropItem(Actor *source, mobjtype_t type, int amount, int chance, bool onfloor = false)
{
  if (P_Random() > chance)
    return;

  fixed_t z = source->pos.z;

  if (!onfloor)
    z += source->height >> 1;

  DActor *item = source->mp->SpawnDActor(source->pos.x, source->pos.y, z, type);

  // add a little bounce
  if (!onfloor)
    item->vel = vec_t<fixed_t>(RandomS(), RandomS(), Random(5, 9));

  item->flags |= MF_DROPPED;
  item->health = amount;
}


/// First A_Fall, the drop something.
void A_NoBlocking(DActor *actor)
{
  A_Fall(actor);

  // Check for monsters dropping things TODO ZDoom-style droplist into ActorInfo, merge with A_Fall
  switch (actor->type)
    {
      // Heretic
    case MT_MUMMY:
    case MT_MUMMYLEADER:
    case MT_MUMMYGHOST:
    case MT_MUMMYLEADERGHOST:
      P_DropItem(actor, MT_AMGWNDWIMPY, 3, 84);
      break;
    case MT_KNIGHT:
    case MT_KNIGHTGHOST:
      P_DropItem(actor, MT_AMCBOWWIMPY, 5, 84);
      break;
    case MT_WIZARD:
      P_DropItem(actor, MT_AMBLSRWIMPY, 10, 84);
      P_DropItem(actor, MT_ARTITOMEOFPOWER, 0, 4);
      break;
    case MT_HHEAD:
      P_DropItem(actor, MT_AMBLSRWIMPY, 10, 84);
      P_DropItem(actor, MT_ARTIEGG, 0, 51);
      break;
    case MT_BEAST:
      P_DropItem(actor, MT_AMCBOWWIMPY, 10, 84);
      break;
    case MT_CLINK:
      P_DropItem(actor, MT_AMSKRDWIMPY, 20, 84);
      break;
    case MT_SNAKE:
      P_DropItem(actor, MT_AMPHRDWIMPY, 5, 84);
      break;
    case MT_MINOTAUR:
      P_DropItem(actor, MT_ARTISUPERHEAL, 0, 51);
      P_DropItem(actor, MT_AMPHRDWIMPY, 10, 84);
      break;

      // Doom
    case MT_WOLFSS:
    case MT_POSSESSED:
      P_DropItem(actor, MT_CLIP, mobjinfo[MT_CLIP].spawnhealth/2, 255, true); // half a clip
      break;
    case MT_SHOTGUY:
      P_DropItem(actor, MT_SHOTGUN, mobjinfo[MT_SHOTGUN].spawnhealth/2, 255, true);
      break;
    case MT_CHAINGUY:
      P_DropItem(actor, MT_CHAINGUN, mobjinfo[MT_CHAINGUN].spawnhealth/2, 255, true);
      break;

    default:
      return;
    }
}


/// A dying thing falls to the ground (monster deaths)
void A_Fall(DActor *actor)
{
  // actor is on ground, it can be walked over
  if (!cv_solidcorpse.value)
    actor->flags &= ~MF_SOLID;

  // So change this if corpse objects
  // are meant to be obstacles.
  actor->flags  |= MF_DROPOFF;
  actor->height *= 0.25;
  actor->radius *= 0.9375; // TODO huh?
  actor->health = actor->info->spawnhealth >> 1;

  if (cv_bodyqueue_monsters.value)
    actor->mp->QueueBody(actor);
}



//=========================================
//  DOOM ACTION ROUTINES
//=========================================

//
// A_PosAttack
//
void A_PosAttack(DActor *actor)
{
  if (!actor->target)
    return;

  A_FaceTarget(actor);
  angle_t angle = actor->yaw;
  float sine;
  actor->AimLineAttack(angle, MISSILERANGE, sine);
  S_StartAttackSound(actor, sfx_pistol);
  angle += P_SignedRandom()<<20;
  int damage = ((P_Random()%5)+1)*3;
  PuffType = MT_PUFF;
  actor->LineAttack(angle, MISSILERANGE, sine, damage);
}

void A_SPosAttack(DActor *actor)
{
  if (!actor->target)
    return;

  S_StartAttackSound(actor, sfx_shotgn);
  A_FaceTarget (actor);
  angle_t bangle = actor->yaw;
  float sine;
  actor->AimLineAttack(bangle, MISSILERANGE, sine);

  PuffType = MT_PUFF;

  for (int i=0 ; i<3 ; i++)
    {
      angle_t angle  = (P_SignedRandom()<<20)+bangle;
      int damage = ((P_Random()%5)+1)*3;
      actor->LineAttack(angle, MISSILERANGE, sine, damage);
    }
}

void A_CPosAttack(DActor *actor)
{
  if (!actor->target)
    return;

  S_StartAttackSound(actor, sfx_shotgn);
  A_FaceTarget (actor);
  angle_t bangle = actor->yaw;
  float sine;
  actor->AimLineAttack(bangle, MISSILERANGE, sine);

  angle_t angle  = (P_SignedRandom()<<20)+bangle;

  int damage = ((P_Random()%5)+1)*3;
  PuffType = MT_PUFF;
  actor->LineAttack(angle, MISSILERANGE, sine, damage);
}

void A_CPosRefire(DActor *actor)
{
  // keep firing unless target got out of sight
  A_FaceTarget (actor);

  if (P_Random () < 40)
    return;

  if (!actor->target
      || actor->target->health <= 0
      || actor->target->flags & MF_CORPSE
      || !actor->mp->CheckSight(actor, actor->target) )
    {
      actor->SetState(actor->info->seestate);
    }
}


void A_SpidRefire(DActor *actor)
{
  // keep firing unless target got out of sight
  A_FaceTarget (actor);

  if (P_Random () < 10)
    return;

  if (!actor->target
      || actor->target->health <= 0
      || actor->target->flags & MF_CORPSE
      || !actor->mp->CheckSight(actor, actor->target) )
    {
      actor->SetState(actor->info->seestate);
    }
}

void A_BspiAttack(DActor *actor)
{
  if (!actor->target)
    return;

  A_FaceTarget(actor);

  // launch a missile
  actor->SpawnMissile(actor->target, MT_ARACHPLAZ);
}


//
// A_TroopAttack
//
void A_TroopAttack(DActor *actor)
{
  int damage;

  if (!actor->target)
    return;

  A_FaceTarget(actor);
  if (actor->CheckMeleeRange())
    {
      S_StartAttackSound(actor, sfx_claw);
      damage = (P_Random()%8+1)*3;
      actor->target->Damage(actor, actor, damage);
      return;
    }

  // launch a missile
  actor->SpawnMissile(actor->target, MT_TROOPSHOT);
}


void A_SargAttack(DActor *actor)
{
  int         damage;

  if (!actor->target)
    return;

  A_FaceTarget (actor);
  if (actor->CheckMeleeRange())
    {
      damage = ((P_Random()%10)+1)*4;
      actor->target->Damage(actor, actor, damage);
    }
}

void A_HeadAttack(DActor *actor)
{
  int         damage;

  if (!actor->target)
    return;

  A_FaceTarget (actor);
  if (actor->CheckMeleeRange())
    {
      damage = (P_Random()%6+1)*10;
      actor->target->Damage(actor, actor, damage);
      return;
    }

  // launch a missile
  actor->SpawnMissile(actor->target, MT_HEADSHOT);
}

void A_CyberAttack(DActor *actor)
{
  if (!actor->target)
    return;

  A_FaceTarget (actor);
  actor->SpawnMissile(actor->target, MT_ROCKET);
}


void A_BruisAttack(DActor *actor)
{
  int         damage;

  if (!actor->target)
    return;

  if (actor->CheckMeleeRange())
    {
      S_StartAttackSound(actor, sfx_claw);
      damage = (P_Random()%8+1)*10;
      actor->target->Damage(actor, actor, damage);
      return;
    }

  // launch a missile
  actor->SpawnMissile(actor->target, MT_BRUISERSHOT);
}


//
// A_SkelMissile
//
void A_SkelMissile(DActor *actor)
{
  DActor  *mo;

  if (!actor->target)
    return;

  A_FaceTarget (actor);
  actor->pos.z += 16;    // so missile spawns higher
  mo = actor->SpawnMissile(actor->target, MT_TRACER);
  actor->pos.z -= 16;    // back to normal

  if(mo)
    {
      mo->pos.x += mo->vel.x;
      mo->pos.y += mo->vel.y;
      mo->target = actor->target;
    }
}


// guide a guiding missile
void A_Tracer(DActor *actor)
{
  Map *m = actor->mp;

  if (m->maptic % 4)
    return;

  // spawn a puff of smoke behind the rocket
  m->SpawnPuff(actor->pos, MT_PUFF, false);

  DActor *th = m->SpawnDActor(actor->pos.x - actor->vel.x, actor->pos.y - actor->vel.y, actor->pos.z, MT_SMOKE);

  th->vel.z = 1;
  th->tics -= P_Random()&3;
  if (th->tics < 1)
    th->tics = 1;

  // adjust direction
  Actor *dest = actor->target;

  if (!dest || dest->health <= 0)
    return;

  // change angle
  const int TRACEANGLE = 0xc000000;
  angle_t exact = R_PointToAngle2(actor->pos.x, actor->pos.y, dest->pos.x, dest->pos.y);

  if (exact != actor->yaw)
    {
      if (exact - actor->yaw > ANG180)
        {
	  actor->yaw -= TRACEANGLE;
	  if (exact - actor->yaw < ANG180)
	    actor->yaw = exact;
        }
      else
        {
	  actor->yaw += TRACEANGLE;
	  if (exact - actor->yaw > ANG180)
	    actor->yaw = exact;
        }
    }

  exact = actor->yaw>>ANGLETOFINESHIFT;
  actor->vel.x = actor->info->speed * finecosine[exact];
  actor->vel.y = actor->info->speed * finesine[exact];

  // change slope
  int dist = (P_XYdist(dest->pos, actor->pos) / actor->info->speed).floor();

  if (dist < 1)
    dist = 1;
  fixed_t slope = (dest->pos.z + 40 - actor->pos.z) / dist;

  if (slope < actor->vel.z)
    actor->vel.z -= 0.125f;
  else
    actor->vel.z += 0.125f;
}


void A_SkelWhoosh(DActor  *actor)
{
  if (!actor->target)
    return;

  A_FaceTarget (actor);
  S_StartAttackSound(actor, sfx_skeswg);
}

void A_SkelFist(DActor  *actor)
{
  if (!actor->target)
    return;

  A_FaceTarget (actor);

  if (actor->CheckMeleeRange())
    {
      int damage = ((P_Random()%10)+1)*6;
      S_StartAttackSound(actor, sfx_skepch);
      actor->target->Damage(actor, actor, damage);
    }
}



//
// PIT_VileCheck
// Detect a corpse that could be raised.
//
static DActor *corpsehit, *vileobj;
static fixed_t  viletryx, viletryy;

bool PIT_VileCheck(Actor *th)
{
  if (!(th->flags & MF_CORPSE) )
    return true;    // not a monster

  DActor *thing = th->Inherits<DActor>();

  if (!thing)
    return true;

  if (thing->tics != -1)
    return true;    // not lying still yet

  if (!thing->info->raisestate)
    return true;    // monster doesn't have a raise state

  fixed_t maxdist = thing->info->radius + mobjinfo[MT_VILE].radius;

  if (abs(thing->pos.x - viletryx) > maxdist ||
      abs(thing->pos.y - viletryy) > maxdist)
    return true;            // not actually touching

  corpsehit = thing;
  corpsehit->vel.x = corpsehit->vel.y = 0;
  corpsehit->height <<= 2;
  bool check = corpsehit->TestLocation();
  corpsehit->height >>= 2;

  if (!check)
    return true;            // doesn't fit here

  return false;               // got one, so stop checking
}



//
// A_VileChase
// Check for ressurecting a body
//
void A_VileChase(DActor *actor)
{
  Actor *temp;
  Map *m = actor->mp;

  if (actor->movedir != DI_NODIR)
    {
      // check for corpses to raise
      viletryx = actor->pos.x + actor->info->speed*xspeed[actor->movedir];
      viletryy = actor->pos.y + actor->info->speed*yspeed[actor->movedir];

      vileobj = actor;
      // Call PIT_VileCheck to check
      // whether object is a corpse
      // that can be raised.
      if (!m->blockmap->IterateThingsRadius(viletryx, viletryy, MAXRADIUS*2, PIT_VileCheck))
	{
	  // got one!
	  temp = actor->target;
	  actor->target = corpsehit;
	  A_FaceTarget (actor);
	  actor->target = temp;

	  actor->SetState(S_VILE_HEAL1);
	  S_StartSound (corpsehit, sfx_gib);
	  const ActorInfo *info = corpsehit->info;

	  corpsehit->SetState(info->raisestate);
	  corpsehit->pres->SetAnim(presentation_t::Raise);

	  corpsehit->height = info->height;
	  corpsehit->radius = info->radius;

	  corpsehit->flags = info->flags;
	  corpsehit->health = info->spawnhealth;
	  corpsehit->target = NULL;

	  return;
	}
    }

  // Return to normal attack.
  A_Chase(actor);
}


//
// A_VileStart
//
void A_VileStart(DActor *actor)
{
  S_StartAttackSound(actor, sfx_vilatk);
}


//
// A_Fire
// Keep fire in front of player unless out of sight
//
void A_Fire(DActor *actor);

void A_StartFire(DActor *actor)
{
  S_StartSound(actor,sfx_flamst);
  A_Fire(actor);
}

void A_FireCrackle(DActor *actor)
{
  S_StartSound(actor,sfx_flame);
  A_Fire(actor);
}

void A_Fire(DActor *actor)
{
  unsigned  an;

  Actor *dest = actor->target;
  if (!dest)
    return;

  // don't move it if the vile lost sight
  if (!actor->mp->CheckSight(actor->owner, dest))
    return;

  an = dest->yaw >> ANGLETOFINESHIFT;

  actor->UnsetPosition();
  actor->pos = dest->pos + vec_t<fixed_t>(24 * finecosine[an], 24 * finesine[an], 0);
  actor->SetPosition();
}



//
// A_VileTarget
// Spawn the hellfire
//
void A_VileTarget(DActor *actor)
{
  if (!actor->target)
    return;

  A_FaceTarget(actor);

  DActor *fire = actor->mp->SpawnDActor(actor->target->pos, MT_FIRE); // was x,x,z
  actor->owner = fire; // TODO slight misuse of owner!
  fire->owner = actor;
  fire->target = actor->target;
  A_Fire(fire);
}




//
// A_VileAttack
//
void A_VileAttack(DActor *actor)
{  
  int an;

  if (!actor->target)
    return;

  A_FaceTarget(actor);

  if (!actor->mp->CheckSight(actor, actor->target))
    return;

  S_StartSound (actor, sfx_barexp);
  actor->target->Damage(actor, actor, 20);
  actor->target->vel.z = 1000 / actor->target->mass;

  an = actor->yaw >> ANGLETOFINESHIFT;

  Actor *fire = actor->owner; // TODO misuse of owner...

  if (!fire)
    return;

  // move the fire between the vile and the player
  fire->pos.x = actor->target->pos.x - 24 * finecosine[an];
  fire->pos.y = actor->target->pos.y - 24 * finesine[an];
  fire->RadiusAttack(actor, 70);
}




//
// Mancubus attack,
// firing three missiles (bruisers)
// in three different directions?
// Doesn't look like it.
//
#define FATSPREAD       (ANG90/8)

void A_FatRaise(DActor *actor)
{
  A_FaceTarget (actor);
  S_StartAttackSound(actor, sfx_manatk);
}


void A_FatAttack1(DActor *actor)
{
  DActor *mo;
  int    an;

  A_FaceTarget(actor);
  // Change direction  to ...
  actor->yaw += FATSPREAD;
  actor->SpawnMissile(actor->target, MT_FATSHOT);

  mo = actor->SpawnMissile(actor->target, MT_FATSHOT);
  if (mo)
    {
      mo->yaw += FATSPREAD;
      an = mo->yaw >> ANGLETOFINESHIFT;
      mo->vel.x = mo->info->speed * finecosine[an];
      mo->vel.y = mo->info->speed * finesine[an];
    }
}

void A_FatAttack2(DActor *actor)
{
  DActor *mo;
  int         an;

  A_FaceTarget (actor);
  // Now here choose opposite deviation.
  actor->yaw -= FATSPREAD;
  actor->SpawnMissile(actor->target, MT_FATSHOT);

  mo = actor->SpawnMissile(actor->target, MT_FATSHOT);
  if(mo)
    {
      mo->yaw -= FATSPREAD*2;
      an = mo->yaw >> ANGLETOFINESHIFT;
      mo->vel.x = mo->info->speed * finecosine[an];
      mo->vel.y = mo->info->speed * finesine[an];
    }
}

void A_FatAttack3(DActor *actor)
{
  DActor *mo;
  int         an;

  A_FaceTarget (actor);

  mo = actor->SpawnMissile(actor->target, MT_FATSHOT);
  if(mo)
    {
      mo->yaw -= FATSPREAD/2;
      an = mo->yaw >> ANGLETOFINESHIFT;
      mo->vel.x = mo->info->speed * finecosine[an];
      mo->vel.y = mo->info->speed * finesine[an];
    }
    
  mo = actor->SpawnMissile(actor->target, MT_FATSHOT);
  if(mo)
    {
      mo->yaw += FATSPREAD/2;
      an = mo->yaw >> ANGLETOFINESHIFT;
      mo->vel.x = mo->info->speed * finecosine[an];
      mo->vel.y = mo->info->speed * finesine[an];
    }
}


//
// SkullAttack
// Fly at the player like a missile.
//
#define SKULLSPEED              (20)

void A_SkullAttack(DActor *actor)
{
  if (!actor->target)
    return;

  Actor *dest = actor->target;
  actor->eflags |= MFE_SKULLFLY;
  S_StartScreamSound(actor, actor->info->attacksound);
  A_FaceTarget (actor);
  angle_t an = actor->yaw >> ANGLETOFINESHIFT;
  actor->vel.x = SKULLSPEED * finecosine[an];
  actor->vel.y = SKULLSPEED * finesine[an];
  int dist = (P_XYdist(dest->pos, actor->pos) / SKULLSPEED).floor();

  if (dist < 1)
    dist = 1;
  actor->vel.z = (dest->Center() - actor->pos.z) / dist;
}


//
// A_PainShootSkull
// Spawn a lost soul and launch it at the target
//
void A_PainShootSkull(DActor *actor, angle_t angle)
{
  fixed_t     x;
  fixed_t     y;
  fixed_t     z;

  DActor *newmobj;
  angle_t     an;

  /*  --------------- SKULL LIMITE CODE -----------------
      int         count;
      thinker_t*  currentthinker;

      // count total number of skull currently on the level
      count = 0;

      currentthinker = thinkercap.next;
      while (currentthinker != &thinkercap)
      {
      if (   (currentthinker->function.acp1 == (actionf_p1)P_MobjThinker)
      && ((Actor *)currentthinker)->type == MT_SKULL)
      count++;
      currentthinker = currentthinker->next;
      }

      // if there are allready 20 skulls on the level,
      // don't spit another one
      if (count > 20)
      return;
      ---------------------------------------------------
  */

  // okay, there's place for another one
  an = angle >> ANGLETOFINESHIFT;

  fixed_t prestep = 4 + 3*(actor->info->radius + mobjinfo[MT_SKULL].radius)/2;

  x = actor->pos.x + prestep * finecosine[an];
  y = actor->pos.y + prestep * finesine[an];
  z = actor->pos.z + 8;

  newmobj = actor->mp->SpawnDActor(x, y, z, MT_SKULL);

  // Check for movements.
  if (!newmobj->TryMove(newmobj->pos.x, newmobj->pos.y, false).first)
    {
      // kill it immediately
      newmobj->Damage(actor,actor,10000);
      return;
    }

  newmobj->target = actor->target;
  A_SkullAttack (newmobj);
}


//
// A_PainAttack
// Spawn a lost soul and launch it at the target
//
void A_PainAttack(DActor *actor)
{
  if (!actor->target)
    return;

  A_FaceTarget (actor);
  A_PainShootSkull (actor, actor->yaw);
}


void A_PainDie(DActor *actor)
{
  A_Fall (actor);
  A_PainShootSkull (actor, actor->yaw+ANG90);
  A_PainShootSkull (actor, actor->yaw+ANG180);
  A_PainShootSkull (actor, actor->yaw+ANG270);
}


void A_Scream(DActor *actor)
{
  int sound;

  switch (actor->info->deathsound)
    {
    case 0:
      return;

    case sfx_podth1:
    case sfx_podth2:
    case sfx_podth3:
      sound = sfx_podth1 + P_Random ()%3;
      break;

    case sfx_bgdth1:
    case sfx_bgdth2:
      sound = sfx_bgdth1 + P_Random ()%2;
      break;

    default:
      sound = actor->info->deathsound;
      break;
    }

  // Check for bosses.
  if (actor->flags2 & MF2_BOSS)
    S_StartAmbSound(NULL, sound); // full volume
  else
    S_StartScreamSound(actor, sound);
}


void A_XScream(DActor *actor)
{
  S_StartScreamSound(actor, sfx_gib);
}


//
// A_Explode
//
void A_Explode(DActor *actor)
{
  int  damage = 128;
  int  dtype = dt_normal;
  int  distance = 128;
  bool damageSelf = true;

  switch (actor->type)
    {
    case MT_SOR2FX1: // D'Sparil missile
      damage = 80 + (P_Random() & 31);
      break;
    case MT_FIREBOMB: // Time Bombs
      actor->pos.z += 32;
      actor->flags &= ~MF_SHADOW;
      break;
    case MT_MNTRFX2: // Minotaur floor fire
      damage = 24;
      break;
    case MT_BISHOP: // Bishop radius death
      damage = 25+(P_Random()&15);
      break;
    case MT_HAMMER_MISSILE: // Fighter Hammer
      damage = 128;
      damageSelf = false;
      break;
    case MT_FSWORD_MISSILE: // Fighter Runesword
      damage = 64;
      damageSelf = false;
      break;
    case MT_CIRCLEFLAME: // Cleric Flame secondary flames
      damage = 20;
      damageSelf = false;
      break;
    case MT_SORCBALL1: 	// Sorcerer balls
    case MT_SORCBALL2:
    case MT_SORCBALL3:
      distance = 255;
      damage = 255;
      actor->args[0] = 1;		// don't play bounce
      break;
    case MT_SORCFX1: 	// Sorcerer spell 1
      damage = 30;
      break;
    case MT_SORCFX4: 	// Sorcerer spell 4
      damage = 20;
      break;
    case MT_TREEDESTRUCTIBLE:
      damage = 10;
      break;
    case MT_DRAGON_FX2:
      damage = 80;
      damageSelf = false;
      break;
    case MT_MSTAFF_FX:
      damage = 64;
      distance = 192;
      damageSelf = false;
      break;
    case MT_MSTAFF_FX2:
      damage = 80;
      distance = 192;
      damageSelf = false;
      break;
    case MT_POISONCLOUD:
      damage = 4;
      dtype = dt_poison;
      distance = 40;
      break;
    case MT_ZXMAS_TREE:
    case MT_ZSHRUB2:
      damage = 30;
      distance = 64;
      break;
    default:
      break;
    }

  actor->RadiusAttack(actor->owner, damage, distance, dtype, damageSelf);

  if (actor->pos.z <= actor->floorz + distance && actor->type != MT_POISONCLOUD)
    actor->HitFloor();
}

//
// A_BossDeath
// Possibly trigger special effects
// if on first boss level
//
void A_BossDeath(DActor *mo)
{
  mo->mp->BossDeath(mo);
}

//
// A_KeenDie
// DOOM II special, map 32.
// Uses special tag 666.
//
void A_KeenDie(DActor *mo)
{
  A_Fall(mo);

  mo->mp->BossDeath(mo);
}

void A_Hoof(DActor *mo)
{
  S_StartSound (mo, sfx_hoof);
  A_Chase (mo);
}

void A_Metal(DActor *mo)
{
  S_StartSound (mo, sfx_metal);
  A_Chase (mo);
}

void A_BabyMetal(DActor *mo)
{
  S_StartSound (mo, sfx_bspwlk);
  A_Chase (mo);
}

void A_BrainAwake(DActor *mo)
{
  S_StartAmbSound(NULL, sfx_bossit);
}


void A_BrainPain(DActor *mo)
{
  S_StartAmbSound(NULL, sfx_bospn);
}


void A_BrainScream(DActor *mo)
{
  fixed_t y = mo->pos.y - 320;
  for (fixed_t x = mo->pos.x - 196; x < mo->pos.x + 320; x += 8)
    {
      DActor *th = mo->mp->SpawnDActor(x, y, Random(128, 640), MT_ROCKET);
      th->vel.z = Random(0, 2);

      th->SetState(S_BRAINEXPLODE1);

      th->tics -= P_Random()&7;
      if (th->tics < 1)
	th->tics = 1;
    }

  S_StartAmbSound(NULL, sfx_bosdth);
}



void A_BrainExplode(DActor *mo)
{
  DActor *th = mo->mp->SpawnDActor(mo->pos.x + 8*RandomS(), mo->pos.y, Random(128, 640), MT_ROCKET);
  th->vel.z = Random(0, 2);

  th->SetState(S_BRAINEXPLODE1);

  th->tics -= P_Random()&7;
  if (th->tics < 1)
    th->tics = 1;
}


void A_BrainDie(DActor *mo)
{
  mo->mp->BossDeath(mo);
}

void A_BrainSpit(DActor *mo)
{
  Map *m = mo->mp;

  static int easy = 0;

  easy ^= 1;
  if (game.skill <= sk_easy && (!easy))
    return;

  int n = m->braintargets.size();
  if (n > 0) 
    {
      // shoot a cube at current target
      Actor *targ = m->braintargets[m->braintargeton]->mobj;
      m->braintargeton = (m->braintargeton+1) % n;
        
      // spawn brain missile
      DActor *newmobj = mo->SpawnMissile(targ, MT_SPAWNSHOT);
      if (newmobj)
        {
	  newmobj->target = targ;
	  newmobj->reactiontime = ((targ->pos.y - mo->pos.y) / newmobj->vel.y).floor()
	    / newmobj->state->tics;
        }

      S_StartAmbSound(NULL, sfx_bospit);
    }
}



void A_SpawnFly(DActor *mo);

// travelling cube sound
void A_SpawnSound(DActor *mo)
{
  S_StartSound (mo,sfx_boscub);
  A_SpawnFly(mo);
}

void A_SpawnFly(DActor *mo)
{
  mobjtype_t  type;

  if (--mo->reactiontime)
    return; // still flying

  Actor *targ = mo->target;

  // First spawn teleport fog.
  DActor *fog = mo->mp->SpawnDActor(targ->pos, MT_SPAWNFIRE);
  S_StartSound (fog, sfx_teleport);

  // Randomly select monster to spawn.
  int r = P_Random ();

  // Probability distribution (kind of :),
  // decreasing likelihood.
  if ( r<50 )
    type = MT_TROOP;
  else if (r<90)
    type = MT_SERGEANT;
  else if (r<120)
    type = MT_SHADOWS;
  else if (r<130)
    type = MT_PAIN;
  else if (r<160)
    type = MT_HEAD;
  else if (r<162)
    type = MT_VILE;
  else if (r<172)
    type = MT_UNDEAD;
  else if (r<192)
    type = MT_BABY;
  else if (r<222)
    type = MT_FATSO;
  else if (r<246)
    type = MT_KNIGHT;
  else
    type = MT_BRUISER;

  DActor *newmobj = mo->mp->SpawnDActor(targ->pos, type);

  if (newmobj->LookForEnemies(true))
    {
      newmobj->SetState(newmobj->info->seestate);
      newmobj->pres->SetAnim(presentation_t::Run);
    }

  // telefrag anything in this spot
  newmobj->flags2 |= MF2_TELESTOMP;
  newmobj->TeleportMove(newmobj->pos);
  newmobj->flags2 &= ~MF2_TELESTOMP;

  // remove self (i.e., cube).
  mo->Remove();
}


void A_PlayerScream(DActor *actor)
{
  // Default death sound.
  int sound = 0, i = 0;

  // Handle the different player death screams
  /*
  if (actor->vel.z <= -39)
    sound = SFX_PLAYER_FALLING_SPLAT; // TODO Falling splat
  */

  if (actor->type == MT_PLAYER_FIGHTER ||
      actor->type == MT_PLAYER_CLERIC  ||
      actor->type == MT_PLAYER_MAGE)
    {
      if (actor->health > -50)
	i = 0;
      else if(actor->health > -100)
	i = 1; // Crazy death sound
      else
	i = 2 + P_Random() % 3; // Three different extreme deaths
    }

  switch (actor->type)
    {
    case MT_PLAYER:
      if (actor->health > -50)
	sound = sfx_pldeth;
      else
	sound = sfx_pdiehi;
      break;
    case MT_PLAYER_FIGHTER:
      sound = SFX_PLAYER_FIGHTER_NORMAL_DEATH + i;
      break;
    case MT_PLAYER_CLERIC:
      sound = SFX_PLAYER_CLERIC_NORMAL_DEATH + i;
      break;
    case MT_PLAYER_MAGE:
      sound = SFX_PLAYER_MAGE_NORMAL_DEATH + i;
      break;
    default:
      sound = actor->info->deathsound;
    }

  S_StartScreamSound(actor, sound);
}
