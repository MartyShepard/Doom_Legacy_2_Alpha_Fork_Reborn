// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_inter.cpp 522 2008-01-20 22:42:23Z smite-meister $
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
/// \brief Actor interactions (collisions, damage, death).

#include <math.h>

#include "doomdef.h"
#include "command.h"
#include "cvars.h"

#include "dstrings.h"
#include "m_random.h"
#include "g_damage.h"

#include "g_game.h"
#include "g_type.h"
#include "g_player.h"
#include "g_map.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "g_decorate.h"

#include "p_enemy.h"
#include "p_maputl.h"
#include "p_spec.h"
#include "sounds.h"
#include "r_presentation.h"
#include "tables.h"



#define BASETHRESHOLD 100

extern int ArmorIncrement[NUMCLASSES][NUMARMOR];

void P_TeleportOther(Actor *v);
bool P_AutoUseChaosDevice(PlayerPawn *p);
void P_AutoUseHealth(PlayerPawn *p, int saveHealth);


// TODO owner chains, must not be looped!
Actor *FindOwner(Actor *a)
{
  int count = 0;
  while (a->owner)
    {
      a = a->owner;
      if (++count > 10)
	I_Error("Actor::owner loop detected!\n");
    }

  return a;
}


//======================================================
//   Killing a PlayerPawn
//======================================================

// XXX::Killed is called when a class XXX member kills a PlayerPawn.
// It returns the proper death message and updates the score.

void Actor::Killed(PlayerPawn *victim, Actor *inflictor)
{
  CONS_Printf("%s is killed by an inanimate carbon rod.\n", victim->player->name.c_str());
}


void DActor::Killed(PlayerPawn *victim, Actor *inflictor)
{
  // monster killer

  const char *str = NULL;
  if (!inflictor && !info->hitobituary.empty())
    str = info->hitobituary.c_str();
  else if (!info->obituary.empty())
    str = info->obituary.c_str();
  else
    str = text[TXT_DEATHMSG_DEAD];

  victim->player->SetMessage(va(str, victim->player->name.c_str()));
}


void PlayerPawn::Killed(PlayerPawn *victim, Actor *inflictor)
{
  // player killer
  game.gtype->Frag(player, victim->player);

  S_StartAmbSound(player, sfx_frag);

  if (game.mode == gm_heretic)
    {
      // Make a super chicken
      if (morphTics)
	GivePower(pw_weaponlevel2);
    }

  const char *str = NULL;

  if (player == victim->player)
    {
      player->SetMessage(va(text[TXT_DEATHMSG_SUICIDE], player->name.c_str()));
      return;
    }

  if (victim->health < -9000) // telefrag !
    str = text[TXT_DEATHMSG_TELEFRAG];
  else
    {
      int w = -1;
      DActor *inf;
      if (inflictor && (inf = inflictor->Inherits<DActor>()))
	{
	  switch (inf->type)
	    {
	    case MT_BARREL:
	      w = wp_barrel;
	      break;
	    case MT_ROCKET   :
	      w = wp_missile;
	      break;
	    case MT_PLASMA   :
	      w = wp_plasma;
	      break;
	    case MT_EXTRABFG :
	    case MT_BFG      :
	      w = wp_bfg;
	      break;
	    default :
	      w = readyweapon;
	      break;
	    }
	}

      switch(w)
	{
	case wp_fist:
	  str = text[TXT_DEATHMSG_FIST];
	  break;
	case wp_pistol:
	  str = text[TXT_DEATHMSG_GUN];
	  break;
	case wp_shotgun:
	  str = text[TXT_DEATHMSG_SHOTGUN];
	  break;
	case wp_chaingun:
	  str = text[TXT_DEATHMSG_MACHGUN];
	  break;
	case wp_missile:
	  str = text[TXT_DEATHMSG_ROCKET];
	  if (victim->health < -victim->maxhealth)
	    str = text[TXT_DEATHMSG_GIBROCKET];
	  break;
	case wp_plasma:
	  str = text[TXT_DEATHMSG_PLASMA];
	  break;
	case wp_bfg:
	  str = text[TXT_DEATHMSG_BFGBALL];
	  break;
	case wp_chainsaw:
	  str = text[TXT_DEATHMSG_CHAINSAW];
	  break;
	case wp_supershotgun:
	  str = text[TXT_DEATHMSG_SUPSHOTGUN];
	  break;
	case wp_barrel:
	  str = text[TXT_DEATHMSG_BARRELFRAG];
	  break;
	default:
	  str = text[TXT_DEATHMSG_PLAYUNKNOW];
	  break;
	}
    }

  // send message to both parties
  str = va(str, victim->player->name.c_str(), player->name.c_str());
  player->SetMessage(str);
  victim->player->SetMessage(str);
}




