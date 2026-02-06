// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_spec.h 518 2007-12-31 02:54:58Z smite-meister $
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
/// \brief Dynamic geometry elements and related utilities.
///
/// Sector-based Thinker classes defined.


#ifndef p_spec_h
#define p_spec_h 1

#include <list>

#include "r_defs.h"
#include "g_think.h" // Thinker class

using namespace std;

extern int boomsupport;


/// linedef specials
enum linedefspecial_e
{
  LINE_PO_START = 1,
  LINE_PO_EXPLICIT = 5,

  /// All Legacy extensions to Hexen linedef system are under this type.
  LINE_LEGACY_EXT = 150,
  /// subtypes, stored in args[0]
  LINE_LEGACY_BOOM_SCROLLERS = 0,
  LINE_LEGACY_EXOTIC_TEXTURE = 4,
  LINE_LEGACY_FAKEFLOOR      = 10,
  LINE_LEGACY_RENDERER       = 11,
  LINE_LEGACY_MISC           = 13,
  LINE_LEGACY_FS             = 128,
};

/// Editor numbers for certain special mapthings (more are defined in p_setup.cpp)
enum mapthingspecial_e
{
  EN_PO_ANCHOR       = 9300, ///< ZDoom compatible polyobjects
  EN_PO_SPAWN        = 9301,
  EN_PO_SPAWNCRUSH   = 9302,
};

/// ingame sector special effects
enum sectorspecial_t
{
  SS_none = 0,

  // low byte holds an individual sectorspecial number 0-255 (if needed)
  SS_SPECIALMASK  = 0x00FF,  // bits 0-7
  // Doom
  SS_EndLevelHurt = 11,
  // Hexen
  SS_LightSequence_1 = 3,
  SS_LightSequence_2 = 4,
  SS_Stairs_Special1 = 26,
  SS_Stairs_Special2 = 27,
  SS_IndoorLightning1 = 198,
  SS_IndoorLightning2 = 199,
  SS_Sky2 = 200,

  // the high byte holds flags (mostly remapped Boom flags)
  SS_friction = 0x0100,
  SS_wind     = 0x0200,
  SS_secret   = 0x0400,

  // damage frequency (bits 11-12)
  SS_DAMAGEMASK = 0x1800,
  SS_damage_16  = 0x0800, // every 16 tics
  SS_damage_32  = 0x1000,
  SS_damage_XX  = 0x1800,

  // bits 13-15 are free
};


// geom. info, independent of Map
sector_t *getNextSector(line_t *line, sector_t *sec);



/// the result of a plane movement
enum planeresult_e
{
  res_ok = 0,
  res_crushed,
  res_pastdest
};


/// teleporter flags etc.
enum teleport_e
{
  TP_toTID = 0,
  TP_toThingInSector = 1,
  TP_toLine = 2,

  // flags
  TP_noplayer = 0x1,
  TP_silent = 0x2,
  TP_reldir = 0x4,
  TP_flip = 0x8
};


//======================================
///  Timed buttons
//======================================

class button_t : public Thinker
{
  friend class Map;
  DECLARE_CLASS(button_t)
public:
  enum button_e
  {
    none = 0,
    top,
    middle,
    bottom
  };

private:
  line_t     *line;
  mappoint_t *soundorg;
  Material   *texture;
  int         timer;
  byte where; // button_e

public:
  button_t(line_t *l, button_e w, Material *tex, int time);
  
  virtual void Think();
};

void P_InitSwitchList();

// 1 second, in tics.
const int BUTTONTIME = 35;



//========================================================
//  Sector effects
//========================================================

/// Base class for most moving geometry and other single sector -based effects
class sectoreffect_t : public Thinker
{
  friend class Map;
  DECLARE_CLASS(sectoreffect_t)

protected:
  sector_t *sector;
  sectoreffect_t(Map *m, sector_t *s);
};


//======================================
///   Sector light effects
//======================================

