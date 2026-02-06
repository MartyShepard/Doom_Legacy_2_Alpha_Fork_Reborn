// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: p_saveg.cpp 522 2008-01-20 22:42:23Z smite-meister $
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
/// \brief Archiving: SaveGame I/O.

#include "doomdef.h"
#include "doomdata.h"

#include "command.h"
#include "console.h"

#include "m_archive.h"

#include "g_game.h"
#include "g_level.h"
#include "g_mapinfo.h"
#include "g_map.h"
#include "g_team.h"
#include "g_player.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "g_decorate.h"

#include "n_interface.h"

#include "p_spec.h"
#include "p_polyobj.h"

#include "r_data.h"
#include "r_sprite.h"
#include "hardware/md3.h"
#include "t_acs.h"
#include "t_vari.h"
#include "t_script.h"
#include "t_parse.h"
#include "m_random.h"
#include "m_misc.h"
#include "m_swap.h"
#include "m_menu.h"

#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"
#include "tables.h"

#include "hud.h"

/*
  Rough saving order:
  Game: mapinfo, clustermap, episodes, teams, players, rest.
  MapInfo: Map
  Map: sectors, lines/sides, polyobjs, scripts, Thinkers, rest.
  Thinker: stuff, pointers to other Thinkers.
  PlayerInfo: stuff, pawn, pov, hubsavepawn.

  Extraction of Thinkers requires active_map, which is only set at Map::Unserialize.
  Ergo, if a Thinker needs a Map during unserialization, its Map must be Unserialized first,
  or must save the Map reference as well.
*/

enum consistency_marker_t
{
  // consistency markers in the savegame
  MARK_GROUP  = 0x71717171,
  MARK_MAP    = 0x8ae51d73,
  MARK_POLYOBJ = 0x3c3c3c3c,
  MARK_SCRIPT = 0x37c4fe01,
  MARK_THINK  = 0x1c02fa39,
  MARK_MISC   = 0xabababab,

  // this special marker is used to denote the end of a collection of objects
  MARK_END = 0xffffffff,
};


// assumes a is the LArchive
#define ST_PTR(p, array) {int temp; a << (temp = p ? (p - array) : -1);}
#define RE_PTR(p, array) {int temp; a << temp; p = (temp == -1) ? NULL : &array[temp];}

//==============================================
// Marshalling functions for various Thinkers
//==============================================

int acs_t::Marshal(LArchive &a)
{
  int temp;
  if (a.IsStoring())
    {
      a << def->number;
      a << (temp = (reinterpret_cast<byte*>(ip) - mp->ACS_base)); // byte offset
      a.Write((byte *)stack, sizeof(stack));
      a.Write((byte *)vars, sizeof(vars));
      Thinker::Serialize(triggerer, a);
      ST_PTR(line, mp->lines);
    }
  else
    {
      a << temp; def = mp->ACS_FindScript(temp);
      a << temp; ip = reinterpret_cast<Sint32*>(&mp->ACS_base[temp]);
      a.Read((byte *)stack, sizeof(stack));
      a.Read((byte *)vars, sizeof(vars));
      triggerer = reinterpret_cast<Actor *>(Thinker::Unserialize(a));
      RE_PTR(line, mp->lines);
    }

  a << sp << side << delay;
  return 0;
}

int sectoreffect_t::Marshal(LArchive &a)
{
  int temp;
  if (a.IsStoring())
    {
      if (!sector)
	I_Error("Sectoreffect with no sector!\n");
      a << (temp = sector - mp->sectors);
    }
  else
    {
      a << temp;
      if (temp >= mp->numsectors)
	I_Error("Invalid sector for a Sectoreffect!\n");
      sector = &mp->sectors[temp];
    }

  return 0;
}

int lightfx_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    sector->lightingdata = this;

  a << type << count << maxlight << minlight << maxtime << mintime;
  return 0;
}

int phasedlight_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    sector->lightingdata = this;

  a << base << index;
  return 0;
}

int floor_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    sector->floordata = this;

  a << type << crush << speed << modelsec << texture << destheight;
  return 0;
}

int stair_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    sector->floordata = this;

  a << state << resetcount << wait << stepdelay << speed << destheight << originalheight << delayheight << stepdelta;
  return 0;
}

int elevator_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    sector->floordata = sector->ceilingdata = this;

  a << type << crush << floordest << ceilingdest << floorspeed << ceilingspeed;
  return 0;
}

int floorwaggle_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    sector->floordata = this;

  a << state << phase << freq << baseheight << amp << maxamp << ampdelta << wait;
  return 0;
}

int plat_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    {
      sector->floordata = this;
      mp->AddActivePlat(this);
    }

  a << type << status;
  a << speed << low << high << wait << count;
  
  return 0;
}

int ceiling_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    {
      sector->ceilingdata = this;
      mp->AddActiveCeiling(this);
    }

  a << type << crush << speed << destheight << modelsec << texture;

  return 0;
}

int crusher_t::Marshal(LArchive &a)
{
  ceiling_t::Marshal(a);
  a << downspeed << upspeed << bottomheight << topheight;
  return 0;
}

int vdoor_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    sector->ceilingdata = this;

  a << type << direction << topheight << speed << topwait << topcount << boomlighttag;
  return 0;
}

int button_t::Marshal(LArchive &a)
{
  if (a.IsStoring())
    {
      ST_PTR(line, mp->lines);
    }
  else
    {
      RE_PTR(line, mp->lines);
      soundorg = &line->frontsector->soundorg;
    }

  a << texture << timer << where;
  return 0;
}

int scroll_t::Marshal(LArchive &a)
{
  a << type << affectee << vx << vy;
  a << last_height << accel << vdx << vdy;

  if (a.IsStoring())
    {
      ST_PTR(control, mp->sectors);
    }
  else
    {
      RE_PTR(control, mp->sectors);
    }
  return 0;
}

int pusher_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);

  a << type << x_mag << y_mag << magnitude << radius;
  a << x << y;
  if (a.IsStoring())
    Thinker::Serialize(source, a);
  else
    source = reinterpret_cast<DActor*>(Thinker::Unserialize(a));
  return 0;
}


int polyobject_t::Marshal(LArchive &a)
{
  a << poly->id;
  return 0;
}

int polyrotator_t::Marshal(LArchive &a)
{
  polyobject_t::Marshal(a);
  a << ang_vel << dist;
  return 0;
}

int polymover_t::Marshal(LArchive &a)
{
  polyobject_t::Marshal(a);
  a << speed << dist << angle << xs << ys;
  return 0;
}

int polydoor_swing_t::Marshal(LArchive &a)
{
  polyrotator_t::Marshal(a);
  a << closing << open_delay << delay << initial_dist;
  return 0;
}

int polydoor_slide_t::Marshal(LArchive &a)
{
  polymover_t::Marshal(a);
  a << closing << open_delay << delay << initial_dist;
  return 0;
}

enum presentation_id_t
{
  PID_NONE = 0,
  PID_SPRITE,
  PID_MODEL
};

int presentation_t::Serialize(presentation_t *p, LArchive &a)
{
  // much simpler than the Thinker serialization.
  int temp;

  if (p)
    p->Marshal(a); // handles the type id as well.
  else
    a << (temp = PID_NONE);
  return 0;
}