//======================================================
//    Touching another Actor
//======================================================

// Called when two actors touch one another
// Returns true if any interaction takes place.

bool Actor::Touch(Actor *p)
{

  // missiles can hit other things
  if (flags & MF_MISSILE)
    {
      // Check for passing through a ghost (heretic)
      if ((p->flags & MF_SHADOW) && (flags2 & MF2_THRUGHOST))
	return false;

      if (!(p->flags & MF_SHOOTABLE))
        {
	  // didn't do any damage
	  return (p->flags & MF_SOLID);
        }

      // don't traverse any more
      return true;
    }


  return (flags & MF_SOLID) && (p->flags & MF_SOLID);
}


/// DActor physically touching an Actor. Returns true if an interaction takes place.
bool DActor::Touch(Actor *p)
{
  int damage = info->damage & dt_DAMAGEMASK;
  int dtype = info->damage & dt_TYPEMASK;

  // check for skulls slamming into things
  if (eflags & MFE_SKULLFLY)
    {
      if (flags & MF_TOUCHFUNC)
	{
	  int temp = info->touchf(this, p);
	  if (temp >= 0)
	    return temp;
	}

      // Slamming monsters shouldn't move non-creatures
      if (!(p->flags & MF_SHOOTABLE))
	return true;

      damage = ((P_Random()%8) + 1) * damage;

      p->Damage(this, this, damage, dtype);

      eflags &= ~MFE_SKULLFLY;
      vel.Set(0,0,0);
      return true; // stop moving
    }


  // Check for blasted thing running into another
  if (eflags & MFE_BLASTED &&
      p->flags & MF_SHOOTABLE &&
      p->flags & MF_MONSTER &&
      !(p->flags2 & MF2_BOSS))
    {
      // what a mockery of physics :)
      p->vel.x += vel.x;
      p->vel.y += vel.y;
      if ((p->vel.x + p->vel.y) > 3)
	{
	  p->Damage(this, this, int(mass/100)+1);
	  Damage(p, p, int(((p->mass/100)+1)/4));
	}
      return true;
    }


  // missiles can hit other things
  if (flags & MF_MISSILE)
    {
      // nonshootables are transparent to missiles
      if (p->flags2 & MF2_NONSHOOTABLE)
	return false;

      // Check for passing through a ghost (heretic)
      if ((p->flags & MF_SHADOW) && (flags2 & MF2_THRUGHOST))
	return false;

      // Don't hit the originator.
      if (p == owner)
	return false;

      // superbouncy missiles
      if (flags2 & MF2_FULLBOUNCE)
	{
	  if (!(p->flags & MF_SOLID))
	    return false;

	  if ((p->flags2 & MF2_REFLECTIVE) ||
	      !(p->flags & MF_VALIDTARGET)) // TODO why? isn't MF_SHOOTABLE enough?
	    {
	      yaw = R_PointToAngle2(p->pos, pos) + ANGLE_1*((P_Random()%16) - 8);
	      fixed_t speed = 0.75 * P_AproxDistance(vel.x, vel.y);

	      vel.x = speed * Cos(yaw);
	      vel.y = speed * Sin(yaw);
	      if (info->seesound)
		S_StartSound(this, info->seesound);

	      return true; // block but do not explode missile
	    }

	  goto explode; // Struck a player/creature
	}
      else if (p->flags2 & MF2_REFLECTIVE)
	{
	  yaw = R_PointToAngle2(p->pos, pos);

	  DActor *t = p->Inherits<DActor>();
	  mobjtype_t temp = t ? t->type : MT_NONE;

	  // Change angle for deflection/reflection
	  switch (temp)
	    {
	    case MT_CENTAUR:
	    case MT_CENTAURLEADER:
	      if ((Abs(yaw - p->yaw) >> 24) > 45 || type == MT_HOLY_FX) // flank shot or wraithverge
		goto explode;

	      // fallthru
	    case MT_SORCBOSS:
	      // Deflection
	      if (P_Random() < 128)
		yaw += ANG45;
	      else
		yaw -= ANG45;
	      break;

	    default:
	      // Reflection
	      yaw += ANGLE_1 * ((P_Random()%16)-8);
	      break;
	    }

	  // Reflect the missile along angle
	  float speed = t ? 0.5*t->info->speed : 0.5*sqrt(vel.XYNorm2().Float());
	  vel.x = speed * Cos(yaw);
	  vel.y = speed * Sin(yaw);
	  // vel.z = -vel.z;

	  // exchange target and owner
	  if (flags2 & MF2_SEEKERMISSILE)
	    target = owner;

	  owner = p;
	  return true; // block but do not explode missile
	}

    explode:
      target = p;

      if (flags & MF_TOUCHFUNC)
	{
	  int temp = info->touchf(this, p);
	  if (temp >= 0)
	    return temp; // fixme explode?
	}

      // Don't damage the same species as the originator.
      if (owner)
	{
	  DActor *ow = owner->Inherits<DActor>();
	  DActor *t = p->Inherits<DActor>();

	  if (ow && t)
	  if (ow->type == t->type ||
	      (ow->type == MT_KNIGHT  && t->type == MT_BRUISER) ||
	      (ow->type == MT_BRUISER && t->type == MT_KNIGHT)) // damn complicated!
	    {
	      ExplodeMissile();
	      return true; // Explode, but do no damage.
	    }
        }

      if (!(p->flags & MF_SHOOTABLE))
        {
	  // didn't do any damage
	  if (p->flags & MF_SOLID)
	    {
	      ExplodeMissile();
	      return true;
	    }

	  return false;
        }

      // ripping projectiles
      if (flags2 & MF2_RIP)
        {
	  damage = ((P_Random () & 3) + 2) * damage;
	  S_StartSound(this, sfx_ripslop);
	  if (p->Damage(this, owner, damage, dtype))
            {
	      if (!(p->flags & MF_NOBLOOD))
		mp->SpawnBlood(pos, damage);
            }

	  if ((p->flags2 & MF2_PUSHABLE) && !(flags2 & MF2_CANNOTPUSH))
            { // Push thing
	      p->vel.x += vel.x >> 2;
	      p->vel.y += vel.y >> 2;
            }
	  //spechit.clear(); FIXME why?
	  return false;
        }

      // damage / explode
      damage = ((P_Random()%8)+1) * damage;
      if (damage &&
	  p->Damage(this, owner, damage, dtype) &&
	  !(p->flags & MF_NOBLOOD))
	{
	  /* Hexen:
	  if (P_Random() < 192)
	    P_BloodSplatter(tmthing->x, tmthing->y, tmthing->z, thing);
	  */
	  mp->SpawnBloodSplats(pos, damage, p->vel.x, p->vel.y);
	}
	
      // don't traverse any more
      ExplodeMissile();
      return true;
    }

  if ((p->flags2 & MF2_PUSHABLE) && !(flags2 & MF2_CANNOTPUSH))
    {
      // Push thing
      p->vel.x += vel.x >> 2;
      p->vel.y += vel.y >> 2;
    }

  // check for special pickup
  if (p->flags & MF_SPECIAL)
    {
      if (flags & MF_PICKUP)
        {
	  // can remove thing
	  // TODO: some beneficial effects for DActors too?
	  p->Remove();
        }
      return p->flags & MF_SOLID; // most specials are not solid...
    }

  return (p->flags & MF_SOLID);
}