class lightfx_t : public sectoreffect_t
{
  friend class Map;
  DECLARE_CLASS(lightfx_t)

protected:
  short     type;
  short     count;
  short     maxlight, minlight;
  union
  {
    short maxtime;
    short rate; // 10.6 fixed point for smoother light changes
  };
  union
  {
    short mintime;
    short currentlight; // 10.6 fixed point for smoother light changes
  };

public:
  enum lightfx_e
  {
    AbsChange,   // one-time absolute change
    RelChange,   // one-time relative change
    Fade,        // linear ramp to maxlight, rate is speed
    Glow,        // sawtooth wave between min and max, rate is speed
    Strobe,      // square wave, constant min and max times
    Flicker,     // square wave, random min and max times
    FireFlicker  // square wave with more randomness
  };

  lightfx_t(Map *m, sector_t *sec, lightfx_e type, short maxlight, short minlight = 0, short maxtime = 0, short mintime = 0);
  
  virtual void Think();
};

// strobe light timings (tics)
const int STROBEBRIGHT = 5;
const int FASTDARK     = 15;
const int SLOWDARK     = 35;


/// Sequential Hexen light effect
class phasedlight_t : public sectoreffect_t
{
  friend class Map;
  DECLARE_CLASS(phasedlight_t)

protected:
  short base;
  short index;

public:
  phasedlight_t(Map *m, sector_t *sec, int base, int index);
  
  virtual void Think();
};


//======================================================
///      Platforms/Lifts (complex moving floors)
//======================================================

class plat_t : public sectoreffect_t
{
  friend class Map;
  DECLARE_CLASS(plat_t)
public:
  enum plat_e
  {
    // targets
    RelHeight = 0,
    AbsHeight,
    LnF,
    NLnF,
    NHnF,
    LnC,
    Perpetual = 0x8, // will not stop until stopped. Perpetual types follow.
    LHF       = Perpetual,
    CeilingToggle,
    TMASK = 0xF,

    // flags
    Returning   = 0x10, // coming back, will stop
    InStasis    = 0x20,
    SetTexture  = 0x40,
    ZeroSpecial = 0x80,
  };

  enum status_e
  {
    up,
    down,
    waiting
  };

private:
  byte     type;
  byte     status;
  fixed_t  speed;
  fixed_t  low, high;
  int      wait, count;

  list<plat_t *>::iterator li;

public:
  plat_t(Map *m, int type, sector_t *sec, fixed_t speed, int wait, fixed_t height);
  
  virtual void Think();
};


const int PLATWAIT = 3;
const float PLATSPEED = 1;


//======================================
///  Doors (complex moving ceilings)
//======================================

class vdoor_t : public sectoreffect_t
{
  friend class Map;
  DECLARE_CLASS(vdoor_t)
public:
  enum vdoor_e
  {
    // 2 bits, type of door
    OwC,
    Open,
    CwO,
    Close,
    TMASK   = 0x3,
    Blazing = 0x4,
    Delayed = 0x8 // initial delay (topcount)
  };

private:
  byte      type;
  char      direction; // 1 = up, 0 = waiting at top, -1 = down, 2 = initial delay
  fixed_t   topheight;
  fixed_t   speed;
  int       topwait;   // tics to wait at the top
  int       topcount;   // when it reaches 0, start going down

  short     boomlighttag; // for Boom push-door light effect

public:
  vdoor_t(Map *m, byte type, sector_t *sec, fixed_t speed, int delay);
  
  virtual void Think();
  void MakeSound(bool open) const;
};


const float VDOORSPEED = 2;
const int VDOORWAIT = 150;


// Sliding doors removed


//======================================
///  Moving ceilings (and crushers)
//======================================

class ceiling_t : public sectoreffect_t
{
  friend class Map;
  DECLARE_CLASS(ceiling_t)
public:

  enum ceiling_e
  {
    // target
    RelHeight = 0,
    AbsHeight,
    Floor,
    HnC,
    UpNnC,
    DownNnC,
    LnC,
    HnF,
    UpSUT,
    DownSUT,
    TMASK  = 0xF,
    // flags
    SetTexture   = 0x010,
    SetSpecial   = 0x020,
    SetTxTy      = 0x010 + 0x20,
    ZeroSpecial  = 0x040,
    NumericModel = 0x080,
    Silent       = 0x100,
    InStasis     = 0x200
  };

protected:
  int       type;
  short     crush;
  fixed_t   speed;

