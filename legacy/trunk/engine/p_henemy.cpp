// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_henemy.cpp 531 2008-11-02 09:03:56Z smite-meister $
//
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
/// \brief Heretic enemy AI.

#include "doomdef.h"

#include "g_actor.h"
#include "g_map.h"
#include "g_pawn.h"
#include "g_player.h"
#include "g_decorate.h"

#include "p_enemy.h"
#include "p_maputl.h"
#include "r_defs.h"
#include "sounds.h"
#include "m_random.h"
#include "m_fixed.h"
#include "tables.h"


void A_AddPlayerCorpse(DActor *actor) {}


void P_MinotaurSlam(Actor *source, Actor *target)
{
  angle_t angle = R_PointToAngle2(source->pos, target->pos);
  fixed_t thrust = Random(16, 20);
  target->vel.x += thrust * Cos(angle);
  target->vel.y += thrust * Sin(angle);
  target->Damage(NULL, NULL, HITDICE(6)); // FIXME Hexen minotaur HITDICE(4)

  //if(target->player)
  target->reactiontime = 14 + (P_Random()&7);
  source->args[0] = 0; // Stop charging
}


bool P_TouchWhirlwind(Actor *target)
{
  target->yaw += P_SignedRandom()<<20;
  target->vel.x += RandomS()*4;
  target->vel.y += RandomS()*4;
  if (target->mp->maptic & 16 && !(target->flags2 & MF2_BOSS))
    {
      fixed_t randVal = P_Random();
      if (randVal > 160)
	randVal = 160;

      target->vel.z += randVal >> 6;
      if (target->vel.z > 12)
	target->vel.z = 12;
    }

  if (!(target->mp->maptic & 7))
    return target->Damage(NULL, NULL, 3);

  return false;
}


//----------------------------------------------------------------------------
//
// PROC A_DripBlood
//
//----------------------------------------------------------------------------

void A_DripBlood(DActor *actor)
{
  DActor *mo;
  fixed_t r,s;

  r = P_SignedRandom() >> 5;
  s = P_SignedRandom() >> 5;
  mo = actor->mp->SpawnDActor(actor->pos.x + r, actor->pos.y + s, actor->pos.z, MT_BLOOD);
  mo->vel.x = RandomS()*4;
  mo->vel.y = RandomS()*4;
  mo->flags2 |= MF2_LOGRAV;
}

//----------------------------------------------------------------------------
//
// PROC A_KnightAttack
//
//----------------------------------------------------------------------------

void A_KnightAttack(DActor *actor)
{
  if (!actor->target)
    return;

  if (actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(3));
      S_StartSound(actor, sfx_kgtat2);
      return;
    }

  // Throw axe
  S_StartSound(actor, actor->info->attacksound);
  if (actor->type == MT_KNIGHTGHOST || P_Random() < 40)
    { // Red axe
      actor->SpawnMissile(actor->target, MT_REDAXE, 36);
      return;
    }
  // Green axe
  actor->SpawnMissile(actor->target, MT_KNIGHTAXE, 36);
}

//----------------------------------------------------------------------------
//
// PROC A_ImpExplode
//
//----------------------------------------------------------------------------

