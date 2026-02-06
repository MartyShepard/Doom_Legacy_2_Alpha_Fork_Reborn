// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: g_actor.h 522 2008-01-20 22:42:23Z smite-meister $
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
/// \brief Actor and DActor class definitions

#ifndef g_actor_h
#define g_actor_h 1

#include "tnl/tnlNetObject.h"
#include "m_fixed.h"  // Basics.
#include "vect.h"
#include "g_think.h"  // We need the Thinker stuff.
#include "g_damage.h" // and damage types
#include "info.h"     // mobjtype_t

using namespace TNL;

const fixed_t ONFLOORZ   = fixed_t::FMIN;
const fixed_t ONCEILINGZ = fixed_t::FMAX;

const fixed_t TELEFOGHEIGHT = 32;
const fixed_t FOOTCLIPSIZE  = 10;


/// Actor flags. More or less permanent attributes of the Actor.
enum mobjflag_t
{
  // physical properties
  MF_NOSECTOR         = 0x0001, ///< Don't link to sector (invisible but touchable)
  MF_NOBLOCKMAP       = 0x0002, ///< Don't link to blockmap (inert but visible)
  MF_SOLID            = 0x0004, ///< Blocks
  MF_SHOOTABLE        = 0x0008, ///< Can be hit
  MF_NOCLIPLINE       = 0x0010, ///< Does not clip against lines (walls)
  MF_NOCLIPTHING      = 0x0020, ///< Not blocked by other Actors. (chasecam, for example)
  MF_NOGRAVITY        = 0x0040, ///< Does not feel gravity

  // game mechanics
  MF_PICKUP           = 0x0200, ///< Can/will pick up items. (players)
  MF_FLOAT            = 0x0400, ///< Active floater, can move freely in air (cacodemons etc.)
  MF_DROPOFF          = 0x0800, ///< Can jump/drop from high places
  MF_AMBUSH           = 0x1000, ///< *Not to be activated by sound, deaf monster.
  MF_NOSPLASH         = 0x2000, ///< Does not cause a splash when hitting water
  // appearance
  MF_SHADOW           = 0x4000, ///< Partial invisibility (spectre). Makes targeting harder.
  MF_ALTSHADOW        = 0x8000, ///< Alternate fuzziness
  MF_NOBLOOD         = 0x10000, ///< Does not bleed when shot (furniture)
  MF_NOSCORCH     = MF_NOBLOOD, ///< TEST flag double use for MF_MISSILEs, which cannot be hit (and thus cannot bleed)
  // spawning
  MF_SPAWNCEILING = 0x00020000, ///< Spawned hanging from the ceiling
  MF_SPAWNFLOAT   = 0x00040000, ///< Spawned at random height
  MF_NOTDMATCH    = 0x00080000, ///< Not spawned in DM (keycards etc.)
  MF_NORESPAWN    = 0x00100000, ///< Will not respawn after being picked up. Pretty similar to MF_DROPPED?
  // classification
  MF_COUNTKILL    = 0x00200000, ///< On kill, count this enemy object towards intermission kill total. Happy gathering.
  MF_COUNTITEM    = 0x00400000, ///< On picking up, count this item object towards intermission item total.
  MF_SPECIAL      = 0x00800000, ///< Call TouchSpecialThing when touched. Mostly items you can pick up.
  MF_DROPPED      = 0x01000000, ///< *Dropped by a monster
  MF_MISSILE      = 0x02000000, ///< Player missiles as well as fireballs. Don't hit same species, explode on block.
  MF_CORPSE       = 0x04000000, ///< Dead. Acts like a corpse, falls down stairs etc.
  MF_MONSTER      = 0x08000000, ///< Monster. Possible threat, can activate some SPAC_CROSS and SPAC_USE lines etc.
  MF_PLAYER       = 0x10000000, ///< Treated as a player. Possible threat, can activate SPAC_CROSS and SPAC_USE lines.
  MF_VALIDTARGET  = MF_MONSTER | MF_PLAYER, ///< Valid target for autoaim and bots (if not already dead)

  MF_TOUCHFUNC    = 0x80000000, ///< Actor has a touch function. A HACK to handle complex Hexen mapthing behavior.
  // 4 bits free
};


