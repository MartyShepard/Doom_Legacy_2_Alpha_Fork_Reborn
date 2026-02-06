// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: g_pawn.h 483 2007-07-21 19:31:43Z smite-meister $
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
/// \brief Pawn and PlayerPawn classes.

#ifndef g_pawn_h
#define g_pawn_h 1

#include <vector>

#include "g_actor.h" // Actor class
#include "g_decorate.h"
#include "d_items.h"
#include "p_pspr.h" // 1st person weapon sprites

using namespace std;


/// Player internal flags, for cheats and debug. Must fit into an int.
enum cheat_t
{
  CF_NOCLIP      = 1,  // No clipping, walk through barriers.
  CF_GODMODE     = 2,  // No damage, no health loss.
  CF_NOMOMENTUM  = 4,  // Not really a cheat, just a debug aid.
  CF_FLYAROUND   = 8,  // Fly using jump key
};

/// Player class, from Hexen.
enum pclass_t
{
  PCLASS_NONE = 0,
  PCLASS_FIGHTER,
  PCLASS_CLERIC,
  PCLASS_MAGE,
  PCLASS_PIG,
  NUMCLASSES
};



/// \brief Pawn type definition. Each instant defines a DECORATE class.
/// \ingroup g_thing
/*!
  We need to inherit ActorInfo because sprite presentations require the state information.
 */
class PawnAI : public ActorInfo
{
public:
  
  byte         pclass;  ///< player class, a Hexen kludge
  weapontype_t bweapon; ///< beginning weapon (besides fist/staff)
  int          bammo;   ///< ammo for bweapon

  int     gruntsound; ///< hitting ground at high speed
  int  puzzfailsound; ///< failing to solve a puzzle
  int burndeathsound; ///< fire death

public:
  PawnAI(const ActorInfo& base, pclass_t cls, weapontype_t bw);
};

/// All known pawn types.
extern vector<PawnAI*> pawn_aid;



/// \brief An Actor that has active AI or human control.
/*!
  Currently this is an unnecessary layer between Actor and PlayerPawn, but in future
  we might want to use this to define monsters with complex AI.
 */
class Pawn : public Actor
{
  TNL_DECLARE_CLASS(Pawn);
  DECLARE_CLASS(Pawn)
public:
  const class PawnAI *info; ///< Pawn type.

  class skin_t *skin; // TEMP

  byte  pclass;    ///< Current player class, a Hexen kludge. Affects many things.
  int   maxhealth; ///< Maximum health value.
  float speed;     ///< Walking speed (units/tic), runspeed = 2*speed.
  float toughness; ///< Natural armor, depends on pclass.

  int attackphase; ///< Counter for the more complex weapons.
  Actor *attacker; ///< Who last damaged the Pawn? (NULL for floors/ceilings).

public:
  Pawn(fixed_t x, fixed_t y, fixed_t z, int type);

  virtual void Detach();
  virtual void CheckPointers();

  bool GiveBody(int num);
  void AdjustPlayerAngle(Actor *t);
};


/// \brief A Pawn that represents a player avatar.
/// \ingroup g_central
/*!
  It takes orders from its player, which is represented by a PlayerInfo instance, 
  or from a BotAI instance.
*/
class PlayerPawn : public Pawn
{
  TNL_DECLARE_CLASS(PlayerPawn);
  DECLARE_CLASS(PlayerPawn);
public:
  /// Controlling player
  class PlayerInfo *player; 

  /// \name Controls.
  /// True if the corresponding button was down last tic.
  //@{
  bool attackdown;
  bool usedown;
  bool jumpdown;   ///< don't jump like a monkey!
  //@}

  int refire;     ///< Refired shots are less accurate. to Pawn?
  int morphTics;  ///< Player is in a morphed state if >0
  int fly_zspeed; ///< For smoothing the z motion while flying

  /// First person sprites (weapon and muzzle flash)
  pspdef_t psprites[NUMPSPRITES];

