// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_user.cpp 518 2007-12-31 02:54:58Z smite-meister $
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
/// \brief Morphing, artifacts, inventory responder.


#include "doomdef.h"
#include "command.h"
#include "cvars.h"
#include "d_event.h"

#include "g_game.h"
#include "g_map.h"
#include "g_pawn.h"
#include "g_player.h"
#include "g_input.h"
#include "g_decorate.h"

#include "p_camera.h"

#include "p_enemy.h"
#include "p_maputl.h"
#include "r_sprite.h"
#include "hardware/md3.h"
#include "r_defs.h"

#include "sounds.h"
#include "m_random.h"
#include "tables.h"
#include "dstrings.h"


//============================================================
//  Morphing
//============================================================

const int MORPHTICS = 40*TICRATE;

bool Actor::Morph(mobjtype_t form)
{
  return false;
}

bool DActor::Morph(mobjtype_t form)
{
  if (!(flags & MF_MONSTER))
    return false;

  // TODO replace MF2_BOSS and the switch here with NOMORPH flag
  if (flags2 & MF2_BOSS)
    return false;

  switch (type)
    {
    case MT_CHICKEN:
    case MT_HHEAD:
    case MT_PIG:
    case MT_FIGHTER_BOSS:
    case MT_CLERIC_BOSS:
    case MT_MAGE_BOSS:
      return false;

    default:
      break;
    }

  const ActorInfo *ai = aid[form];
  if (!ai)
    {
      I_Error("Corrupted morph target!\n");
      return false;
    }

  // see if we have room to morph
  radius = ai->radius;
  height = ai->height;

  if (!TestLocation())
    { // Didn't fit
      radius = info->radius;
      height = info->height;
      return false;
    }
 
  // NOTE: the morphing used to work by Removing the old DActor and
  // Spawning a new one, but this is simpler and more logical.
  // Now we can keep all pointers pointing to this DActor.
  // Also TID, team, targets etc. are retained automatically.

  // spawn some FX
  DActor *fog = mp->SpawnDActor(pos.x, pos.y, pos.z + TELEFOGHEIGHT, MT_TFOG);
  S_StartSound(fog, sfx_teleport);

  // store old type (NOTE: morphed form must not change these fields!)
  special2 = type;
  special1 = MORPHTICS + P_Random();

  // get info on new type
  info = ai;
  type = ai->GetMobjType();

  // get new properties (radius and height are OK)
  mass   = info->mass;
  health = info->spawnhealth;

  flags  = info->flags | (flags & MF_SHADOW); // keep invisibility
  flags2 = info->flags2;

  if (game.skill != sk_nightmare)
    reactiontime = info->reactiontime;

  movedir = movecount = threshold = 0;
  lastlook = -1;
  special3 = 0;

  // new state
  if (info->spawnstate)
    {
      state = info->spawnstate;
      tics = state->tics;
    }
  else
    {
      state = NULL;
      tics = -1;
    }

  // new presentation
  if (pres)
    delete pres;

  if (!info->modelname.empty())
    pres = new modelpres_t(info->modelname.c_str());
  else
    pres = new spritepres_t(info);

  return true;
}


// Returns true if the monster morphs back.
bool DActor::UpdateMorph(int tics)
{
  special1 -= tics;
  if (special1 > 0)
    return false;  

  // undo the morph
  mobjtype_t orig = mobjtype_t(special2);
 
  switch (orig)
    {
    case MT_WRAITHB:	   // These must remain morphed
    case MT_SERPENT:
    case MT_SERPENTLEADER:
    case MT_XMINOTAUR:
      return false;
    default:
      break;
    }

  // FIXME remove nomorph flag!
  if (!Morph(orig))
    {
      special1 = 5*TICRATE; // Next try in 5 seconds
      return false;
    }

  // clear morph-related parameters
  special1 = special2 = 0;
  return true;
}



