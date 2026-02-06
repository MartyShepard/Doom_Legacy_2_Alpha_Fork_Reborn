// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: g_pawn.cpp 526 2008-01-22 23:28:24Z smite-meister $
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
/// \brief Pawn and PlayerPawn class implementations


#include "g_pawn.h"
#include "g_player.h"
#include "g_game.h"
#include "g_map.h"
#include "g_decorate.h"

#include "command.h"
#include "cvars.h"

#include "d_ticcmd.h"

#include "p_spec.h"
#include "p_enemy.h"

#include "dstrings.h"

#include "hud.h"
#include "sounds.h"
#include "tables.h"

#include "r_sprite.h"
#include "hardware/md3.h"

#include "m_random.h"

#define ANG5    (ANG90/18)

#define BLINKTHRESHOLD  (4*32) // for powers
#define INVERSECOLORMAP  32    // special effects (INVUL inverse) colormap.


int ArmorIncrement[NUMCLASSES][NUMARMOR] =
{
  { 0, 0, 0, 0, 0 },
  { 0, 25, 20, 15, 5 },
  { 0, 10, 25, 5, 20 },
  { 0, 5, 15, 10, 25 },
  { 0, 0, 0, 0, 0 }
};

int MaxArmor[NUMCLASSES] = { 200, 100, 90, 80, 5 };

vector<PawnAI*> pawn_aid;


PawnAI::PawnAI(const ActorInfo& base, pclass_t cls, weapontype_t bw)
  : ActorInfo(base)
{
  pclass = cls;
  bweapon = bw;
  bammo = 50;

  gruntsound     = sfx_grunt;
  puzzfailsound  = sfx_grunt;
  burndeathsound = sfx_hedat1;
  /*
    // TODO, proper sounds
    SFX_PLAYER_FIGHTER_GRUNT
    SFX_PLAYER_CLERIC_GRUNT
    SFX_PLAYER_MAGE_GRUNT

    SFX_PUZZLE_FAIL_FIGHTER
    SFX_PUZZLE_FAIL_CLERIC
    SFX_PUZZLE_FAIL_MAGE

    SFX_PLAYER_FIGHTER_BURN_DEATH
    SFX_PLAYER_CLERIC_BURN_DEATH
    SFX_PLAYER_MAGE_BURN_DEATH
    sfx_hedat1 // Burn sound
  */


}

#warning TODO temp solution, we should use DECORATE instead.
// lists of mobjtypes that can be played by humans!
// TODO nproj should be replaced with a function pointer.
// Somebody should then write these shooting functions...
struct pawn_info_t
{
  mobjtype_t   mt;
  pclass_t     pclass;
  weapontype_t bweapon; // beginning weapon (besides fist/staff)
  int          bammo;   // ammo for bweapon
  mobjtype_t   nproj;   // natural projectile, if any
} pawndata[] = 
{
  {MT_PLAYER,   PCLASS_NONE, wp_pistol,  50, MT_NONE}, // 0
  {MT_POSSESSED, PCLASS_NONE, wp_pistol,  20, MT_NONE},
  {MT_SHOTGUY,  PCLASS_NONE, wp_shotgun,  8, MT_NONE},
  {MT_TROOP,    PCLASS_NONE, wp_none, 0, MT_TROOPSHOT},
  {MT_SERGEANT, PCLASS_NONE, wp_none, 0, MT_NONE},
  {MT_SHADOWS,  PCLASS_NONE, wp_none, 0, MT_NONE},
  {MT_SKULL,    PCLASS_NONE, wp_none, 0, MT_NONE},
  {MT_HEAD,     PCLASS_NONE, wp_none, 0, MT_HEADSHOT},
  {MT_BRUISER,  PCLASS_NONE, wp_none, 0, MT_BRUISERSHOT},
  {MT_SPIDER,   PCLASS_NONE, wp_chaingun, 100, MT_NONE},
  {MT_CYBORG,   PCLASS_NONE, wp_missile,  20,  MT_NONE}, //10

  {MT_WOLFSS,   PCLASS_NONE, wp_chaingun, 50, MT_NONE},
  {MT_CHAINGUY, PCLASS_NONE, wp_chaingun, 50, MT_NONE},
  {MT_KNIGHT,   PCLASS_NONE, wp_none, 0,  MT_BRUISERSHOT},
  {MT_BABY,     PCLASS_NONE, wp_plasma,  50,  MT_ARACHPLAZ},
  {MT_PAIN,     PCLASS_NONE, wp_none, 0,  MT_SKULL},
  {MT_UNDEAD,   PCLASS_NONE, wp_none, 0,  MT_TRACER},
  {MT_FATSO,    PCLASS_NONE, wp_none, 0,  MT_FATSHOT},
  {MT_VILE,     PCLASS_NONE, wp_none, 0,  MT_FIRE}, // 18

  {MT_HPLAYER,  PCLASS_NONE, wp_goldwand, 50, MT_NONE},
  {MT_CHICKEN,  PCLASS_NONE, wp_beak,      0, MT_NONE},
  {MT_MUMMY,    PCLASS_NONE, wp_none, 0, MT_NONE},
  {MT_MUMMYLEADER, PCLASS_NONE, wp_none, 0, MT_MUMMYFX1},
  {MT_MUMMYGHOST,  PCLASS_NONE, wp_none, 0, MT_NONE},
  {MT_MUMMYLEADERGHOST, PCLASS_NONE, wp_none, 0, MT_MUMMYFX1},
  {MT_BEAST,    PCLASS_NONE, wp_none, 0, MT_BEASTBALL},
  {MT_SNAKE,    PCLASS_NONE, wp_none, 0, MT_SNAKEPRO_A},
  {MT_HHEAD,    PCLASS_NONE, wp_none, 0, MT_HEADFX1},
  {MT_CLINK,    PCLASS_NONE, wp_none, 0, MT_NONE},
  {MT_WIZARD,   PCLASS_NONE, wp_none, 0, MT_WIZFX1},
  {MT_IMP,      PCLASS_NONE, wp_none, 0, MT_NONE},
  {MT_IMPLEADER,PCLASS_NONE, wp_none, 0, MT_IMPBALL},
  {MT_HKNIGHT,  PCLASS_NONE, wp_none, 0, MT_KNIGHTAXE},
  {MT_KNIGHTGHOST, PCLASS_NONE, wp_none, 0, MT_REDAXE},
  {MT_SORCERER1, PCLASS_NONE, wp_none, 0, MT_SRCRFX1},
  {MT_SORCERER2, PCLASS_NONE, wp_none, 0, MT_SOR2FX1},
  {MT_MINOTAUR,  PCLASS_NONE, wp_none, 0, MT_MNTRFX1}, // 36

  {MT_PLAYER_FIGHTER, PCLASS_FIGHTER, wp_fpunch, 0, MT_NONE},
  {MT_PLAYER_CLERIC, PCLASS_CLERIC, wp_cmace, 0, MT_NONE},
  {MT_PLAYER_MAGE, PCLASS_MAGE, wp_mwand, 0, MT_NONE},
  {MT_PIGPLAYER, PCLASS_PIG, wp_snout, 0, MT_NONE}
};


