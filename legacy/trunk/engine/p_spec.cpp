// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: p_spec.cpp 522 2008-01-20 22:42:23Z smite-meister $
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
/// \brief Utilities, LineDef and Sector specials
///
/// Map geometry utility functions
/// Tag and lineid hashing
/// Linedef specials
/// Sector specials
/// Scrollers, friction, pushers


#include "doomdef.h"

#include "g_game.h"
#include "g_map.h"
#include "g_blockmap.h"
#include "g_actor.h"
#include "g_pawn.h"

#include "p_spec.h"
#include "p_maputl.h"
#include "r_data.h"
#include "r_main.h" // extra colormaps

#include "m_bbox.h" // bounding boxes
#include "m_random.h"
#include "tables.h"
#include "z_zone.h"


//SoM: Enable Boom features?
int boomsupport = 1;
int variable_friction = 1;
int allow_pushers = 1;


//========================================================
//  Sector effects: base class for most moving geometry
//========================================================

IMPLEMENT_CLASS(sectoreffect_t, Thinker);
sectoreffect_t::sectoreffect_t() {}

sectoreffect_t::sectoreffect_t(Map *m, sector_t *s)
{
  m->AddThinker(this); // when a sectoreffect is constructed, it is immediately added to the Thinker list.
  sector = s;
}


//========================================================
// UTILITIES
//========================================================


/// Given a line number within a sector,
/// it will tell you whether the line is two-sided or not.
bool sector_t::twoSided(int line)
{
  return boomsupport ? (lines[line]->sideptr[1] != NULL) : (lines[line]->flags & ML_TWOSIDED);
}


/// Return sector_t* of sector next to current.
/// NULL if not two-sided line
sector_t *getNextSector(line_t *line, sector_t *sec)
{
  if (!boomsupport)
    {
      if (!(line->flags & ML_TWOSIDED))
	return NULL;
    }

  if (line->frontsector == sec)
    {
      if (!boomsupport || line->backsector!=sec)
	return line->backsector;
      else
	return NULL;
    }
  return line->frontsector;
}


/// Find the lowest floor height in the surrounding sectors, THIS SECTOR INCLUDED.
fixed_t sector_t::FindLowestFloorSurrounding()
{
  fixed_t h = floorheight;

  for (int i=0; i < linecount; i++)
    {
      sector_t *other = getNextSector(lines[i], this);

      if (!other)
	continue;

      if (other->floorheight < h)
	h = other->floorheight;
    }
  return h;
}


/// Find the highest floor height in the surrounding sectors, not including this sector.
fixed_t sector_t::FindHighestFloorSurrounding()
{
  fixed_t h = -500; // why?
  bool foundsector = false;

  for (int i=0; i < linecount; i++)
    {
      sector_t *other = getNextSector(lines[i], this);

      if (!other)
	continue;

      if (other->floorheight > h || !foundsector)
	h = other->floorheight;

      if (!foundsector)
	foundsector = true;
    }
  return h;
}


/// Find the lowest ceiling height in the surrounding sectors, not including this sector.
fixed_t sector_t::FindLowestCeilingSurrounding()
{
  fixed_t h = fixed_t::FMAX;
  bool foundsector = false;

  if (boomsupport)
    h = 32000; //SoM: 3/7/2000: Remove ovf
                                              
  for (int i=0; i < linecount; i++)
    {
      sector_t *other = getNextSector(lines[i], this);

      if (!other)
	continue;

      if (other->ceilingheight < h || !foundsector)
	h = other->ceilingheight;

      if (!foundsector)
	foundsector = true;
    }
  return h;
}


/// Find the highest ceiling height in the surrounding sectors, not including this sector.
fixed_t sector_t::FindHighestCeilingSurrounding()
{
  fixed_t h = 0;
  bool foundsector = false;

  for (int i=0; i < linecount; i++)
    {
      sector_t *other = getNextSector(lines[i], this);

      if (!other)
	continue;

      if (other->ceilingheight > h || !foundsector)
	h = other->ceilingheight;

      if (!foundsector)
	foundsector = true;
    }
  return h;
}


/// Finds the highest floor in the surrounding sectors lower than currentheight.
/// If no such sectors exist, returns currentheight.
fixed_t sector_t::FindNextLowestFloor(fixed_t currentheight)
{
  sector_t *other;

  for (int i=0; i < linecount; i++)
    if ((other = getNextSector(lines[i], this)) && other->floorheight < currentheight)
      {
	fixed_t h = other->floorheight;
	while (++i < linecount)
	  if ((other = getNextSector(lines[i], this)) &&
	      other->floorheight > h &&
	      other->floorheight < currentheight)
	    h = other->floorheight;
	return h;
      }
  return currentheight;
}


/// Finds the lowest floor in the surrounding sectors higher than currentheight.
/// If no such sectors exist, returns currentheight.
// Rewritten by Lee Killough to avoid fixed array and to be faster
fixed_t sector_t::FindNextHighestFloor(fixed_t currentheight)
{
  sector_t *other;

  for (int i=0; i < linecount; i++)
    if ((other = getNextSector(lines[i], this)) && other->floorheight > currentheight)
      {
	fixed_t h = other->floorheight;
	while (++i < linecount)
	  if ((other = getNextSector(lines[i], this)) &&
	      other->floorheight < h &&
	      other->floorheight > currentheight)
	    h = other->floorheight;
	return h;
      }
  return currentheight;
}


/// Finds the highest ceiling in the surrounding sectors lower than currentheight.
/// If no such sectors exist, returns currentheight.
fixed_t sector_t::FindNextLowestCeiling(fixed_t currentheight)
{
  sector_t *other;

  for (int i=0; i < linecount; i++)
    if ((other = getNextSector(lines[i], this)) && other->ceilingheight < currentheight)
      {
	fixed_t h = other->ceilingheight;
	while (++i < linecount)
	  if ((other = getNextSector(lines[i], this)) &&
	      other->ceilingheight > h &&
	      other->ceilingheight < currentheight)
	    h = other->ceilingheight;
	return h;
      }
  return currentheight;
}


/// Finds the lowest ceiling in the surrounding sectors higher than currentheight.
/// If no such sectors exist, returns currentheight.
fixed_t sector_t::FindNextHighestCeiling(fixed_t currentheight)
{
  sector_t *other;

  for (int i=0; i < linecount; i++)
    if ((other = getNextSector(lines[i], this)) && other->ceilingheight > currentheight)
      {
	fixed_t h = other->ceilingheight;
	while (++i < linecount)
	  if ((other = getNextSector(lines[i], this)) &&
	      other->ceilingheight < h &&
	      other->ceilingheight > currentheight)
	    h = other->ceilingheight;
	return h;
      }
  return currentheight;
}


