// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: d_items.cpp 447 2007-04-06 15:31:11Z smite-meister $
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
/// \brief Game items and pickups: keys, armor, artifacts, weapons, ammo.

#include "command.h"
#include "cvars.h"
#include "dstrings.h"
#include "d_items.h"

#include "g_game.h"
#include "g_map.h"
#include "g_player.h"
#include "g_pawn.h"
#include "g_decorate.h"

#include "info.h"
#include "sounds.h"


#define BONUSADD 6


// TODO add first weapons as THINGs: goldwand 25 ammo, pistol 20 ammo, hexen first weapons 0 ammo


//======================================================
//  inventory_t implementation
//======================================================

inventory_t::inventory_t()
{
  type = count = 0;
}

inventory_t::inventory_t(byte t, byte c)
{
  type = t;
  count = c;
}


//======================================================
//  Weapon data
//======================================================

/// weapon grouping
weapon_group_t weapongroup[NUMWEAPONS] =
{
  // Doom
  {0, wp_chainsaw}, {0, wp_staff},
  {1, wp_goldwand},
  {2, wp_supershotgun}, {2, wp_crossbow},
  {3, wp_blaster},
  {4, wp_phoenixrod},
  {5, wp_skullrod},
  {6, wp_mace},

  // Heretic
  {0, wp_gauntlets}, {0, wp_fpunch},
  {1, wp_timons_axe},
  {2, wp_hammer_of_retribution},
  {3, wp_quietus},
  {4, wp_missile},
  {5, wp_plasma},
  {6, wp_bfg},
  {7, wp_snout},

  // Hexen
  {0, wp_cmace}, {0, wp_mwand}, {0, wp_fist},
  {1, wp_serpent_staff}, {1, wp_cone_of_shards}, {1, wp_pistol},
  {2, wp_firestorm}, {2, wp_arc_of_death}, {2, wp_shotgun},
  {3, wp_wraithverge}, {3, wp_bloodscourge}, {3, wp_chaingun},
  {7, wp_beak}
};


/// Max ammo capacity for players
int maxammo1[NUMAMMO] =
{
  200, 50, 300, 50, // Doom
  100, 50, 200, 200, 20, 150,  // Heretic
  200, 200 // Hexen
};

int maxammo2[NUMAMMO] =
{
  400, 100, 600, 100, // Doom
  200, 100, 400, 400, 40, 300, // Heretic
  200, 200 // Hexen
};