presentation_t *presentation_t::Unserialize(LArchive &a)
{
  presentation_t *p;
  int temp;
  a << temp; // read the type id

  switch (temp)
    {
    case PID_NONE:
      return NULL;

    case PID_SPRITE:
      p = new spritepres_t();
      break;

    case PID_MODEL:
      p = new modelpres_t();
      break;

    default:
      I_Error("Unknown presentation id %d!\n", temp);
      break;
    }

  p->Marshal(a);
  return p;
}

int spritepres_t::Marshal(LArchive &a)
{
  int temp;
  if (a.IsStoring())
    {
      a << (temp = PID_SPRITE); // type id
      a << (temp = info->GetMobjType());
      ST_PTR(state, states);
      a << (temp = animseq);
    }
  else
    {
      a << temp; info = aid[static_cast<mobjtype_t>(temp)];
      state_t *st;
      RE_PTR(st, states);
      spr = NULL;
      SetFrame(st);
      a << temp; animseq = static_cast<animseq_e>(temp);
    }

  a << color;
  return 0;
}


int modelpres_t::Marshal(LArchive &a)
{
  int temp;
  if (a.IsStoring())
    {
      a << (temp = PID_MODEL); // type id
      a.Write(reinterpret_cast<const byte*>(mdl->GetName()), 64);
      // TODO animseq. necessary?
    }
  else
    {
      char str[64];
      a.Read(reinterpret_cast<byte*>(str), 64);
      mdl = models.Get(str);
      lastupdate = 0;
      animseq = animseq_e(0);
    }

  for (int k=0; k<3; k++)
    a << st[k].seq << st[k].frame << st[k].nextframe << st[k].interp;

  a << color << flags;
  return 0;
}

int Actor::Marshal(LArchive &a)
{
  a << pos << vel;
  a << yaw << pitch;
  a << mass << radius << height;
  a << health;

  a << flags << flags2 << eflags;

  a << tid << special;
  for (int i=0; i<5; i++)
    a << args[i];

  a << reactiontime << floorclip;
  a << team;

  if (a.IsStoring())
    {
      presentation_t::Serialize(pres, a);
      ST_PTR(spawnpoint, mp->mapthings);
      Thinker::Serialize(owner, a);
      Thinker::Serialize(target, a);
    }
  else
    {
      pres = presentation_t::Unserialize(a);
      RE_PTR(spawnpoint, mp->mapthings);
      owner  = reinterpret_cast<Actor*>(Thinker::Unserialize(a));
      target = reinterpret_cast<Actor*>(Thinker::Unserialize(a));

      if (mp)
	{
	  if (spawnpoint)
	    spawnpoint->mobj = this;
	  // set links to map geometry
	  floorz = ceilingz = 0; // TEST
	  SetPosition();
	}
    }

  return 0;
}


int DActor::Marshal(LArchive &a)
{ 
  // NOTE is it really worth the effort to do this delta-coding?

  enum dactor_diff_e
  {
    MD_SPAWNPOINT = 0x000001,
    MD_TYPE       = 0x000002,
    MD_XY         = 0x000004,
    MD_Z          = 0x000008,
    MD_MOM        = 0x000010,
    MD_MASS       = 0x000020,
    MD_RADIUS     = 0x000040,
    MD_HEIGHT     = 0x000080,
    MD_HEALTH     = 0x000100,

    MD_FLAGS      = 0x000200,
    MD_FLAGS2     = 0x000400,
    MD_EFLAGS     = 0x000800,

    MD_TID        = 0x001000,
    MD_SPECIAL    = 0x002000,
    MD_RTIME      = 0x004000,
    MD_FLOORCLIP  = 0x008000,
    MD_TEAM       = 0x010000,

    MD_STATE      = 0x020000,
    MD_TICS       = 0x040000,
    MD_MOVEDIR    = 0x080000,
    MD_MOVECOUNT  = 0x100000,
    MD_THRESHOLD  = 0x200000,
    MD_LASTLOOK   = 0x400000,
    MD_SPECIAL1   = 0x800000,
    MD_SPECIAL2  = 0x1000000,
    MD_SPECIAL3  = 0x2000000,

    MD_TARGET    = 0x4000000,
    MD_OWNER     = 0x8000000,
  };

  int temp;
  short stemp;
  int i;

  unsigned diff;

  if (a.IsStoring())
    {
      // find the differences
      if (spawnpoint)
	{
	  diff = MD_SPAWNPOINT;
    
	  if ((pos.x != spawnpoint->x) || (pos.y != spawnpoint->y) ||
	      (yaw != unsigned(ANG45 * (spawnpoint->angle/45))) || pitch)
	    diff |= MD_XY;

	  if (type != spawnpoint->ai->GetMobjType())
	    diff |= MD_TYPE;
	}
      else
	{
	  // not a map spawned thing so make it from scratch
	  diff = MD_XY | MD_TYPE;
	}

      // not the default but the most probable
      if (pos.z != floorz)             diff |= MD_Z;
      if (vel != vec_t<fixed_t>(0,0,0)) diff |= MD_MOM;

      if (mass   != info->mass)        diff |= MD_MASS;
      if (radius != info->radius)      diff |= MD_RADIUS;
      if (height != info->height)      diff |= MD_HEIGHT;

      if (health != info->spawnhealth) diff |= MD_HEALTH;
      if (flags  != info->flags)       diff |= MD_FLAGS;
      if (flags2 != info->flags2)      diff |= MD_FLAGS2;
      if (eflags)                      diff |= MD_EFLAGS;

      if (tid)       diff |= MD_TID;
      if (special)   diff |= MD_SPECIAL;
      if (reactiontime != info->reactiontime) diff |= MD_RTIME;
      if (floorclip != 0) diff |= MD_FLOORCLIP;
      if (team) diff |= MD_TEAM;

      if (state != info->spawnstate)   diff |= MD_STATE;
      if (tics  != state->tics)        diff |= MD_TICS;

      if (movedir)        diff |= MD_MOVEDIR;
      if (movecount)      diff |= MD_MOVECOUNT;
      if (threshold)      diff |= MD_THRESHOLD;
      if (lastlook != -1) diff |= MD_LASTLOOK;
      if (special1)  diff |= MD_SPECIAL1;
      if (special2)  diff |= MD_SPECIAL2;
      if (special3)  diff |= MD_SPECIAL3;

      if (owner)   diff |= MD_OWNER;
      if (target)  diff |= MD_TARGET;

      a << diff; // store the bit field

      if (diff & MD_SPAWNPOINT)
	{
	  stemp = spawnpoint - mp->mapthings;
	  a << stemp;
	}

      if (diff & MD_TYPE) a << (stemp = type);

      if (diff & MD_XY)   a << pos.x << pos.y << yaw << pitch;
      if (diff & MD_Z)    a << pos.z;
      if (diff & MD_MOM)  a << vel;

      if (diff & MD_MASS)   a << mass;
      if (diff & MD_RADIUS) a << radius;
      if (diff & MD_HEIGHT) a << height;
      if (diff & MD_HEALTH) a << health;

      if (diff & MD_FLAGS)  a << flags;
      if (diff & MD_FLAGS2) a << flags2;
      if (diff & MD_EFLAGS) a << eflags;

      if (diff & MD_TID)    a << tid;
      if (diff & MD_SPECIAL)
	{
	  a << special;
	  for (i=0; i<5; i++)
	    a << args[i];
	}
      if (diff & MD_RTIME)     a << reactiontime;
      if (diff & MD_FLOORCLIP) a << floorclip;
      if (diff & MD_TEAM)      a << team;

      if (diff & MD_STATE)
	{
	  temp = state - states;
	  a << temp;
	}
      if (diff & MD_TICS)      a << tics;

      if (diff & MD_MOVEDIR)   a << movedir;
      if (diff & MD_MOVECOUNT) a << movecount;
      if (diff & MD_THRESHOLD) a << threshold;
      if (diff & MD_LASTLOOK)  a << lastlook;

      if (diff & MD_SPECIAL1)  a << special1;
      if (diff & MD_SPECIAL2)  a << special2;
      if (diff & MD_SPECIAL3)  a << special3;

      if (diff & MD_OWNER)  Thinker::Serialize(owner, a);
      if (diff & MD_TARGET) Thinker::Serialize(target, a);
    }
  else
    {
      // retrieving
      a << diff;

      if (diff & MD_SPAWNPOINT)
	{
	  a << stemp;
	  spawnpoint = mp->mapthings + stemp;
	  spawnpoint->mobj = this;
	}

      if (diff & MD_TYPE)
	{
	  a << stemp;
	  type = mobjtype_t(stemp);
	}
      else
	type = spawnpoint->ai->GetMobjType();

      info = aid[type];
      {
	// use info for init, correct later
	mass         = info->mass;
	radius       = info->radius;
	height       = info->height;
	health       = info->spawnhealth;
	flags        = info->flags;
	flags2       = info->flags2;
	reactiontime = info->reactiontime;
	state        = info->spawnstate;
      }

      if (diff & MD_XY)
	a << pos.x << pos.y << yaw << pitch;
      else
	{
	  pos.x = spawnpoint->x;
	  pos.y = spawnpoint->y;
	  yaw = ANG45 * (spawnpoint->angle/45);
	  pitch = 0;
	}

      if (diff & MD_Z)      a << pos.z;
      if (diff & MD_MOM)    a << vel; // else zero (by constructor)

      if (diff & MD_MASS)   a << mass;
      if (diff & MD_RADIUS) a << radius;
      if (diff & MD_HEIGHT) a << height;
      if (diff & MD_HEALTH) a << health;

      if (diff & MD_FLAGS)  a << flags;
      if (diff & MD_FLAGS2) a << flags2;
      if (diff & MD_EFLAGS) a << eflags;

      if (diff & MD_TID)      a << tid;
      if (diff & MD_SPECIAL)
	{
	  a << special;
	  for (i=0; i<5; i++)
	    a << args[i];
	}
      if (diff & MD_RTIME)     a << reactiontime;
      if (diff & MD_FLOORCLIP) a << floorclip;
      if (diff & MD_TEAM)      a << team;

      if (diff & MD_STATE)
	{
	  a << temp;
	  state = &states[temp];
	}
      if (diff & MD_TICS)
	a << tics;
      else
	tics = state->tics;

      if (diff & MD_MOVEDIR)   a << movedir;
      if (diff & MD_MOVECOUNT) a << movecount;
      if (diff & MD_THRESHOLD) a << threshold;
      if (diff & MD_LASTLOOK)  a << lastlook;

      if (diff & MD_SPECIAL1)  a << special1;
      if (diff & MD_SPECIAL2)  a << special2;
      if (diff & MD_SPECIAL3)  a << special3;

      if (diff & MD_OWNER)  owner  = reinterpret_cast<Actor*>(Thinker::Unserialize(a));
      if (diff & MD_TARGET) target = reinterpret_cast<Actor*>(Thinker::Unserialize(a));

      // set sprev, snext, bprev, bnext, subsector
      if (mp)
	{
	  floorz = ceilingz = 0; // TEST
	  SetPosition();
	}

      if (!(diff & MD_Z))
	pos.z = floorz;

      if (!info->modelname.empty())
	pres = new modelpres_t(info->modelname.c_str());
      else
	pres = new spritepres_t(info, 0);
  
      pres->SetFrame(state);
    }

  return 0;
}