bool PlayerPawn::Touch(Actor *p)
{
  if ((p->flags2 & MF2_PUSHABLE) && !(flags2 & MF2_CANNOTPUSH))
    {
      // Push thing
      p->vel.x += vel.x >> 2;
      p->vel.y += vel.y >> 2;
    }

  // check for special pickup
  if (p->flags & MF_SPECIAL)
    {
      if (flags & MF_PICKUP)
        {
	  // can remove thing
	  DActor *dp = p->Inherits<DActor>();
	  if (dp)
	    TouchSpecialThing(dp); // this also Removes() the item

	  return false; // no collision, just possible pickup
        }
    }

  return (p->flags & MF_SOLID);
}



//======================================================
//    Giving damage
//======================================================

// Gives damage to the Actor. If the damage is "stopped" (absorbed), returns true.
// Inflictor is the thing that caused the damage
// (creature or missile, can also be NULL (slime, etc.))
// Source is the thing responsible for the damage (the one to get angry at).
// It may also be NULL. Source and inflictor are the same for melee attacks.

bool Actor::Damage(Actor *inflictor, Actor *source, int damage, int dtype)
{
  if (!(flags & MF_SHOOTABLE))
    return false;

  if ((dtype & dt_TYPEMASK) >= dt_LIVING && (flags & MF_NOBLOOD))
    return false; // only damage living things with poison

  // old recoil code
  // Some close combat weapons do not inflict thrust and push the victim out of reach
  if (inflictor && !(flags & MF_NOCLIPTHING) && !(dtype & dt_norecoil) &&
      !(inflictor->flags2 & MF2_NODMGTHRUST))      
    {
      fixed_t apx, apy, apz = 0;
      fixed_t thrust;  

      angle_t ang = R_PointToAngle2(inflictor->pos, pos);

      if (game.mode >= gm_heretic)
	thrust = damage * 18.75f/(mass+1);
      else
	thrust = damage * 12.5f/(mass+1);

      // sometimes a target shot down might fall off a ledge forwards
      if (damage < 40 && damage > health
	  && (pos.z - inflictor->pos.z) > 64 && (P_Random() & 1))
        {
	  ang += ANG180;
	  thrust *= 4;
        }

      ang >>= ANGLETOFINESHIFT;

      apx = thrust * finecosine[ang];
      apy = thrust * finesine[ang];
      vel.x += apx;
      vel.y += apy;
            
      // pz (do it better for explosions)
      if (!cv_allowrocketjump.value)
	{
	  fixed_t dist, sx, sy, sz;

	  sx = inflictor->pos.x;
	  sy = inflictor->pos.y;
	  sz = inflictor->pos.z;

	  dist = R_PointToDist2(sx, sy, pos.x, pos.y);
                
	  ang = R_PointToAngle2(0, sz, dist, pos.z);
                
	  ang >>= ANGLETOFINESHIFT;
	  apz = thrust * finesine[ang];
	}
      else
	{
	  // rocket jump code
	  fixed_t delta1 = abs(inflictor->pos.z - pos.z);
	  fixed_t delta2 = abs(inflictor->Feet() - Top());
	  apz = (abs(apx) + abs(apy)) >> 1;
	  
	  if (delta1 >= delta2 && inflictor->vel.z < 0)
	    apz = -apz;
	}
      vel.z += apz;
    }
  
  // do the damage
  health -= damage;
  if (health <= 0)
    Die(inflictor, source, dtype);

  return true;
}