bool PlayerPawn::Morph(mobjtype_t form)
{
  if (morphTics)
    {
      if ((morphTics < MORPHTICS-TICRATE) && !powers[pw_weaponlevel2])
	GivePower(pw_weaponlevel2); // Make a super beast
      return false;
    }

  if (powers[pw_invulnerability])
    return false; // Immune when invulnerable

  const ActorInfo *ai = aid[form];
  if (!ai)
    {
      I_Error("Corrupted morph target!\n");
      return false;
    }

  // see if we have room to morph
  radius = ai->radius;
  height = ai->height;

  if (!TestLocation())
    { // Didn't fit
      radius = info->radius;
      height = info->height;
      return false;
    }

  // spawn FX
  DActor *fog = mp->SpawnDActor(pos.x, pos.y, pos.z+TELEFOGHEIGHT, MT_TFOG);
  S_StartSound(fog, sfx_teleport);

  morphTics = MORPHTICS;

  mass   = ai->mass;
  health = maxhealth = ai->spawnhealth;
  speed  = ai->speed;
  pclass = PCLASS_PIG;

  attackphase = readyweapon; // store current weapon
  armorfactor[0] = armorpoints[0] = 0;
  powers[pw_invisibility] = 0;
  powers[pw_weaponlevel2] = 0;
  weaponinfo = wpnlev1info;

  ActivateMorphWeapon();
  return true;
}


bool PlayerPawn::UndoMorph()
{
  // store the current values
  fixed_t r = radius;
  fixed_t h = height;

  radius = info->radius;
  height = info->height;

  if (!TestLocation())
    {
      // Didn't fit, continue morph for two more seconds
      morphTics = 2*TICRATE;
      radius = r;
      height = h;
      // TODO some sound to indicate unsuccesful morph?
      return false;
    }

  // spawn FX
  DActor *fog = mp->SpawnDActor(pos.x+20*Cos(yaw), pos.y+20*Sin(yaw), pos.z+TELEFOGHEIGHT, MT_TFOG);
  S_StartSound(fog, sfx_teleport);

  morphTics = 0;

  mass   = info->mass;
  health = maxhealth = info->spawnhealth;
  speed  = info->speed;
  pclass = info->pclass;

  reactiontime = 18;
  powers[pw_weaponlevel2] = 0;
  weaponinfo = wpnlev1info;

  PostMorphWeapon(weapontype_t(attackphase));
  return true;
}



//============================================================
//  Artifacts
//============================================================

extern consvar_t cv_deathmatch;

void P_AutoUseHealth(PlayerPawn *p, int saveHealth)
{
  int i, n = p->inventory.size();
  int count;
  int normalSlot;
  int superSlot;
  int normalCount = 0;
  int superCount = 0;

  for (i = 0; i < n; i++)
    {
      if (p->inventory[i].type == arti_health)
        {
	  normalSlot = i;
	  normalCount = p->inventory[i].count;
        }
      else if (p->inventory[i].type == arti_superhealth)
        {
	  superSlot = i;
	  superCount = p->inventory[i].count;
        }
    }

  if (game.skill == sk_baby && normalCount*25 >= saveHealth)
    { // Use quartz flasks
      count = (saveHealth+24)/25;
      for (i = 0; i < count; i++)
	p->UseArtifact(arti_health);
    }
  else if(superCount*100 >= saveHealth)
    { // Use mystic urns
      count = (saveHealth+99)/100;
      for (i = 0; i < count; i++)
	p->UseArtifact(arti_superhealth);
    }
  else if (game.skill == sk_baby && superCount*100 + normalCount*25 >= saveHealth)
    { // Use mystic urns and quartz flasks
      count = (saveHealth+24)/25;
      for (i = 0; i < count; i++)
	p->UseArtifact(arti_health);

      saveHealth -= count*25;
      count = (saveHealth+99)/100;
      for (i = 0; i < count; i++)
	p->UseArtifact(arti_superhealth);
    }
}


static void P_TeleportToPlayerStarts(Actor *v, int n, int ep)
{
  Map *m = v->mp;
  mapthing_t *mt = NULL;
  multimap<int, mapthing_t *>::iterator s, t;

  if (n)
    {
      s = m->playerstarts.lower_bound(n);
      t = m->playerstarts.upper_bound(n);
    }
  else
    {
      // TODO teleport to random ep 0 start, not first
      s = m->playerstarts.begin();
      t = m->playerstarts.end();
    }

  for ( ; s != t; s++)
    {
      mt = (*s).second;
      if (mt->args[0] == ep)
	break;
    }

  // if nothing suitable is found, just pick any start
  if (s == t)
    mt = (*m->playerstarts.begin()).second;

  v->Teleport(vec_t<fixed_t>(mt->x, mt->y, 0), ANG45 * (mt->angle / 45));
}