  // ceiling changers
  int       modelsec; ///< model sector number
  Material *texture;

  list<ceiling_t *>::iterator li;

public:
  fixed_t   destheight;

public:
  ceiling_t(Map *m, int type, sector_t *sec, fixed_t speed, int crush, fixed_t height);
  
  virtual void Think();
};


const float CEILSPEED = 1;
const int CEILWAIT = 150;



class crusher_t : public ceiling_t
{
  friend class Map;
  DECLARE_CLASS(crusher_t)
public:
  enum crusher_e
  {
    // flags (also uses ceiling_t flags)
    CrushOnce  = 0x01,
  };

private:
  fixed_t downspeed, upspeed;
  fixed_t bottomheight, topheight;

public:
  crusher_t(Map *m, int type, sector_t *sec, fixed_t downspeed, fixed_t upspeed, int crush, fixed_t height);
  
  virtual void Think();
};



//======================================
///  Moving floors (also stairs, donuts)
//======================================

class floor_t : public sectoreffect_t
{
  friend class Map;
  DECLARE_CLASS(floor_t)
public:
  enum floor_e
  {
    // target
    RelHeight = 0,
    AbsHeight,
    Ceiling,
    LnF,
    UpNnF,
    DownNnF,
    HnF,
    LnC,
    UpSLT,
    DownSLT,
    TMASK = 0xF,
    // flags
    SetTexture = 0x10,
    SetSpecial = 0x20,
    SetTxTy    = 0x10 + 0x20,
    ZeroSpecial = 0x40, // extra attribute for SetSpecial
    NumericModel = 0x80 // otherwise assume "trigger model"
  };

private:
  int       type;
  short     crush;
  fixed_t   speed; // sign denotes direction

  // floor changers
  int       modelsec; ///< model sector number
  Material *texture;

public:
  fixed_t   destheight;

public:
  floor_t(Map *m, int type, sector_t *sec, fixed_t speed, int crush, fixed_t height);
  
  virtual void Think();
};

enum stair_e
{
  STAIRS_NORMAL,
  STAIRS_SYNC,
  STAIRS_PHASED
};


//======================================
///  Stairbuilders
//======================================

class stair_t : public sectoreffect_t
{
  friend class Map;
  DECLARE_CLASS(stair_t)
public:
  enum stair_e
  {
    // types
    Normal = 0,
    Sync,
    // internal states
    Moving = 0,
    Waiting,
    Done
  };

private:
  char state;
  int resetcount; // ticks until reset, if 0, no reset
  int wait, stepdelay;

  fixed_t speed;
  fixed_t destheight, originalheight;
  fixed_t delayheight, stepdelta;

public:
  stair_t(Map *m, int ty, sector_t *sec, fixed_t h, fixed_t sp, int rcount, int sdelay);
  
  virtual void Think();
};


//==================================================
///  Moving floor and ceiling (elevators, pillars)
//==================================================

class elevator_t : public sectoreffect_t
{
  friend class Map;
  DECLARE_CLASS(elevator_t)
public:
  enum elevator_e
  {
    Up = 0,
    Down,
    Current,
    RelHeight,
    OpenPillar,
    ClosePillar
  };

private:
  int     type;
  int     crush;
  fixed_t floordest, ceilingdest;
  fixed_t floorspeed, ceilingspeed;

public:
  elevator_t(Map *m, int type, sector_t *s, fixed_t speed, fixed_t height_f, fixed_t height_c, int crush);
  
  virtual void Think();
};


const float ELEVATORSPEED = 4;
const float FLOORSPEED    = 1;

//======================================
///  Sine wave floorshake (Hexen)
//======================================