bool DActor::Damage(Actor *inflictor, Actor *source, int damage, int dtype)
{
  if (!(flags & MF_SHOOTABLE))
    return false; // shouldn't happen...

  if (dtype & dt_always)
    return Actor::Damage(inflictor, source, damage, dtype);

  if (health <= 0)
    return false;

  /* TODO ice corpses
	if(target->health <= 0)
	{
		if (inflictor && inflictor->flags2&MF2_ICEDAMAGE)
		{
			return;
		}
		else if (target->flags&MF_ICECORPSE) // frozen
		{
			target->tics = 1;
			target->momx = target->momy = 0;
		}
		return;
	}

  if ((target->flags2 & MF2_INVULNERABLE) && damage < 10000)
	{ // mobj is invulnerable
		if(target->player) return;	// for player, no exceptions
		if(inflictor)
		{
			switch(inflictor->type)
			{
				// These inflictors aren't foiled by invulnerability
				case MT_HOLY_FX:
				case MT_POISONCLOUD:
				case MT_FIREBOMB:
					break;
				default:
					return;
			}
		}
		else
		{
			return;
		}
	}
  */

  if (eflags & MFE_SKULLFLY)
    {
      // Minotaur is invulnerable during charge attack
      if (type == MT_MINOTAUR)
	return false;
      vel.Set(0,0,0);
    }

  if (flags2 & MF2_DORMANT)
    {
      // Invulnerable, and won't wake up
      return false;
    }


  // Special damage types
  DActor *inf;
  if (inflictor && (inf = inflictor->Inherits<DActor>()))
    {
      switch (inf->type)
        {
        case MT_EGGFX:
	  Morph(MT_CHICKEN);
	  return false; // Always return

        case MT_XEGGFX:
	  Morph(MT_PIG);
	  return false;


        case MT_WHIRLWIND:
	  return P_TouchWhirlwind(this);

        case MT_MINOTAUR:
	  if (inflictor->eflags & MFE_SKULLFLY)
            { // Slam only when in charge mode
	      P_MinotaurSlam(inflictor, this);
	      return true;
            }
	  break;
        case MT_MACEFX4: // Death ball
	  if ((flags2 & MF2_BOSS) || type == MT_HHEAD)
	    // Don't allow cheap boss kills
	    break;
	  damage = 10000; // Something's gonna die
	  break;
        case MT_RAINPLR1: // Rain missiles
        case MT_RAINPLR2:
        case MT_RAINPLR3:
        case MT_RAINPLR4:
	  if (flags2 & MF2_BOSS)
            { // Decrease damage for bosses
	      damage = (P_Random()&7)+1;
            }
	  break;
        case MT_HORNRODFX2:
        case MT_PHOENIXFX1:
	  if (type == MT_SORCERER2 && P_Random() < 96)
            { // D'Sparil teleports away
	      DSparilTeleport();
	      return false;
            }
	  break;
        case MT_BLASTERFX1:
        case MT_RIPPER:
	  if (type == MT_HHEAD)
            { // Less damage to Ironlich bosses
	      damage = P_Random()&1;
	      if (!damage)
		return false;
            }
	  break;

	  // Hexen
	case MT_TELOTHER_FX1:
	case MT_TELOTHER_FX2:
	case MT_TELOTHER_FX3:
	case MT_TELOTHER_FX4:
	case MT_TELOTHER_FX5:
	  P_TeleportOther(this);
	  return true;
	case MT_SHARDFX1:
	  damage <<= inf->special2; // shard damage depends on spermcount
	  break;

        default:
	  break;
        }
    }

  if (!Actor::Damage(inflictor, source, damage, dtype))
    return false; // not hurt after all

  if (health <= 0)
    {
      //special1 = damage; // wtf?
      return true;
    }

  // Hurt but not dead. Should we go into painstate?
  if ((P_Random () < info->painchance)
      && !(eflags & MFE_SKULLFLY || flags & MF_CORPSE))
    {
      if ((dtype & dt_TYPEMASK) == dt_shock)
	{
	  if (P_Random() < 96)
	    {
	      eflags |= MFE_JUSTHIT; // fight back!
	      SetState(info->painstate);
	      pres->SetAnim(presentation_t::Pain);
	    }
	  else
	    { // "electrocute" the target
	      /* TODO fullbright
	      frame |= FF_FULLBRIGHT;
	      if (flags & MF_MONSTER && P_Random() < 128)
		Howl();
	      */
	    }
	}
      else
	{
	  eflags |= MFE_JUSTHIT;    // fight back!
	  SetState(info->painstate);
	  pres->SetAnim(presentation_t::Pain);

	  /* TODO poisonhowl
	  if (inflictor && inflictor->type == MT_POISONCLOUD)
	    {
	      if (flags & MF_MONSTER && P_Random() < 128)
		Howl();
	    }
	  */
	}
    }

  // we're awake now...
  reactiontime = 0;

  // get angry
  if ((!threshold || (flags2 & MF2_QUICKTORETALIATE)) &&
      source &&
      source != target)
    {
      if (source->team == team &&
	  (cv_infighting.value == 0 || source->flags2 & MF2_NOTARGET))
	return true;

      DActor *ds = source->Inherits<DActor>();
      if (ds)
	{
	  // let's not get angry, after all
	  if ((ds->type == MT_WIZARD && type == MT_SORCERER2) ||
	      (ds->type == MT_MINOTAUR && type == MT_MINOTAUR))
	    //|| (target->type == MT_BISHOP) || (target->type == MT_XMINOTAUR))
	    return true;

	  /*
		if((target->type == MT_CENTAUR && source->type == MT_CENTAURLEADER)
			|| (target->type == MT_CENTAURLEADER 
				&& source->type == MT_CENTAUR))
	  */
	}

      // if not intent on another player,
      // chase after this one
      target = source;
      threshold = BASETHRESHOLD;
      if (state == info->spawnstate && info->seestate)
	{
	  SetState(info->seestate);
	  pres->SetAnim(presentation_t::Run);
	}
    }

  return true;
}