// Passed a sector number, returns the shortest lower texture on a
// linedef bounding the sector.
//
// TODO in this and FindShortestUpperAround: replace all indices with pointers

fixed_t sector_t::FindShortestLowerAround()
{
  float minsize = boomsupport ? 32000 : MAXINT; // texture height!

  for (int i = 0; i < linecount; i++)
    {
      if (twoSided(i))
	{
	  side_t *side = getSide(i, 0);
	  if (side->bottomtexture)
	    if (side->bottomtexture->worldheight < minsize)
	      minsize = side->bottomtexture->worldheight;
	  side = getSide(i, 1);
	  if (side->bottomtexture)
	    if (side->bottomtexture->worldheight < minsize)
	      minsize = side->bottomtexture->worldheight;
	}
    }
  return minsize;
}



// Passed a sector number, returns the shortest upper texture on a
// linedef bounding the sector.

fixed_t sector_t::FindShortestUpperAround()
{
  float minsize = boomsupport ? 32000 : MAXINT; // texture height!

  for (int i = 0; i < linecount; i++)
    {
      if (twoSided(i))
	{
	  side_t *side = getSide(i, 0);
	  if (side->toptexture > 0)
	    if (side->toptexture->worldheight < minsize)
	      minsize = side->toptexture->worldheight;
	  side = getSide(i, 1);
	  if (side->toptexture > 0)
	    if (side->toptexture->worldheight < minsize)
	      minsize = side->toptexture->worldheight;
	}
    }
  return minsize;
}



// Passed a floor height and a sector number, return a pointer to a
// a sector with that floor height across the lowest numbered two sided
// line surrounding the sector.
//
// Note: If no sector at that height bounds the sector passed, return NULL

sector_t *sector_t::FindModelFloorSector(fixed_t floordestheight)
{
  sector_t *sec = this; // Boom demo compatibility with a Doom bug?
  for (int i = 0; i < (!boomsupport && sec->linecount<linecount ? sec->linecount : linecount); i++)
    {
      if (twoSided(i))
	{
	  sec = getSector(i, 0);
	  if (sec == this)
	    sec = getSector(i, 1);

	  if (sec->floorheight == floordestheight)
	    return sec;
	}
    }
  return NULL;
}



// Passed a ceiling height and a sector number, return a pointer to a
// a sector with that ceiling height across the lowest numbered two sided
// line surrounding the sector.
//
// Note: If no sector at that height bounds the sector passed, return NULL

sector_t *sector_t::FindModelCeilingSector(fixed_t ceildestheight)
{
  sector_t *sec = this; // Boom demo compatibility with a Doom bug?
  for (int i = 0; i < (!boomsupport && sec->linecount<linecount ? sec->linecount : linecount); i++)
    {
      if (twoSided(i))
	{
	  sec = getSector(i, 0);
	  if (sec == this)
	    sec = getSector(i, 1);

	  if (sec->ceilingheight == ceildestheight)
	    return sec;
	}
    }
  return NULL;
}


/// Find minimum light from an adjacent sector
int sector_t::FindMinSurroundingLight(int max)
{
  int min = max;
  for (int i=0; i < linecount; i++)
    {
      sector_t *check = getNextSector(lines[i], this);

      if (!check)
	continue;

      if (check->lightlevel < min)
	min = check->lightlevel;
    }
  return min;
}


// Passed a linedef special class (floor, ceiling, lighting) and a sector
// returns whether the sector is already busy with a linedef special of the
// same class. If old demo compatibility true, all linedef special classes
// are the same.

bool sector_t::Active(special_e t)
{
  if (!boomsupport)
    return floordata || ceilingdata || lightingdata;
  else switch (t)
    {
    case floor_special:
      return floordata;

    case ceiling_special:
      return ceilingdata;

    case lighting_special:
      return lightingdata;
    }

  return true;
}



//====================================================================
//                 Sector tags and line id's
//====================================================================


// RETURN NEXT SECTOR # THAT LINE TAG REFERS TO
//SoM: 3/7/2000: Killough wrote this to improve the process.
int Map::FindSectorFromTag(unsigned tag, int start)
{
  start = (start >= 0) ? sectors[start].nexttag :
    sectors[(unsigned) tag % (unsigned) numsectors].firsttag;
  while (start >= 0 && sectors[start].tag != tag)
    start = sectors[start].nexttag;
  return start;
}


//SoM: 3/7/2000: More boom specific stuff...
// killough 4/16/98: Same thing, only for linedefs
// called from ACS, line teleport code/bot code, wall scrollers, 260 transparency
#warning TODO lineid: 260 transparency, line teleport 70, scrollers
line_t *Map::FindLineFromID(unsigned lineid, int *start)
{
  int index = (*start >= 0) ? lines[*start].nextid :
    lines[lineid % (unsigned) numlines].firstid;

  for ( ; index >= 0; index = lines[index].nextid)
    if (lines[index].lineid == lineid)
      {
	*start = index;
	return &lines[index];
      }

  // not found
  *start = index;
  return NULL;
}


//SoM: 3/7/2000: Oh joy!
// Hash the sector tags across the sectors and line ids across the linedefs.
void Map::InitTagLists()
{
  register int i;

  for (i=numsectors; --i>=0; )
    sectors[i].firsttag = -1;
  for (i=numsectors; --i>=0; )
    {
      int j = sectors[i].tag % (unsigned) numsectors;
      sectors[i].nexttag = sectors[j].firsttag;
      sectors[j].firsttag = i;
    }

  for (i=numlines; --i>=0; )
    lines[i].firstid = -1;
  for (i=numlines; --i>=0; )
    {
      int j = lines[i].lineid % (unsigned) numlines;
      lines[i].nextid = lines[j].firstid;
      lines[j].firstid = i;
    }
}



// Checks if the sector(s) with a given tag are still active
bool Map::TagBusy(unsigned tag)
{
  int i = -1;
  while ((i = FindSectorFromTag(tag, i)) >= 0)
    {
      if (sectors[i].floordata ||
	  sectors[i].ceilingdata ||
	  sectors[i].lightingdata)
	return true;
    }
  return false;
}






//====================================================================
//                       Sector specials
//====================================================================