/// More semi-permanent flags. Mostly came with Heretic.
enum mobjflag2_t : unsigned int
{
  // physical properties
  MF2_LOGRAV         =     0x0001,    ///< Experiences only 1/8 gravity
  MF2_WINDTHRUST     =     0x0002,    ///< Is affected by wind
  MF2_FLOORBOUNCE    =     0x0004,    ///< Bounces off the floor once and immediately explodes.
  MF2_FULLBOUNCE     =     0x0008,    ///< Bounces off all surfaces, loses kinetic energy, does not explode.
  MF2_SLIDE          =     0x0010,    ///< Slides against walls
  MF2_PUSHABLE       =     0x0020,    ///< Can be pushed by other moving actors
  MF2_CANNOTPUSH     =     0x0040,    ///< Cannot push other pushable actors
  MF2_FLOORHUGGER    =     0x0080,    ///< Stays on floor, climbs any step up or down
  MF2_CEILINGHUGGER  =     0x0100,    ///< Stays on ceiling, climbs any step down or up
  MF2_NONBLASTABLE   =     0x0200,    ///< Cannot be blasted. Implied by MF2_BOSS.

  // game mechanics
  MF2_QUICKTORETALIATE =   0x0400,    ///< Will immediately switch targets and retaliate when attacked.
  MF2_NOTARGET       =     0x0800,    ///< Will not be targeted by other monsters of the same team (like Arch-Vile)
  MF2_FLOATBOB       =     0x1000,    ///< Bobs up and down in the air (item)
  MF2_THRUGHOST      =     0x2000,    ///< Will pass through ghosts (missile)
  MF2_RIP            =     0x4000,    ///< Rips through solid targets (missile)
  MF2_NOPASSMOBJ     =     0x8000,    ///< Cannot move over/under other Actors with this flag
  MF2_NOTELEPORT     =    0x10000,    ///< Does not teleport
  MF2_NONSHOOTABLE   =    0x20000,    ///< Transparent to MF_MISSILEs
  MF2_INVULNERABLE   =    0x40000,    ///< Does not take damage
  MF2_DORMANT	     =    0x80000,    ///< Cannot be damaged, is not noticed by seekers
  MF2_CANTLEAVEFLOORPIC =0x100000,    ///< Stays within a certain floor texture
  MF2_BOSS           =   0x200000,    ///< Is a major boss, not as easy to kill (odd collection of immunities)
  //dragon, sorcboss,korax, spidermm, cyborg, both dsparils, minotaur
  // full vol seesound and activesound, cannot be blasted, some resistance to wraithverge,
  // not affected by others blasted to them, some res to magelightning, some res to bloodscourge,
  // doom and heretic bosses take no radiusdamage, no morph or teleportother
  // heretic and hexen bosses do not aggravate teammates ever
  // hexen class bosses: some res to bloodsc., no morphing

  MF2_SEEKERMISSILE  = 0x00400000,    ///< Is a seeker (for reflection)
  MF2_REFLECTIVE     = 0x00800000,    ///< Reflects missiles

  // rendering
  MF2_FOOTCLIP       = 0x01000000,    ///< Feet should be be clipped by floorclip units when standing on liquid floor.
  MF2_DONTDRAW       = 0x02000000,    ///< Invisible (does not generate a vissprite)

  // giving hurt
  MF2_NODMGTHRUST    = 0x04000000,    ///< Does not thrust target when damaging        
  MF2_TELESTOMP      = 0x08000000,    ///< Can telefrag another Actor

  // passive linedef activation (using is different)
  // MF_PLAYER and MF_MONSTER can activate SPAC_CROSS.
  MF2_IMPACT	     = 0x10000000,    ///< Can activate SPAC_IMPACT
  MF2_PUSHWALL	     = 0x20000000,    ///< Can activate SPAC_PUSH
  MF2_MCROSS	     = 0x40000000,    ///< Can activate SPAC_MCROSS
  MF2_PCROSS	     = 0x80000000u,    ///< Can activate SPAC_PCROSS
};