void MakePawnAIs()
{
  for (int i=0; i<=40; i++)
    {
      pawn_info_t *x = &pawndata[i];
      ActorInfo *ai = aid[x->mt];
      pawn_aid.push_back(new PawnAI(*ai, x->pclass, x->bweapon));
    }
}


//=====================================
//  Pawn and PlayerPawn classes
//=====================================

IMPLEMENT_CLASS(Pawn, Actor);
TNL_IMPLEMENT_NETOBJECT(Pawn);

IMPLEMENT_CLASS(PlayerPawn, Pawn);
TNL_IMPLEMENT_NETOBJECT(PlayerPawn);

Pawn::Pawn()
  : Actor()
{
  info = NULL;
  skin = GetSkin();

  pclass = PCLASS_NONE;
  maxhealth = 0;
  speed = 0;
  toughness = 0;

  attackphase = 0;
  attacker = NULL;
}

PlayerPawn::PlayerPawn()
  : Pawn()
{
  //CONS_Printf("playerpawn trick-constr. called\n");
  // FIXME NOW initialize more for ghosting?

  // NOTE! This constructor is used when Unserializing or Ghosting, so not everything is initialized!
  player = NULL;
  weaponinfo = NULL;

  for (int i=0; i<NUMPOWERS; i++)
    powers[i] = 0;

  cheats = refire = 0;
  morphTics = 0;

  extralight = fixedcolormap = 0;
  pendingweapon = readyweapon = wp_none;
  SetupPsprites();
}


//=====================================
// Pawn methods
//=====================================


void Pawn::Detach()
{
  attacker = NULL;
  Actor::Detach();
} 

void Pawn::CheckPointers()
{
  if (owner && (owner->eflags & MFE_REMOVE))
    owner = NULL;

  if (target && (target->eflags & MFE_REMOVE))
    target = NULL;

  if (attacker && (attacker->eflags & MFE_REMOVE))
    attacker = NULL;
}



// creates a pawn based on a Doom/Heretic mobj
Pawn::Pawn(fixed_t x, fixed_t y, fixed_t z, int type)
  : Actor(x, y, z)
{
  // TEMP
  if (type < 0)
    {
      if (game.mode == gm_hexen)
	type = 37;
      else if (game.mode == gm_heretic)
	type = 19;
      else
	type = 0;
    }

  info = pawn_aid[type];
  pclass = info->pclass;

  skin = GetSkin();

  const float AutoArmorSave[] = { 0.0, 0.15, 0.10, 0.05, 0.0 };
  toughness = AutoArmorSave[pclass];

  mass   = info->mass;
  radius = info->radius;
  height = info->height;
  maxhealth = health = info->spawnhealth;
  speed  = info->speed;

  flags  = info->flags;
  flags2 = info->flags2;
  eflags = 0;

  reactiontime = info->reactiontime;
  attackphase = 0;
  attacker = NULL;

  if (!info->modelname.empty())
    pres = new modelpres_t(info->modelname.c_str());
  else
    pres = new spritepres_t(info, 0);
}


//=====================================
// PlayerPawn methods
//=====================================

PlayerPawn::PlayerPawn(fixed_t nx, fixed_t ny, fixed_t nz, int type)
  : Pawn(nx, ny, nz, type)
{
  // note! here Map *mp is not yet set! This means you can't call functions such as
  // SetPosition that have something to do with a map.
  flags |= (MF_PLAYER | MF_PICKUP | MF_SHOOTABLE | MF_DROPOFF);
  flags &= ~MF_COUNTKILL;
  flags2 |= (MF2_WINDTHRUST | MF2_SLIDE | MF2_TELESTOMP | MF2_PUSHWALL);

  player = NULL;

  morphTics = 0;

  //inventory.resize(1, inventory_t(arti_none, 0)); // at least 1 empty slot

  usedown = attackdown = jumpdown = true;  // don't do anything immediately
  refire = 0;

  cheats = 0;

  int i;
  for (i=0; i<NUMPOWERS; i++)
    powers[i] = 0;

  keycards = 0;

  weaponinfo = wpnlev1info;

  for (i=0; i<NUMAMMO; i++)
    {
      ammo[i] = 0;
      maxammo[i] = maxammo1[i];
    }

  for (i=0; i<NUMWEAPONS; i++)
    weaponowned[i] = false;

  if (game.mode == gm_heretic)
    weaponowned[wp_staff] = true;
  else if (game.mode == gm_hexen)
    {}
  else
    weaponowned[wp_fist] = true;

  weapontype_t w = info->bweapon;
  readyweapon = pendingweapon = w;

  if (w != wp_none)
    {
      weaponowned[w] = true;
      ammotype_t a = wpnlev1info[w].ammo;
      if (a != am_noammo)
	ammo[a] = info->bammo;
    }

  // armor
  for (i = 0; i < NUMARMOR; i++)
    {
      armorpoints[i] = 0;
      armorfactor[i] = 0;
    }

  poisoncount = 0;
  specialsector = 0;
  extralight = fixedcolormap = 0;

  fly_zspeed = 0;
}


PlayerPawn::~PlayerPawn()
{
  if (player)
    player->pawn = NULL;
}


fixed_t PlayerPawn::GetViewZ() const
{
  if (player)
    return player->viewz;

  return Top();
}