bool PlayerPawn::Damage(Actor *inflictor, Actor *source, int damage, int dtype)
{
  if (dtype & dt_always)
    {
      // unavoidable damage
      // pain flash
      player->damagecount += damage;
      return Actor::Damage(inflictor, source, damage, dtype);
    }

  // TODO this is nastily duplicated in DActor::Damage, but...
  DActor *d;
  if (inflictor && (d = inflictor->Inherits<DActor>()))
    {
      switch (d->type)
	{
	case MT_MACEFX4: // Death ball
	  if (powers[pw_invulnerability])
	    // Can't hurt invulnerable players
	    damage = 0;
	    break;	  
	  if (P_AutoUseChaosDevice(this))
	    // Player was saved using chaos device
	    return false;	
	  damage = 10000; // Something's gonna die
	  break;
        case MT_PHOENIXFX2: // Flame thrower
	  if (P_Random() < 128)
            { // Freeze player for a bit
	      reactiontime += 4;
            }
	  break;

        case MT_EGGFX:
	  Morph(MT_CHICKEN);
	  return false; // Always return
        case MT_WHIRLWIND:
	  return P_TouchWhirlwind(this);

	  // Hexen
	case MT_TELOTHER_FX1:
	case MT_TELOTHER_FX2:
	case MT_TELOTHER_FX3:
	case MT_TELOTHER_FX4:
	case MT_TELOTHER_FX5:
	  P_TeleportOther(this);
	  return true;
	case MT_SHARDFX1:
	  damage <<= d->special2; // shard damage depends on spermcount
	  break;

	  // TODO Hexen damage effects TODO both PPawn and DActor
	  /*
	case MT_BISH_FX:
	  damage >>= 1; // Bishops are just too nasty TODO fix this elsewhere
	  break;
	case MT_ICEGUY_FX2:
	  damage >>= 1; // same here
	  break;
	case MT_CSTAFF_MISSILE:
	  // Cleric Serpent Staff does poison damage
	  P_PoisonPlayer(player, source, 20); // only for ppawns (dactors get full damage instead (all poison))
	  damage >>= 1;
	  break;
	case MT_POISONDART:
	  P_PoisonPlayer(player, source, 20);
	  damage >>= 1;
	  break;
	case MT_POISONCLOUD:
	  if (player->poisoncount < 4)
	    {
	      P_PoisonDamage(player, source, 15+(P_Random()&15), false); // Don't play painsound
	      P_PoisonPlayer(player, source, 50);
	      S_StartSound(target, SFX_PLAYER_POISONCOUGH);
	    }	
	  return;
	case MT_FSWORD_MISSILE:
	  if (player)
	    damage -= damage>>2;
	  break;
	  */

	default:
	  break;
	}
    }

  if (game.skill == sk_baby)
    damage >>= 1;   // take half damage in trainer mode

  int i, temp;
  // player specific
  if (!(flags & MF_CORPSE))
    {
      // end of game hellslime hack
      if (subsector->sector->special == SS_EndLevelHurt && damage >= health)
	damage = health - 1;

      // ignore damage in GOD mode, or with INVUL power.
      if ((cheats & CF_GODMODE) || powers[pw_invulnerability])
	return false;

      // doom armor
      temp = armorpoints[armor_field];
      if (temp > 0)
        {
	  int saved = int(damage * armorfactor[armor_field]);

	  if (temp <= saved)
            {
	      // armor is used up
	      saved = temp;
	      armorfactor[armor_field] = 0;
            }
	  armorpoints[armor_field] -= saved;
	  damage -= saved;
        }

      // hexen armor
      float save = toughness;
      for (i = armor_armor; i < NUMARMOR; i++)
	save += float(armorpoints[i])/100;
      if (save > 0)
	{
	  // armor absorbed some damage
	  if (save > 1)
	    save = 1;

	  // armor deteriorates
	  for (i = armor_armor; i < NUMARMOR; i++)
	    if (armorpoints[i])
	      {
		armorpoints[i] -= int(damage * ArmorIncrement[pclass][i] / (100 * armorfactor[i]));
		if (armorpoints[i] <= 2)
		  armorpoints[i] = 0;
	      }

	  int saved = int(damage * save);
	  if (damage > 200)
	    saved = int(200 * save);
	  damage -= saved;
	}      

      PlayerPawn *s = source ? source->Inherits<PlayerPawn>() : NULL;

      // added teamplay and teamdamage
      if (s && (s->player->team == player->team) && !cv_teamdamage.value && (s != this))
	return false;

      // autosavers
      if (damage >= health && (game.skill == sk_baby) && !morphTics)
	{ // Try to use some inventory health
	  P_AutoUseHealth(this, damage-health+1);
	}

      // pain flash
      player->damagecount += damage;

      pres->SetAnim(presentation_t::Pain);
    }

  return Actor::Damage(inflictor, source, damage, dtype);
}