void A_ImpExplode(DActor *actor)
{
  DActor *mo;

  mo = actor->mp->SpawnDActor(actor->pos, MT_IMPCHUNK1);
  mo->vel.x = RandomS()*4;
  mo->vel.y = RandomS()*4;
  mo->vel.z = 9;
  mo = actor->mp->SpawnDActor(actor->pos, MT_IMPCHUNK2);
  mo->vel.x = RandomS()*4;
  mo->vel.y = RandomS()*4;
  mo->vel.z = 9;
  if (actor->special1 == 666)
    { // Extreme death crash
      actor->SetState(S_IMP_XCRASH1);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_BeastPuff
//
//----------------------------------------------------------------------------

void A_BeastPuff(DActor *actor)
{
  if(P_Random() > 64)
    {
      fixed_t r,s,t;
      r = RandomS()*4;
      s = RandomS()*4;
      t = RandomS()*4;
        
      actor->mp->SpawnDActor(actor->pos + vec_t<fixed_t>(r,s,t), MT_PUFFY);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_ImpMeAttack
//
//----------------------------------------------------------------------------

void A_ImpMeAttack(DActor *actor)
{
  if(!actor->target)
    {
      return;
    }
  S_StartSound(actor, actor->info->attacksound);
  if (actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, 5+(P_Random()&7));
    }
}

//----------------------------------------------------------------------------
//
// PROC A_ImpMsAttack
//
//----------------------------------------------------------------------------

void A_ImpMsAttack(DActor *actor)
{
  Actor *dest;
  angle_t an;

  if(!actor->target || P_Random() > 64)
    {
      actor->SetState(actor->info->seestate);
      return;
    }
  dest = actor->target;
  actor->eflags |= MFE_SKULLFLY;
  S_StartSound(actor, actor->info->attacksound);
  A_FaceTarget(actor);
  an = actor->yaw >> ANGLETOFINESHIFT;
  actor->vel.x = 12 * finecosine[an];
  actor->vel.y = 12 * finesine[an];
  int dist = (P_XYdist(dest->pos, actor->pos) / 12).floor();
  if(dist < 1)
    {
      dist = 1;
    }
  actor->vel.z = (dest->Center() - actor->pos.z) / dist;
}

//----------------------------------------------------------------------------
//
// PROC A_ImpMsAttack2
//
// Fireball attack of the imp leader.
//
//----------------------------------------------------------------------------

void A_ImpMsAttack2(DActor *actor)
{
  if(!actor->target)
    {
      return;
    }
  S_StartSound(actor, actor->info->attacksound);
  if(actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, 5+(P_Random()&7));
      return;
    }
  actor->SpawnMissile(actor->target, MT_IMPBALL);
}

//----------------------------------------------------------------------------
//
// PROC A_ImpDeath
//
//----------------------------------------------------------------------------

void A_ImpDeath(DActor *actor)
{
  actor->flags &= ~MF_SOLID;
  actor->flags2 |= MF2_FOOTCLIP;

  actor->flags  |= MF_CORPSE|MF_DROPOFF;
  actor->height >>= 2;
  actor->radius -= (actor->radius>>4);      //for solid corpses

  if(actor->pos.z <= actor->floorz)
    {
      actor->SetState(S_IMP_CRASH1);
      actor->flags &= ~MF_CORPSE;
    }
}

//----------------------------------------------------------------------------
//
// PROC A_ImpXDeath1
//
//----------------------------------------------------------------------------

void A_ImpXDeath1(DActor *actor)
{
  actor->flags &= ~MF_SOLID;
  actor->flags |= MF_NOGRAVITY;
  actor->flags  |= MF_CORPSE|MF_DROPOFF;
  actor->height >>= 2;
  actor->radius -= (actor->radius>>4);      //for solid corpses

  actor->flags2 |= MF2_FOOTCLIP;
  actor->special1 = 666; // Flag the crash routine
}

//----------------------------------------------------------------------------
//
// PROC A_ImpXDeath2
//
//----------------------------------------------------------------------------

void A_ImpXDeath2(DActor *actor)
{
  actor->flags &= ~MF_NOGRAVITY;
  if(actor->pos.z <= actor->floorz)
    {
      actor->SetState(S_IMP_CRASH1);
      actor->flags &= ~MF_CORPSE;
    }
}

//----------------------------------------------------------------------------
//
// PROC A_ChicAttack
//
//----------------------------------------------------------------------------

void A_ChicAttack(DActor *actor)
{
  if (actor->UpdateMorph(18))
    return;

  if(!actor->target)
      return;

  if(actor->CheckMeleeRange())
    actor->target->Damage(actor, actor, 1+(P_Random()&1));
}

//----------------------------------------------------------------------------
//
// PROC A_ChicLook
//
//----------------------------------------------------------------------------

void A_ChicLook(DActor *actor)
{
  if (actor->UpdateMorph(10))
    {
      return;
    }
  A_Look(actor);
}

//----------------------------------------------------------------------------
//
// PROC A_ChicChase
//
//----------------------------------------------------------------------------

void A_ChicChase(DActor *actor)
{
  if (actor->UpdateMorph(3))
    {
      return;
    }
  A_Chase(actor);
}

//----------------------------------------------------------------------------
//
// PROC A_ChicPain
//
//----------------------------------------------------------------------------

void A_ChicPain(DActor *actor)
{
  if(actor->UpdateMorph(10))
    {
      return;
    }
  S_StartSound(actor, actor->info->painsound);
}

//----------------------------------------------------------------------------
//
// PROC A_Feathers
//
//----------------------------------------------------------------------------

void A_Feathers(DActor *actor)
{
  int count;
  DActor *mo;

  if(actor->health > 0)
    { // Pain
      count = P_Random() < 32 ? 2 : 1;
    }
  else
    { // Death
      count = 5+(P_Random()&3);
    }
  for (int i = 0; i < count; i++)
    {
      mo = actor->mp->SpawnDActor(actor->pos.x, actor->pos.y, actor->pos.z+20, MT_FEATHER);
      mo->target = actor;
      mo->vel.x = RandomS();
      mo->vel.y = RandomS();
      mo->vel.z = Random(1,3);
      mo->SetState(statenum_t(S_FEATHER1+(P_Random()&7)));
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MummyAttack
//
//----------------------------------------------------------------------------

void A_MummyAttack(DActor *actor)
{
  if(!actor->target)
    {
      return;
    }
  S_StartSound(actor, actor->info->attacksound);
  if(actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(2));
      S_StartSound(actor, sfx_mumat2);
      return;
    }
  S_StartSound(actor, sfx_mumat1);
}

//----------------------------------------------------------------------------
//
// PROC A_MummyAttack2
//
// Mummy leader missile attack.
//
//----------------------------------------------------------------------------

void A_MummyAttack2(DActor *actor)
{
  if (!actor->target)
    return;

  //S_StartSound(actor, actor->info->attacksound);
  if (actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(2));
      return;
    }
  DActor *mo = actor->SpawnMissile(actor->target, MT_MUMMYFX1);
  //mo = actor->SpawnMissile(actor->target, MT_EGGFX);
  if (mo != NULL)
    mo->target = actor->target;
}

//----------------------------------------------------------------------------
//
// PROC A_MummyFX1Seek
//
//----------------------------------------------------------------------------

void A_MummyFX1Seek(DActor *actor)
{
  actor->SeekerMissile(ANGLE_1*10, ANGLE_1*20);
}

//----------------------------------------------------------------------------
//
// PROC A_MummySoul
//
//----------------------------------------------------------------------------

void A_MummySoul(DActor *mummy)
{
  DActor *mo;

  mo = mummy->mp->SpawnDActor(mummy->pos.x, mummy->pos.y, mummy->pos.z+10, MT_MUMMYSOUL);
  mo->vel.z = 1;
}

//----------------------------------------------------------------------------
//
// PROC A_Sor1Pain
//
//----------------------------------------------------------------------------

void A_Sor1Pain(DActor *actor)
{
  actor->special1 = 20; // Number of steps to walk fast
  A_Pain(actor);
}

//----------------------------------------------------------------------------
//
// PROC A_Sor1Chase
//
//----------------------------------------------------------------------------

void A_Sor1Chase(DActor *actor)
{
  if(actor->special1)
    {
      actor->special1--;
      actor->tics -= 3;
    }
  A_Chase(actor);
}

//----------------------------------------------------------------------------
//
// PROC A_Srcr1Attack
//
// Sorcerer demon attack.
//
//----------------------------------------------------------------------------

void A_Srcr1Attack(DActor *actor)
{
  if (!actor->target)
    return;

  S_StartSound(actor, actor->info->attacksound);
  if(actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(8));
      return;
    }
  if(actor->health > (actor->info->spawnhealth/3)*2)
    { // Spit one fireball
      actor->SpawnMissile(actor->target, MT_SRCRFX1, 48);
    }
  else
    { // Spit three fireballs
      DActor *mo = actor->SpawnMissile(actor->target, MT_SRCRFX1, 48);
      if(mo)
	{
	  fixed_t pz = mo->vel.z;
	  angle_t angle = mo->yaw;
	  actor->SpawnMissileAngle(MT_SRCRFX1, angle-ANGLE_1*3, 48, pz);
	  actor->SpawnMissileAngle(MT_SRCRFX1, angle+ANGLE_1*3, 48, pz);
	}
      if(actor->health < actor->info->spawnhealth/3)
	{ // Maybe attack again
	  if(actor->special1)
	    { // Just attacked, so don't attack again
	      actor->special1 = 0;
	    }
	  else
	    { // Set state to attack again
	      actor->special1 = 1;
	      actor->SetState(S_SRCR1_ATK4);
	    }
	}
    }
}

//----------------------------------------------------------------------------
//
// PROC A_SorcererRise
//
//----------------------------------------------------------------------------

void A_SorcererRise(DActor *actor)
{
  DActor *mo;

  actor->flags &= ~MF_SOLID;
  mo = actor->mp->SpawnDActor(actor->pos, MT_SORCERER2);
  mo->SetState(S_SOR2_RISE1);
  mo->yaw = actor->yaw;
  mo->target = actor->target;
}



void DActor::DSparilTeleport()
{
  int n = mp->BossSpots.size(); 
  if (!n)
    return; // No spots, no teleport.

  vec_t<fixed_t> p;

  int i = P_Random();
  int count = 0; // avoid possible infinite loop
  do {
    count++;
    i = (i+1) % n;
    p.x = mp->BossSpots[i]->x;
    p.y = mp->BossSpots[i]->y;
  } while (P_XYdist(pos, p) < 128 && count < n);
  p.z = mp->GetSubsector(p.x, p.y)->sector->floorheight + mp->BossSpots[i]->height;

  vec_t<fixed_t> prev_pos(pos);

  if (TeleportMove(p))
    {
      DActor *mo = mp->SpawnDActor(prev_pos, MT_SOR2TELEFADE);
      S_StartSound(mo, sfx_teleport);
      SetState(S_SOR2_TELE1);
      S_StartSound(this, sfx_teleport);
      pos.z = floorz;
      yaw = ANG45 * (mp->BossSpots[i%n]->angle/45);
      vel.Set(0,0,0);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_Srcr2Decide
//
//----------------------------------------------------------------------------

void A_Srcr2Decide(DActor *actor)
{
  static int chance[] =
  {
    192, 120, 120, 120, 64, 64, 32, 16, 0
  };

  if (actor->mp->BossSpots.size() == 0)
    { // No spots
      return;
    }
  if(P_Random() < chance[actor->health/(actor->info->spawnhealth/8)])
    {
      actor->DSparilTeleport();
    }
}

//----------------------------------------------------------------------------
//
// PROC A_Srcr2Attack
//
//----------------------------------------------------------------------------

void A_Srcr2Attack(DActor *actor)
{
  int chance;

  if(!actor->target)
    {
      return;
    }
  S_StartAmbSound(NULL, actor->info->attacksound);
  if(actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(20));
      return;
    }
  chance = actor->health < actor->info->spawnhealth/2 ? 96 : 48;
  if(P_Random() < chance)
    { // Wizard spawners
      actor->SpawnMissileAngle(MT_SOR2FX2, actor->yaw-ANG45, 32, 0.5f);
      actor->SpawnMissileAngle(MT_SOR2FX2, actor->yaw+ANG45, 32, 0.5f);
    }
  else
    { // Blue bolt
      actor->SpawnMissile(actor->target, MT_SOR2FX1);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_BlueSpark
//
//----------------------------------------------------------------------------

void A_BlueSpark(DActor *actor)
{
  int i;
  DActor *mo;
    
  for(i = 0; i < 2; i++)
    {
      mo = actor->mp->SpawnDActor(actor->pos, MT_SOR2FXSPARK);
      mo->vel.x = RandomS()*2;
      mo->vel.y = RandomS()*2;
      mo->vel.z = Random(1, 2);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_GenWizard
//
//----------------------------------------------------------------------------

void A_GenWizard(DActor *actor)
{
  DActor *mo = actor->mp->SpawnDActor(actor->pos.x, actor->pos.y,
				      actor->pos.z - mobjinfo[MT_WIZARD].height/2, MT_WIZARD);
  if (!mo->TestLocation())
    { // Didn't fit
      mo->Remove();
      return;
    }
  actor->vel.Set(0,0,0);
  actor->SetState(mobjinfo[actor->type].deathstate);
  actor->flags &= ~MF_MISSILE;
  DActor *fog = actor->mp->SpawnDActor(actor->pos, MT_TFOG);
  S_StartSound(fog, sfx_teleport);
}

//----------------------------------------------------------------------------
//
// PROC A_Sor2DthInit
//
//----------------------------------------------------------------------------

void A_Sor2DthInit(DActor *actor)
{
  actor->special1 = 7; // Animation loop counter
  actor->mp->Massacre(); // Kill monsters early
}

//----------------------------------------------------------------------------
//
// PROC A_Sor2DthLoop
//
//----------------------------------------------------------------------------

void A_Sor2DthLoop(DActor *actor)
{
  if(--actor->special1)
    // Need to loop
    actor->SetState(S_SOR2_DIE4);
}

//----------------------------------------------------------------------------
//
// D'Sparil Sound Routines
//
//----------------------------------------------------------------------------

void A_SorZap(DActor *actor) {S_StartAmbSound(NULL, sfx_sorzap);}
void A_SorRise(DActor *actor) {S_StartAmbSound(NULL, sfx_sorrise);}
void A_SorDSph(DActor *actor) {S_StartAmbSound(NULL, sfx_sordsph);}
void A_SorDExp(DActor *actor) {S_StartAmbSound(NULL, sfx_sordexp);}
void A_SorDBon(DActor *actor) {S_StartAmbSound(NULL, sfx_sordbon);}
void A_SorSightSnd(DActor *actor) {S_StartAmbSound(NULL, sfx_sorsit);}

//----------------------------------------------------------------------------
//
// PROC A_MinotaurAtk1
//
// Melee attack.
//
//----------------------------------------------------------------------------

void A_MinotaurAtk1(DActor *actor)
{
  Actor *t = actor->target;

  if (t == NULL)
    return;
    
  S_StartSound(actor, sfx_stfpow);
  if (actor->CheckMeleeRange())
    {
      t->Damage(actor, actor, HITDICE(4));

      PlayerPawn *p = t->Inherits<PlayerPawn>();
      if (p)
	{ // Squish the player
	  p->player->deltaviewheight = -16;
	}
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MinotaurDecide
//
// Choose a missile attack.
//
//----------------------------------------------------------------------------

#define MNTR_CHARGE_SPEED (13)

void A_MinotaurDecide(DActor *actor)
{
  angle_t angle;
  Actor *target = actor->target;
  if (!target)
      return;

  S_StartSound(actor, sfx_minsit);
  fixed_t dist = P_XYdist(actor->pos, target->pos);
  if (target->Top() > actor->Feet() && target->Top() < actor->Top()
      && dist < 8*64 && dist > 1*64
      && P_Random() < 150)
    { // Charge attack
      // Don't call the state function right away
      actor->SetState(S_MNTR_ATK4_1, false);
      actor->eflags |= MFE_SKULLFLY;
      A_FaceTarget(actor);
      angle = actor->yaw>>ANGLETOFINESHIFT;
      actor->vel.x = MNTR_CHARGE_SPEED * finecosine[angle];
      actor->vel.y = MNTR_CHARGE_SPEED * finesine[angle];
      actor->special1 = TICRATE/2; // Charge duration
    }
  else if(target->pos.z == target->floorz
	  && dist < 9*64
	  && P_Random() < 220)
    { // Floor fire attack
      actor->SetState(S_MNTR_ATK3_1);
      actor->special2 = 0;
    }
  else
    { // Swing attack
      A_FaceTarget(actor);
      // Don't need to call SetState because the current state
      // falls through to the swing attack
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MinotaurCharge
//
//----------------------------------------------------------------------------

void A_MinotaurCharge(DActor *actor)
{
  DActor *puff;

  if(actor->special1)
    {
      puff = actor->mp->SpawnDActor(actor->pos, MT_PHOENIXPUFF);
      puff->vel.z = 2;
      actor->special1--;
    }
  else
    {
      actor->eflags &= ~MFE_SKULLFLY;
      actor->SetState(actor->info->seestate);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MinotaurAtk2
//
// Swing attack.
//
//----------------------------------------------------------------------------

void A_MinotaurAtk2(DActor *actor)
{
  if (!actor->target)
    return;

  S_StartSound(actor, sfx_minat2);
  if (actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(5));
      return;
    }

  DActor *mo = actor->SpawnMissile(actor->target, MT_MNTRFX1, 40);
  if (mo)
    {
      S_StartSound(mo, sfx_minat2);
      fixed_t pz = mo->vel.z;
      angle_t angle = mo->yaw;
      actor->SpawnMissileAngle(MT_MNTRFX1, angle-(ANG45/8), 40, pz);
      actor->SpawnMissileAngle(MT_MNTRFX1, angle+(ANG45/8), 40, pz);
      actor->SpawnMissileAngle(MT_MNTRFX1, angle-(ANG45/16), 40, pz);
      actor->SpawnMissileAngle(MT_MNTRFX1, angle+(ANG45/16), 40, pz);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MinotaurAtk3
//
// Floor fire attack.
//
//----------------------------------------------------------------------------

void A_MinotaurAtk3(DActor *actor)
{
  DActor *mo;
  Actor *t = actor->target;

  if (t == NULL)
    return;

  if (actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(5));
      
      PlayerPawn *p = t->Inherits<PlayerPawn>();
      if (p)
	{ // Squish the player
	  ((PlayerPawn *)t)->player->deltaviewheight = -16;
	}
    }
  else
    {
      mo = actor->SpawnMissile(actor->target, MT_MNTRFX2);
      if (mo != NULL)
	{
	  S_StartSound(mo, sfx_minat1);
	}
    }
  if (P_Random() < 192 && actor->special2 == 0)
    {
      actor->SetState(S_MNTR_ATK3_4);
      actor->special2 = 1;
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MntrFloorFire
//
//----------------------------------------------------------------------------

void A_MntrFloorFire(DActor *actor)
{
  actor->pos.z = actor->floorz;
  DActor *mo = actor->mp->SpawnDActor(actor->pos.x + RandomS()*4, actor->pos.y + RandomS()*4, ONFLOORZ, MT_MNTRFX3);
  mo->target = actor->target;
  mo->vel.x = 1; // Force block checking
  mo->CheckMissileSpawn();
}

//----------------------------------------------------------------------------
//
// PROC A_BeastAttack
//
//----------------------------------------------------------------------------

void A_BeastAttack(DActor *actor)
{
  if(!actor->target)
    {
      return;
    }
  S_StartSound(actor, actor->info->attacksound);
  if(actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(3));
      return;
    }
  actor->SpawnMissile(actor->target, MT_BEASTBALL);
}

//----------------------------------------------------------------------------
//
// PROC A_HeadAttack
//
//----------------------------------------------------------------------------

void A_HHeadAttack(DActor *actor)
{
  int i;
  static int atkResolve1[] = { 50, 150 };
  static int atkResolve2[] = { 150, 200 };

  // Ice ball             (close 20% : far 60%)
  // Fire column  (close 40% : far 20%)
  // Whirlwind    (close 40% : far 20%)
  // Distance threshold = 8 cells

  Actor *t = actor->target;
  if (t == NULL)
    return;

  A_FaceTarget(actor);
  if(actor->CheckMeleeRange())
    {
      t->Damage(actor, actor, HITDICE(6));
      return;
    }

  int dist = P_XYdist(actor->pos, t->pos) > 8*64;
  int randAttack = P_Random();
  if (randAttack < atkResolve1[dist])
    { // Ice ball
      actor->SpawnMissile(t, MT_HEADFX1);
      S_StartSound(actor, sfx_hedat2);
    }
  else if(randAttack < atkResolve2[dist])
    { // Fire column
      DActor *baseFire = actor->SpawnMissile(t, MT_HEADFX3);
      if (baseFire != NULL)
	{
	  baseFire->SetState(S_HHEADFX3_4); // Don't grow
	  for (i = 0; i < 5; i++)
	    {
	      DActor *fire = actor->mp->SpawnDActor(baseFire->pos, MT_HEADFX3);
	      if (i == 0)
		S_StartSound(actor, sfx_hedat1);

	      fire->owner = baseFire->owner;
	      fire->yaw = baseFire->yaw;
	      fire->vel = baseFire->vel;
                                //fire->damage = 0;
	      fire->health = (i+1)*2;
	      fire->CheckMissileSpawn();
	    }
	}
    }
  else
    { // Whirlwind
      DActor *mo = actor->SpawnMissile(t, MT_WHIRLWIND);
      if(mo != NULL)
	{
	  mo->pos.z -= 32;
	  mo->target = t;
	  mo->special2 = 50; // Timer for active sound
	  mo->health = 20*TICRATE; // Duration
	  S_StartSound(actor, sfx_hedat3);
	}
    }
}

//----------------------------------------------------------------------------
//
// PROC A_WhirlwindSeek
//
//----------------------------------------------------------------------------

void A_WhirlwindSeek(DActor *actor)
{
  actor->health -= 3;
  if(actor->health < 0)
    {
      actor->vel.Set(0,0,0);
      actor->SetState(mobjinfo[actor->type].deathstate);
      actor->flags &= ~MF_MISSILE;
      return;
    }
  if((actor->special2 -= 3) < 0)
    {
      actor->special2 = 58+(P_Random()&31);
      S_StartSound(actor, sfx_hedat3);
    }
  if (actor->target && actor->target->flags & MF_SHADOW)
    return;

  actor->SeekerMissile(ANGLE_1*10, ANGLE_1*30);
}

//----------------------------------------------------------------------------
//
// PROC A_HeadIceImpact
//
//----------------------------------------------------------------------------

void A_HeadIceImpact(DActor *ice)
{
  int i;
  angle_t angle;
  DActor *shard;

  for(i = 0; i < 8; i++)
    {
      shard = ice->mp->SpawnDActor(ice->pos, MT_HEADFX2);
      angle = i*ANG45;
      shard->owner = ice->owner;
      shard->yaw = angle;
      angle >>= ANGLETOFINESHIFT;
      shard->vel.x = shard->info->speed * finecosine[angle];
      shard->vel.y = shard->info->speed * finesine[angle];
      shard->vel.z = -0.6f;
      shard->CheckMissileSpawn();
    }
}

//----------------------------------------------------------------------------
//
// PROC A_HeadFireGrow
//
//----------------------------------------------------------------------------

void A_HeadFireGrow(DActor *fire)
{
  fire->health--;
  fire->pos.z += 9;
  if(fire->health == 0)
    {
      //fire->damage = fire->info->damage;
      fire->SetState(S_HHEADFX3_4);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_SnakeAttack
//
//----------------------------------------------------------------------------

void A_SnakeAttack(DActor *actor)
{
  if(!actor->target)
    {
      actor->SetState(S_SNAKE_WALK1);
      return;
    }
  S_StartSound(actor, actor->info->attacksound);
  A_FaceTarget(actor);
  actor->SpawnMissile(actor->target, MT_SNAKEPRO_A);
}

//----------------------------------------------------------------------------
//
// PROC A_SnakeAttack2
//
//----------------------------------------------------------------------------

void A_SnakeAttack2(DActor *actor)
{
  if(!actor->target)
    {
      actor->SetState(S_SNAKE_WALK1);
      return;
    }
  S_StartSound(actor, actor->info->attacksound);
  A_FaceTarget(actor);
  actor->SpawnMissile(actor->target, MT_SNAKEPRO_B);
}

//----------------------------------------------------------------------------
//
// PROC A_ClinkAttack
//
//----------------------------------------------------------------------------

void A_ClinkAttack(DActor *actor)
{
  int damage;

  if(!actor->target)
    {
      return;
    }
  S_StartSound(actor, actor->info->attacksound);
  if(actor->CheckMeleeRange())
    {
      damage = ((P_Random()%7)+3);
      actor->target->Damage(actor, actor, damage);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_GhostOff
//
//----------------------------------------------------------------------------

void A_GhostOff(DActor *actor)
{
  actor->flags &= ~MF_SHADOW;
}

//----------------------------------------------------------------------------
//
// PROC A_WizAtk1
//
//----------------------------------------------------------------------------

void A_WizAtk1(DActor *actor)
{
  A_FaceTarget(actor);
  actor->flags &= ~MF_SHADOW;
}

//----------------------------------------------------------------------------
//
// PROC A_WizAtk2
//
//----------------------------------------------------------------------------

void A_WizAtk2(DActor *actor)
{
  A_FaceTarget(actor);
  actor->flags |= MF_SHADOW;
}

//----------------------------------------------------------------------------
//
// PROC A_WizAtk3
//
//----------------------------------------------------------------------------

void A_WizAtk3(DActor *actor)
{
  actor->flags &= ~MF_SHADOW;
  if (!actor->target)
    return;

  S_StartSound(actor, actor->info->attacksound);
  if (actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(4));
      return;
    }
  DActor *mo = actor->SpawnMissile(actor->target, MT_WIZFX1);
  if (mo)
    {
      fixed_t pz = mo->vel.z;
      angle_t angle = mo->yaw;
      actor->SpawnMissileAngle(MT_WIZFX1, angle-(ANG45/8), 32, pz);
      actor->SpawnMissileAngle(MT_WIZFX1, angle+(ANG45/8), 32, pz);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_HScream
//
//----------------------------------------------------------------------------

void A_HScream(DActor *actor)
{
  switch(actor->type)
    {
    case MT_CHICPLAYER:
    case MT_SORCERER1:
    case MT_MINOTAUR:
      // Make boss death sounds full volume
      S_StartAmbSound(NULL, actor->info->deathsound);
      break;
    case MT_HPLAYER:
      // Handle the different player death screams
      if(actor->special1 < 10)
	{ // Wimpy death sound
	  S_StartSound(actor, sfx_plrwdth);
	}
      else if(actor->health > -50)
	{ // Normal death sound
	  S_StartSound(actor, actor->info->deathsound);
	}
      else if(actor->health > -100)
	{ // Crazy death sound
	  S_StartSound(actor, sfx_plrcdth);
	}
      else
	{ // Extreme death sound
	  S_StartSound(actor, sfx_gibdth);
	}
      break;
    default:
      S_StartSound(actor, actor->info->deathsound);
      break;
    }
}


//----------------------------------------------------------------------------
//
// PROC A_PodPain
//
//----------------------------------------------------------------------------

void A_PodPain(DActor *actor)
{
  int i;
  int count;
  int chance;
  DActor *goo;
    
  chance = P_Random();
  if(chance < 128)
    {
      return;
    }
  count = chance > 240 ? 2 : 1;
  for(i = 0; i < count; i++)
    {
      goo = actor->mp->SpawnDActor(actor->pos.x, actor->pos.y,
			actor->pos.z+48, MT_PODGOO);
      goo->target = actor;
      goo->vel.x = RandomS()*2;
      goo->vel.y = RandomS()*2;
      goo->vel.z = Random(0.5, 2.5);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_RemovePod
//
//----------------------------------------------------------------------------

void A_RemovePod(DActor *actor)
{
  if (actor->owner)
    {
      DActor *mo = (DActor *)actor->owner;
      if (mo->special1 > 0)
	mo->special1--;
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MakePod
//
//----------------------------------------------------------------------------

#define MAX_GEN_PODS 16

void A_MakePod(DActor *actor)
{
  if (actor->special1 == MAX_GEN_PODS)
    // Too many generated pods
    return;

  fixed_t x = actor->pos.x;
  fixed_t y = actor->pos.y;

  DActor *mo = actor->mp->SpawnDActor(x, y, ONFLOORZ, MT_POD);
  if (!mo->TestLocation())
    { // Didn't fit
      mo->Remove();
      return;
    }
  mo->SetState(S_POD_GROW1);
  mo->Thrust(P_Random() << 24, 4.5f);
  S_StartSound(mo, sfx_newpod);
  actor->special1++; // Increment generated pod count
  mo->owner = actor; // Link the pod to the generator
  return;
}


//----------------------------------------------------------------------------
//
// PROC A_BossDeath
//
// Trigger special effects if all bosses are dead.
//
//----------------------------------------------------------------------------

void A_HBossDeath(DActor *actor)
{
  actor->mp->BossDeath(actor);
}

//----------------------------------------------------------------------------
//
// PROC A_SpawnTeleGlitter
//
//----------------------------------------------------------------------------

void A_SpawnTeleGlitter(DActor *actor)
{
  DActor *mo;
  int r,s;

  r = P_Random();
  s = P_Random();
  mo = actor->mp->SpawnDActor(actor->pos.x+((r&31)-16),
		   actor->pos.y+((s&31)-16),
		   actor->subsector->sector->floorheight, MT_TELEGLITTER);
  mo->vel.z = 0.25f;
}

//----------------------------------------------------------------------------
//
// PROC A_SpawnTeleGlitter2
//
//----------------------------------------------------------------------------

void A_SpawnTeleGlitter2(DActor *actor)
{
  DActor *mo;
  int r,s;

  r = P_Random();
  s = P_Random();
  mo = actor->mp->SpawnDActor(actor->pos.x+((r&31)-16),
		   actor->pos.y+((s&31)-16),
		   actor->subsector->sector->floorheight, MT_TELEGLITTER2);
  mo->vel.z = 0.25f;
}

//----------------------------------------------------------------------------
//
// PROC A_AccTeleGlitter
//
//----------------------------------------------------------------------------

void A_AccTeleGlitter(DActor *actor)
{
  if(++actor->health > 35)
    {
      actor->vel.z += actor->vel.z/2;
    }
}

//----------------------------------------------------------------------------
//
// PROC A_InitKeyGizmo
//
//----------------------------------------------------------------------------

void A_InitKeyGizmo(DActor *gizmo)
{
  DActor *mo;
  statenum_t state;

  switch(gizmo->type)
    {
    case MT_KEYGIZMOBLUE:
      state = S_KGZ_BLUEFLOAT1;
      break;
    case MT_KEYGIZMOGREEN:
      state = S_KGZ_GREENFLOAT1;
      break;
    case MT_KEYGIZMOYELLOW:
      state = S_KGZ_YELLOWFLOAT1;
      break;
    default:
      state = S_NULL;
      break;
    }
  mo = gizmo->mp->SpawnDActor(gizmo->pos.x, gizmo->pos.y, gizmo->pos.z+60,
			     MT_KEYGIZMOFLOAT);
  mo->SetState(state);
}

//----------------------------------------------------------------------------
//
// PROC A_VolcanoSet
//
//----------------------------------------------------------------------------

void A_VolcanoSet(DActor *volcano)
{
  volcano->tics = 105+(P_Random()&127);
}

//----------------------------------------------------------------------------
//
// PROC A_VolcanoBlast
//
//----------------------------------------------------------------------------

void A_VolcanoBlast(DActor *volcano)
{
  int count = 1+(P_Random()%3);
  for(int i = 0; i < count; i++)
    {
      DActor *blast = volcano->mp->SpawnDActor(volcano->pos.x, volcano->pos.y, volcano->pos.z+44, MT_VOLCANOBLAST); // MT_VOLCANOBLAST
      blast->owner = volcano;
      angle_t angle = P_Random()<<24;
      blast->yaw = angle;
      blast->vel.x = Cos(angle);
      blast->vel.y = Sin(angle);
      blast->vel.z = Random(2.5, 6.5);
      S_StartSound(blast, sfx_volsht);
      blast->CheckMissileSpawn();
    }
}

//----------------------------------------------------------------------------
//
// PROC A_VolcBallImpact
//
//----------------------------------------------------------------------------

void A_VolcBallImpact(DActor *ball)
{
  if (ball->pos.z <= ball->floorz)
    {
      ball->flags |= MF_NOGRAVITY;
      ball->flags2 &= ~MF2_LOGRAV;
      ball->pos.z += 28;
      //ball->vel.z = 3;
    }
  ball->RadiusAttack(ball->owner, 25);
  for (int i = 0; i < 4; i++)
    {
      DActor *tiny = ball->mp->SpawnDActor(ball->pos, MT_VOLCANOTBLAST);
      tiny->owner = ball;
      angle_t angle = i*ANG90;
      tiny->yaw = angle;
      tiny->vel.x = 0.7f * Cos(angle);
      tiny->vel.y = 0.7f * Sin(angle);
      tiny->vel.z = Random(1, 3);
      tiny->CheckMissileSpawn();
    }
}

//----------------------------------------------------------------------------
//
// A_SkullPop
//
//----------------------------------------------------------------------------

void A_SkullPop(DActor *actor)
{
  PlayerPawn *p = actor->Inherits<PlayerPawn>();
  if (!p)
    return;

  p->flags &= ~MF_SOLID;
  DActor *mo = p->mp->SpawnDActor(p->pos.x, p->pos.y, p->pos.z+48, MT_BLOODYSKULL);

  mo->owner = p;
  mo->vel.x = RandomS()*2;
  mo->vel.y = RandomS()*2;
  mo->vel.z = Random(2, 2.25);

  // Attach player mobj to bloody skull
  // Nope.
  /*
  player_t *player = p->player;
  p->player = NULL;
  p->special1 = player->class;
  mo->player = player;
  mo->health = p->health;
  mo->yaw = p->yaw;
  player->mo = mo;
  player->aiming = 0;
  player->damagecount = 32;
  */
}

//----------------------------------------------------------------------------
//
// PROC A_CheckSkullFloor
//
//----------------------------------------------------------------------------

void A_CheckSkullFloor(DActor *actor)
{
  if(actor->pos.z <= actor->floorz)
    {
      actor->SetState(S_BLOODYSKULLX1);
      //S_StartSound(actor, SFX_DRIP);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_CheckSkullDone
//
//----------------------------------------------------------------------------

void A_CheckSkullDone(DActor *actor)
{
  if(actor->special2 == 666)
    {
      actor->SetState(S_BLOODYSKULLX2);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_CheckBurnGone
//
//----------------------------------------------------------------------------

void A_CheckBurnGone(DActor *actor)
{
  if(actor->special2 == 666)
    {
      actor->SetState(S_PLAY_FDTH20);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_FreeTargMobj
//
//----------------------------------------------------------------------------

void A_FreeTargMobj(DActor *mo)
{
  // FIXME TODO since we now have a pointer tracking system for Actors, maybe we should delete the DActor here.

  mo->vel.Set(0,0,0);
  mo->pos.z = mo->ceilingz+4;
  mo->flags &= ~(MF_SHOOTABLE|MF_FLOAT|MF_SOLID|MF_COUNTKILL);
  mo->flags |= MF_CORPSE|MF_DROPOFF|MF_NOGRAVITY;
  mo->flags2 &= ~(MF2_LOGRAV);
  mo->flags2 |= MF2_DONTDRAW;
  mo->eflags &= ~MFE_SKULLFLY;
  //mo->health = -1000;
}


//----------------------------------------------------------------------------
//
// PROC A_FlameSnd
//
//----------------------------------------------------------------------------

void A_FlameSnd(DActor *actor)
{
  S_StartSound(actor, sfx_hedat1); // Burn sound
}
