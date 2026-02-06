// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_ceilng.cpp 485 2007-08-05 13:52:17Z smite-meister $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2004 by DooM Legacy Team.
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
//
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Ceiling movement (lowering, crushing, raising)

#include "doomdef.h"

#include "g_game.h"
#include "g_map.h"

#include "p_spec.h"
#include "sounds.h"


void P_CopySectorProperties(sector_t *sec, sector_t *model);

//==========================================================================
//                              CEILINGS
//==========================================================================

IMPLEMENT_CLASS(ceiling_t, sectoreffect_t);
ceiling_t::ceiling_t() {}

// constructor
ceiling_t::ceiling_t(Map *m, int ty, sector_t *sec, fixed_t sp, int cru, fixed_t height)
  : sectoreffect_t(m, sec)
{
  type = ty;
  crush = cru;
  speed = sp; // contains movement direction
  sec->ceilingdata = this;

  // target?
  switch (type & TMASK)
    {
    case RelHeight:
      destheight = sec->ceilingheight + height;
      // if (height < 0) speed = -speed;
      break;

    case AbsHeight:
      destheight = height;
      if (sec->ceilingheight > height)
	speed = -speed;
      break;

    case Floor:
      destheight = sec->floorheight + height;
      //speed = -speed;
      break;

    case HnC:
      destheight = sec->FindHighestCeilingSurrounding() + height;
      break;

    case UpNnC:
      destheight = sec->FindNextHighestCeiling(sec->ceilingheight) + height;
      break;

    case DownNnC:
      destheight = sec->FindNextLowestCeiling(sec->ceilingheight) + height;
      //speed = -speed;
      break;

    case LnC:
      destheight = sec->FindLowestCeilingSurrounding() + height;
      //speed = -speed;
      break;

    case HnF:
      destheight = sec->FindHighestFloorSurrounding() + height;
      //speed = -speed;
      break;

    case UpSUT:
    case DownSUT:
      destheight = sector->ceilingheight + height;
      if ((type & TMASK) == UpSUT)
	destheight += sec->FindShortestUpperAround();
      else
	destheight -= sec->FindShortestUpperAround();

      if (destheight < -32000)
	destheight = -32000;
      else if (destheight > 32000)
	destheight = 32000;
      break;

    default:
      I_Error("Unknown ceiling target %d!\n", type);
      break;
    }

  if (!(type & Silent))
    mp->SN_StartSequence(&sec->soundorg, SEQ_PLAT + sec->seqType);
}



void ceiling_t::Think()
{
  if (type & InStasis)
    return; // in stasis

  int res = mp->T_MovePlane(sector, speed, destheight, crush, 1);

  if (res == res_pastdest)
    {
      // movers with texture/special change: change the texture then get removed
      if (type & SetTexture)
	sector->ceilingpic = texture;

      if (type & SetSpecial)
	{
	  if (modelsec < 0)
	    sector->special = 0; // just zero the type
	  else
	    P_CopySectorProperties(sector, &mp->sectors[modelsec]);	 
	}

      if (type & Silent)
	S_StartSound(&sector->soundorg, sfx_ceilstop);
      else
	mp->SN_StopSequence(&sector->soundorg);

      mp->RemoveActiveCeiling(this);
    }
}


//
// Move a ceiling up/down and all around!
//
int Map::EV_DoCeiling(unsigned tag, line_t *line, int type, fixed_t speed, int crush, fixed_t height)
{
  sector_t   *sec;

  int secnum = -1;
  int rtn = 0;

  // manual ceiling?
  if (!tag)
    {
      if (!line || !line->backsector)
	return 0;

      ActivateInStasisCeiling(0); // should do no harm (but affects all manual ceilings...)

      sec = line->backsector;
      if (sec->Active(sector_t::ceiling_special))
	return 0;

      goto manual_ceiling;
    }

  // Reactivate in-stasis ceilings...for certain types.
  // TODO formerly only for crushers, is this wrong?
  rtn += ActivateInStasisCeiling(tag);

  while ((secnum = FindSectorFromTag(tag, secnum)) >= 0)
    {
      sec = &sectors[secnum];

      if (sec->Active(sector_t::ceiling_special))
	continue;

    manual_ceiling:

      // new ceiling thinker
      rtn++;
      ceiling_t *ceiling = new ceiling_t(this, type, sec, speed, crush, height);
      AddActiveCeiling(ceiling);

      if (type & ceiling_t::SetTxTy) // either one
	{
	  if (type & ceiling_t::NumericModel)
	    {
	      // make sure these get initialized even if no model sector can be found
	      ceiling->texture = sec->ceilingpic;
	      ceiling->modelsec = sec - sectors;
	      //jff 5/23/98 use model subroutine to unify fixes and handling
	      // BP: heretic have change something here
	      sec = sec->FindModelCeilingSector(ceiling->destheight);
	      if (sec)
		{
		  ceiling->texture = sec->ceilingpic;
		  ceiling->modelsec = sec - sectors;
		}
	    }
	  else
	    {
	      // "trigger model"
	      ceiling->texture = line->frontsector->ceilingpic;
	      ceiling->modelsec = line->frontsector - sectors;
	    }

	  if (type & ceiling_t::ZeroSpecial)
	    ceiling->modelsec = -1;
	}

      if (!tag)
	return rtn;
    }
  return rtn;
}