bool Actor::FallingDamage(float v)
{
  v = fabs(v);

  if (!cv_fallingdamage.value || v <= cv_fallingdamage.value)
    return false; // no damage

  int damage = 10000; // automatic death

  if (v <= 35)
    damage = int(6 * (v - cv_fallingdamage.value));

  //damage = 10000;	// always kill 'em
  return Damage(NULL, NULL, damage, dt_crushing);
}


bool PlayerPawn::FallingDamage(float v)
{
  void P_NoiseAlert(Actor *target, Actor *emitter);

  v = fabs(v);

  if (!cv_fallingdamage.value || v <= cv_fallingdamage.value)
    return false; // no damage

  P_NoiseAlert(this, this); // monsters hear the thump

  if (v >= 63)
    {
      // automatic death
      return Damage(NULL, NULL, 10000, dt_crushing);
    }

  float dist = v * 16.0f / cv_fallingdamage.value;
  int damage = int((dist * dist)/10.0 -24);

  if (v < 39 && damage > health && health != 1)
    { // No-death threshold
      damage = health - 1;
    }

  return Damage(NULL, NULL, damage, dt_crushing);
}



//======================================================
//   Dying
//======================================================

// inflictor is the bullet, source is the one who pulled the trigger ;)
void Actor::Die(Actor *inflictor, Actor *source, int dtype)
{
  // switch physics to inanimate object mode
  eflags &= ~(MFE_INFLOAT | MFE_SKULLFLY | MFE_SWIMMING | MFE_FLY);

  // cream a corpse :)
  if (flags & MF_CORPSE)
    {
      flags &= ~(MF_SOLID | MF_SHOOTABLE);
      height = 0;
      radius = 0;
      return;
    }

  flags |= (MF_CORPSE | MF_DROPOFF); // it's dead now.
  flags &= ~MF_PICKUP;

  // dead target is no more shootable
  if (!cv_solidcorpse.value)
    flags &= ~MF_SHOOTABLE;

  // thing death actions
  if (special) // formerly also demanded MF_MONSTER (MT_ZBELL!)
    {
      mp->ExecuteLineSpecial(special, args, NULL, 0, this);
      special = 0;
    }

  // if a player killed a monster, update kills
  if (flags & MF_COUNTKILL)
    {
      PlayerPawn *s;
      if (source && (s = source->Inherits<PlayerPawn>()))
	{
	  // count for intermission
	  s->player->kills++;    
	}
      else if (!game.multiplayer)
	{
	  // TODO count all monster deaths, even those caused by other monsters?
	  game.Players.begin()->second->kills++;
	}

      //flags &= ~MF_COUNTKILL; // messes up some ACS scripts
    }
}