/// We try to be as tolerant as possible here.
/// Internally we use modified "generalized Boom sector types".
/// The high byte is used for flags.
int Map::SpawnSectorSpecial(int sp, sector_t *sec)
{
  enum
  {
    DOOM_Light_Flicker    = 1,
    DOOM_Light_BlinkFast  = 2,
    DOOM_Light_BlinkSlow  = 3,
    DOOM_Light_StrobeHurt = 4,
    DOOM_Damage_Hellslime = 5,

    DOOM_Unused1,
    DOOM_Damage_Nukage     = 7,
    DOOM_Light_Glow        = 8,
    DOOM_Secret            = 9,
    DOOM_SpawnDoorClose30s = 10,

    DOOM_Damage_EndLevel   = 11,
    DOOM_Light_SyncFast    = 12,
    DOOM_Light_SyncSlow    = 13,
    DOOM_SpawnDoorOpen5min = 14,
    DOOM_Unused2,

    DOOM_DamageSuperHellslime = 16,
    DOOM_Light_Fireflicker    = 17,

    BOOM_LIGHTMASK    = 0x001F, // bits 0-4
    BOOM_DAMAGEMASK   = 0x0060, // bits 5-6
    BOOM_Secret       = 0x0080, // bit 7

    HERETIC_Lava_FlowEast = 4,
    HERETIC_Lava_Wimpy    = 5,
    HERETIC_Damage_Sludge = 7,
    HERETIC_Friction_Low  = 15,
    HERETIC_Lava_Hefty    = 16,

    HEXEN_Light_Phased = 1,
    HEXEN_Light_SequenceStart = 2,
  };


  if (sp == 0)
    {
      sec->special = 0;
      return 0;
    }

  //CONS_Printf("sec %d: %d  => ", sec-sectors, sp);

  if (sp == DOOM_Secret)
    {
      secrets++;
      sp &= ~SS_SPECIALMASK; // zero low byte
      sp |= SS_secret;
      sec->special = sp;
      return sp;
    }

  const float HScrollDirs[4][2] = {{1,0}, {0,1}, {0,-1}, {-1,0}};
  const float HScrollSpeeds[5] = { 5/32.0, 10/32.0, 25/32.0, 30/32.0, 35/32.0 };
  const float d = 0.707;
  const float XScrollDirs[8][2] = {{0,1}, {1,0}, {0,-1}, {-1,0}, {-d,d}, {d,d}, {d,-d}, {-d,-d}};

  int temp;
  if (hexen_format)
    {
      // Boom damage (and secret) bits cannot be interpreted 'cos they are used otherwise

      temp = sp & SS_SPECIALMASK; // low byte

      if (temp >= 40 && temp <= 51)
	{
	  // Hexen winds (just like Heretic?)
	  temp -= 40; // zero base

	  fixed_t dx = HScrollDirs[temp/3][0]*HScrollSpeeds[temp%3];
	  fixed_t dy = HScrollDirs[temp/3][1]*HScrollSpeeds[temp%3];
	  
	  AddThinker(new scroll_t(scroll_t::sc_wind, dx, dy, NULL, sec - sectors, false));
	}
      else if (temp >= 201 && temp <= 224)
	{
	  // Hexen scrollers
	  temp -= 201; // zero base

	  fixed_t dx = XScrollDirs[temp/3][0]*HScrollSpeeds[temp%3];
	  fixed_t dy = XScrollDirs[temp/3][1]*HScrollSpeeds[temp%3];
	  
	  AddThinker(new scroll_t(scroll_t::sc_carry_floor, dx, dy, NULL, sec - sectors, false));
	}
      else switch (temp)
	{
	case HEXEN_Light_Phased:
	  // Hardcoded base, use sector->lightlevel as the index
	  new phasedlight_t(this, sec, 80, -1);
	  break;

	case HEXEN_Light_SequenceStart:
	  SpawnPhasedLightSequence(sec, 1);
	  break;

	case SS_LightSequence_1:
	case SS_LightSequence_2:
	  // Phased light sequencing. Leave them be, they fit into the unused lightmask area.
	case SS_Stairs_Special1:
	case SS_Stairs_Special2:
	  // Same with stair sequences.
	case SS_IndoorLightning1:
	case SS_IndoorLightning2:
	case SS_Sky2:
	  // and these.
	  sec->special = sp;
	  return sp;

	default:
	  break;
	}

      sp &= ~SS_SPECIALMASK; // zero low byte
      sec->special = sp;
      return sp;
    }

  // Hexen done, Doom/Boom/Heretic to go

  if (sp & BOOM_Secret)
    {
      secrets++;
      sp |= SS_secret;
    }

  if (game.mode == gm_heretic)
    {
      temp = sp & 0x3F; // six lowest bits
      sp &= ~SS_SPECIALMASK; // zero low byte

      if (temp < 20)
	switch (temp)
	  {
	  case HERETIC_Damage_Sludge:
	    sec->damage = 4 | dt_corrosive;
	    sp |= SS_damage_32;
	    break;

	  case HERETIC_Lava_FlowEast:
	    AddThinker(new scroll_t(scroll_t::sc_carry_floor, 28.0f/32.0f, 0, NULL, sec - sectors, false));
	    // fallthru
	  case HERETIC_Lava_Wimpy:
	    sec->damage = 5 | dt_heat;
	    sp |= SS_damage_16;
	    break;

	  case HERETIC_Lava_Hefty:
	    sec->damage = 8 | dt_heat;
	    sp |= SS_damage_16;
	    break;

	  case HERETIC_Friction_Low:
	    sec->friction = 0.97266f;
	    sec->movefactor = 0.25f;
	    sp |= SS_friction;
	    break;

	  default:
	    // all the rest of the specials are identical to Doom
	    // so they are handled using the Doom code later on
	    sp |= temp;
	  }
      else if (temp <= 39)
	{
	  // Heretic scrollers
	  temp -= 20; // zero base

	  fixed_t dx = HScrollDirs[temp/5][0]*HScrollSpeeds[temp%5];
	  fixed_t dy = HScrollDirs[temp/5][1]*HScrollSpeeds[temp%5];

	  // texture scrolls, actors are pushed
	  AddThinker(new scroll_t(scroll_t::sc_floor | scroll_t::sc_carry_floor,
				  dx, dy, NULL, sec - sectors, false));
	}
      else if (temp <= 51)
	{
	  // Heretic winds
	  temp -= 40; // zero base

	  fixed_t dx = HScrollDirs[temp/3][0]*HScrollSpeeds[temp%3];
	  fixed_t dy = HScrollDirs[temp/3][1]*HScrollSpeeds[temp%3];
	  
	  AddThinker(new scroll_t(scroll_t::sc_carry_floor | scroll_t::sc_wind,
				  dx, dy, NULL, sec - sectors, false));
	}
    }
  else
    {
      const char BoomDamage[4] = {0, 5, 10, 20};
      // in Heretic (and Hexen), Boom damage bits cannot be used because of winds and scrollers
      // Boom damage flags
      temp = (sp & BOOM_DAMAGEMASK) >> 5;
      if (temp)
	{
	  sp |= SS_damage_32;
	  sec->damage = BoomDamage[temp] | dt_radiation; // could as well choose randomly?
	}
    }


  // Doom / Boom / some Heretic types

  temp = sp & BOOM_LIGHTMASK;
  sp &= ~SS_SPECIALMASK; // zero low byte

  int i, dam = 0;
  lightfx_t *lfx = NULL;

  const short ff_tics = 4;
  const short glowspeed = 8;

  switch (temp)
    {
    case DOOM_Damage_Nukage:  // nukage/slime
      dam = 5;
      break;

    case DOOM_Damage_Hellslime:
      dam = 10;
      break;

    case DOOM_Light_StrobeHurt:
      SpawnStrobeLight(sec, STROBEBRIGHT, FASTDARK, false); // fallthru
    case DOOM_DamageSuperHellslime:
      dam = 20;
      break;

    case DOOM_Damage_EndLevel: // level end hurt need special handling
      dam = 20;
      sp |= SS_EndLevelHurt; // HACK
      break;

    case DOOM_SpawnDoorClose30s: // after 30 s, close door
      SpawnDoorCloseIn30(sec);
      break;

    case DOOM_SpawnDoorOpen5min: // after 5 min, open door
      SpawnDoorRaiseIn5Mins(sec);
      break;

    case DOOM_Light_Flicker:
      i = sec->FindMinSurroundingLight(sec->lightlevel);
      lfx = new lightfx_t(this, sec, lightfx_t::Flicker, sec->lightlevel, i, 64, 7);
      lfx->count = (P_Random() & lfx->maxtime) + 1;
      break;

    case DOOM_Light_BlinkFast:
      SpawnStrobeLight(sec, STROBEBRIGHT, FASTDARK, false);
      break;

    case DOOM_Light_BlinkSlow:
      SpawnStrobeLight(sec, STROBEBRIGHT, SLOWDARK, false);
      break;

    case DOOM_Light_Glow:
      i = sec->FindMinSurroundingLight(sec->lightlevel);
      if (i < sec->lightlevel)
	lfx = new lightfx_t(this, sec, lightfx_t::Glow, sec->lightlevel, i, -glowspeed << 6, -1);
      break;

    case DOOM_Light_SyncFast:
      SpawnStrobeLight(sec, STROBEBRIGHT, FASTDARK, true);
      break;

    case DOOM_Light_SyncSlow:
      SpawnStrobeLight(sec, STROBEBRIGHT, SLOWDARK, true);
      break;

    case DOOM_Light_Fireflicker:
      i = sec->FindMinSurroundingLight(sec->lightlevel) + 16;
      lfx = new lightfx_t(this, sec, lightfx_t::FireFlicker, sec->lightlevel, i, ff_tics);
      lfx->count = ff_tics;
      break;

    default:
      break;
    }

  if (dam)
    {
      sp |= SS_damage_32;
      sec->damage = dam | dt_radiation; // could as well choose randomly?
    }

  /*
    // TODO support phased lighting with specials 21-24 ? (like ZDoom)
    else if (temp < 40)
    {
    temp -= 20;
    }
  */

  sec->special = sp;
  return sp;
}