int Pawn::Marshal(LArchive & a)
{
  Actor::Marshal(a);
  a << pclass << maxhealth << speed << toughness << attackphase;
  // NOTE info is handled at PlayerInfo::Unserialize()

  if (a.IsStoring())
    {
      Thinker::Serialize(attacker, a);
    }
  else
    {
      attacker = (Actor *)Thinker::Unserialize(a);
    }

  return 0;
}


int PlayerPawn::Marshal(LArchive &a)
{
  Pawn::Marshal(a);

  int i, temp;

  // player pointer doesn't need to be stored
  a << attackdown << usedown << jumpdown;
  a << refire << morphTics << fly_zspeed;

  a << keycards;

  for (i=0; i<NUMAMMO; i++)
    a << ammo[i] << maxammo[i];

  for (i=0; i<NUMARMOR; i++)
    a << armorfactor[i] << armorpoints[i];

  for (i=0; i<NUMPOWERS; i++)
    a << powers[i];

  a << poisoncount << cheats;
  a << specialsector << extralight << fixedcolormap;

  if (a.IsStoring())
    {
      for (i=0; i<NUMPSPRITES; i++)
	{
	  ST_PTR(psprites[i].state, weaponstates);
	  a << psprites[i].tics << psprites[i].sx << psprites[i].sy;
	}

      // inventory is closed.
      a << (temp = inventory.size());
      for (i=0; i<temp; i++)
	a << inventory[i].type << inventory[i].count;

      a << (temp = pendingweapon);
      a << (temp = readyweapon);

      // NOTE: NUMWEAPONS better be <= 32
      temp = 0;
      for (i=0; i<NUMWEAPONS; i++)
	if (weaponowned[i])
	  temp |= (1 << i);
      a << temp; // we have already 32 weapon types. whew!
    }
  else
    {
      // loading
      for (i=0; i<NUMPSPRITES; i++)
	{
	  RE_PTR(psprites[i].state, weaponstates);
	  a << psprites[i].tics << psprites[i].sx << psprites[i].sy;
	}

      a << temp;
      inventory.resize(temp);
      for (i=0; i<temp; i++)
	a << inventory[i].type << inventory[i].count;

      a << temp; pendingweapon = weapontype_t(temp);
      a << temp; readyweapon = weapontype_t(temp);

      a << temp;
      for (i=0; i<NUMWEAPONS; i++)
	weaponowned[i] = (temp & (1 << i));

      weaponinfo = powers[pw_weaponlevel2] ? wpnlev2info : wpnlev1info;
    }

  return 0;
}




//==============================================
//   Script serialization stuff
//==============================================

int svariable_t::Serialize(LArchive &a)
{
  a << name;
  a << type;
  switch (type)
    {
    case svt_string:
      a << value.s;
      break;
    case svt_int:
    case svt_fixed:
      a << value.i;
      break;
    case svt_actor:
      Thinker::Serialize(value.mobj, a);
      break;
    }

  return 0;
}