bool P_CheckSpecialDeath(DActor *m, int dtype);

void DActor::Die(Actor *inflictor, Actor *source, int dtype)
{
  bool gib_corpse = flags & MF_CORPSE;
  Actor::Die(inflictor, source, dtype);

  if (gib_corpse)
    {
      if (flags & MF_NOBLOOD)
	Remove();
      else
	{
	  SetState(S_GIBS);
	  pres->SetAnim(presentation_t::Death3);
	  S_StartSound(this, sfx_gib); // lets have a neat 'crunch' sound!
	}

      return;
    }

  if (type != MT_SKULL)
    flags &= ~MF_NOGRAVITY;

  //added:22-02-98: remember who exploded the barrel, so that the guy who
  //                shot the barrel which killed another guy, gets the frag!
  //                (source is passed from barrel to barrel also!)
  if ((type == MT_BARREL || type == MT_POD) && source)
    owner = source;

  if (!inflictor)
    inflictor = source;

  // special deaths based on damage type (burning, freezing...)
  if (dtype)
    {
      if (P_CheckSpecialDeath(this, dtype))
	return;
    }

  if (((game.mode < gm_heretic && health < -info->spawnhealth)
       ||(game.mode >= gm_heretic && health < -(info->spawnhealth>>1)))
      && info->xdeathstate)
    {
      SetState(info->xdeathstate);
      pres->SetAnim(presentation_t::Death2);
    }
  else
    {
      SetState(info->deathstate);
      pres->SetAnim(presentation_t::Death1);
    }
  // Normally, A_Fall or A_NoBlocking follow the deathstate and make the thing a nonsolid corpse

  tics -= P_Random()&3;

  if (tics < 1)
    tics = 1;
}