void PlayerPawn::Think()
{
  // an abandoned corpse, for example. Thinks just like an actor.
  if (!player)
    {
      Actor::Think();
      return;
    }

  ticcmd_t* cmd = &player->cmd;

  // Turn on required cheats
  if (cheats & CF_NOCLIP)
    flags |= (MF_NOCLIPLINE|MF_NOCLIPTHING);
  else
    flags &= ~(MF_NOCLIPLINE|MF_NOCLIPTHING);

  // chain saw run forward
  if (eflags & MFE_JUSTATTACKED)
    {
      cmd->forward = 100;
      cmd->side = 0;
      eflags &= ~MFE_JUSTATTACKED;
    }

  if (flags & MF_CORPSE)
    {
      DeathThink();
      goto actor_think;
    }

  // morphed state?
  if (morphTics)
    MorphThink();

  // CheckWater is also called at Actor::Think later...
  CheckWater();

  // Move around.
  // Reactiontime is used to prevent movement
  //  for a bit after a teleport.
  if (reactiontime > 0)
    reactiontime--;
  else
    Move();

  // check special sectors : damage & secrets
  PlayerInSpecialSector();

  // old water splashes
  /*
  if (specialsector >= 887 && specialsector <= 888)
    {
      if ((px > (2*FRACUNIT) || px < (-2*FRACUNIT) ||
	   py > (2*FRACUNIT) || py < (-2*FRACUNIT) ||
	   pz >  (2*FRACUNIT)) &&  // jump out of water
	  !(game.tic % (32 * NEWTICRATERATIO)))
        {
	  //
	  // make sure we disturb the surface of water (we touch it)
	  //
	  int waterz;
	  if (specialsector == 887)
	    //FLAT TEXTURE 'FWATER'
	    waterz = subsector->sector->floorheight + (FRACUNIT/4);
	  else
	    //FIXME: no hacks! faB's current water hack using negative sector tags
	    waterz = - (subsector->sector->tag << FRACBITS);

	  // half in the water
	  if (eflags & MFE_TOUCHWATER)
            {
	      if (z <= floorz) // onground
                {
		  fixed_t whater_height = waterz-subsector->sector->floorheight;

		  if (whater_height < (height >> 2))
		    S_StartSound(this, sfx_enterwater);
		  else
		    S_StartSound(this, sfx_exitwater);
                }
	      else
		S_StartSound(this, sfx_exitwater);
            }                   
        }
    }
  */

  int w;

  // Check for weapon change.
  if ((w = (cmd->buttons & ticcmd_t::WEAPONMASK)))
    {
      // The actual changing of the weapon is done
      //  when the weapon psprite can do it
      //  (read: not in the middle of an attack).

      w = (w >> ticcmd_t::WEAPONSHIFT) - 1;
      if (w < NUMWEAPONS && weaponowned[w])
	pendingweapon = weapontype_t(w);
    }

  // check for use
  if (cmd->buttons & ticcmd_t::BT_USE)
    {
      if (!usedown)
        {
	  UseLines();
	  usedown = true;
        }
    }
  else
    usedown = false;

  // artifacts
  if (cmd->item)
    UseArtifact(artitype_t(cmd->item - 1));

  // morph counter
  if (morphTics)
    {
      // Chicken attack counter
      if (attackphase)
	attackphase -= 3;
      // Attempt to undo the chicken
      if (--morphTics == 0)
	UndoMorph();
    }

  // cycle psprites
  MovePsprites();

  // Counters, time dependend power ups.
  // Strength counts up to diminish fade.
  if (powers[pw_strength])
    powers[pw_strength]++;

  if (powers[pw_invulnerability])
    powers[pw_invulnerability]--;

  // the MF_SHADOW activates the tr_transhi translucency while it is set
  // (it doesnt use a preset value through FF_TRANSMASK)
  if (powers[pw_invisibility])
    if (--powers[pw_invisibility] == 0)
      flags &= ~MF_SHADOW;

  if (powers[pw_infrared])
    powers[pw_infrared]--;

  if (powers[pw_ironfeet])
    powers[pw_ironfeet]--;

  if (powers[pw_flight])
    {
      if(--powers[pw_flight] == 0)
	{
	  eflags &= ~MFE_FLY;
	  flags &= ~MF_NOGRAVITY;
	}
    }

  if(powers[pw_weaponlevel2])
    {
      if(--powers[pw_weaponlevel2] == 0)
	{
	  weaponinfo = wpnlev1info;
	  // end of weaponlevel2 power
	  if (readyweapon == wp_phoenixrod
	      && (psprites[ps_weapon].state != &weaponstates[S_PHOENIXREADY])
	      && (psprites[ps_weapon].state != &weaponstates[S_PHOENIXUP]))
	    {
	      SetPsprite(ps_weapon, S_PHOENIXREADY);
	      ammo[am_phoenixrod] -= wpnlev2info[wp_phoenixrod].ammopershoot;
	      refire = 0;
	    }
	  else if ((readyweapon == wp_gauntlets) || (readyweapon == wp_staff))
	    pendingweapon = readyweapon;
	}
    }

  // damage and bonus counts are now HUD properties and are ticked there

  // Handling colormaps.
  if (powers[pw_invulnerability])
    {
      if (powers[pw_invulnerability] > BLINKTHRESHOLD
	  || (powers[pw_invulnerability] & 8))
	fixedcolormap = INVERSECOLORMAP;
      else
	fixedcolormap = 0;
    }
  else if (powers[pw_infrared])
    {
      if (powers[pw_infrared] > BLINKTHRESHOLD
	  || (powers[pw_infrared] & 8))
        {
	  // almost full bright
	  fixedcolormap = 1;
        }
      else
	fixedcolormap = 0;
    }
  else
    fixedcolormap = 0;


  // handle poisonings
  if (poisoncount && !(mp->maptic & 15))
    {
      poisoncount -= 5;
      if (poisoncount < 0)
	poisoncount = 0;

      Damage(attacker, attacker, 1, dt_poison); 

      if (!(mp->maptic & 63))
	{
	  // TODO painsound and anim...
	  //pres->SetAnim(presentation_t::Pain);
	}
    }


 actor_think:
  // this is where the "actor part" of the thinking begins
  // we call Actor::Think(), because a playerpawn is an actor too
  Actor::Think();
}