int svariable_t::Unserialize(LArchive &a)
{
  a << name;
  a << type;
  switch (type)
    {
    case svt_string:
      a << value.s;
      break;
    case svt_int:
    case svt_fixed:
      a << value.i;
      break;
    case svt_actor:
      value.mobj = (Actor *)Thinker::Unserialize(a);
      break;
    }

  return 0;
}      


int script_t::Serialize(LArchive &a)
{
  // count number of variables
  int i, n = 0;
  for (i=0; i<VARIABLESLOTS; i++)
    {
      svariable_t *sv = variables[i];
      while (sv && sv->type != svt_label)
        {
          n++;
          sv = sv->next;
        }
    }
  a << n;

  // go thru hash chains, store each variable
  for (i=0; i<VARIABLESLOTS; i++)
    {
      // go thru this hashchain
      svariable_t *sv = variables[i];
      
      // once we get to a label there can be no more actual
      // variables in the list to store
      while (sv && sv->type != svt_label)
        {
	  sv->Serialize(a);
          sv = sv->next;
        }
    }

  return 0;
}


int script_t::Unserialize(LArchive &a)
{
  int i, n;

  // free all the variables in the script first
  for (i=0; i<VARIABLESLOTS; i++)
    {
      svariable_t *sv = variables[i];
      
      while (sv && sv->type != svt_label)
        {
          svariable_t *next = sv->next;
          Z_Free(sv);
          sv = next;
        }
      variables[i] = sv;       // null or label
    }

  // now read the number of variables from the savegame file
  a << n;
  for (i=0; i<n; i++)
    {
      svariable_t *sv = (svariable_t*)Z_Malloc(sizeof(svariable_t), PU_LEVEL, 0);
      sv->Unserialize(a);
      
      // link in the new variable
      int hashkey = script_t::variable_hash(sv->name);
      sv->next = variables[hashkey];
      variables[hashkey] = sv;
    }

  return 0;
}

//==============================================
//  Map serialization
//==============================================

enum mapdiff_e
{
    // sectors
    // diff
    SD_FLOORHT  = 0x01,
    SD_CEILHT   = 0x02,
    SD_FLOORPIC = 0x04,
    SD_CEILPIC  = 0x08,
    SD_LIGHT    = 0x10,
    SD_SPECIAL  = 0x20,
    SD_TAG      = 0x40,
    SD_DIFF2    = 0x80,

    // diff2
    SD_FXOFFS    = 0x01,
    SD_FYOFFS    = 0x02,
    SD_CXOFFS    = 0x04,
    SD_CYOFFS    = 0x08,
    SD_STAIRLOCK = 0x10,
    SD_PREVSEC   = 0x20,
    SD_NEXTSEC   = 0x40,
    SD_SEQTYPE   = 0x80,

    // line- and sidedefs
    // diff
    LD_FLAG     = 0x01,
    LD_SPECIAL  = 0x02,
    LD_ARGS     = 0x04,
    LD_S1TEXOFF = 0x08,
    LD_S1TOPTEX = 0x10,
    LD_S1BOTTEX = 0x20,
    LD_S1MIDTEX = 0x40,
    LD_DIFF2    = 0x80,

    // diff2
    LD_S2TEXOFF = 0x01,
    LD_S2TOPTEX = 0x02,
    LD_S2BOTTEX = 0x04,
    LD_S2MIDTEX = 0x08,
};