static bool P_TeleportToDeathmatchStarts(Actor *v)
{
  int n = v->mp->dmstarts.size();
  if (n == 0)
    return false;

  n = P_Random() % n;
  mapthing_t *m = v->mp->dmstarts[n];
  return v->Teleport(vec_t<fixed_t>(m->x, m->y, 0), ANG45 * (m->angle / 45));
}



//============================================================
// Chaos Device, teleports the player back to a playerstart

bool P_AutoUseChaosDevice(PlayerPawn *p)
{
  int i, n = p->inventory.size();
    
  for (i = 0; i < n; i++)
    {
      if (p->inventory[i].type == arti_teleport)
        {
	  p->UseArtifact(arti_teleport);
	  p->health = (p->health + 1) / 2;
	  return true;
        }
    }
  return false;
}



void P_ArtiTele(PlayerPawn *p)
{
  if (!cv_deathmatch.value || !P_TeleportToDeathmatchStarts(p))
    P_TeleportToPlayerStarts(p, p->player->number, p->player->entrypoint);

  if (p->morphTics)
    p->UndoMorph(); // Teleporting away will undo any morph effects (pig)

  S_StartAmbSound(p->player, sfx_weaponup); // Full volume laugh
}


//============================================================
// Banishment Device

void P_TeleportOther(Actor *v)
{
  Map *m = v->mp;

  if (v->flags & MF_PLAYER)
    {
      if (!cv_deathmatch.value || !P_TeleportToDeathmatchStarts(v))
	P_TeleportToPlayerStarts(v, 0, 0);
    }
  else
    {
      if (!(v->flags & MF_MONSTER) ||
	  v->flags2 & MF2_BOSS ||
	  v->flags2 & MF2_CANTLEAVEFLOORPIC)
	return;

      // For death actions, teleporting is as good as killing
      // FIXME possible bug: see A_SorcDeath
      if (v->special)
	{
	  m->RemoveFromTIDmap(v);
	  m->ExecuteLineSpecial(v->special, v->args, NULL, 0, v);
	  v->special = 0;
	}

      // Send all monsters to deathmatch spots
      if (!P_TeleportToDeathmatchStarts(v))
	P_TeleportToPlayerStarts(v, 0, 0);
    }
}


//============================================================
// Mystic Ambit Incantation, class specific effect for everyone in radius
// if only C++ had functions inside functions!

static Actor *caster;
static bool   given;

static bool IT_HealRadius(Thinker *th)
{
  PlayerPawn *t = th->Inherits<PlayerPawn>();
  if (!t)
    return true;

  if (t->health <= 0)
    return true;
		
  fixed_t dist = P_XYdist(caster->pos, t->pos);
  const fixed_t HEAL_RADIUS_DIST = 255;

  if (dist > HEAL_RADIUS_DIST)
    return true;

  switch (t->pclass)
    {
    case PCLASS_FIGHTER: // armor boost
      if (t->GiveArmor(armor_armor, -3, 1)  || t->GiveArmor(armor_shield, -3, 1) ||
	  t->GiveArmor(armor_helmet, -3, 1) || t->GiveArmor(armor_amulet, -3, 1))
	{
	  given = true;
	  S_StartSound(t, SFX_MYSTICINCANT);
	}
      break;

    case PCLASS_CLERIC: // heal
      if (t->GiveBody(50 + P_Random() % 50))
	{
	  given = true;
	  S_StartSound(t, SFX_MYSTICINCANT);
	}
      break;

    case PCLASS_MAGE: // mana boost
      {
	int amount = 50 + (P_Random()%50);
	if (t->GiveAmmo(am_mana1, amount) || t->GiveAmmo(am_mana2, amount))
	  {
	    given = true;
	    S_StartSound(t, SFX_MYSTICINCANT);
	  }
      }
      break;

    default:
      break;
    }

  return true;
}


bool P_HealRadius(Actor *p)
{
  given = false;
  caster = p;
  p->mp->IterateThinkers(IT_HealRadius);
  return given;
}


//============================================================
// Disc of Repulsion

static const fixed_t BLAST_SPEED = 20;
static const fixed_t BLAST_FULLSTRENGTH = 255;