// Fall on your face when dying.
// Decrease POV height to floor height.
void PlayerPawn::DeathThink()
{
  MovePsprites();

  // fall to the ground
  if (player->viewheight > 6)
    player->viewheight -= 1;

  if (player->viewheight < 6)
    player->viewheight = 6;

  player->deltaviewheight = 0;

  // watch my killer (if there is one)
  if (attacker != NULL && attacker != this)
    {
      angle_t ang = R_PointToAngle2(pos.x, pos.y, attacker->pos.x, attacker->pos.y);
      angle_t delta = ang - yaw;

      if (delta < ANG5 || delta > (unsigned)-ANG5)
        {
	  // Looking at killer,
	  //  so fade damage flash down.
	  yaw = ang;
        }
      else if (delta < ANG180)
	yaw += ANG5;
      else
	yaw -= ANG5;

      // change aiming to look up or down at the attacker (DOESNT WORK)
      // FIXME : the aiming returned seems to be too up or down...
      //fixed_t dist = P_AproxDistance(attacker->x - x, attacker->y - y);
      //if (dist)
      //  pitch = FixedMul ((160<<FRACBITS), FixedDiv (attacker->z + (attacker->height>>1), dist)) >>FRACBITS;
      //else pitch = 0;
      pitch = (attacker->pos.z - pos.z).value() >> 17;
    }

  if (player->cmd.buttons & ticcmd_t::BT_USE)
    mp->RebornPlayer(player);
}



void PlayerPawn::MorphThink()
{
  if (game.mode == gm_heretic)
  {
    if (health > 0)
    {
      // Handle beak movement
      /*
       * g_pawn.cpp: In member function 'void PlayerPawn::MorphThink()':
       * g_pawn.cpp:659:37: warning: suggest parentheses around '+' inside '>>' [-Wparentheses]
       * psprites[ps_weapon].sy = WEAPONTOP + fixed_t(attackphase) >> 1;
       */
      // psprites[ps_weapon].sy = WEAPONTOP + fixed_t(attackphase) >> 1;
      /*
       * Bei Weapon-Sprites (psprites[ps_weapon].sy) ist das eine klassische
       * Bob-/Recoil-Animation:
       * WEAPONTOP = Basis-Position der Waffe (meist ein negativer fixed_t-Wert,
       * z.B. -32 oder -100, damit die Waffe oben am Screen ist)
       * attackphase = Zähler während des Angriffs (z. B. 0 → 16 oder so)
       * Man addiert erst einen Offset (attackphase), und teilt dann das Ergebnis
       * durch 2 → sanfte Bewegung
      */
      fixed_t offset = fixed_t(attackphase);
      psprites[ps_weapon].sy = WEAPONTOP + (offset >> 1);      
    }

    if (morphTics & 15)
      return;

    if ((vel.x == 0 && vel.y == 0) && P_Random() < 160)
    { // Twitch view angle
      yaw += P_SignedRandom() << 19;
    }
    
    if ((pos.z <= floorz) && (P_Random() < 32))
    { // Jump and noise
      vel.z += 1;
      pres->SetAnim(presentation_t::Pain);
      return;
    }
    
    if(P_Random() < 48)
    { // Just noise
      S_StartScreamSound(this, sfx_chicact);
    }
  }
  else if (game.mode == gm_hexen)
    {
      if (morphTics & 15)
	return;

      if ((vel.x == 0 && vel.y == 0) && P_Random() < 64)
	{ // Snout sniff
	  SetPsprite(ps_weapon, S_SNOUTATK2, false);
	  S_StartSound(this, SFX_PIG_ACTIVE1); // snort
	  return;
	}
      if (P_Random() < 48)
	{
	  if(P_Random() < 128)
	    S_StartSound(this, SFX_PIG_ACTIVE1);
	  else
	    S_StartSound(this, SFX_PIG_ACTIVE2);
	}
    }
}



//=================================================
//    Movement
//=================================================

void PlayerPawn::Move()
{
  ticcmd_t *cmd = &player->cmd;

  yaw = cmd->yaw << 16;
  pitch = cmd->pitch << 16;

  fixed_t movepushforward = 0, movepushside = 0;

  //CONS_Printf("p = (%d, %d), v = %f / tic\n", px, py, sqrt(px*px+py*py)/FRACUNIT);

  float mf = GetMoveFactor();

#define F_NORM 0.90625f

  // mf_ice = (1-f)/(1-n)
  // mf_mud = (f-2*n+1)/(1-n)

  // NOTE: limit speed depends on in which order thrust and friction are applied.
  // Currently thrust is first, hence limit speed is
  // v' = f*(v+t) = v  <=>  v = f/(1-f) * t
  // On a normal surface, the max limit speed should be equal to 2*PlayerPawn::speed.
  // Hence t_max = 2 * (1-f_norm)/f_norm * s
  float magic = (2*(1-F_NORM)/F_NORM) * speed * mf;

  // lim v on ice:  v = f/n * 2*s
  // lim v on mud:  v = f/n * (f-xxx)/(1-f) * 2*s
  // a on ice = (1-f)*(f/n * 2*s - v)
  // a on mud = -(1-f)*v + f/n * (f-2*n+1) * 2*s

  // If friction is applied first: TODO this is more natural...
  // v' = f*v + t = v  <=>  v = 1/(1-f) * t
  // t_max = 2 * (1-f_norm) * s
  // float magic = 2*(1-F_NORM) * speed * mf;

  // lim v on ice:  v = 2*s
  // lim v on mud:  v = (f-xxx)/(1-f) * 2*s
  // a on ice = (1-f) * (2*s - v)
  // a on mud = -(1-f)*v + (f-2*n+1)* 2*s

  if (cmd->forward)
    {
      //CONS_Printf("::m %d, %f, magic = %f\n", cmd->forwardmove, mf, magic);
      movepushforward = magic * cmd->forward/100.0f;
      Thrust(yaw, movepushforward);
    }

  if (cmd->side)
    {
      movepushside = magic * cmd->side/100.0f;
      Thrust(yaw-ANG90, movepushside);
    }

  // mouselook swim when waist underwater
  eflags &= ~MFE_SWIMMING;
  if (eflags & MFE_UNDERWATER)
    {
      // swim up/down full move when forward full speed
      fixed_t a = (movepushforward * finesine[pitch >> ANGLETOFINESHIFT] * 50) >> 5;
      
      if ( a != 0 )
	{
	  eflags |= MFE_SWIMMING;
	  vel.z += a;
	}
    }

  bool onground = (pos.z <= floorz) || (eflags & (MFE_ONMOBJ | MFE_FLY)) || (cheats & CF_FLYAROUND);

  // jumping
  if (cmd->buttons & ticcmd_t::BT_JUMP)
    {
      if (eflags & MFE_FLY)
	fly_zspeed = 10;
      else if (eflags & MFE_UNDERWATER)
	//TODO: goub gloub when push up in water
	vel.z = 0.5f;
      else if (onground && !jumpdown && cv_jumpspeed.value) // can't jump while in air or while jumping
	{
	  vel.z = cv_jumpspeed.Get();
	  if (!(cheats & CF_FLYAROUND))
	    {
	      S_StartScreamSound(this, sfx_jump);
	      // keep jumping ok if FLY mode.
	      jumpdown = true;
	    }
	}
    }
  else
    jumpdown = false;

  if (cmd->forward || cmd->side)
    {
      // set the running state if nothing more important is going on
      if (pres->GetAnim() <= presentation_t::LAST_LOOPING)
	pres->SetAnim((cmd->forward >= 0) ? presentation_t::Run : presentation_t::Back);
    }

  if (eflags & MFE_FLY)
    {
      if (cmd->buttons & ticcmd_t::BT_FLYDOWN)
	fly_zspeed = -10;

      vel.z = fly_zspeed;
      if (fly_zspeed)
	fly_zspeed /= 2;
    }
}