int Map::Serialize(LArchive &a)
{
  unsigned temp;
  short stemp;
  int i, j, n;

  a.Marker(MARK_MAP);

  a << lumpname;
  // TODO save map md5 checksum, to make sure the correct map is loaded
  a << starttic << maptic;
  a << kills << items << secrets; // conceivably scripts could change these

  //----------------------------------------------
  {
  // record the changes in static geometry (as compared to wad)
  // "reload" the map to check for differences
  // TODO helluvalot of unneeded stuff is saved because of linedef/sector special mappings...
  // Perhaps we should really fake-setup the comparison map... or perhaps we'll just live with it.

  int statsec = 0, statline = 0;
  byte diff, diff2;

  struct
  {
    mapsector_t  *sectors;
    mapsidedef_t *sides;
    union
    {
      doom_maplinedef_t *dlines;
      hex_maplinedef_t  *hlines;
    };
  } comp;

  mapsector_t *ms = comp.sectors = static_cast<mapsector_t*>(fc.CacheLumpNum(lumpnum + LUMP_SECTORS, PU_DAVE));
  sector_t    *ss = sectors;
  for (i = 0; i<numsectors ; i++, ss++, ms++)
    {
      diff = diff2 = 0;
      if (ss->floorheight != SHORT(ms->floorheight))
	diff |= SD_FLOORHT;
      if (ss->ceilingheight != SHORT(ms->ceilingheight))
	diff |= SD_CEILHT;

      if (ss->floorpic   && strncmp(ss->floorpic->GetName(), ms->floorpic, 8))
	diff |= SD_FLOORPIC;
      if (ss->ceilingpic && strncmp(ss->ceilingpic->GetName(), ms->ceilingpic, 8))
	diff |= SD_CEILPIC;

      if (ss->lightlevel != SHORT(ms->lightlevel)) diff |= SD_LIGHT;
      if (ss->special != SHORT(ms->special))       diff |= SD_SPECIAL;
      if (ss->tag != SHORT(ms->tag))               diff |= SD_TAG;
      // floortype?
      if (ss->floor_xoffs != 0)   diff2 |= SD_FXOFFS;
      if (ss->floor_yoffs != 0)   diff2 |= SD_FYOFFS;
      if (ss->ceiling_xoffs != 0) diff2 |= SD_CXOFFS;
      if (ss->ceiling_yoffs != 0) diff2 |= SD_CYOFFS;
      if (ss->stairlock < 0)      diff2 |= SD_STAIRLOCK;
      if (ss->nextsec != -1)      diff2 |= SD_NEXTSEC;
      if (ss->prevsec != -1)      diff2 |= SD_PREVSEC;
      if (ss->seqType)          diff2 |= SD_SEQTYPE;

      if (diff2)
	diff |= SD_DIFF2;

      if (diff)
        {
	  statsec++;

	  a << i;
	  a << diff;

	  if (diff & SD_DIFF2)
	    a << diff2;
	  if (diff & SD_FLOORHT) a << ss->floorheight;
	  if (diff & SD_CEILHT)  a << ss->ceilingheight;

	  if (diff & SD_FLOORPIC)
	    a.Write((byte*)ss->floorpic->GetName(), 8);
	  if (diff & SD_CEILPIC)
	    a.Write((byte*)ss->ceilingpic->GetName(), 8);

	  if (diff & SD_LIGHT)    a << ss->lightlevel;
	  if (diff & SD_SPECIAL)  a << ss->special;
	  if (diff & SD_TAG)      a << ss->tag;

	  if (diff2 & SD_FXOFFS)  a << ss->floor_xoffs;
	  if (diff2 & SD_FYOFFS)  a << ss->floor_yoffs;
	  if (diff2 & SD_CXOFFS)  a << ss->ceiling_xoffs;
	  if (diff2 & SD_CYOFFS)  a << ss->ceiling_yoffs;
	  if (diff2 & SD_STAIRLOCK) a << ss->stairlock;
	  if (diff2 & SD_NEXTSEC)   a << ss->nextsec;
	  if (diff2 & SD_PREVSEC)   a << ss->prevsec;
	  if (diff2 & SD_SEQTYPE)   a << ss->seqType;
        }
    }
  a << (temp = MARK_END);
  Z_Free(comp.sectors);

  doom_maplinedef_t *mld = comp.dlines = static_cast<doom_maplinedef_t*>(fc.CacheLumpNum(lumpnum + LUMP_LINEDEFS, PU_DAVE));
  hex_maplinedef_t  *hld = comp.hlines;
  line_t *li = lines;

  mapsidedef_t *msd = comp.sides = static_cast<mapsidedef_t*>(fc.CacheLumpNum(lumpnum + LUMP_SIDEDEFS, PU_DAVE));
  side_t *si;

  // do lines
  for (i=0 ; i<numlines ; i++, li++)
    {
      diff = diff2 = 0;

      if (hexen_format)
	{
	  if (li->flags != SHORT(hld->flags))
	    diff |= LD_FLAG;
	  if (li->special != SHORT(hld->special))
	    diff |= LD_SPECIAL;
	  for (j=0; j<5; j++)
	    if (li->args[j] != hld->args[j])
	      diff |= LD_ARGS;
	  hld++;
	}
      else
	{
	  if (li->flags != SHORT(mld->flags))
	    diff |= LD_FLAG;
	  if (li->special != SHORT(mld->special))
	    diff |= LD_SPECIAL;
	  // TODO save tag?
	  mld++;
	}

      if (li->sideptr[0])
        {
	  si = li->sideptr[0];
	  temp = si - sides; // sidedef number

	  if (si->textureoffset != SHORT(msd[temp].textureoffset))
	    diff |= LD_S1TEXOFF;

	  // do texture diffing by comparing names to be sure
	  if (si->toptexture && strncmp(si->toptexture->GetName(), msd[temp].toptexture, 8))
	    diff |= LD_S1TOPTEX;
	  if (si->bottomtexture && strncmp(si->bottomtexture->GetName(), msd[temp].bottomtexture, 8))
	    diff |= LD_S1BOTTEX;
	  if (si->midtexture && strncmp(si->midtexture->GetName(), msd[temp].midtexture, 8))
	    diff |= LD_S1MIDTEX;
        }
      if (li->sideptr[1])
        {
	  si = li->sideptr[1];
	  temp = si - sides; // sidedef number

	  if (si->textureoffset != SHORT(msd[temp].textureoffset))
	    diff2 |= LD_S2TEXOFF;

	  if (si->toptexture && strncmp(si->toptexture->GetName(), msd[temp].toptexture, 8))
	    diff2 |= LD_S2TOPTEX;
	  if (si->bottomtexture && strncmp(si->bottomtexture->GetName(), msd[temp].bottomtexture, 8))
	    diff2 |= LD_S2BOTTEX;
	  if (si->midtexture && strncmp(si->midtexture->GetName(), msd[temp].midtexture, 8))
	    diff2 |= LD_S2MIDTEX;

	  if (diff2)
	    diff |= LD_DIFF2;
        }

      if (diff)
        {
	  statline++;

	  a << i;
	  a << diff;
	  if (diff & LD_DIFF2  )  a << diff2;
	  if (diff & LD_FLAG   )  a << li->flags;
	  if (diff & LD_SPECIAL)  a << li->special;
	  if (diff & LD_ARGS)
	    for (j=0; j<5; j++)
	      a << li->args[j];

	  si = li->sideptr[0];
	  if (diff & LD_S1TEXOFF) a << si->textureoffset;

	  if (diff & LD_S1TOPTEX) a << si->toptexture;
	  if (diff & LD_S1BOTTEX) a << si->bottomtexture;
	  if (diff & LD_S1MIDTEX) a << si->midtexture;

	  si = li->sideptr[1];
	  if (diff2 & LD_S2TEXOFF) a << si->textureoffset;

	  if (diff2 & LD_S2TOPTEX) a << si->toptexture;
	  if (diff2 & LD_S2BOTTEX) a << si->bottomtexture;
	  if (diff2 & LD_S2MIDTEX) a << si->midtexture;
        }
    }
  a << (temp = MARK_END);
  Z_Free(comp.dlines);
  Z_Free(comp.sides);
  CONS_Printf("%d/%d sectors, %d/%d lines saved\n", statsec, numsectors, statline, numlines);
  }
  //----------------------------------------------
  // polyobjs
  a.Marker(MARK_POLYOBJ);

  for (i = 0; i < NumPolyobjs; i++)
    a << polyobjs[i].id << polyobjs[i].angle
      << polyobjs[i].origin.x << polyobjs[i].origin.y;

  //----------------------------------------------
  // scripts
  a.Marker(MARK_SCRIPT);

  for (acs_script_iter_t i = ACS_scripts.begin(); i != ACS_scripts.end(); i++)
    {
      acs_script_t &s = i->second;
      a << (int &)s.state;
      a << s.wait_data;
    }

  a.Write(reinterpret_cast<byte*>(ACS_map_vars), sizeof(ACS_map_vars));

  // FS: levelscript contains the map global variables (everything else can be loaded from the WAD)
  FS_levelscript->Serialize(a);

  //runningscripts (scripts currently suspended)
  runningscript_t *rs;
  
  // count runningscripts
  n = 0;
  for (rs = FS_runningscripts; rs; rs = rs->next)
    n++;
  a << n;
  
  // now archive them
  for (rs = FS_runningscripts; rs; rs = rs->next)
    {
      a << rs->script->scriptnum;
      a << (n = (rs->savepoint - rs->script->data)); // offset
      a << (int &)rs->wait_type;
      a << rs->wait_data;

      Thinker::Serialize(rs->trigger, a);
  
      // count number of variables
      n = 0;
      for (i=0; i<VARIABLESLOTS; i++)
	{
	  svariable_t *sv = rs->variables[i];
	  while (sv && sv->type != svt_label)
	    {
	      n++;
	      sv = sv->next;
	    }
	}
      a << n;

      // go thru hash chains, store each variable
      for (i=0; i<VARIABLESLOTS; i++)
	{
	  svariable_t *sv = rs->variables[i];
	  while (sv && sv->type != svt_label)
	    {
	      sv->Serialize(a);
	      sv = sv->next;
	    }
	}
    }

  // TODO Archive the script camera.

  //----------------------------------------------
  // Thinkers (including map objects aka Actors)
  a.Marker(MARK_THINK);

  Thinker *th;
  // Now we use a one-pass recursive iteration of thinkers.
  //   Another possible way to do this:
  //   First do a recursive iteration of the thinkers, ONLY to create the pointer->id map.
  //   Then iterate the map serializing each element in turn (first writing the ID, of course).

  i = 0;
  // only the stuff in the thinker ring is checked
  for (th = thinkercap.next; th != &thinkercap; th = th->next)
    {
      th->CheckPointers(); // clear pointers to deleted items
      i++;
    }

  a << i; // store the number of thinkers in ring

  for (th = thinkercap.next; th != &thinkercap; th = th->next)
    {
      Thinker::Serialize(th, a);
    }

  a.Marker(MARK_MISC);
  //----------------------------------------------
  // respawnqueue
  n = itemrespawnqueue.size();
  a << n;
  for (i=0; i<n; i++)
    {
      temp = itemrespawnqueue[i] - mapthings;
      a << temp;
      a << itemrespawntime[i];
    }

  //----------------------------------------------
  // the rest

  n = TID_map.size();
  a << n;
  for (TID_map_iter_t t = TID_map.begin(); t != TID_map.end(); t++)
    {
      stemp = t->first;
      a << stemp;
      if (a.HasStored(t->second, temp))
	a << temp;
      else
	I_Error("Crap in TIDmap!\n");
    }

  // TODO Sound sequences

  return 0;
}