class floorwaggle_t : public sectoreffect_t
{
  friend class Map;
  DECLARE_CLASS(floorwaggle_t)
public:
  enum floorwaggle_e
  {
    Expand = 0,
    Stable,
    Reduce
  };

private:
  char    state;
  angle_t phase, freq;
  fixed_t baseheight, amp, maxamp, ampdelta;
  int     wait;

public:
  floorwaggle_t(Map *m, sector_t *sec, fixed_t a, angle_t f, angle_t ph, int w);
  
  virtual void Think();
};


//======================================
// BOOM generalized linedefs
//======================================

/* SoM: 3/4/2000: This is a large section of copied code. Sorry if this offends people, but
   I really don't want to read, understand and rewrite all the changes to the source and entire
   team made! Anyway, this is for the generalized linedef types. */

//jff 02/04/98 Define masks, shifts, for fields in 
// generalized linedef types

#define GenFloorBase          0x6000
#define GenCeilingBase        0x4000
#define GenDoorBase           0x3c00
#define GenLockedBase         0x3800
#define GenLiftBase           0x3400
#define GenStairsBase         0x3000
#define GenCrusherBase        0x2F80

#define TriggerType           0x0007
#define TriggerTypeShift      0

// define masks and shifts for the floor type fields

#define FloorCrush            0x1000
#define FloorChange           0x0c00
#define FloorTarget           0x0380
#define FloorDirection        0x0040
#define FloorModel            0x0020
#define FloorSpeed            0x0018

#define FloorCrushShift           12
#define FloorChangeShift          10
#define FloorTargetShift           7
#define FloorDirectionShift        6
#define FloorModelShift            5
#define FloorSpeedShift            3
                               
// define masks and shifts for the ceiling type fields

#define CeilingCrush          0x1000
#define CeilingChange         0x0c00
#define CeilingTarget         0x0380
#define CeilingDirection      0x0040
#define CeilingModel          0x0020
#define CeilingSpeed          0x0018

#define CeilingCrushShift         12
#define CeilingChangeShift        10
#define CeilingTargetShift         7
#define CeilingDirectionShift      6
#define CeilingModelShift          5
#define CeilingSpeedShift          3

// define masks and shifts for the lift type fields

#define LiftTarget            0x0300
#define LiftDelay             0x00c0
#define LiftMonster           0x0020
#define LiftSpeed             0x0018

#define LiftTargetShift            8
#define LiftDelayShift             6
#define LiftMonsterShift           5
#define LiftSpeedShift             3

// define masks and shifts for the stairs type fields

#define StairIgnore           0x0200
#define StairDirection        0x0100
#define StairStep             0x00c0
#define StairMonster          0x0020
#define StairSpeed            0x0018

#define StairIgnoreShift           9
#define StairDirectionShift        8
#define StairStepShift             6
#define StairMonsterShift          5
#define StairSpeedShift            3

// define masks and shifts for the crusher type fields

#define CrusherSilent         0x0040
#define CrusherMonster        0x0020
#define CrusherSpeed          0x0018

#define CrusherSilentShift         6
#define CrusherMonsterShift        5
#define CrusherSpeedShift          3

// define masks and shifts for the door type fields

#define DoorDelay             0x0300
#define DoorMonster           0x0080
#define DoorKind              0x0060
#define DoorSpeed             0x0018

#define DoorDelayShift             8
#define DoorMonsterShift           7
#define DoorKindShift              5
#define DoorSpeedShift             3

// define masks and shifts for the locked door type fields

#define LockedNKeys           0x0200
#define LockedKey             0x01c0
#define LockedKind            0x0020
#define LockedSpeed           0x0018

#define LockedNKeysShift           9
#define LockedKeyShift             6
#define LockedKindShift            5
#define LockedSpeedShift           3


// define names for the TriggerType field of the general linedefs

typedef enum
{
  WalkOnce,
  WalkMany,
  SwitchOnce,
  SwitchMany,
  GunOnce,
  GunMany,
  PushOnce,
  PushMany,
} triggertype_e;

// define names for the Speed field of the general linedefs

typedef enum
{
  SpeedSlow,
  SpeedNormal,
  SpeedFast,
  SpeedTurbo,
} motionspeed_e;