/// Extra flags. They describe the transient state of the Actor.
enum mobjeflag_t
{
  // location
  MFE_ONGROUND      = 0x0001,  ///< Stands on solid floor (not on another Actor or in air)
  MFE_ONMOBJ        = 0x0002,  ///< Stands on top of another Actor
  // (instant damage in lava/slime sectors to prevent jump cheat..)
  MFE_JUSTHITFLOOR   = 0x0004,  ///< Just hit the floor while falling, cleared on next frame
  MFE_JUSTHITCEILING = 0x0008,  ///< Same but for ceiling hits.

  MFE_TOUCHWATER    = 0x0010,  ///< Touches water surface
  MFE_UNDERWATER    = 0x0020,  ///< Waist below water surface (swimming is possible)
  // active physics mode
  MFE_SWIMMING      = 0x0040,  ///< Swimming physics used (different gravity)
  MFE_FLY           = 0x0080,  ///< Well, flying. No gravity, some bobbing.
  MFE_INFLOAT       = 0x0100,  ///< Floating move in progress, don't auto float to target's height.
  MFE_SKULLFLY      = 0x0200,  ///< A charging skull or minotaur.
  MFE_BLASTED       = 0x0400,  ///< Uncontrollably thrown by a blast wave

  // combat
  MFE_JUSTHIT       = 0x1000,  ///< Got hit, will try to attack right back.
  MFE_JUSTATTACKED  = 0x2000,  ///< Will take at least one step before attacking again.

  MFE_REMOVE    = 0x80000000   ///< Actor will be deleted after the tic
};



/// \brief Basis class for all Thinkers with a well-defined location.
/// \ingroup g_central
/*!
  Actors are used to tell the renderer where to draw an image,
  tell the world simulation when objects are contacted,
  and tell the sound system how to position a sound.
 
  The renderer uses the snext and sprev links to follow
  lists of things in sectors as they are being drawn.

  The xyz origin point represents a point at the bottom middle
  of the sprite (between the feet of a biped).
  This is the default origin position for patch_ts grabbed
  with lumpy.exe.
  A walking creature will have its z equal to the floor it is standing on.
 
  The sound code uses the x,y, and subsector fields
  to do stereo positioning of any sound emitted by the Actor.
 
  The play simulation uses the blocklinks, x,y,z, radius, height
  to determine when Actors are touching each other,
  touching lines in the map, or hit by trace lines (gunshots, lines of sight, etc).
  The flags fields have various bit flags used by the simulation.
 
  Every Actor is linked into a single sector based on its origin coordinates.
  The subsector_t is found with R_PointInSubsector(x,y),
  and the sector_t can be found with subsector->sector.
  The sector links are only used by the rendering code,
  the play simulation does not care about them at all.
 
  Any Actor that needs to be acted upon by something else
  in the play world (block movement, be shot, etc) will also
  need to be linked into the blockmap.
  If the thing has the MF_NOBLOCK flag set, it will not use
  the block links. It can still interact with other things,
  but only as the instigator (missiles will run into other
  things, but nothing can run into a missile).
  Each block in the grid is 128*128 units, and knows about
  every line_t that it contains a piece of, and every
  interactable Actor that has its origin contained.
 
  A valid Actor is a Actor that has the proper subsector_t
  filled in for its xy coordinates and is linked into the
  sector from which the subsector was made, or has the
  MF_NOSECTOR flag set (the subsector_t needs to be valid
  even if MF_NOSECTOR is set), and is linked into a blockmap
  block or has the MF_NOBLOCKMAP flag set.
  Links should only be modified by the [Un]SetPosition() functions.
  Do not change the MF_NO? flags while a thing is valid.
 
  Any questions?
 */
class Actor : public Thinker, public NetObject
{
  typedef NetObject Parent;
  TNL_DECLARE_CLASS(Actor);
  DECLARE_CLASS(Actor);

  /// netcode
  virtual bool onGhostAdd(GhostConnection *c);
  virtual void onGhostRemove();
  virtual U32 packUpdate(GhostConnection *c, U32 updateMask, BitStream *s);
  virtual void unpackUpdate(GhostConnection *c, BitStream *s);

public:
  /// Ghosting flags
  enum mask_e
    {
      M_MOVE = BIT(1),
      M_ANIM = BIT(2),
      M_PRES = BIT(3),
      M_EVERYTHING = 0xFFFFFFFF
    };

public:
  class presentation_t *pres;  ///< graphic presentation