static void P_BlastMobj(Actor *source, Actor *victim, fixed_t strength)
{
  angle_t angle = R_PointToAngle2(source->pos, victim->pos);
  angle_t ang = (angle + ANG180) >> ANGLETOFINESHIFT;
  angle >>= ANGLETOFINESHIFT;

  if (strength < BLAST_FULLSTRENGTH)
    {
      victim->vel.x = strength * finecosine[angle];
      victim->vel.y = strength * finesine[angle];
    }
  else // full strength blast from artifact
    {
      if (victim->flags & MF_MISSILE)
	{
	  // guided missiles change owner!
	  if (victim->target == source)
	    {
	      victim->target = victim->owner;
	      victim->owner = source;
	    }
	}

      victim->vel.x = BLAST_SPEED * finecosine[angle];
      victim->vel.y = BLAST_SPEED * finesine[angle];

      // Spawn blast puff
      fixed_t x, y, z;
      x = victim->pos.x + (victim->radius + 1) * finecosine[ang];
      y = victim->pos.y + (victim->radius + 1) * finesine[ang];
      z = victim->Center() - victim->floorclip;

      DActor *m = victim->mp->SpawnDActor(x, y, z, MT_BLASTEFFECT);
      if (m)
	{
	  m->vel.x = victim->vel.x;
	  m->vel.y = victim->vel.y;
	}

      if (victim->flags & MF_MISSILE)
	{
	  victim->vel.z = 8;
	  m->vel.z = victim->vel.z;
	}
      else
	victim->vel.z = 1000 / victim->mass;
    }


  victim->flags2 |= MF2_SLIDE;
  victim->eflags |= MFE_BLASTED;
}


static bool IT_BlastRadius(Actor *a)
{
  if (a->flags2 & (MF2_NONBLASTABLE | MF2_DORMANT | MF2_BOSS))
    return true;

  if (a == caster)
    return true;

  if (!(a->flags & (MF_CORPSE | MF_MONSTER | MF_MISSILE)))
    return true;

  DActor *m = a->Inherits<DActor>();
  if (m)
    {
      // blastable must be live monster, poisoncloud, holyfx, icecorpse, missile
      // TODO MT_POISONCLOUD can be blasted
    }

  // dactors and playerpawns are blasted
  fixed_t dist = P_XYdist(caster->pos, a->pos);
  if (dist > 255)
    return true; // Out of range

  P_BlastMobj(caster, a, BLAST_FULLSTRENGTH);
  return true;
}


// Blast all mobj things away
static void P_BlastRadius(PlayerPawn *p)
{
  S_StartSound(p, SFX_ARTIFACT_BLAST);
  P_NoiseAlert(p, p);
  caster = p;
  p->mp->IterateActors(IT_BlastRadius);
}


//============================================================
//  Activating an artifact