void PlayerPawn::Die(Actor *inflictor, Actor *source, int dtype)
{
  attacker = source;

  // start playing death anim. sequence
  if (health < -maxhealth >> 1)
    pres->SetAnim(presentation_t::Death2);
  else
    pres->SetAnim(presentation_t::Death1);

  // TODO the player death sounds!

  if (!source)
    {
      // environment kills
      int w = specialsector;
      const char *str = text[TXT_DEATHMSG_SPECUNKNOW];

      if (w & SS_DAMAGEMASK)
	{
	  sector_t *sec = subsector->sector;
	  if (sec->damage <= 5)
	    str = text[TXT_DEATHMSG_NUKE];
	  else if (sec->damage <= 10)
	    str = text[TXT_DEATHMSG_HELLSLIME];
	  else
	    str = text[TXT_DEATHMSG_SUPHELLSLIME];
	}

      player->SetMessage(va(str, player->name.c_str()));

      // count environment kills against you (you fragged yourself!)
      game.gtype->Frag(player, player);
    }
  else if (!(flags & MF_CORPSE))
    source->Killed(this, inflictor); // you can kill 'em only once!

  Actor::Die(inflictor, source, dtype);

  // dead guy attributes
  powers[pw_flight] = 0;
  powers[pw_weaponlevel2] = 0;
  DropWeapon();  // put weapon away

  // (since A_Fall is not called for PlayerPawns)
  // actor is on ground, it can be walked over
  if (!cv_solidcorpse.value)
    flags &= ~MF_SOLID;
  else
    {
      height >>= 2;
      radius -= (radius >> 4);
      health = maxhealth >> 1;
    }

  // So change this if corpse objects
  // are meant to be obstacles.

  // TODO Player flame and ice death, use DECORATE

  if ((dtype & dt_TYPEMASK) == dt_heat)
    {
      // Player flame death
      S_StartSound(this, info->burndeathsound);
      //pres->SetAnim(presentation_t::Burn);
      // SetState(S_PLAY_FDTH1);
      // SetState(S_PLAY_F_FDTH1);
      // SetState(S_PLAY_C_FDTH1);
      // SetState(S_PLAY_M_FDTH1);
      return;
    }
  else if ((dtype & dt_TYPEMASK) == dt_cold)
    {
      // Player ice death
      // TODO flags |= MF_ICECORPSE;
      // pres->SetAnim(presentation_t::Ice);
      // SetState(S_FPLAY_ICE);
      // SetState(S_CPLAY_ICE);
      // SetState(S_MPLAY_ICE);
      // SetState(S_PIG_ICE);
    }
}


/// set up a poisoning
void PlayerPawn::Poison(Actor *culprit, int poison)
{
  if ((cheats & CF_GODMODE) || powers[pw_invulnerability])
    return;

  poisoncount += poison;
  attacker = culprit;
  if (poisoncount > 100)
    poisoncount = 100;
}





/*
#ifdef thatsbuggycode
//
//  Tell each supported thing to check again its position,
//  because the 'base' thing has vanished or diminished,
//  the supported things might fall.
//
//added:28-02-98:
void P_CheckSupportThings (Actor* mobj)
{
  fixed_t   supportz = mobj->z + mobj->height;

  while ((mobj = mobj->supportthings))
    {
      // only for things above support thing
      if (mobj->z > supportz)
	mobj->eflags |= MF_CHECKPOS;
    }
}


//
//  If a thing moves and supportthings,
//  move the supported things along.
//
//added:28-02-98:
void P_MoveSupportThings (Actor* mobj, fixed_t xmove, fixed_t ymove, fixed_t zmove)
{
  fixed_t   supportz = mobj->z + mobj->height;
  Actor    *mo = mobj->supportthings;

  while (mo)
    {
      //added:28-02-98:debug
      if (mo==mobj)
        {
	  mobj->supportthings = NULL;
	  break;
        }
      
      // only for things above support thing
      if (mobj->z > supportz)
        {
	  mobj->eflags |= MF_CHECKPOS;
	  mobj->px += xmove;
	  mobj->py += ymove;
	  mobj->pz += zmove;
        }

      mo = mo->supportthings;
    }
}


//
//  Link a thing to it's 'base' (supporting) thing.
//  When the supporting thing will move or change size,
//  the supported will then be aware.
//
//added:28-02-98:
void P_LinkFloorThing(Actor*   mobj)
{
    Actor*     mo;
    Actor*     nmo;

    // no supporting thing
    if (!(mo = mobj->floorthing))
        return;

    // link mobj 'above' the lower mobjs, so that lower supporting
    // mobjs act upon this mobj
    while ( (nmo = mo->supportthings) &&
            (nmo->z<=mobj->z) )
    {
        // dont link multiple times
        if (nmo==mobj)
            return;

        mo = nmo;
    }
    mo->supportthings = mobj;
    mobj->supportthings = nmo;
}


//
//  Unlink a thing from it's support,
//  when it's 'floorthing' has changed,
//  before linking with the new 'floorthing'.
//
//added:28-02-98:
void P_UnlinkFloorThing(Actor*   mobj)
{
  Actor*     mo;

  if (!(mo = mobj->floorthing))      // just to be sure (may happen)
    return;

  while (mo->supportthings)
    {
      if (mo->supportthings == mobj)
        {
	  mo->supportthings = NULL;
	  break;
        }
      mo = mo->supportthings;
    }
}
#endif
*/