  /// \name Links to Map geometry
  //@{
  Actor *sprev, *snext;  ///< sector links
  Actor *bprev, *bnext;  ///< blockmap links

  struct subsector_t *subsector; ///< current location in BSP

  /// The closest interval over all contacted Sectors (or Things).
  fixed_t floorz, ceilingz;

  /// a linked list of sectors where this object appears
  struct msecnode_t* touching_sectorlist;

  /// For nightmare and itemrespawn respawn.
  struct mapthing_t *spawnpoint;
  //@}

public:
  /// position
  vec_t<fixed_t> pos;

  /// velocity, used to update position
  vec_t<fixed_t> vel;

  /// orientation using Euler angles: yaw is left-right, pitch is up-down, roll is "around"
  angle_t yaw, pitch; //, roll;

  /// TEST: "actual" loc. data for netcode
  vec_t<fixed_t> apos, avel;


  /// For movement checking.
  float   mass;
  fixed_t radius;
  fixed_t height;

  /// quick z positions
  inline fixed_t Top()    const { return pos.z + height; }
  inline fixed_t Center() const { return pos.z + (height >> 1); }
  inline fixed_t Feet()   const { return pos.z; }

  int  health;

  Uint32  flags;
  Uint32  flags2;
  Uint32  eflags;

  /// \name Hexen fields
  //@{
  short	tid;     ///< thing identifier
  byte	special; ///< special type
  byte	args[5]; ///< special arguments
  //@}

  Actor *owner;   ///< Owner of this Actor. For example, for missiles this is the shooter.
  Actor *target;  ///< Thing being chased/attacked (or NULL), also the target for missiles.

  int reactiontime; ///< Time (in tics) before the thing can attack or move again. For MF2_FLOATBOB actors this is the bob phase.

  fixed_t floorclip; ///< cut this amount from legs (deep water illusion) (Hexen)

  short team; ///< see TeamInfo

public:
  // in g_actor.cpp
  Actor(fixed_t nx, fixed_t ny, fixed_t nz); ///< construct a new Actor
  //Actor(const Actor &a); // TODO is the auto-generated copy constructor bad because of NetObject fields?
  virtual ~Actor();

  void Remove();  ///< delayed destruction
  virtual void Detach();  ///< detaches the Actor from the Map

  virtual void Think();
  virtual void ClientThink(); ///< movement interpolation
  virtual void CheckPointers();

  void ClientInterpolate(); ///< Netcode: clientside movement interpolation

  /// If this actor is used as a pov, at which height are the eyes?
  virtual fixed_t GetViewZ() const { return Top(); }

  void CheckWater();  ///< set some eflags if sector contains water

  float GetMoveFactor();
  virtual void XYMovement();
  virtual void ZMovement();
  virtual void LandOnThing(Actor *a);
  virtual void LandOnFloor(bool floor = true); ///< hit either floor or ceiling

  virtual void XYFriction(fixed_t oldx, fixed_t oldy);
  virtual void Thrust(angle_t angle, fixed_t move);

  int  HitFloor();

  // in p_inter.cpp
  virtual bool Touch(Actor *a); ///< Actor touches another Actor
  virtual void Die(Actor *inflictor, Actor *source, int dtype);
  virtual void Killed(class PlayerPawn *victim, Actor *inflictor);
  virtual bool Morph(mobjtype_t form);
  virtual bool Damage(Actor *inflictor, Actor *source, int damage, int dtype = dt_normal);
  virtual bool FallingDamage(float v);

  // in p_telept.cpp
  virtual bool Teleport(const vec_t<fixed_t> &p, angle_t nangle, bool silent = false);

  // in p_map.cpp
  void SetPosition();
  void UnsetPosition(bool clear_touching_sectorlist = false);
  bool TestLocation(const vec_t<fixed_t> &p);
  inline bool TestLocation() { return TestLocation(pos); }

  enum poscheck_e
    {
      PC_THINGS       = 0x01, ///< check things
      PC_LINES        = 0x02, ///< check lines
      PC_TOUCH_THINGS = 0x04, ///< Touch() contacted things
      PC_TOUCH_LINES  = 0x08, ///< collect special lines, push
      PC_TELEFRAG     = 0x10, ///< telefrag contacted things