// flat floortypes
static struct
{
  const char        *name;
  floortype_t  type;
}
ftypes[] =
{
  // Heretic
  {"FWATER",   FLOOR_WATER},
  {"FLTWAWA1", FLOOR_WATER},
  {"FLTFLWW1", FLOOR_WATER},
  {"FLTLAVA1", FLOOR_LAVA},
  {"FLATHUH1", FLOOR_LAVA},
  {"FLTSLUD1", FLOOR_SLUDGE},

  // Hexen
  {"X_005", FLOOR_WATER},
  {"X_001", FLOOR_LAVA},
  {"X_009", FLOOR_SLUDGE},
  {"F_033", FLOOR_ICE}
};


/// Sets some floortype-dependent attributes depending on the floor texture.
void sector_t::SetFloorType(const char *pic)
{
  // Ohhhh... This sucks so much... FIXME, TERRAIN lump?
  for (int i=0; i<10; i++)
    if (!strncasecmp(pic, ftypes[i].name, 8))
      {
	floortype = ftypes[i].type;
	goto found;
      }

  floortype = FLOOR_SOLID;

 found:

  extern float normal_friction;
  const float friction_low = 0.973f; // 0xf900

  switch (floortype)
    {
    case FLOOR_ICE:
      friction = friction_low;
      movefactor = 0.5f;
      special |= SS_friction;
      break;

    default:
      friction = normal_friction;
      movefactor = 1.0f;
      special &= ~SS_friction;
      break;
    }
}



//====================================================================
//         Clearable line specials (handled during Map setup)
//====================================================================


//SoM: 3/23/2000: Adds a sectors floor and ceiling to a sector's ffloor list
static void P_AddFFloor(sector_t* sec, ffloor_t* ffloor)
{
  ffloor_t* rover;

  if(!sec->ffloors)
    {
      sec->ffloors = ffloor;
      ffloor->next = 0;
      ffloor->prev = 0;
      return;
    }

  for(rover = sec->ffloors; rover->next; rover = rover->next);

  rover->next = ffloor;
  ffloor->prev = rover;
  ffloor->next = 0;
}


void Map::AddFakeFloor(sector_t* sec, sector_t* sec2, line_t* master, int flags)
{
  //Add the floor
  ffloor_t *ffloor = (ffloor_t*)Z_Malloc(sizeof(ffloor_t), PU_LEVEL, NULL);
  ffloor->secnum = sec2 - sectors;
  ffloor->target = sec;
  ffloor->bottomheight     = &sec2->floorheight;
  ffloor->bottompic        = &sec2->floorpic;
  ffloor->bottomlightlevel = &sec2->lightlevel;
  ffloor->bottomxoffs      = &sec2->floor_xoffs;
  ffloor->bottomyoffs      = &sec2->floor_yoffs;

  //Add the ceiling
  ffloor->topheight     = &sec2->ceilingheight;
  ffloor->toppic        = &sec2->ceilingpic;
  ffloor->toplightlevel = &sec2->lightlevel;
  ffloor->topxoffs      = &sec2->ceiling_xoffs;
  ffloor->topyoffs      = &sec2->ceiling_yoffs;

  ffloor->flags = ffloortype_e(flags);
  ffloor->master = master;

  if (flags & FF_TRANSLUCENT)
    {
      if (master->sideptr[0]->toptexture > 0)
	ffloor->alpha = 0; // FIXME NOW master->sideptr[0]->toptexture;
      else
	ffloor->alpha = 0x70;
    }

  if (sec2->numattached == 0)
    {
      sec2->attached = (int *)malloc(sizeof(int));
      sec2->attached[0] = sec - sectors;
      sec2->numattached = 1;
    }
  else
    {
      sec2->attached = (int *)realloc(sec2->attached, sizeof(int) * (sec2->numattached + 1));
      sec2->attached[sec2->numattached] = sec - sectors;
      sec2->numattached ++;
    }

  P_AddFFloor(sec, ffloor);
}