bool P_UseArtifact(PlayerPawn *p, artitype_t arti)
{
  DActor *mo;
  angle_t ang;
  int count;
  vector<inventory_t>::iterator t;

  switch (arti)
    {
    case arti_invulnerability:
      return p->GivePower(pw_invulnerability);

    case arti_invisibility:
      return p->GivePower(pw_invisibility);

    case arti_health:
      return p->GiveBody(25);

    case arti_superhealth:
      return p->GiveBody(100);

    case arti_tomeofpower:
      if (p->morphTics)
        { // Attempt to undo chicken
	  if (p->UndoMorph() == false)
            { // Failed
	      p->Damage(NULL, NULL, 10000, dt_always);
            }
	  else
            { // Succeeded
	      p->morphTics = 0;
	      S_StartScreamSound(p, sfx_weaponup);
            }
        }
      else
        {
	  if (!p->GivePower(pw_weaponlevel2))
	    return false;

	  if (p->readyweapon == wp_staff)
	    p->SetPsprite(ps_weapon, S_STAFFREADY2_1);
	  else if (p->readyweapon == wp_gauntlets)
	    p->SetPsprite(ps_weapon, S_GAUNTLETREADY2_1);
        }
      break;

    case arti_torch:
      return p->GivePower(pw_infrared);

    case arti_firebomb:
      ang = p->yaw >> ANGLETOFINESHIFT;
      mo = p->mp->SpawnDActor(p->pos.x+24*finecosine[ang], p->pos.y+24*finesine[ang],
			      p->pos.z - p->floorclip, MT_FIREBOMB);
      mo->owner = p;
      break;

    case arti_egg:
      p->SpawnPlayerMissile(MT_EGGFX);
      p->SPMAngle(MT_EGGFX, p->yaw-(ANG45/6));
      p->SPMAngle(MT_EGGFX, p->yaw+(ANG45/6));
      p->SPMAngle(MT_EGGFX, p->yaw-(ANG45/3));
      p->SPMAngle(MT_EGGFX, p->yaw+(ANG45/3));
      break;

    case arti_fly:
      return p->GivePower(pw_flight);

    case arti_teleport:
      P_ArtiTele(p);
      break;

    case arti_healingradius:
      return P_HealRadius(p);

    case arti_summon:
      mo = p->SpawnPlayerMissile(MT_SUMMON_FX);
      if (mo)
	{
	  mo->owner = p;
	  mo->vel.z = 5;
	}
      break;

    case arti_pork:
      p->SpawnPlayerMissile(MT_XEGGFX);
      p->SPMAngle(MT_XEGGFX, p->yaw-(ANG45/6));
      p->SPMAngle(MT_XEGGFX, p->yaw+(ANG45/6));
      p->SPMAngle(MT_XEGGFX, p->yaw-(ANG45/3));
      p->SPMAngle(MT_XEGGFX, p->yaw+(ANG45/3));
      break;

    case arti_blastradius:
      P_BlastRadius(p);
      break;

    case arti_poisonbag:
      ang = p->yaw >> ANGLETOFINESHIFT;
      switch (p->pclass)
	{
	case PCLASS_CLERIC:
	  mo = p->mp->SpawnDActor(p->pos.x + 16*finecosine[ang], p->pos.y + 24*finesine[ang],
				  p->pos.z - p->floorclip + 8, MT_POISONBAG);
	  break;
	case PCLASS_MAGE:
	  mo = p->mp->SpawnDActor(p->pos.x + 16*finecosine[ang], p->pos.y + 24*finesine[ang],
				  p->pos.z - p->floorclip + 8, MT_FIREBOMB);
	  break;
	default:
	  mo = p->mp->SpawnDActor(p->pos.x, p->pos.y, p->pos.z - p->floorclip + 35, MT_THROWINGBOMB);
	  if (mo)
	    {
	      mo->owner = p;
	      mo->yaw = p->yaw + (((P_Random() & 7)-4) << 24);
	      ang = p->pitch >> ANGLETOFINESHIFT;
	      float sp = mo->info->speed;
	      mo->Thrust(mo->yaw, sp * finecosine[ang]);
	      mo->vel.z = 4 + sp * finesine[ang];
	      mo->vel.x += p->vel.x >> 1;
	      mo->vel.y += p->vel.y >> 1;
	      mo->tics -= P_Random() & 3;
	      mo->CheckMissileSpawn();
	    }
	}

      if (mo)
	mo->owner = p;
      break;
      
    case arti_teleportother:
      mo = p->SpawnPlayerMissile(MT_TELOTHER_FX1);
      if (mo)
	mo->owner = p;
      break;

    case arti_speed:
      return p->GivePower(pw_speed);

    case arti_boostmana:
      if (!p->GiveAmmo(am_mana1, 200))
	return p->GiveAmmo(am_mana2, 200);
      else
	p->GiveAmmo(am_mana2, 200);
      break;

    case arti_boostarmor:
      count = 0;
      for (int i = armor_armor; i < NUMARMOR; ++i)
	count += p->GiveArmor(armortype_t(i), -3.0, 1); // 1 point per armor type
      return count;

    case arti_puzzskull:
    case arti_puzzgembig:
    case arti_puzzgemred:
    case arti_puzzgemgreen1:
    case arti_puzzgemgreen2:
    case arti_puzzgemblue1:
    case arti_puzzgemblue2:
    case arti_puzzbook1:
    case arti_puzzbook2:
    case arti_puzzskull2:
    case arti_puzzfweapon:
    case arti_puzzcweapon:
    case arti_puzzmweapon:
    case arti_puzzgear1:
    case arti_puzzgear2:
    case arti_puzzgear3:
    case arti_puzzgear4:
      if (p->UsePuzzleItem(arti - arti_firstpuzzitem))
	return true;
      else
	{
	  p->player->SetMessage(text[TXT_USEPUZZLEFAILED], false);
	  return false;
	}
      break;

    case arti_fsword1:
    case arti_fsword2:
    case arti_fsword3:
    case arti_choly1:
    case arti_choly2:
    case arti_choly3:
    case arti_mstaff1:
    case arti_mstaff2:
    case arti_mstaff3:
      {
	// first check which pieces we have
	count = 0;
	for (t = p->inventory.begin(); t < p->inventory.end(); t++)
	  if (t->type >= arti_fsword1)
	    count |= (1 << (t->type - arti_fsword1));

	int wp = (arti - arti_fsword1) / 3; // 0, 1 or 2
	int mask = 7 << (3 * wp); // hack...

	// have we enough pieces to assemble a weapon?
	if ((count & mask) == mask)
	  {
	    p->weaponowned[wp + wp_quietus] = true;
	    p->pendingweapon = weapontype_t(wp + wp_quietus);
	    p->player->SetMessage(text[TXT_WEAPON_F4 + 3*wp], false);
	    S_StartAmbSound(NULL, SFX_WEAPON_BUILD); // warn all players!
	  }
	else
	  {
	    p->player->SetMessage("you don't yet have all the pieces", false);
	    return false;
	  }
      }
      break;

    default:
      return false;
    }
  return true;
}