/// Weapon info, weapon animations
weaponinfo_t wpnlev1info[NUMWEAPONS] =
{
  // Doom weapons
  {am_noammo, 0, S_PUNCHUP, S_PUNCHDOWN, S_PUNCH, S_PUNCH1, S_PUNCH1, S_WNULL},             // fist
  {am_noammo, 0, S_SAWUP, S_SAWDOWN, S_SAW, S_SAW1, S_SAW1, S_WNULL},      // chainsaw
  {am_clip,   1, S_PISTOLUP, S_PISTOLDOWN, S_PISTOL, S_PISTOL1, S_PISTOL1, S_PISTOLFLASH}, // pistol
  {am_shell,  1, S_SGUNUP, S_SGUNDOWN, S_SGUN, S_SGUN1, S_SGUN1, S_SGUNFLASH1},            // shotgun
  {am_shell,  2, S_DSGUNUP, S_DSGUNDOWN, S_DSGUN, S_DSGUN1, S_DSGUN1, S_DSGUNFLASH1}, // super shotgun
  {am_clip,   1, S_CHAINUP, S_CHAINDOWN, S_CHAIN, S_CHAIN1, S_CHAIN1, S_CHAINFLASH1},      // chaingun
  {am_misl,   1, S_MISSILEUP, S_MISSILEDOWN, S_MISSILE, S_MISSILE1, S_MISSILE1, S_MISSILEFLASH1}, // missile launcher
  {am_cell,   1, S_PLASMAUP, S_PLASMADOWN, S_PLASMA, S_PLASMA1, S_PLASMA1, S_PLASMAFLASH1},       // plasma rifle
  {am_cell,  40, S_BFGUP, S_BFGDOWN, S_BFG, S_BFG1, S_BFG1, S_BFGFLASH1}, // bfg 9000

  // Heretic weapons
  {am_noammo,   0, S_STAFFUP, S_STAFFDOWN, S_STAFFREADY, S_STAFFATK1_1, S_STAFFATK1_1, S_WNULL}, // Staff
  {am_noammo,   0, S_GAUNTLETUP, S_GAUNTLETDOWN, S_GAUNTLETREADY, S_GAUNTLETATK1_1, S_GAUNTLETATK1_3, S_WNULL}, // Gauntlets
  {am_goldwand, 1, S_GOLDWANDUP, S_GOLDWANDDOWN, S_GOLDWANDREADY, S_GOLDWANDATK1_1, S_GOLDWANDATK1_1, S_WNULL}, // Gold wand
  {am_crossbow, 1, S_CRBOWUP, S_CRBOWDOWN, S_CRBOW1, S_CRBOWATK1_1, S_CRBOWATK1_1, S_WNULL}, // Crossbow
  {am_blaster,  1, S_BLASTERUP, S_BLASTERDOWN, S_BLASTERREADY, S_BLASTERATK1_1, S_BLASTERATK1_3, S_WNULL}, // Blaster
  {am_phoenixrod, 1, S_PHOENIXUP, S_PHOENIXDOWN, S_PHOENIXREADY, S_PHOENIXATK1_1, S_PHOENIXATK1_1, S_WNULL}, // Phoenix rod
  {am_skullrod, 1, S_HORNRODUP, S_HORNRODDOWN, S_HORNRODREADY, S_HORNRODATK1_1, S_HORNRODATK1_1, S_WNULL}, // Skull rod
  {am_mace,     1, S_MACEUP, S_MACEDOWN, S_MACEREADY, S_MACEATK1_1, S_MACEATK1_2, S_WNULL}, // Mace
  {am_noammo,   0, S_BEAKUP, S_BEAKDOWN, S_BEAKREADY, S_BEAKATK1_1, S_BEAKATK1_1, S_WNULL}, // Beak

  // Hexen weapons
  {am_noammo, 0, S_XPUNCHUP, S_XPUNCHDOWN, S_XPUNCHREADY, S_PUNCHATK1_1, S_PUNCHATK1_1, S_WNULL}, // Fighter - Punch
  {am_noammo, 0, S_CMACEUP, S_CMACEDOWN, S_CMACEREADY, S_CMACEATK_1, S_CMACEATK_1, S_WNULL},   // Cleric - Mace
  {am_noammo, 0, S_MWANDUP, S_MWANDDOWN, S_MWANDREADY, S_MWANDATK_1, S_MWANDATK_1, S_WNULL },  // Mage - Wand
  {am_noammo, 2, S_FAXEUP, S_FAXEDOWN, S_FAXEREADY, S_FAXEATK_1, S_FAXEATK_1, S_WNULL},        // Fighter - Axe
  {am_mana1,  1, S_CSTAFFUP, S_CSTAFFDOWN, S_CSTAFFREADY, S_CSTAFFATK_1, S_CSTAFFATK_1, S_WNULL}, // Cleric - Serpent Staff
  {am_mana1,  3, S_CONEUP, S_CONEDOWN, S_CONEREADY, S_CONEATK1_1, S_CONEATK1_3, S_WNULL},         // Mage - Cone of shards
  {am_noammo, 3, S_FHAMMERUP, S_FHAMMERDOWN, S_FHAMMERREADY, S_FHAMMERATK_1, S_FHAMMERATK_1, S_WNULL}, // Fighter - Hammer
  {am_mana2,  4, S_CFLAMEUP, S_CFLAMEDOWN, S_CFLAMEREADY1, S_CFLAMEATK_1, S_CFLAMEATK_1, S_WNULL}, // Cleric - Flame Strike
  {am_mana2,  5, S_MLIGHTNINGUP, S_MLIGHTNINGDOWN, S_MLIGHTNINGREADY, S_MLIGHTNINGATK_1, S_MLIGHTNINGATK_1, S_WNULL}, // Mage - Lightning
  {am_manaboth, 14, S_FSWORDUP, S_FSWORDDOWN, S_FSWORDREADY, S_FSWORDATK_1, S_FSWORDATK_1, S_WNULL}, // Fighter - Rune Sword
  {am_manaboth, 18, S_CHOLYUP, S_CHOLYDOWN, S_CHOLYREADY, S_CHOLYATK_1, S_CHOLYATK_1, S_WNULL},      // Cleric - Holy Symbol
  {am_manaboth, 15, S_MSTAFFUP, S_MSTAFFDOWN, S_MSTAFFREADY, S_MSTAFFATK_1, S_MSTAFFATK_1, S_WNULL}, // Mage - Staff
  {am_noammo, 0, S_SNOUTUP, S_SNOUTDOWN, S_SNOUTREADY, S_SNOUTATK1, S_SNOUTATK1, S_WNULL} // Pig - Snout
};