int Map::Unserialize(LArchive &a)
{
  unsigned temp;
  short stemp;
  int i, n;

  if (!a.Marker(MARK_MAP))
    return -1;

  a.active_map = this; // so the Thinkers can be extracted OK

  // first we load and setup the map, but without spawning any Thinkers if possible
  a << lumpname;
  // TODO load map md5 checksum, make sure the correct map is loaded
  Setup(0, false);
  // remove all the current thinkers (could be done more elegantly by not spawning them
  // in the first place, but... well, at least the Actors are not spawned.)
  Thinker *th, *next;
  for (th = thinkercap.next; th != &thinkercap; th = next)
    {
      next = th->next;
      delete th;
    }
  InitThinkers();

  a << starttic << maptic;
  a << kills << items << secrets;

  line_t *li;
  side_t *si;
  byte    diff, diff2;

  // load changes in geometry
  while (1)
    {
      a << temp;
      if (temp == MARK_END)
	break;
      i = temp; // sector number

      a << diff;
      if (diff & SD_DIFF2)
	a << diff2;
      else
	diff2 = 0;

      char picname[8];

      if (diff & SD_FLOORHT ) a << sectors[i].floorheight;
      if (diff & SD_CEILHT  ) a << sectors[i].ceilingheight;
      if (diff & SD_FLOORPIC)
        {
	  a.Read((byte *)picname, 8);
	  sectors[i].floorpic = materials.Get8char(picname, TEX_floor);
        }
      if (diff & SD_CEILPIC)
        {
	  a.Read((byte *)picname, 8);
	  sectors[i].ceilingpic = materials.Get8char(picname, TEX_floor);
        }
      if (diff & SD_LIGHT)    a << sectors[i].lightlevel;
      if (diff & SD_SPECIAL)  a << sectors[i].special;
      if (diff & SD_TAG)      a << sectors[i].tag;

      if (diff2 & SD_FXOFFS)  a << sectors[i].floor_xoffs;
      if (diff2 & SD_FYOFFS)  a << sectors[i].floor_yoffs;
      if (diff2 & SD_CXOFFS)  a << sectors[i].ceiling_xoffs;
      if (diff2 & SD_CYOFFS)  a << sectors[i].ceiling_yoffs;

      if (diff2 & SD_STAIRLOCK) a << sectors[i].stairlock;
      else sectors[i].stairlock = 0;
      if (diff2 & SD_NEXTSEC)   a << sectors[i].nextsec;
      else sectors[i].nextsec = -1;
      if (diff2 & SD_PREVSEC)   a << sectors[i].prevsec;
      else sectors[i].prevsec = -1;

      if (diff2 & SD_SEQTYPE)   a << sectors[i].seqType;
    }

  while (1)
    {
      a << temp; // line number
      if (temp == MARK_END)
	break;
      li = &lines[temp];

      a << diff;

      if (diff & LD_DIFF2) a << diff2;
      else diff2 = 0;
      if (diff & LD_FLAG)    a << li->flags;
      if (diff & LD_SPECIAL) a << li->special;
      if (diff & LD_ARGS)
	for (i=0; i<5; i++)
	  a << li->args[i];

      si = li->sideptr[0];
      if (diff & LD_S1TEXOFF) a << si->textureoffset;

      if (diff & LD_S1TOPTEX) a << si->toptexture;
      if (diff & LD_S1BOTTEX) a << si->bottomtexture;
      if (diff & LD_S1MIDTEX) a << si->midtexture;

      si = li->sideptr[1];
      if (diff2 & LD_S2TEXOFF) a << si->textureoffset;

      if (diff2 & LD_S2TOPTEX) a << si->toptexture;
      if (diff2 & LD_S2BOTTEX) a << si->bottomtexture;
      if (diff2 & LD_S2MIDTEX) a << si->midtexture;
    }

  //----------------------------------------------
  if (!a.Marker(MARK_POLYOBJ))
    return -2;

  for (i = 0; i < NumPolyobjs; i++)
    {
      a << n;
      if (n != polyobjs[i].id)
	I_Error("Invalid polyobj tag!\n");

      angle_t ang;
      a << ang;
      polyobjs[i].Rotate(ang);
      fixed_t x, y;
      a << x << y;
      polyobjs[i].Move(x - polyobjs[i].origin.x, y - polyobjs[i].origin.y);
    }

  //----------------------------------------------
  // scripts
  if (!a.Marker(MARK_SCRIPT))
    return -3;

  for (acs_script_iter_t i = ACS_scripts.begin(); i != ACS_scripts.end(); i++)
    {
      acs_script_t &s = i->second;
      a << (int &)s.state;
      //a << n; ACSInfo[i].state = acs_state_t(n);
      a << s.wait_data;
    }

  a.Read((byte *)ACS_map_vars, sizeof(ACS_map_vars));

  // FS: restore levelscript
  FS_levelscript->Unserialize(a);

  // restore runningscripts
  // remove all runningscripts first: levelscript may have started them
  FS_ClearRunningScripts(); 

  a << n;
  for (i=0; i<n; i++)
    {
      // create a new runningscript
      runningscript_t *rs = new runningscript_t();
  
      int scriptnum;
      a << scriptnum;
    
      // levelscript?
      if (scriptnum == -1)
	rs->script = FS_levelscript;
      else
	rs->script = FS_levelscript->children[scriptnum];

      a << n; // read out offset from save
      rs->savepoint = rs->script->data + n;
      a << n;
      rs->wait_type = fs_wait_e(n);
      a << rs->wait_data;
      rs->trigger = (Actor *)Thinker::Unserialize(a);
    
      // read out the variables now (fun!)
      // start with basic script slots/labels
  
      for (i=0; i<VARIABLESLOTS; i++)
	rs->variables[i] = rs->script->variables[i];

      // get number of variables
      a << n;
      for (i=0; i<n; i++)
	{
	  svariable_t *sv = (svariable_t*)Z_Malloc(sizeof(svariable_t), PU_LEVEL, 0);
	  sv->Unserialize(a);

	  // link in the new variable
	  int hashkey = script_t::variable_hash(sv->name);
	  sv->next = rs->variables[hashkey];
	  rs->variables[hashkey] = sv;
	}
      
      // hook into chain
      FS_AddRunningScript(rs);
    }

  // TODO Unarchive the script camera

  //----------------------------------------------
  // Thinkers
  if (!a.Marker(MARK_THINK))
    return -4;

  a << n;
  for (i=0; i<n; i++)
    {
      Thinker *th = Thinker::Unserialize(a);
      AddThinker(th);
    }

  if (!a.Marker(MARK_MISC))
    return -5;
  //----------------------------------------------
  // respawnqueue
  a << n;
  for (i=0; i<n; i++)
    {
      a << temp;
      itemrespawnqueue.push_back(&mapthings[temp]);
      a << temp;
      itemrespawntime.push_back(temp);
    }

  //----------------------------------------------
  // the rest
  TID_map.clear();
  a << n;
  for (i=0; i<n; i++)
    {
      Actor *p = NULL;
      a << stemp << temp;
      if (a.GetPtr((int &)temp, (void * &)p))
	TID_map.insert(pair<const short, Actor*>(stemp, p));
      else
	I_Error("Crap in TIDmap!\n");
    }


  return 0;
}