#define ITEMUSE_FRAMES 4

bool Map::EV_LineSearchForPuzzleItem(line_t *line, byte *args, Actor *a)
{
  if (!a)
    return false;

  PlayerPawn *p = a->Inherits<PlayerPawn>();

  if (!p)
    return false;

  // Search player's inventory for puzzle items
  vector<inventory_t>::iterator t;
  for (t = p->inventory.begin(); t < p->inventory.end(); t++)
    {
      artitype_t arti = artitype_t(t->type);
      int type = arti - arti_firstpuzzitem;
      if (type < 0)
	continue;

      if (type == args[0])
	{
	  // A puzzle item was found for the line
	  if (P_UseArtifact(p, arti))
	    {
	      // Artifact was used - remove it from inventory
	      if (--(t->count) == 0)
		{
		  // Used last of a type - compact the artifact list
		  p->inventory.erase(t);
		}

	      S_StartSound(p, SFX_PUZZLE_SUCCESS);
	      p->player->itemuse = ITEMUSE_FRAMES;

	      return true;
	    }
	}
    }

  return false;
}



/// called by the server
void PlayerPawn::UseArtifact(artitype_t arti)
{
  for (vector<inventory_t>::iterator i = inventory.begin(); i < inventory.end(); i++) 
    if (i->type == arti)
      {
	// Found match - try to use
	if (P_UseArtifact(this, arti))
	  {
	    // Artifact was used - remove it from inventory
	    if (--(i->count) == 0)
	      {
		// Used last of a type - compact the artifact list
		inventory.erase(i);
	      }

	    S_StartSound(this, sfx_artiuse);
	    player->itemuse = ITEMUSE_FRAMES;
	  }
	else
	  { // Unable to use artifact
	  }
	break;
      }
}


/// called by the client
bool PlayerInfo::InventoryResponder(short (*gc)[2], event_t *ev)
{
  if (!pawn)
    return false;

  //gc is a pointer to array[num_gamecontrols][2]

  switch (ev->type)
    {
    case ev_keydown :
      if (ev->data1 == gc[gc_invprev][0] || ev->data1 == gc[gc_invprev][1])
        {
          if (invTics)
            {
              if (--invSlot < 0)
                invSlot = 0;
              else if (--invPos < 0)
                invPos = 0;
            }
          invTics = 5*TICRATE;
          return true;
        }
      else if (ev->data1 == gc[gc_invnext][0] || ev->data1 == gc[gc_invnext][1])
        {
          int n = pawn->inventory.size();

          if (invTics && n > 0)
            {
              if (++invSlot >= n)
                invSlot = n-1;
              else if (++invPos > 6)
                invPos = 6;
            }
          invTics = 5*TICRATE;
          return true;
        }
      else if (ev->data1 == gc[gc_invuse ][0] || ev->data1 == gc[gc_invuse ][1])
        {
          int n = pawn->inventory.size();

          if (invTics)
            invTics = 0;
          else if (invSlot < n && pawn->inventory[invSlot].count > 0)
	    cmd.item = pawn->inventory[invSlot].type + 1;

          return true;
        }
      break;

    case ev_keyup:
      if (ev->data1 == gc[gc_invuse ][0] || ev->data1 == gc[gc_invuse ][1] ||
          ev->data1 == gc[gc_invprev][0] || ev->data1 == gc[gc_invprev][1] ||
          ev->data1 == gc[gc_invnext][0] || ev->data1 == gc[gc_invnext][1])
        return true;
      break;

    default:
      break; // shut up compiler
    }
  return false;
}
