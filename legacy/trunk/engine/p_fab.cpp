// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_fab.cpp 518 2007-12-31 02:54:58Z smite-meister $
//
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
/// \brief New action functions.
///
/// Separated from the other code, so that you can include it or remove it easily.

#include "doomdef.h"
#include "command.h"
#include "cvars.h"

#include "g_game.h"
#include "g_map.h"
#include "g_actor.h"
#include "g_pawn.h"

#include "info.h"
#include "p_fab.h"
#include "p_pspr.h"
#include "m_random.h"
#include "r_draw.h"
#include "sounds.h"

// action function for running FS
void A_StartFS(DActor *actor)
{
  int script = actor->tics;
  actor->tics = 0; // takes no time
  actor->mp->FS_RunScript(script, actor);
}


// weapon action function for running FS
void A_StartWeaponFS(PlayerPawn *p, pspdef_t *psp)
{
  int script = psp->tics;
  psp->tics = 0; // takes no time
  p->mp->FS_RunScript(script, p);
}


// action function for running ACS
void A_StartACS(DActor *actor)
{
  unsigned script = actor->tics;
  byte args[5] = {0,0,0,0,0}; // TODO take args from sprite and frame fields

  actor->tics = 0; // takes no time
  actor->mp->ACS_StartScript(script, args, actor, NULL, 0);
}


// weapon action function for running ACS
void A_StartWeaponACS(PlayerPawn *p, pspdef_t *psp)
{
  unsigned script = psp->tics;
  byte args[5] = {0,0,0,0,0};

  psp->tics = 0; // takes no time
  p->mp->ACS_StartScript(script, args, p, NULL, 0);
}


// Plays the activesound, if there is one. Replaces A_ContMobjSound and A_ESound.
void A_ActiveSound(DActor *actor)
{
  if (actor->info->activesound)
    S_StartSound(actor, actor->info->activesound);
}



// Action routine, for the ROCKET thing.
// This one adds trails of smoke to the rocket.
// The action pointer of the S_ROCKET state must point here to take effect.
// This routine is based on the Revenant Fireball Tracer code A_Tracer()
//
void A_SmokeTrailer(DActor *actor)
{
  if (game.tic % 4)
    return;

  // add the smoke behind the rocket
  DActor *th = actor->mp->SpawnDActor(actor->pos.x - actor->vel.x,
				      actor->pos.y - actor->vel.y, actor->pos.z, MT_SMOK);

  th->vel.z = 1;
  th->tics -= P_Random()&3;
  if (th->tics < 1)
    th->tics = 1;
}


static bool resettrans = false;
//  Set the translucency map for each frame state of mobj
//
static void R_SetTrans(statenum_t state1, statenum_t state2, transnum_t transmap)
{
  state_t *state = &states[state1];
  int s1 = state1;
  int s2 = state2;
  do {
    state->frame &= ~TFF_TRANSMASK;
    if (!resettrans)
      state->frame |= (transmap << TFF_TRANSSHIFT);
    state++;
  } while (s1++ < s2);
}