void PlayerPawn::ZMovement()
{
  if (player)
    {
      // corpses etc. do not do this
      // check for smooth step up
      if (pos.z < floorz)
	{
	  player->viewheight -= floorz - pos.z;
	  player->deltaviewheight = (cv_viewheight.value - player->viewheight) >> 3;
	}
    }

  Actor::ZMovement();
}



void PlayerPawn::XYFriction(fixed_t oldx, fixed_t oldy)
{
  // slow down
  if (cheats & CF_NOMOMENTUM)
    {
      // debug option for no sliding at all
      vel.x = vel.y = 0;
      return;
    }
  else if (cheats & CF_FLYAROUND)
    {
      // no friction when airborne
      if (pos.z > floorz)
	return;
    }

  const fixed_t STOPSPEED = 0.0625f;

  if (player &&
      vel.x > -STOPSPEED && vel.x < STOPSPEED && vel.y > -STOPSPEED && vel.y < STOPSPEED && 
      (player->cmd.forward == 0 && player->cmd.side == 0 ))
    {
      // if in a walking frame, stop moving
      if (pres->GetAnim() <= presentation_t::LAST_LOOPING)
	pres->SetAnim(presentation_t::Idle);
    
      vel.x = 0;
      vel.y = 0;
    }
  else Actor::XYFriction(oldx, oldy);
}



bool PlayerPawn::Teleport(const vec_t<fixed_t> &p, angle_t nangle, bool silent)
{
  bool ret = Actor::Teleport(p, nangle, silent);

  // don't move for a bit
  if (!silent && !powers[pw_weaponlevel2])
    reactiontime = 18;
    
  // FIXME code below is useless, right?
  // Adjust player's view, in case there has been a height change
  
  // Save the current deltaviewheight, used in stepping
  //  fixed_t deltaviewheight = player->deltaviewheight;
  // Clear deltaviewheight, since we don't want any changes
  //  player->deltaviewheight = 0;
  // Set player's view according to the newly set parameters
  //  CalcHeight(player);
  // Reset the delta to have the same dynamics as before
  // player->deltaviewheight = deltaviewheight;

  return ret;
}


//=================================================
//   Other stuff
//=================================================

//  Called when a player exits a map.
//  Throws away extra items, removes powers, keys, curses
void PlayerPawn::Reset()
{
  int i, n = inventory.size();
  for (i=0; i<n; i++)
    if (inventory[i].count > 1) 
      inventory[i].count = 1; // why? not fair

  /*
  if (!cv_deathmatch.value)
    for(i = 0; i < MAXARTECONT; i++)
      UseArtifact(arti_fly);
  */

  keycards = 0;
  memset(powers, 0, sizeof (powers));
  weaponinfo = wpnlev1info;    // cancel power weapons
  flags &= ~MF_SHADOW;         // cancel invisibility
  extralight = 0;              // cancel gun flashes
  fixedcolormap = 0;           // cancel ir goggles

  if (morphTics)
    {
      readyweapon = weapontype_t(attackphase); // Restore weapon
      morphTics = 0;
    }
}


// returns the next owned weapon in group g
weapontype_t PlayerPawn::FindWeapon(int g)
{
  if (readyweapon != wp_none && g == weapongroup[readyweapon].group)
    {
      weapontype_t n = weapongroup[readyweapon].next;
      while (n != readyweapon)
	{
	  if (n == wp_none || weaponowned[n])
	    return n;

	  n = weapongroup[n].next;
	}
    }
  else
    for (int i=0; i<NUMWEAPONS; i++)
      if (g == weapongroup[i].group && weaponowned[i])
	return weapontype_t(i);

  return wp_none;
}



// Tries to aim at a nearby monster
DActor *PlayerPawn::SPMAngle(mobjtype_t type, angle_t ang)
{
  float sine = 0;

  if (player->options.autoaim && cv_allowautoaim.value)
    {
      // see which target is to be aimed at
      Actor *targ = AimLineAttack(ang, AIMRANGE, sine);

      if (!targ)
        {
	  ang += 1<<26;
	  targ = AimLineAttack(ang, AIMRANGE, sine);

	  if (!targ)
            {
	      ang -= 2<<26;
	      targ = AimLineAttack(ang, AIMRANGE, sine);
            }

	  if (!targ)
	    sine = Sin(pitch).Float();
        }
    }
  else
    sine = Sin(pitch).Float();

  // if not autoaim, or if the autoaim didnt aim something, use the mouseaiming    

  fixed_t mz = pos.z - floorclip + 32;

  DActor *th = mp->SpawnDActor(pos.x, pos.y, mz, type);

  if (th->info->seesound)
    S_StartSound(th, th->info->seesound);

  th->owner = this;

  th->yaw = ang;

  th->Thrust(ang, th->info->speed * sqrtf(1 - sine*sine));

  if (th->flags2 & (MF2_CEILINGHUGGER | MF2_FLOORHUGGER))
    sine = 0;

  th->vel.z = sine * th->info->speed;

  return (th->CheckMissileSpawn()) ? th : NULL;
}