/// After the map has been loaded, scan for linedefs
/// that spawn thinkers or confer properties
void Map::SpawnLineSpecials()
{
  //  Init line EFFECTs
  for (int i = 0; i < numlines; i++)
    {
      line_t *l = &lines[i];
      int special = l->special;
      if (!special)
	continue;

      int subtype;
      int tag = l->tag; // Doom format: use the tag if we have one

      // Legacy extensions are mapped to Hexen linedef namespace so they are reachable from Hexen as well!
      // only check for clearable stuff here
      if (special == LINE_LEGACY_EXT && (subtype = l->args[0]) < 128)
	{
	  int sec = l->sideptr[0]->sector - sectors;
	  int kind = l->args[1];

	  switch (subtype)
	    {
	    case LINE_LEGACY_BOOM_SCROLLERS:
	      SpawnScroller(l, tag, kind, l->args[2]);
	      break;

	    case LINE_LEGACY_EXOTIC_TEXTURE:
	      // types which store data in the texture name fields
	      switch (kind)
		{
		  // Boom 242: fake floor and ceiling
		case 1:
		  for (int s = -1; (s = FindSectorFromTag(tag, s)) >= 0; )
		    {
		      sectors[s].heightsec = sec;
		    }
		  break;

		  // Boom 260: transparent middle texture
		case 2:
		  {
		    int temp = l->transmap;
		    if (temp == -1)
		      temp = 0; // default, TRANMAP

		    if (l->lineid)
		      {
			l->transmap = -1;
			line_t *l2;
			for (int s = -1; (l2 = FindLineFromID(l->lineid, &s)); )
			  l2->transmap = temp; // make tagged lines translucent too
		      }
		    else
		      l->transmap = temp;
		  }
		  break;

		  // Legacy 280: swimmable water with Boom 242-style colormaps
		case 3:
		  for (int s = -1; (s = FindSectorFromTag(tag, s)) >= 0; )
		    {
		      sectors[s].heightsec = sec;
		      sectors[s].heightsec_type = sector_t::CS_water;
		    }
		  break;

		  // Legacy 282: easy colormap/fog effect
		case 4:
		  for (int s = -1; (s = FindSectorFromTag(tag, s)) >= 0; )
		    {
		      sectors[s].midmap = l->frontsector->midmap;
		    }
		  break;

		default:
		  goto error;
		}
	      break;

	    case LINE_LEGACY_FAKEFLOOR:  // fake floors
	      {
	      int ff_flags = FF_EXISTS;

	      // make it simple for now
	      switch (kind + 281)
		{
		case 281: // Legacy: 3D floor
		  ff_flags |= FF_SOLID|FF_RENDERALL|FF_CUTLEVEL;
		  break;

		case 289: // Legacy: 3D floor without shadow
		  ff_flags |= FF_SOLID|FF_RENDERALL|FF_NOSHADE|FF_CUTLEVEL;
		  break;

		case 300: // Legacy: translucent 3D floor
		  ff_flags |= FF_SOLID|FF_RENDERALL|FF_NOSHADE|FF_TRANSLUCENT|FF_EXTRA|FF_CUTEXTRA;
		  break;

		case 301: // Legacy: translucent swimmable water
		  ff_flags |= FF_RENDERALL|FF_TRANSLUCENT|FF_SWIMMABLE|FF_BOTHPLANES |
		    FF_ALLSIDES|FF_CUTEXTRA|FF_EXTRA|FF_DOUBLESHADOW|FF_CUTSPRITES;
		  break;

		case 304: // Legacy: Opaque water
		  ff_flags |= FF_RENDERALL|FF_SWIMMABLE|FF_BOTHPLANES |
		    FF_ALLSIDES|FF_CUTEXTRA|FF_EXTRA|FF_DOUBLESHADOW|FF_CUTSPRITES;
		  break;

		case 302: // Legacy: 3D fog
		  // SoM: Because it's fog, check for an extra colormap and set the fog flag...
		  if (sectors[sec].extra_colormap)
		    sectors[sec].extra_colormap->fog = 1;
		  ff_flags |= FF_RENDERALL|FF_FOG|FF_BOTHPLANES|FF_INVERTPLANES |
		    FF_ALLSIDES|FF_INVERTSIDES|FF_CUTEXTRA|FF_EXTRA|FF_DOUBLESHADOW|FF_CUTSPRITES;
		  break;

		case 303: // Legacy: Light effect
		  ff_flags |= FF_CUTSPRITES;
		  break;

		case 305: // Legacy: Double light effect
		  ff_flags |= FF_CUTSPRITES|FF_DOUBLESHADOW;
		  break;

		case 306: // Legacy: Invisible barrier
		  ff_flags |= FF_SOLID;
		  break;

		default:
		  goto error;
		}

	      if (ff_flags)
		for (int s = -1; (s = FindSectorFromTag(tag, s)) >= 0; )
		  AddFakeFloor(&sectors[s], &sectors[sec], lines+i, ff_flags);
	      }
	      break;

	    case LINE_LEGACY_RENDERER:
	      if (kind >= 100)
		l->transmap = kind - 100; // transmap number
	      else if (kind == 0) // 283 (legacy fog sheet)
		continue;  // FIXME fog sheet requires keeping (renderer!, r_segs):
	      break;
	    
	    case LINE_LEGACY_MISC:
	      switch (kind)
		{
		  // Instant lower for floor SSNTails 06-13-2002
		case 0:
		  EV_DoFloor(tag, l, floor_t::LnF, fixed_t::FMAX/2, 0, 0);
		  break;
	  
		  // Instant raise for ceilings SSNTails 06-13-2002
		case 1:
		  EV_DoCeiling(tag, l, ceiling_t::HnC, fixed_t::FMAX/2, 0, 0);
		  break;

		default:
		  goto error;
		}
	      break;

	    default:
	      goto error;
	    }

	  l->special = 0;
	  continue;

	error:
	  CONS_Printf("Unknown Legacy linedef subtype %d, %d in line %d.\n", subtype, kind, i);
	}


      // finally check ungrouped specials 
      switch (special)
        {
	  // Hexen
	case 100: // Scroll_Texture_Left
          AddThinker(new scroll_t(scroll_t::sc_side, fixed_t(l->args[0]) >> 6, 0, NULL, l->sideptr[0] - sides, false));
	  break;
	case 101: // Scroll_Texture_Right
          AddThinker(new scroll_t(scroll_t::sc_side, fixed_t(-l->args[0]) >> 6, 0, NULL, l->sideptr[0] - sides, false));
	  break;
	case 102: // Scroll_Texture_Up
          AddThinker(new scroll_t(scroll_t::sc_side, 0, fixed_t(l->args[0]) >> 6, NULL, l->sideptr[0] - sides, false));
	  break;
	case 103: // Scroll_Texture_Down
          AddThinker(new scroll_t(scroll_t::sc_side, 0, fixed_t(-l->args[0]) >> 6, NULL, l->sideptr[0] - sides, false));
	  break;

	  //============   ZDoom specials   =============

	case 209: // Transfer_Heights
	case 210: // Transfer_FloorLight
	case 211: // Transfer_CeilingLight
	  {
	    int sec = l->sideptr[0]->sector - sectors;

	    for (int s = -1; (s = FindSectorFromTag(tag, s)) >= 0; )
	      if (special == 209)
		sectors[s].heightsec = sec; // TODO "when" parameter
	      else if (special == 210)
		sectors[s].floorlightsec = sec;
	      else
		sectors[s].ceilinglightsec = sec;
	  }
	  break;

	case 218: // Sector_SetWind
	  SpawnPusher(l, tag, pusher_t::p_wind);
	  break;

	case 219: // Sector_SetFriction
	  SpawnFriction(l, tag);
	  break;

	case 220: // Sector_SetCurrent
	  SpawnPusher(l, tag, pusher_t::p_current);
	  break;

	case 222: // Scroll_Texture_Model
	  SpawnScroller(l, tag, scroll_t::sc_side, l->args[1] & 0x3);
	  break;

	case 223: // Scroll_Floor
	  {
	    int type = (l->args[2] > 0) ? scroll_t::sc_carry_floor : 0;
	    if (l->args[2] != 1)
	      type |= scroll_t::sc_floor;

	    if (l->args[1] & 0x4)
	      SpawnScroller(l, tag, type, l->args[1] & 0x3);
	    /* TODO
	    else
	      SpawnScroller(l->args[3]-128, l->args[4]-128, tag, type, l->args[1] & 0x3);
	    */
	  }
	  break;

	case 224: // Scroll_Ceiling
	  {
	    if (l->args[1] & 0x4)
	      SpawnScroller(l, tag, scroll_t::sc_ceiling, l->args[1] & 0x3);
	    /*
	    else
	      SpawnScroller(l->args[3]-128, l->args[4]-128, tag, scroll_t::sc_ceiling, l->args[1] & 0x3);
	    */
	  }
	  break;

	case 225: // Scroll_Texture_Offsets
	  SpawnScroller(l, tag, scroll_t::sc_side, scroll_t::sc_offsets);
	  break;

	case 227: // PointPush_SetForce
	  SpawnPusher(l, tag, pusher_t::p_point);
	  break;

	default:
	  continue; // do nothing
        }


      l->special = 0;
    }
}