//==========================================================================
//                               CRUSHERS
//==========================================================================

IMPLEMENT_CLASS(crusher_t, sectoreffect_t);
crusher_t::crusher_t() {}

// constructor
crusher_t::crusher_t(Map *m, int ty, sector_t *sec, fixed_t upsp, fixed_t downsp, int cru, fixed_t height)
  : ceiling_t(m, ceiling_t::Floor, sec, -downsp, cru, height)   // start by going down
{
  type = ty;
  upspeed = upsp;
  downspeed = downsp;

  topheight = sec->ceilingheight;
  bottomheight = sec->floorheight + height;
}



void crusher_t::Think()
{
  if (type & InStasis)
    return; // in stasis

  int res = mp->T_MovePlane(sector, speed, destheight, crush, 1);

  if (res == res_pastdest)
    {
      if (speed >= 0) // going up
	{
	  if (type & CrushOnce)
	    {
	      if (type & Silent)
		S_StartSound(&sector->soundorg, sfx_ceilstop);
	      else
		mp->SN_StopSequence(&sector->soundorg);

	      mp->RemoveActiveCeiling(this); // cycle done
	    }
	  else
	    {
	      speed = -downspeed;
	      destheight = bottomheight;
	    }
	}
      else // going down
	{
	  speed = upspeed;
	  destheight = topheight;
	}

      // normal crushers are never deleted, they can only be put into stasis (paused)
    }
  else if (res == res_crushed)
    {
      // slow down slow crushers on obstacle (unless they are really fast)
      if (game.mode != gm_hexen && downspeed < CEILSPEED*3)
	speed = -CEILSPEED / 8;
    }
}




int Map::EV_DoCrusher(unsigned tag, int type, fixed_t uspeed, fixed_t dspeed, int crush, fixed_t height)
{
  sector_t   *sec;

  int secnum = -1;
  int rtn = 0;

  if (!tag)
    return 0;

  // activate stopped crushers
  rtn += ActivateInStasisCeiling(tag);

  while ((secnum = FindSectorFromTag(tag, secnum)) >= 0)
    {
      sec = &sectors[secnum];

      if (sec->Active(sector_t::ceiling_special))
	continue;

      // new ceiling thinker
      rtn++;
      crusher_t *crusher = new crusher_t(this, type, sec, uspeed, dspeed, crush, height);
      AddActiveCeiling(crusher);
    }
  return rtn;
}



//==========================================================================
//                     Common ceiling_t utilities
//==========================================================================



// Add an active ceiling
void Map::AddActiveCeiling(ceiling_t *ceiling)
{
  activeceilings.push_front(ceiling);
  ceiling->li = activeceilings.begin();
  // list iterators are only invalidated when erased
}



// Remove a ceiling's thinker
void Map::RemoveActiveCeiling(ceiling_t *ceiling)
{
  activeceilings.erase(ceiling->li); // remove the pointer from list
  ceiling->sector->ceilingdata = NULL;
  RemoveThinker(ceiling);
  TagFinished(ceiling->sector->tag);
}


//
// Restart a ceiling that's in-stasis
//
int Map::ActivateInStasisCeiling(unsigned tag)
{
  int rtn = 0;
  list<ceiling_t *>::iterator i;
  for (i = activeceilings.begin(); i != activeceilings.end(); i++)
    {
      ceiling_t *ceil = *i;
      if (ceil->sector->tag == tag && ceil->type & ceiling_t::InStasis)
	{
	  SN_StartSequence(&ceil->sector->soundorg, SEQ_PLAT + ceil->sector->seqType);
	  ceil->sector->ceilingdata = ceil; // TODO what if it is already in use?
	  ceil->type &= ~ceiling_t::InStasis;
	  rtn++;
	}
    }
  return rtn;
}


// Stop a ceiling from crushing!
int Map::EV_StopCeiling(unsigned tag)
{
  int rtn = 0;
  list<ceiling_t *>::iterator i;
  for (i = activeceilings.begin(); i != activeceilings.end(); i++)
    {
      ceiling_t *ceil = *i;
      if (!(ceil->type & ceiling_t::InStasis) && ceil->sector->tag == tag)
	{
	  SN_StopSequence(&ceil->sector->soundorg);
	  ceil->sector->ceilingdata = NULL;
	  TagFinished(ceil->sector->tag);
	  ceil->type |= ceiling_t::InStasis;
	  rtn++;
	}
    }
  return rtn;
}


// Removes all ceilings from the active ceiling list
void Map::RemoveAllActiveCeilings()
{
  activeceilings.clear();
}