// checks if the player has the correct key for 'lock'
bool P_CheckKeys(Actor *mo, int lock)
{
  PlayerPawn *p = mo->Inherits<PlayerPawn>();

  if (!p)
    return false;

  if (!lock)
    return true;

  if (lock > NUMKEYS)
    return false;

  if (!(p->keycards & (1 << (lock-1))))
    {
      static char buf[80];
      if (lock <= 11)
	{
	  sprintf(buf, "You need the %s\n",  text[TXT_KEY_STEEL + lock - 1]);
	  p->player->SetMessage(buf);
	  S_StartSound(mo, SFX_DOOR_LOCKED);
	}
      else
	{
	  // skulls and cards are equivalent
	  if (p->keycards & (1 << (lock+2)))
	    return true;

	  p->player->SetMessage(text[TXT_PD_BLUEK + lock - 12]);
	  S_StartScreamSound(p, sfx_usefail);
	}

      return false; // no ticket!
    }
  return true;
}


// Passed a generalized locked door linedef and a player, returns whether
// the player has the keys necessary to unlock that door.
//
// Note: The linedef passed MUST be a generalized locked door type
//       or results are undefined.
bool PlayerPawn::CanUnlockGenDoor(line_t *line)
{
  // does this line special distinguish between skulls and keys?
  bool skulliscard = (line->special & LockedNKeys)>>LockedNKeysShift;
  bool ok = true;

  // determine for each case of lock type if player's keys are adequate
  switch ((line->special & LockedKey)>>LockedKeyShift)
    {
    case AnyKey_:
      if (!(keycards & it_redcard) &&
	  !(keycards & it_redskull) &&
	  !(keycards & it_bluecard) &&
	  !(keycards & it_blueskull) &&
	  !(keycards & it_yellowcard) &&
	  !(keycards & it_yellowskull))
	{
	  player->SetMessage(PD_ANY);
	  ok = false;
	}
      break;
    case RCard:
      if
	(
	 !(keycards & it_redcard) &&
	 (!skulliscard || !(keycards & it_redskull))
	 )
	{
	  player->SetMessage(skulliscard? PD_REDK : PD_REDC);
	  ok = false;
	}
      break;
    case BCard:
      if
	(
	 !(keycards & it_bluecard) &&
	 (!skulliscard || !(keycards & it_blueskull))
	 )
	{
	  player->SetMessage(skulliscard? PD_BLUEK : PD_BLUEC);
	  ok = false;
	}
      break;
    case YCard:
      if
	(
	 !(keycards & it_yellowcard) &&
	 (!skulliscard || !(keycards & it_yellowskull))
	 )
	{
	  player->SetMessage(skulliscard? PD_YELLOWK : PD_YELLOWC);
	  ok = false;
	}
      break;
    case RSkull:
      if
	(
	 !(keycards & it_redskull) &&
	 (!skulliscard || !(keycards & it_redcard))
	 )
	{
	  player->SetMessage(skulliscard? PD_REDK : PD_REDS);
	  ok = false;
	}
      break;
    case BSkull:
      if
	(
	 !(keycards & it_blueskull) &&
	 (!skulliscard || !(keycards & it_bluecard))
	 )
	{
	  player->SetMessage(skulliscard? PD_BLUEK : PD_BLUES);
	  ok = false;
	}
      break;
    case YSkull:
      if
	(
	 !(keycards & it_yellowskull) &&
	 (!skulliscard || !(keycards & it_yellowcard))
	 )
	{
	  player->SetMessage(skulliscard? PD_YELLOWK : PD_YELLOWS);
	  ok = false;
	}
      break;
    case AllKeys:
      if
	(
	 !skulliscard &&
	 (
          !(keycards & it_redcard) ||
          !(keycards & it_redskull) ||
          !(keycards & it_bluecard) ||
          !(keycards & it_blueskull) ||
          !(keycards & it_yellowcard) ||
          !(keycards & it_yellowskull)
	  )
	 )
	{
	  player->SetMessage(PD_ALL6);
	  ok = false;
	}
      if
	(
	 skulliscard &&
	 (
          (!(keycards & it_redcard) &&
	   !(keycards & it_redskull)) ||
          (!(keycards & it_bluecard) &&
	   !(keycards & it_blueskull)) ||
          (!(keycards & it_yellowcard) &&
	   !(keycards & it_yellowskull))
	  )
	 )
	{
	  player->SetMessage(PD_ALL3);
	  ok = false;
	}
      break;
    }
  if (ok == false) S_StartScreamSound(this, sfx_usefail);

  return ok;
}



// Function that actually applies the sector special to the player.
void PlayerPawn::ProcessSpecialSector(sector_t *sector)
{
  if (sector->damage)
    {
      int damage = sector->damage & dt_DAMAGEMASK;
      int dtype = sector->damage & dt_TYPEMASK;

      if ((sector->special & SS_SPECIALMASK) == SS_EndLevelHurt)
	{
          // EXIT SUPER DAMAGE! (for E1M8 finale)
	  cheats &= ~CF_GODMODE;
	  Damage(NULL, NULL, 20);
          if (health <= 10)
	    {
	      mp->ExitMap(this, 0);
	      return;
	    }
	}
      else if (!powers[pw_ironfeet] ||
	       (damage >= 20 && P_Random() < 5))
	Damage(NULL, NULL, damage, dtype);
      
      mp->SpawnSmoke(pos);
    }
}