//==========================================================================
//  Scrollers
//==========================================================================

IMPLEMENT_CLASS(scroll_t, Thinker);
scroll_t::scroll_t() {}

//
// This function, with the help of r_plane.c and r_bsp.c, supports generalized
// scrolling floors and walls, with optional mobj-carrying properties, e.g.
// conveyor belts, rivers, etc. A linedef with a special type affects all
// tagged sectors the same way, by creating scrolling and/or object-carrying
// properties. Multiple linedefs may be used on the same sector and are
// cumulative, although the special case of scrolling a floor and carrying
// things on it, requires only one linedef. The linedef's direction determines
// the scrolling direction, and the linedef's length determines the scrolling
// speed. This was designed so that an edge around the sector could be used to
// control the direction of the sector's scrolling, which is usually what is
// desired.
//

void scroll_t::Think()
{
  fixed_t tdx = vx, tdy = vy;

  if (control)
    {   // compute scroll amounts based on a sector's height changes
      fixed_t height = control->floorheight + control->ceilingheight;
      fixed_t delta = height - last_height;
      last_height = height;
      tdx *= delta;
      tdy *= delta;
    }

  if (accel)
    {
      vdx = tdx += vdx;
      vdy = tdy += vdy;
    }

  if (!tdx && !tdy)                   // no-op if both (x,y) offsets 0
    return;

  if (type == sc_side)  //Scroll wall texture
    {
      side_t *side = mp->sides + affectee;
      side->textureoffset += tdx;
      side->rowoffset += tdy;
      return;
    }

  sector_t *sec = mp->sectors + affectee;

  if (type & sc_floor)  //Scroll floor texture
    {
      sec->floor_xoffs += tdx;
      sec->floor_yoffs += tdy;
    }
  
  if (type & sc_ceiling)  //Scroll ceiling texture
    {
      sec->ceiling_xoffs += tdx;
      sec->ceiling_yoffs += tdy;
    }


  // Factor to scale scrolling effect into mobj-carrying properties = 3/32.
  // (This is so scrolling floors and objects on them can move at same speed.)
  const fixed_t CARRYFACTOR = 0.09375f;

  if (type & sc_carry_floor)
    {
      tdx *= -CARRYFACTOR; // it seems texture offsets go the other way?
      tdy *=  CARRYFACTOR;

      fixed_t height = sec->floorheight;
      fixed_t waterheight = sec->heightsec != -1 &&
        mp->sectors[sec->heightsec].floorheight > height ?
        mp->sectors[sec->heightsec].floorheight : fixed_t::FMIN;

      for (msecnode_t *node = sec->touching_thinglist; node; node = node->m_snext)
	{
	  Actor *thing = node->m_thing;

	  if (type & sc_wind && !(thing->flags2 & MF2_WINDTHRUST))
	    continue;

	  if (!(thing->flags & MF_NOCLIPLINE) &&
	      (!(thing->flags & MF_NOGRAVITY || thing->Feet() > height) ||
	       thing->Feet() < waterheight))
          {
            // Move objects only if on floor or underwater,
            // non-floating, and clipped.
            thing->vel.x += tdx;
            thing->vel.y += tdy;
          }
	}
    }
}

// Add a generalized scroller to the thinker list.
//
// type: the enumerated type of scrolling: floor, ceiling, floor carrier,
//   wall, floor carrier & scroller
//
// (dx,dy): the direction and speed of the scrolling or its acceleration
//
// control: the sector whose heights control this scroller's effect
//   remotely, or -1 if no control sector
//
// affectee: the index of the affected object (sector or sidedef)
//
// accel: non-zero if this is an accelerative effect

scroll_t::scroll_t(short t, fixed_t dx, fixed_t dy, sector_t *csec, int aff, bool acc)
{
  type = t;
  vx = dx;
  vy = dy;
  accel = acc;
  vdx = vdy = 0;
  control = csec;
  affectee = aff;

  if (control)
    last_height = control->floorheight + control->ceilingheight;
}