// define names for the Target field of the general floor

typedef enum
{
  FtoHnF,
  FtoLnF,
  FtoNnF,
  FtoLnC,
  FtoC,
  FbyST,
  Fby24,
  Fby32,
} floortarget_e;

// define names for the Changer Type field of the general floor

typedef enum
{
  FNoChg,
  FChgZero,
  FChgTxt,
  FChgTyp,
} floorchange_e;

// define names for the Change Model field of the general floor

typedef enum
{
  FTriggerModel,
  FNumericModel,
} floormodel_t;

// define names for the Target field of the general ceiling

typedef enum
{
  CtoHnC,
  CtoLnC,
  CtoNnC,
  CtoHnF,
  CtoF,
  CbyST,
  Cby24,
  Cby32,
} ceilingtarget_e;

// define names for the Changer Type field of the general ceiling

typedef enum
{
  CNoChg,
  CChgZero,
  CChgTxt,
  CChgTyp,
} ceilingchange_e;

// define names for the Change Model field of the general ceiling

typedef enum
{
  CTriggerModel,
  CNumericModel,
} ceilingmodel_t;

// define names for the Target field of the general lift

typedef enum
{
  F2LnF,
  F2NnF,
  F2LnC,
  LnF2HnF,
} lifttarget_e;

// define names for the door Kind field of the general ceiling

typedef enum
{
  OdCDoor,
  ODoor,
  CdODoor,
  CDoor,
} doorkind_e;

// define names for the locked door Kind field of the general ceiling

typedef enum
{
  AnyKey_,
  RCard,
  BCard,
  YCard,
  RSkull,
  BSkull,
  YSkull,
  AllKeys,
} keykind_e;



//============================================
/// Generalized scroller

class scroll_t : public Thinker
{
  friend class Map;
  DECLARE_CLASS(scroll_t)
public:
  enum scroll_e
  {
    // what is being scrolled?
    sc_floor         = 0x01,
    sc_ceiling       = 0x02,
    sc_carry_floor   = 0x04,
    sc_carry_ceiling = 0x08, ///< not implemented

    sc_side          = 0x10, ///< not stackable with the other types

    sc_wind = 0x20,  ///< only affects things with MF2_WINDTHRUST flag

    // how is the scrolling controlled?
    sc_constant     = 0x0,
    sc_displacement = 0x1,
    sc_accelerative = 0x2,
    sc_offsets      = 0x4,

    /// Amount the linedef length is shifted right to get scroll amount
    SCROLL_SHIFT = 5
  };

private:
  short     type;
  fixed_t   vx, vy;      ///< scroll speed
  int       affectee;    ///< Number of affected sidedef, sector, tag, or whatever
  sector_t *control;     ///< Control sector (NULL if none) used to control scrolling
  fixed_t   last_height; ///< Last known height of control sector
  bool      accel;       ///< Whether it's accelerative
  fixed_t   vdx, vdy;    ///< Accumulated velocity if accelerative

public:

  scroll_t(short type, fixed_t dx, fixed_t dy, sector_t *csec, int aff, bool acc);
  
  virtual void Think();
};


//============================================
/// Boom push/pull effect

class pusher_t : public sectoreffect_t
{
  friend class Map;
  DECLARE_CLASS(pusher_t)
public:
  enum pusher_e
  {
    p_wind        = 0,
    p_current     = 1,
    p_point       = 2,
    p_upcurrent,
    p_downcurrent,

    PUSH_FACTOR = 7
  };

private:
  short type;
  class DActor *source;  ///< Point source if point pusher
  fixed_t x_mag, y_mag;  ///< X, Y strength
  fixed_t magnitude;     ///< Length of strength-vector for point pusher
  fixed_t radius;        ///< Effective radius for point pusher
  fixed_t x, y;          ///< X, Y of point source if point pusher

public:
  pusher_t(Map *m, sector_t *sec, pusher_e t, fixed_t x_m, fixed_t y_m, DActor *src);
  
  virtual void Think();

  friend bool PIT_PushThing(class Actor *thing);
};


#endif