// right now Doom and Hexen weapons are exactly the same when tome of power is active, but who knows...
weaponinfo_t wpnlev2info[NUMWEAPONS] =
{
  // Doom weapons
  {am_noammo, 0, S_PUNCHUP, S_PUNCHDOWN, S_PUNCH, S_PUNCH1, S_PUNCH1, S_WNULL},             // fist
  {am_noammo, 0, S_SAWUP, S_SAWDOWN, S_SAW, S_SAW1, S_SAW1, S_WNULL},      // chainsaw
  {am_clip,   1, S_PISTOLUP, S_PISTOLDOWN, S_PISTOL, S_PISTOL1, S_PISTOL1, S_PISTOLFLASH}, // pistol
  {am_shell,  1, S_SGUNUP, S_SGUNDOWN, S_SGUN, S_SGUN1, S_SGUN1, S_SGUNFLASH1},            // shotgun
  {am_shell,  2, S_DSGUNUP, S_DSGUNDOWN, S_DSGUN, S_DSGUN1, S_DSGUN1, S_DSGUNFLASH1}, // super shotgun
  {am_clip,   1, S_CHAINUP, S_CHAINDOWN, S_CHAIN, S_CHAIN1, S_CHAIN1, S_CHAINFLASH1},      // chaingun
  {am_misl,   1, S_MISSILEUP, S_MISSILEDOWN, S_MISSILE, S_MISSILE1, S_MISSILE1, S_MISSILEFLASH1}, // missile launcher
  {am_cell,   1, S_PLASMAUP, S_PLASMADOWN, S_PLASMA, S_PLASMA1, S_PLASMA1, S_PLASMAFLASH1},       // plasma rifle
  {am_cell,  40, S_BFGUP, S_BFGDOWN, S_BFG, S_BFG1, S_BFG1, S_BFGFLASH1}, // bfg 9000

  // Heretic weapons
  {am_noammo,   0, S_STAFFUP2, S_STAFFDOWN2, S_STAFFREADY2_1, S_STAFFATK2_1, S_STAFFATK2_1, S_WNULL}, // Staff
  {am_noammo,   0, S_GAUNTLETUP2, S_GAUNTLETDOWN2, S_GAUNTLETREADY2_1, S_GAUNTLETATK2_1, S_GAUNTLETATK2_3, S_WNULL}, // Gauntlets
  {am_goldwand, 1, S_GOLDWANDUP, S_GOLDWANDDOWN, S_GOLDWANDREADY, S_GOLDWANDATK2_1, S_GOLDWANDATK2_1, S_WNULL}, // Gold wand
  {am_crossbow, 1, S_CRBOWUP, S_CRBOWDOWN, S_CRBOW1, S_CRBOWATK2_1, S_CRBOWATK2_1, S_WNULL}, // Crossbow
  {am_blaster,  5, S_BLASTERUP, S_BLASTERDOWN, S_BLASTERREADY, S_BLASTERATK2_1, S_BLASTERATK2_3, S_WNULL}, // Blaster
  {am_phoenixrod, 1, S_PHOENIXUP, S_PHOENIXDOWN, S_PHOENIXREADY, S_PHOENIXATK2_1, S_PHOENIXATK2_2, S_WNULL}, // Phoenix rod
  {am_skullrod, 5, S_HORNRODUP, S_HORNRODDOWN, S_HORNRODREADY, S_HORNRODATK2_1, S_HORNRODATK2_1, S_WNULL}, // Skull rod
  {am_mace,     5, S_MACEUP, S_MACEDOWN, S_MACEREADY, S_MACEATK2_1, S_MACEATK2_1, S_WNULL}, // Mace
  {am_noammo,   0, S_BEAKUP, S_BEAKDOWN, S_BEAKREADY, S_BEAKATK2_1, S_BEAKATK2_1, S_WNULL}, // Beak

  // Hexen weapons
  {am_noammo, 0, S_XPUNCHUP, S_XPUNCHDOWN, S_XPUNCHREADY, S_PUNCHATK1_1, S_PUNCHATK1_1, S_WNULL}, // Fighter - Punch
  {am_noammo, 0, S_CMACEUP, S_CMACEDOWN, S_CMACEREADY, S_CMACEATK_1, S_CMACEATK_1, S_WNULL},   // Cleric - Mace
  {am_noammo, 0, S_MWANDUP, S_MWANDDOWN, S_MWANDREADY, S_MWANDATK_1, S_MWANDATK_1, S_WNULL },  // Mage - Wand
  {am_noammo, 2, S_FAXEUP, S_FAXEDOWN, S_FAXEREADY, S_FAXEATK_1, S_FAXEATK_1, S_WNULL},        // Fighter - Axe
  {am_mana1,  1, S_CSTAFFUP, S_CSTAFFDOWN, S_CSTAFFREADY, S_CSTAFFATK_1, S_CSTAFFATK_1, S_WNULL}, // Cleric - Serpent Staff
  {am_mana1,  3, S_CONEUP, S_CONEDOWN, S_CONEREADY, S_CONEATK1_1, S_CONEATK1_3, S_WNULL},         // Mage - Cone of shards
  {am_noammo, 3, S_FHAMMERUP, S_FHAMMERDOWN, S_FHAMMERREADY, S_FHAMMERATK_1, S_FHAMMERATK_1, S_WNULL}, // Fighter - Hammer
  {am_mana2,  4, S_CFLAMEUP, S_CFLAMEDOWN, S_CFLAMEREADY1, S_CFLAMEATK_1, S_CFLAMEATK_1, S_WNULL}, // Cleric - Flame Strike
  {am_mana2,  5, S_MLIGHTNINGUP, S_MLIGHTNINGDOWN, S_MLIGHTNINGREADY, S_MLIGHTNINGATK_1, S_MLIGHTNINGATK_1, S_WNULL}, // Mage - Lightning
  {am_manaboth, 14, S_FSWORDUP, S_FSWORDDOWN, S_FSWORDREADY, S_FSWORDATK_1, S_FSWORDATK_1, S_WNULL}, // Fighter - Rune Sword
  {am_manaboth, 18, S_CHOLYUP, S_CHOLYDOWN, S_CHOLYREADY, S_CHOLYATK_1, S_CHOLYATK_1, S_WNULL},      // Cleric - Holy Symbol
  {am_manaboth, 15, S_MSTAFFUP, S_MSTAFFDOWN, S_MSTAFFREADY, S_MSTAFFATK_1, S_MSTAFFATK_1, S_WNULL}, // Mage - Staff
  {am_noammo, 0, S_SNOUTUP, S_SNOUTDOWN, S_SNOUTREADY, S_SNOUTATK1, S_SNOUTATK1, S_WNULL} // Pig - Snout
};