/// spawns Boom wall, floor and ceiling scrollers
void Map::SpawnScroller(line_t *l, unsigned tag, int type, int control)
{
  int s = l->sideptr[0] - sides;
  line_t *l2;
  fixed_t dx = l->dx >> scroll_t::SCROLL_SHIFT;  // direction and speed of scrolling
  fixed_t dy = l->dy >> scroll_t::SCROLL_SHIFT;

  // possible control sector
  bool accel = control & scroll_t::sc_accelerative; // implies also sc_displacement
  sector_t *csec = (control & scroll_t::sc_displacement) || accel ? sides[s].sector : NULL;

  if (type == scroll_t::sc_side)
    {
      if (control & scroll_t::sc_offsets)
	{
	  AddThinker(new scroll_t(scroll_t::sc_side, -sides[s].textureoffset,
				  sides[s].rowoffset, NULL, s, accel));
	}
      else
	for (s = -1; (l2 = FindLineFromID(tag, &s)) != NULL; ) // NOTE: "tag" actually a line id, only done with ZDoom type 222
	  if (l2 != l)
	    {
	      // Adds wall scroller. Scroll amount is rotated with respect to wall's
	      // linedef first, so that scrolling towards the wall in a perpendicular
	      // direction is translated into vertical motion, while scrolling along
	      // the wall in a parallel direction is translated into horizontal motion.

	      fixed_t x = abs(l2->dx), y = abs(l2->dy), d;
	      if (y > x)
		d = x, x = y, y = d;
	      d = x / finesine[(ArcTan(y / x) + ANG90) >> ANGLETOFINESHIFT];
	      x = -((dy * l2->dy) + (dx * l2->dx)) / d;
	      y = -((dx * l2->dy) - (dy * l2->dx)) / d;
	      AddThinker(new scroll_t(scroll_t::sc_side, x, y, csec, l2->sideptr[0] - sides, accel));
	    }
      return;
    }

  // okay, the rest of the types are stackable (flags)
  for (s = -1; (s = FindSectorFromTag(tag, s)) >= 0;)
    AddThinker(new scroll_t(type, -dx, dy, csec, s, accel));
}




//==========================================================================
//  Friction
//==========================================================================



//============================================
//SoM: 3/8/2000: added new model of friction for ice/sludge effects
// FIXME Do we even need a friction thinker?
/*
class friction_t : public Thinker
{
  friend class Map;
  DECLARE_CLASS(friction_t)
private:
  float friction;        // friction value (E800 = normal)
  float movefactor;      // inertia factor when adding to momentum
  int   affectee;        // Number of affected sector
public:
  friction_t(float fri, float mf, int aff);
  
  virtual void Think();
};


IMPLEMENT_CLASS(friction_t, Thinker);
friction_t::friction_t() {}

// constructor
// Adds friction thinker.
friction_t::friction_t(float fri, float mf, int aff)
{
  friction = fri;
  movefactor = mf;
  affectee = aff;
}

int friction_t::Marshal(LArchive & a)
{
  return 0;
}

//Function to apply friction to all the things in a sector.
void friction_t::Think()
{
  if (!boomsupport || !variable_friction)
    return;

  sector_t *sec = mp->sectors + affectee;

  // Be sure the special sector type is still turned on. If so, proceed.
  // Else, bail out; the sector type has been changed on us.

  if (!(sec->special & SS_friction))
    return;

  // Assign the friction value to players on the floor, non-floating,
  // and clipped. Normally the object's friction value is kept at
  // ORIG_FRICTION and this thinker changes it for icy or muddy floors.

  // In Phase II, you can apply friction to Things other than players.

  // When the object is straddling sectors with the same
  // floorheight that have different frictions, use the lowest
  // friction value (muddy has precedence over icy).

  Actor      *thing;
  msecnode_t *node = sec->touching_thinglist; // things touching this sector
  while (node)
    {
      thing = node->m_thing;
      // FIXME friction ought to apply to monsters as well
      if (thing->Type() == Thinker::tt_ppawn &&
	  !(thing->flags & (MF_NOGRAVITY | MF_NOCLIPLINE)) &&
	  thing->z <= sec->floorheight)
	{
	  if ((thing->friction == normal_friction) ||
	      (friction < thing->friction))
	    {
	      thing->friction   = friction;
	      thing->movefactor = movefactor;
	    }
	}
      node = node->m_snext;
    }
}
*/

//Spawn all friction.
void Map::SpawnFriction(line_t *l, unsigned tag)
{
  extern float normal_friction;

  // line length controls magnitude
  // ZDoom option: give magnitude in arg2
  float length = l->args[1] ? l->args[1] : P_AproxDistance(l->dx, l->dy).Float();

  // l = 200 gives 1, l = 100 gives the original, l = 0 gives 0.8125

  // friction value to be applied during movement
  //friction = (0x1EB8*length)/0x80 + 0xD000;
  float friction = length * 9.375e-4 + 0.8125;

  if (friction > 1.0)
    friction = 1.0;
  if (friction < 0.0)
    friction = 0.0;

  // object max speed is proportional to movefactor/(1-friction)
  // higher friction value actually means 'less friction'.
  // the movefactors are a bit different from BOOM (better;)

  float movefactor; // applied to each player move to simulate inertia

  if (friction > normal_friction)
    // ice, max speed is unchanged.
    //movefactor = ((0x10092 - friction)*(0x70))/0x158;
    //movefactor = (65682 - friction) * 112/344;
    movefactor = (1.0 - friction) * 10.667;
  else
    // mud
    //movefactor = ((friction - 0xDB34)*(0xA))/0x80;
    //movefactor = (friction - 56116) * 10/128;
    movefactor = (friction - 0.8125) * 10.667;

  // minimum movefactor (1/64)
  if (movefactor < 1.0/64)
    movefactor = 1.0/64;

  for (int s = -1; (s = FindSectorFromTag(tag, s)) >= 0 ; )
    {
      sectors[s].friction   = friction;
      sectors[s].movefactor = movefactor;
      sectors[s].special |= SS_friction;
    }
}


//==========================================================================
//  Pushers
//==========================================================================

IMPLEMENT_CLASS(pusher_t, Thinker);
pusher_t::pusher_t() {}


// constructor
pusher_t::pusher_t(Map *m, sector_t *s, pusher_e t, fixed_t x_m, fixed_t y_m, DActor *src)
  : sectoreffect_t(m, s)
{
  s->special |= SS_wind; // enable pushing
  type = t;
  source = src;
  x_mag = x_m;
  y_mag = y_m;
  magnitude = P_AproxDistance(x_mag, y_mag);
  if (source) // point source exist?
    {
      radius = magnitude << 1; // where force goes to zero
      x = src->pos.x;
      y = src->pos.y;
    }
}