      PC_FIT      = PC_THINGS | PC_LINES,
      PC_MOVE     = PC_FIT | PC_TOUCH_THINGS | PC_TOUCH_LINES,
      PC_TELEPORT = PC_FIT | PC_TELEFRAG,
    };

  struct position_check_t *CheckPosition(const vec_t<fixed_t> &p, poscheck_e mode);
  pair<bool, position_check_t*> TryMove(fixed_t nx, fixed_t ny, bool allowdropoff);
  bool TeleportMove(const vec_t<fixed_t> &p);

protected:
  void CheckLineImpact(vector<struct line_t*> &spechit); ///< only used with TryMove
  void SlideMove(fixed_t nx, fixed_t ny);
  void BounceWall(fixed_t nx, fixed_t ny);
public:
  Actor *AimLineAttack(angle_t ang, float distance, float& sinpitch);
  Actor *LineTrace(angle_t ang, float distance, float sine, bool interact);
  Actor *LineAttack(angle_t ang, float distance, float sine, int damage, int dtype = dt_normal);
  void   RadiusAttack(Actor *culprit, int damage, fixed_t radius = -1, int dtype = dt_normal, bool downer = true);

  virtual void Howl() {};
};


//========================================================
/// \brief Doom Actor.
/// \ingroup g_central
/// \ingroup g_thing
/*!
  An Actor with the standard Doom/Heretic AI. Also known as THING or mobj_t.
  Uses the A_* action functions and the state table in ai_*.cpp.

  The sprite and frame elements of state_t (together with angle) determine which patch_t
  is used to draw the sprite if it is visible.
*/
class DActor : public Actor
{
  TNL_DECLARE_CLASS(DActor);
  DECLARE_CLASS(DActor);
public:
  /// What kind of thing is it?
  const class ActorInfo *info;
  mobjtype_t type;

  // state machine variables
  const state_t *state;   ///< current state
  int            tics;    ///< state tic counter

  // Movement direction, movement generation (zig-zagging).
  Uint16  movedir;    ///< 0-8
  Sint16  movecount;  ///< when 0, select a new dir

  Sint16  threshold;  ///< If >0, the current target will be chased no matter what (even if shot)
  Sint16  lastlook;   ///< Player number last looked for.

  Sint32  special1, special2, special3; ///< mobjtype dependent general storage


public:
  /// create a nonfunctional special DActor
  //DActor(mobjtype_t t);
  /// create a new DActor
  DActor(fixed_t nx, fixed_t ny, fixed_t nz, const class ActorInfo *ai);

  virtual void Think();

  // in p_inter.cpp
  virtual bool Touch(Actor *a);
  virtual void Die(Actor *inflictor, Actor *source, int dtype);
  virtual void Killed(PlayerPawn *victim, Actor *inflictor);
  virtual bool Morph(mobjtype_t form);
  virtual bool Damage(Actor *inflictor, Actor *source, int damage, int dtype = dt_normal);

  inline bool SetState(statenum_t snum, bool call = true) { return SetState(&states[snum], call); };
  bool SetState(const state_t *ns, bool call = true);

  void NightmareRespawn();

  DActor *SpawnMissile(Actor *dest, mobjtype_t type, fixed_t h = 32);
  bool   CheckMissileSpawn();
  void   ExplodeMissile();
  void   FloorBounceMissile();

  DActor *SpawnMissileAngle(mobjtype_t type, angle_t angle, fixed_t h = 32, fixed_t vz = 0);

  // in p_enemy.cpp
  bool CheckMeleeRange();
  bool CheckMissileRange();
  bool LookForEnemies(bool allaround);
  void P_NewChaseDir();
  bool P_TryWalk();
  bool P_Move();
  
  // in p_hpspr.cpp
  void BlasterMissileThink();

  // in p_henemy.cpp
  void DSparilTeleport();
  bool UpdateMorph(int tics);

  // in p_heretic.cpp
  bool SeekerMissile(angle_t thresh, angle_t turnMax);

  // in p_things.cpp
  bool Activate();
  bool Deactivate();

  // elsewhere
  virtual void Howl();
};

#endif