int Episode::Serialize(LArchive &a)
{
  int n;
  a << name << namepic << maplump;
  a << entrypoint << active;
  a << (n = minfo ? minfo->mapnumber : 0);
  return 0;
}


int Episode::Unserialize(LArchive &a)
{
  int n;
  a << name << namepic << maplump;
  a << entrypoint << active;
  a << n;
  minfo = game.FindMapInfo(n);
  return 0;
}


int MapCluster::Serialize(LArchive &a)
{
  a << number;
  a << clustername;
  a << hub << keepstuff;

  a << time << partime;
  a << entertext << exittext << finalepic << finalemusic << episode;

  int n;
  a << (n = maps.size());
  for (int i=0; i<n; i++)
    a << maps[i]->mapnumber;
  return 0;
}


int MapCluster::Unserialize(LArchive &a)
{
  a << number;
  a << clustername;
  a << hub << keepstuff;

  a << time << partime;
  a << entertext << exittext << finalepic << finalemusic << episode;

  int n, temp;
  a << n;
  for (int i=0; i<n; i++)
    {
      a << temp;
      maps.push_back(game.FindMapInfo(temp));
    }
  return 0;
}


int MapInfo::Serialize(LArchive &a)
{
  int temp;
  a << (temp = state);
  a << found;

  a << lumpname << nicename << savename;
  a << cluster << mapnumber << hub;

  a << version << author << description;
  a << namepic;

  // only save those items that cannot be found in the MapInfo separator lump for sure!
  // note that MAPINFO is not read when loading a game!
  a << partime << gravity;
  // TODO thing number mappings?
  a << musiclump;
  // the warp* numbers are not used and hence need not be stored
  a << nextlevel << secretlevel;
  a << doublesky << lightning;

  a << sky1 << sky1sp << sky2 << sky2sp;
  a << cdtrack << fadetablelump;
  a << BossDeathKey;

  a << interpic << intermusic;

  if (state == MAP_SAVED)
    {
      // utilize the existing hubsave file
      a << (temp = 2);
      byte *buffer;
      int length = FIL_ReadFile(savename.c_str(), &buffer);
      if (!length)
	{
	  I_Error("Couldn't read hubsave file %s", savename.c_str());
	  return -1;
	}

      a << length;
      a.Write(buffer, length);
      Z_Free(buffer);
    }
  else if (me)
    {
      a << (temp = 1);
      me->Serialize(a);
    }
  else
    a << (temp = 0);

  // deferred scripts
  a << (temp = ACS_deferred.size());
  for (int i = 0; i < temp; i++)
    a.Write((byte *)&ACS_deferred[i], sizeof(acs_deferred_t));

  return 0;
}


int MapInfo::Unserialize(LArchive &a)
{
  int temp;
  a << temp; state = mapstate_e(temp);
  a << found;

  a << lumpname << nicename << savename;
  a << cluster << mapnumber << hub;

  a << version << author << description;
  a << namepic;

  // only save those items that cannot be found in the MapInfo separator lump for sure!
  // note that MAPINFO is not read when loading a game!
  a << partime << gravity;
  a << musiclump;

  a << nextlevel << secretlevel;
  a << doublesky << lightning;

  a << sky1 << sky1sp << sky2 << sky2sp;
  a << cdtrack << fadetablelump;
  a << BossDeathKey;

  a << interpic << intermusic;

  a << temp;
  if (temp == 2)
    {
      // extract the hubsave file
      int length;
      a << length;
      byte *buffer = (byte *)Z_Malloc(length, PU_STATIC, NULL);
      a.Read(buffer, length);
      FIL_WriteFile(savename.c_str(), buffer, length);
      Z_Free(buffer);
    }
  else if (temp == 1)
    {
      me = new Map(this);
      if (me->Unserialize(a))
	{
	  I_Error("Map archive corrupted!\n");
	  return -1;
	}
      a.active_map = NULL; // this map is done
    }
  else
    me = NULL;

  a << temp;
  for (int i = 0; i < temp; i++)
    {
      acs_deferred_t d;
      a.Read((byte *)&d, sizeof(acs_deferred_t));
      ACS_deferred.push_back(d);
    }

  return 0;
}



//==============================================
//  Player serialization
//==============================================

int PlayerInfo::Serialize(LArchive &a)
{
  int n;
  a << number << team << name;
  a << client_hash;

  a << (n = playerstate);
  a << spectator;

  a << requestmap << entrypoint;

  // cmd can be ignored

  // scoring
  a << (n = Frags.size());
  map<int, int>::iterator t;
  for (t = Frags.begin(); t != Frags.end(); t++)
    {
      int m = t->first;
      int n = t->second;
      a << m << n;
    }
  a << score << kills << items << secrets << time;

  options.Serialize(a);

  // current feedback is lost

  // players are serialized after maps, so pawn may already be stored
  Thinker::Serialize(pawn, a); 
  Thinker::Serialize(pov, a); 
  Thinker::Serialize(hubsavepawn, a); 

  // TEST, save current map number
  a << (n = mp ? mp->info->mapnumber : 0);

  return 0;
}

int PlayerInfo::Unserialize(LArchive &a)
{
  int i, n;
  a << number << team << name;
  a << client_hash; // so we can recognize the clients after loading

  a << n; playerstate = playerstate_t(n);
  a << spectator;

  a << requestmap << entrypoint;

  a << n;
  for (i=0; i<n; i++)
    {
      int t1, t2;
      a << t1 << t2;
      Frags.insert(pair<int, int>(t1, t2));
    }
  a << score << kills << items << secrets << time;

  options.Unserialize(a);

  pawn = static_cast<PlayerPawn*>(Thinker::Unserialize(a));
  pov = static_cast<Actor*>(Thinker::Unserialize(a));
  hubsavepawn = static_cast<PlayerPawn*>(Thinker::Unserialize(a));

  // TEST, get current map and link PI to it
  a << n;
  mp = n ? game.FindMapInfo(n)->me : NULL;
  if (mp)
    {
      mp->players.push_back(this);
    }

  if (pawn)
    {
      if (mp != pawn->mp)
	I_Error("LoadGame: corrupted PlayerInfo\n");

      pawn->player = this;
      pawn->info = pawn_aid[options.ptype];
    }

  return 0;
}


int PlayerOptions::Serialize(LArchive &a)
{
  a << ptype << color << skin;

  a << autoaim << originalweaponswitch;
  for (int i=0; i<NUMWEAPONS; i++)
    a << weaponpref[i];

  a << messagefilter;
  return 0;
}

int PlayerOptions::Unserialize(LArchive &a)
{
  a << ptype << color << skin;

  a << autoaim << originalweaponswitch;
  for (int i=0; i<NUMWEAPONS; i++)
    a << weaponpref[i];

  a << messagefilter;
  return 0;
}


//==============================================
//  Game serialization
//==============================================