//
// Called every tic frame
//  that the player origin is in a special sector
//
void PlayerPawn::PlayerInSpecialSector()
{
  sector_t *sec;

  //Fab: jumping in lava/slime does instant damage (no jump cheat)
  bool instantdamage = (eflags & MFE_JUSTHITFLOOR) && (mp->maptic & 2);

  // More realistic damage: check all sectors we touch!
  msecnode_t *node = touching_sectorlist;
  while (node)
    {
      sec = node->m_sector;

      specialsector = sec->special; //Fab: keep track of what sector type the player's currently in
      bool onground = (Feet() <= sec->floorheight);

      if (specialsector &&
	  ((sec->heightsec != -1 && Feet() <= mp->sectors[sec->heightsec].floorheight) ||
	   onground))
	{
	  //SoM: 3/17/2000: Damage if in slimey water!

	  int temp = specialsector & SS_DAMAGEMASK;
	  if ((temp == SS_damage_32 && !(mp->maptic & 31)) ||
	      (temp == SS_damage_16 && !(mp->maptic & 15)) ||
	      instantdamage)
	    {
	      ProcessSpecialSector(sec);
	      break; // just one damage effect per round
	    }
	}

      if (sec->floortype == FLOOR_LAVA && onground)
	{
	  if (instantdamage || !(mp->maptic & 31))
	    {
	      Damage(NULL, NULL, 10, dt_heat);
	      S_StartSound(this, SFX_LAVA_SIZZLE);
	      mp->SpawnSmoke(pos);
	      break;
	    }
	}

      ffloor_t *rover = sec->ffloors;
      sec = NULL;
      // Check if a player is standing on or is inside a 3D floor (water)
      for ( ; rover; rover = rover->next)
	{
	  // each floor has its own control sector and thus own effect
	  specialsector = rover->master->frontsector->special;

	  if (!specialsector)
	    continue;

	  // Check the 3D floor's type...
	  if (rover->flags & FF_SOLID)
	    {
	      // Player must be on top of the floor to be affected...
	      if (Feet() != *rover->topheight)
		continue;
	    }
	  else
	    {
	      // Water and DEATH FOG!!! heh
	      if (Feet() > *rover->topheight || Top() < *rover->bottomheight)
		continue;
	    }

	  int temp = specialsector & SS_DAMAGEMASK;
	  if ((temp == SS_damage_32 && !(mp->maptic & 31)) ||
	      (temp == SS_damage_16 && !(mp->maptic & 15)) ||
	      instantdamage)
	    {
	      sec = rover->master->frontsector;
	      break; // just one damage effect per round
	    }
	}

      if (sec)
	{
	  // do the damage
	  ProcessSpecialSector(sec);
	  break; // just one damage effect per round
	}

      node = node->m_tnext;
    }


  // finally check secret, but only if we are _in_ the sector
  sec = subsector->sector;
  if (sec->special & SS_secret)
    {
      // SECRET SECTOR
      player->secrets++;
      player->SetMessage("\a\2You found a secret area!\n");
      sec->special &= ~SS_secret;
    }
}




//===========================================================
//   Giving playerpawns stuff
//===========================================================

extern int  p_sound;  // pickupsound
extern bool p_remove; // should the stuff be removed?

// Returns false if the health isn't needed at all
bool Pawn::GiveBody(int num)
{
  if (health >= maxhealth)
    return false;

  health += num;
  if (health > maxhealth)
    health = maxhealth;

  return true;
}


bool PlayerPawn::GivePower(int power)
{
  switch (power)
    {
    case pw_invulnerability:
      // Already have it?
      if (powers[power] > BLINKTHRESHOLD)
	return false;
      powers[power] = INVULNTICS;
      flags2 |= MF2_INVULNERABLE;
      if (pclass == PCLASS_MAGE)
	flags2 |= MF2_REFLECTIVE; // Hexen mage
      break;

    case pw_weaponlevel2:
      if (powers[power] > BLINKTHRESHOLD)
	return false;
      powers[power] = WPNLEV2TICS;
      weaponinfo = wpnlev2info;
      break;
  
    case pw_invisibility:
      if (powers[power] > BLINKTHRESHOLD)
	return false;
      powers[power] = INVISTICS;
      flags |= MF_SHADOW;
      break;

    case pw_flight:
      if (powers[power] > BLINKTHRESHOLD)
	return false;
      powers[power] = FLIGHTTICS;
      eflags |= MFE_FLY;
      flags |= MF_NOGRAVITY;
      if (pos.z <= floorz)
	fly_zspeed = 10; // thrust the player in the air a bit
      break;

    case pw_infrared:
      if (powers[power] > BLINKTHRESHOLD)
	return false;
      powers[power] = INFRATICS;
      break;

    case pw_ironfeet:
      powers[power] = IRONTICS;
      break;

    case pw_strength:
      GiveBody(100);
      powers[power] = 1;
      break;

    case pw_speed:
      if (powers[power] > BLINKTHRESHOLD)
	return false;
      powers[power] = SPEEDTICS;
      break;

    case pw_minotaur:
      powers[power] = MAULATORTICS;
      break;

    default:
      if (powers[power] != 0)
	return false;
      powers[power] = 1;
    }

  p_sound = sfx_powerup;

  return true;
}


// Num is the number of clip loads,
// not the individual count (0= 1/2 clip).
// Returns false if the ammo can't be picked up at all

bool PlayerPawn::GiveAmmo(ammotype_t at, int count)
{
  static const weapontype_t GetAmmoChange[] =
  {
    wp_chaingun, wp_shotgun, wp_plasma, wp_missile,
    wp_goldwand,
    wp_crossbow,
    wp_blaster,
    wp_skullrod,
    wp_phoenixrod,
    wp_mace
  };

  if (at == am_noammo)
    return false;

  if (at == am_manaboth)
    {
      bool ret = GiveAmmo(am_mana1, count) || GiveAmmo(am_mana2, count);
      return ret;
    }

  if (at < 0 || at >= NUMAMMO)
    {
      CONS_Printf ("\2P_GiveAmmo: bad type %i", at);
      return false;
    }

  if (ammo[at] >= maxammo[at])
    return false;

  if (game.skill == sk_baby || game.skill == sk_nightmare)
    {
      if (game.mode == gm_heretic || game.mode == gm_hexen)
	count += count>>1;
      else
	// give double ammo in trainer mode,
	// you'll need it in nightmare
	count <<= 1;
    }
  int oldammo = ammo[at];
  ammo[at] += count;

  if (ammo[at] > maxammo[at])
    ammo[at] = maxammo[at];

  // If non zero ammo,
  // don't change up weapons,
  // player was lower on purpose.
  if (oldammo)
    return true;

  // We were down to zero,
  // so select a new weapon.
  // Preferences are not user selectable.

  // Boris hack for preferred weapons order...
  if (!player->options.originalweaponswitch)
    {
      if (ammo[weaponinfo[readyweapon].ammo]
	  < weaponinfo[readyweapon].ammopershoot)
	UseFavoriteWeapon();
      return true;
    }
  else if (game.mode == gm_heretic)
    {
      if ((readyweapon == wp_staff || readyweapon == wp_gauntlets) 
	  && weaponowned[GetAmmoChange[at]])
	pendingweapon = GetAmmoChange[at];
    }
  else switch (at)
    {
    case am_clip:
      if (readyweapon == wp_fist)
        {
	  if (weaponowned[wp_chaingun])
	    pendingweapon = wp_chaingun;
	  else
	    pendingweapon = wp_pistol;
        }
      break;

    case am_shell:
      if (readyweapon == wp_fist
	  || readyweapon == wp_pistol)
        {
	  if (weaponowned[wp_shotgun])
	    pendingweapon = wp_shotgun;
        }
      break;

    case am_cell:
      if (readyweapon == wp_fist
	  || readyweapon == wp_pistol)
        {
	  if (weaponowned[wp_plasma])
	    pendingweapon = wp_plasma;
        }
      break;

    case am_misl:
      if (readyweapon == wp_fist)
        {
	  if (weaponowned[wp_missile])
	    pendingweapon = wp_missile;
        }
    default:
      break;
    }

  return true;
}