// PIT_PushThing determines the angle and magnitude of the effect.
// The object's x and y momentum values are changed.

static pusher_t *tmpusher; // pusher structure for blockmap searches

bool PIT_PushThing(Actor* thing)
{
  if ((thing->flags2 & MF2_WINDTHRUST) && !(thing->flags & (MF_NOCLIPTHING | MF_NOCLIPLINE)))
    {
      fixed_t sx = tmpusher->x;
      fixed_t sy = tmpusher->y;
      fixed_t dist = P_AproxDistance(thing->pos.x - sx, thing->pos.y - sy);
      fixed_t speed = (tmpusher->magnitude - (dist >> 1)) >> (pusher_t::PUSH_FACTOR + 1);

      // If speed <= 0, you're outside the effective radius. You also have
      // to be able to see the push/pull source point.

      if ((speed > 0) && (thing->mp->CheckSight(thing, tmpusher->source)))
	{
	  angle_t pushangle = R_PointToAngle2(thing->pos.x, thing->pos.y, sx, sy);
	  if (tmpusher->source->type == MT_PUSH)
	    pushangle += ANG180;    // away
	  pushangle >>= ANGLETOFINESHIFT;
	  thing->vel.x += speed * finecosine[pushangle];
	  thing->vel.y += speed * finesine[pushangle];
	}
    }
  return true;
}

// looks for all objects that are inside the radius of
// the effect.

void pusher_t::Think()
{
  if (!allow_pushers)
    return;

  // Be sure the special sector type is still turned on. If so, proceed.
  // Else, bail out; the sector type has been changed on us.

  if (!(sector->special & SS_wind))
    return;

  // For constant pushers (wind/current) there are 3 situations:
  //
  // 1) Affected Thing is above the floor.
  //    Apply the full force if wind, no force if current.
  //
  // 2) Affected Thing is on the ground.
  //    Apply half force if wind, full force if current.
  //
  // 3) Affected Thing is below the ground (underwater effect).
  //    Apply no force if wind, full force if current.
  //
  // Apply the effect to clipped players only for now.
  //
  // In Phase II, you can apply these effects to Things other than players.

  if (type == p_point)
    {
      // Seek out all pushable things within the force radius of this
      // point pusher. Crosses sectors, so use blockmap.

      tmpusher = this; // MT_PUSH/MT_PULL point source
      mp->blockmap->IterateThingsRadius(x, y, radius + MAXRADIUS, PIT_PushThing);
      return;
    }

  // constant pushers p_wind and p_current
  fixed_t ht = 0;

  if (sector->heightsec != -1) // special water sector?
    ht = mp->sectors[sector->heightsec].floorheight;

  msecnode_t *node = sector->touching_thinglist; // things touching this sector

  for ( ; node ; node = node->m_snext)
    {
      Actor *thing = node->m_thing;

      // does the push affect it?
      if (!(thing->flags2 & MF2_WINDTHRUST) || (thing->flags & (MF_NOCLIPTHING | MF_NOCLIPLINE)))
	continue;

      fixed_t xspeed, yspeed;

      if (type == p_wind)
	{
	  if (sector->heightsec == -1) // NOT special water sector
	    if (thing->Feet() > thing->floorz) // above ground
	      {
		xspeed = x_mag; // full force
		yspeed = y_mag;
	      }
	    else // on ground
	      {
		xspeed = (x_mag)>>1; // half force
		yspeed = (y_mag)>>1;
	      }
	  else // special water sector
	    {
	      if (thing->Feet() > ht) // above ground
		{
		  xspeed = x_mag; // full force
		  yspeed = y_mag;
		}
	      else if (thing->Top() < ht) // underwater
		xspeed = yspeed = 0; // no force
	      else // wading in water
		{
		  xspeed = (x_mag)>>1; // half force
		  yspeed = (y_mag)>>1;
		}
	    }
	}
      else // p_current
	{
	  if (sector->heightsec == -1) // NOT special water sector
	    if (thing->Feet() > sector->floorheight) // above ground
	      xspeed = yspeed = 0; // no force
	    else // on ground
	      {
		xspeed = x_mag; // full force
		yspeed = y_mag;
	      }
	  else // special water sector
	    if (thing->Feet() > ht) // above ground
	      xspeed = yspeed = 0; // no force
	    else // underwater
	      {
		xspeed = x_mag; // full force
		yspeed = y_mag;
	      }
	}
      thing->vel.x += xspeed >> PUSH_FACTOR;
      thing->vel.y += yspeed >> PUSH_FACTOR;
    }
}

// Get pusher object.
static DActor *GetPushThing(sector_t *sec)
{
  Actor *thing = sec->thinglist;

  while (thing)
    {
      DActor *dp = thing->Inherits<DActor>();
      if (dp)
	switch (dp->type)
	  {
	  case MT_PUSH:
	  case MT_PULL:
	    return dp;
	  default:
	    break;
	  }

      thing = thing->snext;
    }
  return NULL;
}

// Spawn pushers.
void Map::SpawnPusher(line_t *l, unsigned tag, int type)
{
  int s;

  // Interprets the ZDoom args
  fixed_t dx, dy;

  if (l->args[3]) // useline?
    {
      dx = l->dx;
      dy = l->dy;
    }
  else switch (type)
    {
    case pusher_t::p_point:
      dx = l->args[2]; // amount
      dy = 0;
      break;

    default:
      {
	angle_t ang = l->args[2] << 24; // angle
	dx = l->args[1] * Cos(ang);
	dy = l->args[1] * Sin(ang);
      }
      break;
    }

  switch (type)
    {
    case pusher_t::p_wind:
      for (s = -1; (s = FindSectorFromTag(tag, s)) >= 0 ; )	  
	new pusher_t(this, &sectors[s], pusher_t::p_wind, dx, dy, NULL);
      break;

    case pusher_t::p_current:
      for (s = -1; (s = FindSectorFromTag(tag, s)) >= 0 ; )
	new pusher_t(this, &sectors[s], pusher_t::p_current, dx, dy, NULL);
      break;

    case pusher_t::p_point:
      if (tag)
	for (s = -1; (s = FindSectorFromTag(tag, s)) >= 0 ; )
	  {
	    DActor *thing = GetPushThing(&sectors[s]);
	    if (thing) // No MT_P* means no effect
	      new pusher_t(this, &sectors[s], pusher_t::p_point, dx, dy, thing);
	  }
      else
	{
	  DActor *d;
	  Iterate_TID iter(this, l->args[1]);
	  for (Actor *m = iter.Next(); m; m = iter.Next())
	    if ((d = m->Inherits<DActor>()))
	      new pusher_t(this, m->subsector->sector, pusher_t::p_point, dx, dy, d);
	}
      break;
    }
}