//======================================================
//   Picking things up
//======================================================


//---------------------------------------------------------------------------
// The Heretic/Hexen artifact and item respawn system.
// The artifact is restored after a number of tics by an action function.
//---------------------------------------------------------------------------

void P_SetDormantArtifact(DActor *arti)
{
  arti->flags &= ~MF_SPECIAL; // can no longer be picked up
  if (cv_deathmatch.value && !(arti->flags & MF_DROPPED))
    {
      // three different respawn delays
      switch (arti->type)
	{
	case MT_ARTIINVULNERABILITY: // In Heretic, these two did not respawn at all...
	case MT_ARTIINVISIBILITY:

	case MT_XARTIINVULNERABILITY:
	  arti->SetState(S_DORMANTARTI3_1); // 600 s.
	  break;
	  
	case MT_SUMMONMAULATOR:
	case MT_XARTIFLY:
	  arti->SetState(S_DORMANTARTI2_1); // 120 s.
	  break;

	default:
	  arti->SetState(S_DORMANTARTI1_1); // 40 s. Identical to the Heretic state sequence S_DORMANTARTI1.
	  break;
	}
    }
  else
    arti->SetState(S_XDEADARTI1); // Don't respawn at all. Identical to Heretic sequence S_DEADARTI1.
}


void A_RestoreArtifact(DActor *arti)
{
  arti->flags |= MF_SPECIAL;
  arti->SetState(arti->info->spawnstate);
  S_StartSound(arti, sfx_itemrespawn);
}


//  The Heretic way of respawning items. Unused.
void P_HideSpecialThing(DActor *thing)
{
  thing->flags &= ~MF_SPECIAL;
  thing->flags2 |= MF2_DONTDRAW;
  thing->SetState(S_HIDESPECIAL1);
}

// Make a special thing visible again.
void A_RestoreSpecialThing1(DActor *thing)
{
  if (thing->type == MT_WMACE)
    { // Do random mace placement
      thing->mp->RepositionMace(thing);
    }
  thing->flags2 &= ~MF2_DONTDRAW;
  S_StartSound(thing, sfx_itemrespawn);
}

// And finally make it a pickup again.
void A_RestoreSpecialThing2(DActor *thing)
{
  thing->flags |= MF_SPECIAL;
  thing->SetState(thing->info->spawnstate);
}



int  p_sound;  // pickupsound
bool p_remove; // should the stuff be removed?