//  hack the translucency in the states for a set of standard doom sprites
//
void P_SetTranslucencies()
{
  struct
  {
    statenum_t first, last;
    transnum_t table;
  } xxx[] = {
    //revenant fireball
    {S_TRACER    , S_TRACER2    , tr_transfir},
    {S_TRACEEXP1 , S_TRACEEXP3  , tr_transmed},
                                           
    //rev. fireball. smoke trail           
    {S_SMOKE1    , S_SMOKE5     , tr_transmed},
                                           
    //imp fireball                         
    {S_TBALL1    , S_TBALL2     , tr_transfir},
    {S_TBALLX1   , S_TBALLX3    , tr_transmed},
                                           
    //archvile attack                      
    {S_FIRE1     , S_FIRE30     , tr_transfir},
                                           
    //bfg ball                             
    {S_BFGSHOT   , S_BFGSHOT2   , tr_transfir},
    {S_BFGLAND   , S_BFGLAND3   , tr_transmed},
    {S_BFGLAND4  , S_BFGLAND6   , tr_transmor},
    {S_BFGEXP    , S_NULL       , tr_transmed},
    {S_BFGEXP2   , S_BFGEXP4    , tr_transmor},
                                           
    //plasma bullet                        
    {S_PLASBALL  , S_PLASBALL2  , tr_transfir},
    {S_PLASEXP   , S_PLASEXP2   , tr_transmed},
    {S_PLASEXP3  , S_PLASEXP5   , tr_transmor},
                                           
    //bullet puff                          
    {S_PUFF1     , S_PUFF4      , tr_transmor},
                                           
    //teleport fog                         
    {S_TFOG      , S_TFOG5      , tr_transmed},
    {S_TFOG6     , S_TFOG10     , tr_transmor},
                                           
    //respawn item fog                     
    {S_IFOG      , S_IFOG5      , tr_transmed},
                                           
    //soulsphere                           
    {S_SOUL      , S_SOUL6      , tr_transmed},
    //invulnerability                      
    {S_PINV      , S_PINV4      , tr_transmed},
    //blur artifact                        
    {S_PINS      , S_PINS4      , tr_transmed},
    //megasphere                           
    {S_MEGA      , S_MEGA4      , tr_transmed},
                            
    {S_GREENTORCH, S_REDTORCH4  , tr_transfx1}, // blue torch
    {S_GTORCHSHRT, S_RTORCHSHRT4, tr_transfx1}, // short blue torch

    // flaming barrel !!
    {S_BBAR1     , S_BBAR3      , tr_transfx1},

    //lost soul
    {S_SKULL_STND, S_SKULL_DIE6 , tr_transfx1},
    //baron shot
    {S_BRBALL1   , S_BRBALL2    , tr_transfir},
    {S_BRBALLX1 , S_BRBALLX3   , tr_transmed},
    //demon spawnfire
    {S_SPAWNFIRE1, S_SPAWNFIRE3 , tr_transfir},
    {S_SPAWNFIRE4, S_SPAWNFIRE8 , tr_transmed},
    //caco fireball
    {S_RBALL1    , S_RBALL2     , tr_transfir},
    {S_RBALLX1   , S_RBALLX3    , tr_transmed},

    //arachno shot
    {S_ARACH_PLAZ, S_ARACH_PLAZ2, tr_transfir},
    {S_ARACH_PLEX, S_ARACH_PLEX2, tr_transmed},
    {S_ARACH_PLEX3,S_ARACH_PLEX4, tr_transmor},
    {S_ARACH_PLEX5,       S_NULL, tr_transhi},

    //blood puffs!
    //{S_BLOOD1   ,            0, tr_transmed},
    //{S_BLOOD2   , S_BLOOD3    , tr_transmor},

    //eye in symbol
    {S_EVILEYE    , S_EVILEYE4  , tr_transmed},
                                          
    //mancubus fireball
    {S_FATSHOT1   , S_FATSHOT2  , tr_transfir},
    {S_FATSHOTX1  , S_FATSHOTX3 , tr_transmed},

    // rockets explosion
    {S_EXPLODE1   , S_EXPLODE2  , tr_transfir},
    {S_EXPLODE3   ,       S_NULL, tr_transmed},

    //Fab: lava/slime damage smoke test
    {S_SMOK1      , S_SMOK5     , tr_transmed},
    {S_SPLASH1    , S_SPLASH3   , tr_transmor},

    // Heretic TODO rest
    {S_BLASTERFX1_1, S_BLASTERFXI1_7, tr_transmed},
    {S_BLASTERPUFF1_1, S_BLASTERPUFF2_7, tr_transmed},
    {S_HRODFX1_1, S_HRODFXI2_8, tr_transmed},

    // Hexen TODO rest
    {S_AXEPUFF_GLOW1, S_AXEPUFF_GLOW7, tr_transmed},
    {S_FSWORD_MISSILE1, S_FSWORD_FLAME10, tr_transmed},

    {S_NULL, S_NULL, tr_transmed} // terminator
  };

  for (int k=0; xxx[k].first != S_NULL; k++)
    R_SetTrans(xxx[k].first, xxx[k].last, xxx[k].table);
}

void Translucency_OnChange()
{
  if (cv_translucency.value == 0)
    resettrans = true;
  if (cv_fuzzymode.value == 0)
    P_SetTranslucencies();
  resettrans = false;
}



// =======================================================================
//                    FUNKY DEATHMATCH COMMANDS
// =======================================================================


// Called when var. 'bloodtime' is changed : set the blood states duration
//
void BloodTime_OnChange()
{
    states[S_BLOOD1].tics = 8;
    states[S_BLOOD2].tics = 8;
    states[S_BLOOD3].tics = (cv_bloodtime.value*TICRATE) - 16;

    CONS_Printf ("blood lasts for %d seconds\n", cv_bloodtime.value);
}


// extra pain
void DActor::Howl()
{
  int sound = 0;

  switch (type)
    {
    case MT_CENTAUR:
    case MT_CENTAURLEADER:
    case MT_ETTIN:
      sound = SFX_PUPPYBEAT;
      break;
    default:
      break;
    }

  if (!S_PlayingSound(this, sound))
    S_StartSound(this, sound);
}