  /// Inventory
  vector<inventory_t> inventory;

  int  keycards; ///< Bit field, see the definition of keycard_t

  /// \name Guns & Ammo, armor
  //@{
  weapontype_t readyweapon;   ///< Current weapon
  weapontype_t pendingweapon; ///< Weapon we are changing to or wp_nochange
  bool         weaponowned[NUMWEAPONS]; ///< owned weapons

  const weaponinfo_t *weaponinfo; ///< Changed when using level2 weapons (Heretic)

  int ammo[NUMAMMO];
  int maxammo[NUMAMMO];

  float armorfactor[NUMARMOR];
  int   armorpoints[NUMARMOR];
  //@}

  /// Tic counters for power ups.
  int powers[NUMPOWERS];

  int poisoncount; ///< poisoning

  /// Bit flags, for cheats and debug.
  int cheats;

  /// Current sector special (lava/slime/water...)
  int specialsector;

  /// Gun flashes light up nearby areas.
  int extralight;

  /// Colormap to replace the lightlevel-based colormap in rendering.
  /// Used for invulnerability, IR goggles etc.
  int fixedcolormap;

public:
  // in g_pawn.cpp
  PlayerPawn(fixed_t x, fixed_t y, fixed_t z, int type);
  ~PlayerPawn();

  /// If this actor is used as a pov, at which height are the eyes?
  virtual fixed_t GetViewZ() const;

  virtual void Think();
  void DeathThink();
  void MorphThink();

  void Move();
  virtual void ZMovement();
  virtual void XYFriction(fixed_t oldx, fixed_t oldy);
  virtual bool Teleport(const vec_t<fixed_t> &p, angle_t nangle, bool silent = false);
  virtual void LandOnThing(Actor *a);
  virtual void LandOnFloor(bool floor);

  void Reset();
  weapontype_t FindWeapon(int g);

  inline DActor *SpawnPlayerMissile(mobjtype_t type) { return SPMAngle(type, yaw); }
  DActor *SPMAngle(mobjtype_t type, angle_t ang);

  bool CanUnlockGenDoor(struct line_t *line);
  void ProcessSpecialSector(struct sector_t *sector);
  void PlayerInSpecialSector();

  bool GivePower(int power);
  bool GiveAmmo(ammotype_t at, int count);
  bool GiveWeapon(weapontype_t wt, int ammocontent, bool dropped = false);
  bool GiveArmor(armortype_t type, float factor, int points);
  bool GiveKey(keycard_t k);
  bool GiveArtifact(artitype_t arti, DActor *from);

  // in p_user.cpp
  virtual bool Morph(mobjtype_t form);
  bool UndoMorph();

  void UseArtifact(artitype_t arti);

  // in p_map.cpp
  void UseLines();
  bool UsePuzzleItem(int type);

  // in p_inter.cpp
  void TouchSpecialThing(DActor *special);
  virtual bool Touch(Actor *a);
  virtual void Die(Actor *inflictor, Actor *source, int dtype);
  virtual void Killed(PlayerPawn *victim, Actor *inflictor);
  virtual bool Damage(Actor *inflictor, Actor *source, int damage, int dtype = dt_normal);
  virtual bool FallingDamage(float v);
  void Poison(Actor *culprit, int poison);

  // in p_pspr.cpp
  void MovePsprites();
  void UseFavoriteWeapon();
  void SetupPsprites();
  void SetPsprite(int position, weaponstate_t *st, bool call = true);
  inline void SetPsprite(int position, weaponstatenum_t stnum, bool call = true) { SetPsprite(position, &weaponstates[stnum], call); }

  void DropWeapon();
  void FireWeapon();
  bool CheckAmmo();
  void BringUpWeapon();

  // in p_hpspr.cpp
  void ActivateMorphWeapon();
  void PostMorphWeapon(weapontype_t weapon);
};



#endif