bool PlayerPawn::GiveWeapon(weapontype_t wt, int ammocontent, bool dropped)
{
  ammotype_t at = wpnlev1info[wt].ammo;

  if (game.multiplayer && (cv_deathmatch.value != 2) && !dropped)
    {
      // leave placed weapons forever on net games
      if (weaponowned[wt])
	return false;

      weaponowned[wt] = true;

      if (cv_deathmatch.value == 1)
	ammocontent = int(ammocontent * 5/2);

      GiveAmmo(at, ammocontent);

      // Boris hack preferred weapons order...
      if (player->options.originalweaponswitch ||
	  player->options.weaponpref[wt] > player->options.weaponpref[readyweapon])
	pendingweapon = wt; // do like Doom2 original

      S_StartAmbSound(player, sfx_weaponup);
      return false;
    }

  bool gaveammo = false;
  bool gaveweapon = false;

  if (at != am_noammo)
    gaveammo = GiveAmmo(at, ammocontent);

  if (!weaponowned[wt])
    {
      gaveweapon = true;
      weaponowned[wt] = true;
      if (player->options.originalweaponswitch ||
	  player->options.weaponpref[wt] > player->options.weaponpref[readyweapon])
	pendingweapon = wt;    // Doom2 original stuff
    }

  p_sound = sfx_weaponup;
  return (gaveweapon || gaveammo);
}


// Returns false if the armor is worse
// than the current armor.
bool PlayerPawn::GiveArmor(armortype_t type, float factor, int points)
{
  // Kludgy mess. The correct way would be making each pickup-item a separate class
  // with a Give method... same thing with weapons and artifacts
  if (factor > 0)
    {
      // new piece of armor
      if (points < 0) // means use standard Hexen armor increments
	points = ArmorIncrement[pclass][type];

      if (armorpoints[type] >= points)
	return false; // don't pick up

      armorfactor[type] = factor;
      armorpoints[type] = points;
    }
  else
    {
      // negative factor means bonus to current armor
      int i, total = int(100 * toughness);
      for (i = 0; i < NUMARMOR; i++)
	total += armorpoints[i];

      if (total >= MaxArmor[pclass])
	return false;

      if (armorfactor[type] < -factor)
	armorfactor[type] = -factor;
      armorpoints[type] += points;
    }

  return true;
}


bool PlayerPawn::GiveKey(keycard_t k)
{
  if (keycards & k)
    return false;

  keycards |= k;

  int i, j = k;
  for (i = -1; j; i++)
    j >>= 1; // count the key number
    
  player->SetMessage(text[TXT_KEY_STEEL + i]);

  p_sound = sfx_keyup;
  if (game.multiplayer) // Only remove keys in single player game
    p_remove = false;

  return true;
}


void P_SetDormantArtifact(DActor *arti);

bool PlayerPawn::GiveArtifact(artitype_t arti, DActor *from)
{
  if (arti >= NUMARTIFACTS || arti <= arti_none)
    return false;

  vector<inventory_t>::iterator i = inventory.begin();

  // find the right slot
  while (i < inventory.end() && i->type != arti)
    i++;

  if (i != inventory.end())
    {
      // player already has some of these
      if (i->count >= MAXARTECONT)
	return false;
    }

  p_remove = false;  // TODO we could just as well remove the artifacts (they will respawn in dm!)

  if (arti < arti_fsword1)
    {
      // artifact or puzzle artifact
      int j = TXT_ARTIINVULNERABILITY - arti_invulnerability + arti;

      player->SetMessage(text[j], false);
      P_SetDormantArtifact(from);
      p_sound = sfx_artiup;
      /*
	if (!game.multiplayer || cv_deathmatch.value)
	p_remove = true; // TODO remove puzzle items if not cooperative netplay?
      */
    }
  else
    {
      // weapon piece
      p_remove = true;
      int  wclass = 1 + ((arti - arti_fsword1) / 3);

      bool gave_mana = false;
      bool gave_piece = false;
      bool coop_multiplayer = (game.multiplayer && !cv_deathmatch.value);

      if (pclass != wclass)
	{
	  // wrong class, but try to pick up for mana
	  if (coop_multiplayer)
	    return false; // you can't steal other players' weapons

	  gave_mana = GiveAmmo(am_mana1, 20) || GiveAmmo(am_mana2, 20);
	}
      else 
	{
	  // right class
	  gave_piece = (i == inventory.end()); // only pick up pieces you don't have

	  if (coop_multiplayer)
	    {
	      if (!gave_piece)
		return false;

	      // TODO if you die, you lose all pieces gathered so far... leave them near the corpse?
	      GiveAmmo(am_mana1, 20);
	      GiveAmmo(am_mana2, 20);
	      p_remove = false;
	    }
	  else
	    {
	      // dm or single player game
	      gave_mana = GiveAmmo(am_mana1, 20) || GiveAmmo(am_mana2, 20);
	    }
	}

      if (!gave_mana && !gave_piece)
	return false;

      // Picked up the weapon piece (or just the mana)
      player->SetMessage(text[wclass - 1 + TXT_QUIETUS_PIECE], false);
      p_sound = sfx_weaponup;

      if (!gave_piece)
	return true;

      // TODO Automatically check if we have all pieces... For now you have to assemble the weapons yourself.
      /*
      if (all_pieces)
	{
	  int wp = wp_quietus + wclass - 1;
	  weaponowned[wp] = true;
	  pendingweapon = wp;

	  player->SetMessage(..., false);
	  // Play the build-sound full volume for all players
	  S_StartAmbSound(NULL, SFX_WEAPON_BUILD);
	}
      */
    }


  if (i == inventory.end())
    {
      // give one artifact
      inventory.push_back(inventory_t(arti, 1));
    }
  else
    {
      // player already has some of these
      if (i->count >= MAXARTECONT)
	// Player already has 16 of this item
	return false;
      
      i->count++; // one more
    }

  return true;
}