Actor *FindOwner(Actor *a);

int MT_HOLY_FX_touchfunc(DActor *d, Actor *p)
{
  if (!(p->flags & MF_SHOOTABLE) || p == d->owner)
    return false;

  if (d->owner->team && p->team == d->owner->team)
    {
      // don't attack teammembers
      return false;
    }

  bool isplayer = (p->flags & MF_PLAYER);

  if ((p->flags2 & MF2_REFLECTIVE)
      && (isplayer || (p->flags2 & MF2_BOSS)))
    {
      // reflected back at previous owner
      d->target = d->owner;
      d->owner = p;
      return false;
    }

  if (p->flags & MF_VALIDTARGET)
    d->target = p; // target monsters and players

  if (P_Random() < 96)
    {
      int damage = 12;
      if (isplayer || p->flags2 & MF2_BOSS)
	{
	  damage = 3;
	  // ghost burns out faster when attacking players/bosses
	  d->health -= 6;
	}

      p->Damage(d, d->owner, damage);
      if (P_Random() < 128)
	{
	  d->mp->SpawnDActor(d->pos, MT_HOLY_PUFF);
	  S_StartSound(d, SFX_SPIRIT_ATTACK);
	  if ((p->flags & MF_MONSTER) && P_Random() < 128)
	    p->Howl();
	}
    }

  if (p->health <= 0)
    d->target = NULL;

  return false;
}



int MT_LIGHTNING_touchfunc(DActor *d, Actor *p)
{
  if (!(p->flags & MF_SHOOTABLE))
    return false;

  Actor *ultimateowner = FindOwner(d);

  if (p == ultimateowner)
    return false;

  if (p->mass != MAXINT)
    {
      p->vel.x += d->vel.x>>4;
      p->vel.y += d->vel.y>>4;
    }

  DActor *m = p->Inherits<DActor>();
  mobjtype_t temp = m ? m->type : MT_NONE;

  // players and bosses take less damage
  if (!(temp == MT_NONE || (p->flags2 & MF2_BOSS))
      || !(d->mp->maptic & 1))
    {
      if (temp == MT_CENTAUR ||
	  temp == MT_CENTAURLEADER)
	{ // Lightning does more damage to centaurs
	  p->Damage(d, ultimateowner, 9);
	}
      else
	{
	  p->Damage(d, ultimateowner, 3);
	}

      if (!(S_PlayingSound(d, SFX_MAGE_LIGHTNING_ZAP)))
	S_StartSound(d, SFX_MAGE_LIGHTNING_ZAP);

      if ((p->flags & MF_MONSTER) && P_Random() < 64)
	p->Howl();
    }

  if (--(d->health) <= 0 || p->health <= 0)
    return true;

  if (d->type == MT_LIGHTNING_FLOOR)
    {
      // complicated. see A_MLightningAttack2.
      if (d->target && !d->target->target)
	d->target->target = p;
    }
  else if (!d->target)
    {
      d->target = p;
    }

  return false; // lightning zaps through all sprites
}



int MT_LIGHTNING_ZAP_touchfunc(DActor *d, Actor *p)
{
  if (!(p->flags & MF_SHOOTABLE))
    return -1;

  Actor *ultimateowner = FindOwner(d);

  if (p == ultimateowner)
    return -1;

  if (d->owner)
    {
      DActor *lmo = reinterpret_cast<DActor*>(d->owner);
  
      if (lmo->type == MT_LIGHTNING_FLOOR)
	{
	  // complicated. see A_MLightningAttack2.
	  if (lmo->target && !lmo->target->target)
	    lmo->target->target = p;
	}
      else if (!lmo->target)
	{
	  lmo->target = p;
	}

      if (!(d->mp->maptic & 3))
	lmo->health--;
    }

  return -1; // do not force return
}


int MT_MSTAFF_FX2_touchfunc(DActor *d, Actor *p)
{
  if (!(p->flags & MF_SHOOTABLE))
    return -1;

  Actor *ultimateowner = FindOwner(d);
  
  DActor *m = p->Inherits<DActor>();
  mobjtype_t temp = m ? m->type : MT_NONE;

  if (p == ultimateowner || temp == MT_NONE || (p->flags2 & MF2_BOSS))
    return -1;

  switch (temp)
    {
    case MT_FIGHTER_BOSS:	// these not flagged boss
    case MT_CLERIC_BOSS:	// so they can be blasted
    case MT_MAGE_BOSS:
      break;
    default:
      p->Damage(d, ultimateowner, 10, dt_heat);
      return false; // force return
      break;
    }

  //do not force return
  return -1;
}