int TeamInfo::Serialize(LArchive &a)
{
  a << name << color << score << resources;
  return 0;
}

int TeamInfo::Unserialize(LArchive &a)
{
  a << name << color << score << resources;
  return 0;
}


// takes a snapshot of the entire game state and stores it in the archive
int GameInfo::Serialize(LArchive &a)
{
  int i, n;

  // treat all enums as ints
  a << demoversion;
  a << (n = mode);
  a << (n = state);
  a << (n = skill);

  // flags
  a << netgame << multiplayer << modified << paused << inventory;

  a << maxteams;
  a << maxplayers;

  a.Marker(MARK_GROUP);

  // mapinfo (and maps)
  a << (n = mapinfo.size());
  mapinfo_iter_t k;
  for (k = mapinfo.begin(); k != mapinfo.end(); k++)
    k->second->Serialize(a);

  a.Marker(MARK_GROUP);

  // clustermap
  a << (n = clustermap.size());
  cluster_iter_t l;
  for (l = clustermap.begin(); l != clustermap.end(); l++)
    l->second->Serialize(a);

  a << currentcluster->number;

  a.Marker(MARK_GROUP);

  // episodes
  a << (n = episodes.size());
  for (i = 0; i<n; i++)
    episodes[i]->Serialize(a);

  a.Marker(MARK_GROUP);

  // teams
  a << (n = teams.size());
  for (i = 0; i < n; i++)
    teams[i]->Serialize(a);

  a.Marker(MARK_GROUP);

  // players 
  a << (n = Players.size());
  player_iter_t j;
  for (j = Players.begin(); j != Players.end(); j++)
    j->second->Serialize(a);

  a.Marker(MARK_GROUP);

  // global script data
  a.Write((byte *)ACS_world_vars, sizeof(ACS_world_vars));
  // TODO FS hub_script, global_script...

  a.Marker(MARK_GROUP);
  {
    // client stuff
    int temp = NUM_LOCALPLAYERS;
    a << temp;
    for (i = 0; i < temp; i++)
      a << (n = LocalPlayers[i].info ? LocalPlayers[i].info->number : -1);

    // TODO how to save net info???
  }

  a.Marker(MARK_GROUP);
  {
    a << (n = P_GetRandIndex());

    const char *temp = S.GetMusic();
    string mus;
    if (temp)
      mus = temp;
    else
      mus = "";
    a << mus;
  }

  //CV_SaveNetVars((char**)&save_p);
  //CV_LoadNetVars((char**)&save_p);

  // TODO
  // client info and other net stuff (player authentication?)
  // consvars
  // check if required resource files are to be found / can be downloaded

  return 0;
}


int GameInfo::Unserialize(LArchive &a)
{
  int i, n;
  // treat all enums as ints
  a << demoversion;
  a << n; mode = gamemode_t(n);
  a << n; state = gamestate_t(n);
  a << n; skill = skill_t(n);

  // flags
  a << netgame << multiplayer << modified << paused << inventory;

  a << maxteams;
  a << maxplayers;

  if (!a.Marker(MARK_GROUP))
    return -1;

  // mapinfo (and maps)
  a << n;
  for (i = 0; i < n; i++)
    {
      MapInfo *m = new MapInfo;
      if (m->Unserialize(a))
	return -2;
      mapinfo[m->mapnumber] = m;
    }

  if (!a.Marker(MARK_GROUP))
    return -1;

  // clustermap
  a << n;
  for (i = 0; i < n; i++)
    {
      MapCluster *c = new MapCluster;
      if (c->Unserialize(a))
	return -3;
      clustermap[c->number] = c;
    }

  a << n;
  currentcluster = clustermap[n];

  if (!a.Marker(MARK_GROUP))
    return -1;

  // episodes
  a << n;
  episodes.resize(n);
  for (i = 0; i < n; i++)
    {
      episodes[i] = new Episode;
      if (episodes[i]->Unserialize(a))
	return -666;
    }

  if (!a.Marker(MARK_GROUP))
    return -1;

  // teams
  a << n;
  teams.resize(n);
  for (i = 0; i < n; i++)
    {
      teams[i] = new TeamInfo;
      if (teams[i]->Unserialize(a))
	return -4;
    }

  if (!a.Marker(MARK_GROUP))
    return -1;

  // players 
  a << n;
  for (i = 0; i < n; i++)
    {
      PlayerInfo *p = new PlayerInfo;
      if (p->Unserialize(a))
	return -5;
      Players[p->number] = p;
    }

  if (!a.Marker(MARK_GROUP))
    return -1;

  // global script data
  a.Read((byte *)ACS_world_vars, sizeof(ACS_world_vars));

  if (!a.Marker(MARK_GROUP))
    return -1;

  // client stuff
  a << n;
  for (i = 0; i < n; i++)
    {
      int num;
      a << num;
      if (num == -1)
	LocalPlayers[i].info = NULL;
      else
	LocalPlayers[i].info = FindPlayer(num);
    }

  if (!a.Marker(MARK_GROUP))
    return -1;
  // misc shit

  a << n;
  P_SetRandIndex(n);

  string mus;
  a << mus;
  S.StartMusic(mus.c_str(), true);

  return 0;
}



void GameInfo::LoadGame(int slot)
{
  CONS_Printf("Loading a game...\n");
  char  savename[255];
  byte *savebuffer;

  sprintf(savename, savegamename, slot);

  int length = FIL_ReadFile(savename, &savebuffer);
  if (!length)
    {
      CONS_Printf("Couldn't open save file %s\n", savename);
      return;
    }

  LArchive a;
  if (!a.Open(savebuffer, length))
    return;

  Z_Free(savebuffer); // the compressed buffer is no longer needed

  Downgrade(LEGACY_VERSION); // reset the game version
  SV_Reset();
  ReadResourceLumps();

  // dearchive all the modifications
  if (Unserialize(a))
    {
      CONS_Printf("\aSavegame file corrupted!\n\n");
      SV_Reset();
      return;
    }

  SetState(GS_LEVEL);

  if (netgame)
    net->SV_Open(true); // let the remote players in

  paused = false;

  // view the local human players by default
  for (int i=0; i < NUM_LOCALHUMANS; i++)
    if (LocalPlayers[i].info)
      ViewPlayers.push_back(LocalPlayers[i].info);

  // TODO have other playerinfos waiting for clients to rejoin
  if (ViewPlayers.size())
    hud.ST_Start();
  // done
  /*
  if (setsizeneeded)
    R_ExecuteSetViewSize();

  R_FillBackScreen();  // draw the pattern into the back screen
  */
  con.Toggle(true);
  CONS_Printf("...done.\n");
}


void GameInfo::SaveGame(int savegameslot, const char *description)
{
  CONS_Printf("Saving the game...\n");

  LArchive a;
  a.Create(description); // create a new save archive
  Serialize(a);          // store the game state into it

  byte *buffer;
  unsigned length = a.Compress(&buffer, 1);  // take out the compressed data

  char filename[256];
  sprintf(filename, savegamename, savegameslot);

  CONS_Printf("Savegame: %d bytes\n", length);
  FIL_WriteFile(filename, buffer, length);

  Z_Free(buffer);

  CONS_Printf("...done.\n");
  //R_FillBackScreen();  // draw the pattern into the back screen
}