void PlayerPawn::TouchSpecialThing(DActor *thing)
{                  
  // Dead thing touching.
  // Can happen with a sliding player corpse.
  if (health <= 0 || flags & MF_CORPSE)
    return;

  p_remove = true; // should the item be removed from map?
  p_sound = sfx_itemup;

  int stype = thing->type;
  int amount = thing->health; // item amounts are stored in health
  float quality = thing->info->speed; // "quality" is stored in speed
  bool dropped = thing->flags & MF_DROPPED;

  // Identify item

  int script = thing->info->damage; // this field holds the script number
  if (script)
    {
      // TEST: New gettable things with scripting! These override any original behavior.
      mp->FS_RunScript(script, this); // too bad FS can't use parameters, maybe we should use ACS instead...
    }
  else switch (stype)
    {
    case MT_ARMOR_1:
      if (!GiveArmor(armor_armor, 3.0, -1))
	return;
      player->SetMessage(text[TXT_ARMOR1]);
      break;
    case MT_ARMOR_2:
      if (!GiveArmor(armor_shield, 3.0, -1))
	return;
      player->SetMessage(text[TXT_ARMOR2]);
      break;
    case MT_ARMOR_3:
      if (!GiveArmor(armor_helmet, 3.0, -1))
	return;
      player->SetMessage(text[TXT_ARMOR3]);
      break;
    case MT_ARMOR_4:
      if (!GiveArmor(armor_amulet, 3.0, -1))
	return;
      player->SetMessage(text[TXT_ARMOR4]);
      break;

    case MT_ITEMSHIELD1:
    case MT_ITEMSHIELD2:
      if (!GiveArmor(armor_field, quality, amount))
	return;
      player->SetMessage(text[stype - MT_ITEMSHIELD1 + TXT_ITEMSHIELD1]);
      break;

    case MT_GREENARMOR:
    case MT_BLUEARMOR:
      if (!GiveArmor(armor_field, quality, amount))
	return;
      player->SetMessage(text[stype - MT_GREENARMOR + TXT_GOTARMOR]);
      break;

    case MT_HEALTHBONUS:  // health bonus
      health += amount;   // can go over 100%
      if (health > 2*maxhealth)
	health = 2*maxhealth;
      player->SetMessage(GOTHTHBONUS, 2);
      break;

    case MT_ARMORBONUS:  // spirit armor
      GiveArmor(armor_field, -quality, amount);
      player->SetMessage(GOTARMBONUS, 2);
      break;

    case MT_SOULSPHERE:
      health += amount;
      if (health > 2*maxhealth)
	health = 2*maxhealth;
      player->SetMessage(GOTSUPER);
      p_sound = sfx_powerup;
      break;

    case MT_MEGA:
      health += amount;
      if (health > 2*maxhealth)
	health = 2*maxhealth;
      GiveArmor(armor_field, quality, amount);
      player->SetMessage(GOTMSPHERE);
      p_sound = sfx_powerup;
      break;

      // keys
    case MT_KEY1:
    case MT_KEY2:
    case MT_KEY3:
    case MT_KEY4:
    case MT_KEY5:
    case MT_KEY6:
    case MT_KEY7:
    case MT_KEY8:
    case MT_KEY9:
    case MT_KEYA:
    case MT_KEYB:
      if (!GiveKey(keycard_t(1 << (stype - MT_KEY1))))
	return;
      break;

      // leave cards for everyone
    case MT_BKEY: // Key_Blue
    case MT_BLUECARD:
      if (!GiveKey(it_bluecard))
	return;
      break;

    case MT_CKEY: // Key_Yellow
    case MT_YELLOWCARD:
      if (!GiveKey(it_yellowcard))
	return;
      break;

    case MT_AKEY: // Key_Green
    case MT_REDCARD:
      if (!GiveKey(it_redcard))
	return;
      break;

    case MT_BLUESKULL:
      if (!GiveKey(it_blueskull))
	return;
      break;

    case MT_YELLOWSKULL:
      if (!GiveKey(it_yellowskull))
        return;
      break;

    case MT_REDSKULL:
      if (!GiveKey(it_redskull))
        return;
      break;

      // medikits, heals
    case MT_STIM:
      if (!GiveBody(amount))
	return;
      player->SetMessage(GOTSTIM, 1);
      break;

    case MT_XHEALINGBOTTLE:
    case MT_HEALINGBOTTLE:
      if (!GiveBody(amount))
	return;
      player->SetMessage(text[TXT_ITEMHEALTH], 1);
      break;

    case MT_MEDI:
      if (!GiveBody(amount))
	return;
      if (health < 25)
	player->SetMessage(GOTMEDINEED, 1);
      else
	player->SetMessage(GOTMEDIKIT, 1);
      break;

      // Artifacts :
    case MT_XHEALTHFLASK:
    case MT_HEALTHFLASK:
      if (!GiveArtifact(arti_health, thing))
	return;
      break;
    case MT_XARTIFLY:
    case MT_ARTIFLY:
      if (!GiveArtifact(arti_fly, thing))
	return;
      break;
    case MT_ARTIINVULNERABILITY:
      if (!GiveArtifact(arti_invulnerability, thing))
	return;
      break;
    case MT_ARTITOMEOFPOWER:
      if (!GiveArtifact(arti_tomeofpower, thing))
	return;
      break;
    case MT_ARTIINVISIBILITY:
      if (!GiveArtifact(arti_invisibility, thing))
	return;
      break;
    case MT_ARTIEGG:
      if (!GiveArtifact(arti_egg, thing))
	return;
      break;
    case MT_XARTISUPERHEAL:
    case MT_ARTISUPERHEAL:
      if (!GiveArtifact(arti_superhealth, thing))
	return;
      break;
    case MT_XARTITORCH:
    case MT_ARTITORCH:
      if (!GiveArtifact(arti_torch, thing))
	return;
      break;
    case MT_ARTIFIREBOMB:
      if (!GiveArtifact(arti_firebomb, thing))
	return;
      break;
    case MT_XARTITELEPORT:
    case MT_ARTITELEPORT:
      if (!GiveArtifact(arti_teleport, thing))
	return;
      break;

    case MT_SUMMONMAULATOR:
      if (!GiveArtifact(arti_summon, thing))
	return;
      break;
    case MT_XARTIEGG:
      if (!GiveArtifact(arti_pork, thing))
	return;
      break;
    case MT_HEALRADIUS:
      if (!GiveArtifact(arti_healingradius, thing))
	return;
      break;
    case MT_TELEPORTOTHER:
      if (!GiveArtifact(arti_teleportother, thing))
	return;
      break;
    case MT_ARTIPOISONBAG:
      if (!GiveArtifact(arti_poisonbag, thing))
	return;
      break;
    case MT_SPEEDBOOTS:
      if (!GiveArtifact(arti_speed, thing))
	return;
      break;
    case MT_BOOSTMANA:
      if (!GiveArtifact(arti_boostmana, thing))
	return;
      break;
    case MT_BOOSTARMOR:
      if (!GiveArtifact(arti_boostarmor, thing))
	return;
      break;
    case MT_BLASTRADIUS:
      if (!GiveArtifact(arti_blastradius, thing))
	return;
      break;
    case MT_XARTIINVULNERABILITY:
      if (!GiveArtifact(arti_xinvulnerability, thing))
	return;
      break;

      // Puzzle artifacts
    case MT_ARTIPUZZSKULL:
    case MT_ARTIPUZZGEMBIG:
    case MT_ARTIPUZZGEMRED:
    case MT_ARTIPUZZGEMGREEN1:
    case MT_ARTIPUZZGEMGREEN2:
    case MT_ARTIPUZZGEMBLUE1:
    case MT_ARTIPUZZGEMBLUE2:
    case MT_ARTIPUZZBOOK1:
    case MT_ARTIPUZZBOOK2:
    case MT_ARTIPUZZSKULL2:
    case MT_ARTIPUZZFWEAPON:
    case MT_ARTIPUZZCWEAPON:
    case MT_ARTIPUZZMWEAPON:
    case MT_ARTIPUZZGEAR:
    case MT_ARTIPUZZGEAR2:
    case MT_ARTIPUZZGEAR3:
    case MT_ARTIPUZZGEAR4:
      if (!GiveArtifact(artitype_t(arti_puzzskull + stype - MT_ARTIPUZZSKULL), thing))
	return;
      break;

      // power ups
    case MT_INV:
      if (!GivePower(pw_invulnerability))
	return;
      player->SetMessage(GOTINVUL);
      break;

    case MT_BERSERKPACK:
      if (!GivePower(pw_strength))
	return;
      player->SetMessage(GOTBERSERK);
      if (readyweapon != wp_fist)
	pendingweapon = wp_fist;
      break;

    case MT_INS:
      if (!GivePower(pw_invisibility))
	return;
      player->SetMessage(GOTINVIS);
      break;

    case MT_RADSUIT:
      if (!GivePower(pw_ironfeet))
	return;
      player->SetMessage(GOTSUIT);
      break;

    case MT_MAPSCROLL:
    case MT_COMPUTERMAP:
      if (!GivePower(pw_allmap))
	return;
      if (stype == MT_MAPSCROLL)
	player->SetMessage(text[TXT_ITEMSUPERMAP]);
      else
	player->SetMessage(GOTMAP);
      break;

    case MT_IRVISOR:
      if (!GivePower(pw_infrared))
	return;
      player->SetMessage(GOTVISOR);
      break;

      // Mana
    case MT_MANA1:
      if (!GiveAmmo(am_mana1, amount))
	return;
      player->SetMessage(text[TXT_MANA_1]);
      break;
    case MT_MANA2:
      if (!GiveAmmo(am_mana2, amount))
	return;
      player->SetMessage(text[TXT_MANA_2]);
      break;
    case MT_MANA3:
      if (GiveAmmo(am_mana1, amount))
	{
	  if (!GiveAmmo(am_mana2, amount))
	    return;
	}
      else
	GiveAmmo(am_mana2, amount);
      player->SetMessage(text[TXT_MANA_BOTH]);
      break;

      // heretic Ammo
    case MT_AMGWNDWIMPY:
      if (!GiveAmmo(am_goldwand, amount))
	return;
      player->SetMessage(GOT_AMMOGOLDWAND1, 1);
      break;

    case MT_AMGWNDHEFTY:
      if (!GiveAmmo(am_goldwand, amount))
	return;
      player->SetMessage(GOT_AMMOGOLDWAND2, 1);
      break;

    case MT_AMMACEWIMPY:
      if (!GiveAmmo(am_mace, amount))
	return;
      player->SetMessage(GOT_AMMOMACE1, 1);
      break;

    case MT_AMMACEHEFTY:
      if (!GiveAmmo(am_mace, amount))
	return;
      player->SetMessage(GOT_AMMOMACE2, 1);
      break;

    case MT_AMCBOWWIMPY:
      if (!GiveAmmo(am_crossbow, amount))
	return;
      player->SetMessage(GOT_AMMOCROSSBOW1, 1);
      break;

    case MT_AMCBOWHEFTY:
      if (!GiveAmmo(am_crossbow, amount))
	return;
      player->SetMessage(GOT_AMMOCROSSBOW2, 1);
      break;

    case MT_AMBLSRWIMPY:
      if (!GiveAmmo(am_blaster, amount))
	return;
      player->SetMessage(GOT_AMMOBLASTER1, 1);
      break;

    case MT_AMBLSRHEFTY:
      if (!GiveAmmo(am_blaster, amount))
	return;
      player->SetMessage(GOT_AMMOBLASTER2, 1);
      break;

    case MT_AMSKRDWIMPY:
      if (!GiveAmmo(am_skullrod, amount))
	return;
      player->SetMessage(GOT_AMMOSKULLROD1, 1);
      break;

    case MT_AMSKRDHEFTY:
      if (!GiveAmmo(am_skullrod, amount))
	return;
      player->SetMessage(GOT_AMMOSKULLROD2, 1);
      break;

    case MT_AMPHRDWIMPY:
      if (!GiveAmmo(am_phoenixrod, amount))
	return;
      player->SetMessage(GOT_AMMOPHOENIXROD1, 1);
      break;

    case MT_AMPHRDHEFTY:
      if (!GiveAmmo(am_phoenixrod, amount))
	return;
      player->SetMessage(GOT_AMMOPHOENIXROD2, 1);
      break;

      // doom ammo
    case MT_CLIP:
      if (!GiveAmmo(am_clip, amount))
	return;
      player->SetMessage(GOTCLIP, 1);
      break;

    case MT_AMMOBOX:
      if (!GiveAmmo(am_clip, amount))
	return;
      player->SetMessage(GOTCLIPBOX, 1);
      break;

    case MT_ROCKETAMMO:
      if (!GiveAmmo(am_misl, amount))
	return;
      player->SetMessage(GOTROCKET, 1);
      break;

    case MT_ROCKETBOX:
      if (!GiveAmmo(am_misl, amount))
	return;
      player->SetMessage(GOTROCKBOX, 1);
      break;

    case MT_CELL:
      if (!GiveAmmo(am_cell, amount))
	return;
      player->SetMessage(GOTCELL, 1);
      break;

    case MT_CELLPACK:
      if (!GiveAmmo(am_cell, amount))
	return;
      player->SetMessage(GOTCELLBOX, 1);
      break;

    case MT_SHELL:
      if (!GiveAmmo(am_shell, amount))
	return;
      player->SetMessage(GOTSHELLS, 1);
      break;

    case MT_SHELLBOX:
      if (!GiveAmmo(am_shell, amount))
	return;
      player->SetMessage(GOTSHELLBOX, 1);
      break;

    case MT_BACKPACK:
      for (int j=0; j<NUMAMMO; j++)
	maxammo[j] = max(maxammo[j], maxammo2[j]);

      GiveAmmo(am_clip, mobjinfo[MT_CLIP].spawnhealth);
      GiveAmmo(am_shell, mobjinfo[MT_SHELL].spawnhealth);
      GiveAmmo(am_cell, mobjinfo[MT_CELL].spawnhealth);
      GiveAmmo(am_misl, mobjinfo[MT_ROCKETAMMO].spawnhealth);
      player->SetMessage(GOTBACKPACK);
      break;

    case MT_BAGOFHOLDING:
      for (int j=0; j<NUMAMMO; j++)
	maxammo[j] = max(maxammo[j], maxammo2[j]);

      GiveAmmo(am_goldwand, mobjinfo[MT_AMGWNDWIMPY].spawnhealth);
      GiveAmmo(am_blaster, mobjinfo[MT_AMBLSRWIMPY].spawnhealth);
      GiveAmmo(am_crossbow, mobjinfo[MT_AMCBOWWIMPY].spawnhealth);
      GiveAmmo(am_skullrod, mobjinfo[MT_AMSKRDWIMPY].spawnhealth);
      GiveAmmo(am_phoenixrod, mobjinfo[MT_AMPHRDWIMPY].spawnhealth);
      player->SetMessage(text[TXT_ITEMBAGOFHOLDING]);
      break;

        // weapons
    case MT_BFG9000:
      if (!GiveWeapon(wp_bfg, amount, dropped))
	return;
      player->SetMessage(GOTBFG9000);
      break;
    case MT_CHAINGUN:
      if (!GiveWeapon(wp_chaingun, amount, dropped))
	return;
      player->SetMessage(GOTCHAINGUN);
      break;
    case MT_SHAINSAW:
      if (!GiveWeapon(wp_chainsaw, amount, dropped))
	return;
      player->SetMessage(GOTCHAINSAW);
      break;
    case MT_ROCKETLAUNCH:
      if (!GiveWeapon(wp_missile, amount, dropped))
	return;
      player->SetMessage(GOTLAUNCHER);
      break;
    case MT_PLASMAGUN:
      if (!GiveWeapon(wp_plasma, amount, dropped))
	return;
      player->SetMessage(GOTPLASMA);
      break;
    case MT_SHOTGUN:
      if (!GiveWeapon(wp_shotgun, amount, dropped))
	return;
      player->SetMessage(GOTSHOTGUN);
      break;
    case MT_SUPERSHOTGUN:
      if (!GiveWeapon(wp_supershotgun, amount, dropped))
	return;
      player->SetMessage(GOTSHOTGUN2);
      break;

      // heretic weapons
    case MT_WMACE:
      if (!GiveWeapon(wp_mace, amount, dropped))
	return;
      player->SetMessage(GOT_WPNMACE);
      break;
    case MT_WCROSSBOW:
      if (!GiveWeapon(wp_crossbow, amount, dropped))
	return;
      player->SetMessage(GOT_WPNCROSSBOW);
      break;
    case MT_WBLASTER:
      if (!GiveWeapon(wp_blaster, amount, dropped))
	return;
      player->SetMessage(GOT_WPNBLASTER);
      break;
    case MT_WSKULLROD:
      if (!GiveWeapon(wp_skullrod, amount, dropped))
	return;
      player->SetMessage(GOT_WPNSKULLROD);
      break;
    case MT_WPHOENIXROD:
      if (!GiveWeapon(wp_phoenixrod, amount, dropped))
	return;
      player->SetMessage(GOT_WPNPHOENIXROD);
      break;
    case MT_WGAUNTLETS:
      if (!GiveWeapon(wp_gauntlets, amount, dropped))
	return;
      player->SetMessage(GOT_WPNGAUNTLETS);
      break;

      // Hexen weapons
    case MT_MW_CONE:
      if (!GiveWeapon(wp_cone_of_shards, amount, dropped))
	return;
      player->SetMessage(text[TXT_WEAPON_M2]);
      break;
    case MT_MW_LIGHTNING:
      if (!GiveWeapon(wp_arc_of_death, amount, dropped))
	return;
      player->SetMessage(text[TXT_WEAPON_M3]);
      break;
    case MT_FW_AXE:
      if (!GiveWeapon(wp_timons_axe, amount, dropped))
	return;
      player->SetMessage(text[TXT_WEAPON_F2]);
      break;
    case MT_FW_HAMMER:
      if (!GiveWeapon(wp_hammer_of_retribution, amount, dropped))
	return;
      player->SetMessage(text[TXT_WEAPON_F3]);
      break;
    case MT_CW_SERPSTAFF:
      if (!GiveWeapon(wp_serpent_staff, amount, dropped))
	return;
      player->SetMessage(text[TXT_WEAPON_C2]);
      break;
    case MT_CW_FLAME:
      if (!GiveWeapon(wp_firestorm, amount, dropped))
	return;
      player->SetMessage(text[TXT_WEAPON_C3]);
      break;

      // Fourth Weapon Pieces
    case MT_FW_SWORD1:
    case MT_FW_SWORD2:
    case MT_FW_SWORD3:
    case MT_CW_HOLY1:
    case MT_CW_HOLY2:
    case MT_CW_HOLY3:
    case MT_MW_STAFF1:
    case MT_MW_STAFF2:
    case MT_MW_STAFF3:
      if (!GiveArtifact(artitype_t(arti_fsword1 + stype - MT_FW_SWORD1), thing))
	return;
      break;

    default:
      {
	CONS_Printf("\2TouchSpecialThing: Unknown pickup type (%d)!\n", stype);
	return;
      }
    }

  if (thing->flags & MF_COUNTITEM)
    player->items++;

  player->bonuscount += BONUSADD;

  S_StartAmbSound(player, p_sound);

  // pickup special (Hexen)
  if (thing->special)
    {
      mp->ExecuteLineSpecial(thing->special, thing->args, NULL, 0, this);
      thing->special = 0;
    }

  if (p_remove)
    {
      // respawning items (actually their spawnpoints) are added to queue
      if (!dropped && !(thing->flags & MF_NORESPAWN) && cv_itemrespawn.value)
	{
	  mp->itemrespawnqueue.push_back(thing->spawnpoint);
	  mp->itemrespawntime.push_back(mp->maptic);
	}

      thing->Remove();
    }
}
