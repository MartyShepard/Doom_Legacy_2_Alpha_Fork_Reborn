// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_genlin.cpp 485 2007-08-05 13:52:17Z smite-meister $
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
/// \brief BOOM generalized linedef types
///
///  Floors, Ceilings, Doors, Locked Doors, Lifts, Stairs, Crushers

#include "doomdef.h"

#include "g_game.h"
#include "g_map.h"

#include "p_spec.h" // geometry-related thinker classes
#include "m_random.h"

/*
  SoM: 3/9/2000: Copied this entire file from Boom sources to Legacy sources.
  This file contains all routiens for Generalized linedef types.
*/

//
// Handle generalized floor types
//
// Passed the line activating the generalized floor function
// Returns true if a thinker is created
//
int Map::EV_DoGenFloor(line_t *line)
{
  int       secnum;
  sector_t *sec;
  unsigned  value = unsigned(line->special - GenFloorBase);

  // parse the bit fields in the line's special type

  int Crsh = (value & FloorCrush) >> FloorCrushShift;
  int ChgT = (value & FloorChange) >> FloorChangeShift;
  int Targ = (value & FloorTarget) >> FloorTargetShift;
  int Dirn = (value & FloorDirection) >> FloorDirectionShift;
  int ChgM = (value & FloorModel) >> FloorModelShift;
  int Sped = (value & FloorSpeed) >> FloorSpeedShift;
  int Trig = (value & TriggerType) >> TriggerTypeShift;

  int rtn = 0;

  fixed_t speed; // set the speed of motion
  switch (Sped)
    {
    case SpeedNormal:
      speed = FLOORSPEED*2;
      break;
    case SpeedFast:
      speed = FLOORSPEED*4;
      break;
    case SpeedTurbo:
      speed = FLOORSPEED*8;
      break;
    case SpeedSlow:
    default:
      speed = FLOORSPEED;
      break;
    }

  fixed_t height = 0;
  int type;
  switch (Targ)
    {
    case FtoHnF:
      type = floor_t::HnF;
      break;
    case FtoLnF:
      type = floor_t::LnF;
      break;
    case FtoNnF:
      type = Dirn ? floor_t::UpNnF : floor_t::DownNnF;
      break;
    case FtoLnC:
      type = floor_t::LnC;
      break;
    case FtoC:
      type = floor_t::Ceiling;
      break;
    case FbyST:
      type = Dirn ? floor_t::UpSLT : floor_t::DownSLT;
      break;
    case Fby24:
      type = floor_t::RelHeight;
      height = (Dirn ? 1 : -1) * 24;
      break;
    case Fby32:
    default:
      type = floor_t::RelHeight;
      height = (Dirn ? 1 : -1) * 32;
      break;
    }

  // check if a manual trigger, if so do just the sector on the backside
  bool manual = false;
  if (Trig==PushOnce || Trig==PushMany)
    {
      if (!(sec = line->backsector))
	return rtn;
      secnum = sec-sectors;
      manual = true;
      goto manual_floor;
    }

  secnum = -1;
  // if not manual do all sectors tagged the same as the line
  while ((secnum = FindSectorFromTag(line->tag,secnum)) >= 0)
    {
      sec = &sectors[secnum];

    manual_floor:                
      // Do not start another function if floor already moving
      if (sec->Active(sector_t::floor_special))
	{
	  if (!manual)
	    continue;
	  else
	    return rtn;
	}

      // new floor thinker
      rtn++;
      floor_t *floor = new floor_t(this, type, sec, speed, Crsh ? 10 : 0, height);

      floor->speed = Dirn ? speed : -speed;

      floor->texture = sec->floorpic;
      floor->modelsec = secnum;

      // set texture/type change properties
      if (ChgT)   // if a texture change is indicated
	{
	  if (ChgM) // if a numeric model change
	    {
	      sector_t *sec2 = (Targ==FtoLnC || Targ==FtoC) ?
		sec->FindModelCeilingSector(floor->destheight) :
		sec->FindModelFloorSector(floor->destheight);

	      if (sec2)
		{
		  floor->texture = sec2->floorpic;
		  switch(ChgT)
		    {
		    case FChgZero:  // zero type
		      floor->modelsec = -1;
		      floor->type |= floor_t::SetTxTy;
		      break;
		    case FChgTyp:   // copy type
		      floor->modelsec = sec2 - sectors;
		      floor->type |= floor_t::SetTxTy;
		      break;
		    case FChgTxt:   // leave type be
		      floor->type |= floor_t::SetTexture;
		      break;
		    default:
		      break;
		    }
		}
	    }
	  else     // else if a trigger model change
	    {
	      floor->texture = line->frontsector->floorpic;
	      switch (ChgT)
		{
		case FChgZero:    // zero type
		  floor->modelsec = -1;
		  floor->type |= floor_t::SetTxTy;
		  break;
		case FChgTyp:     // copy type
		  floor->modelsec = line->frontsector - sectors;
		  floor->type |= floor_t::SetTxTy;
		  break;
		case FChgTxt:     // leave type be
		  floor->type |= floor_t::SetTexture;
		default:
		  break;
		}
	    }
	}
      if (manual) return rtn;
    }
  return rtn;
}


//
// Handle generalized ceiling types
//
// Passed the linedef activating the ceiling function
// Returns true if a thinker created
//
int Map::EV_DoGenCeiling(line_t *line)
{
  int       secnum;
  sector_t *sec;
  unsigned  value = unsigned(line->special) - GenCeilingBase;

  // parse the bit fields in the line's special type

  int Crsh = (value & CeilingCrush) >> CeilingCrushShift;
  int ChgT = (value & CeilingChange) >> CeilingChangeShift;
  int Targ = (value & CeilingTarget) >> CeilingTargetShift;
  int Dirn = (value & CeilingDirection) >> CeilingDirectionShift;
  int ChgM = (value & CeilingModel) >> CeilingModelShift;
  int Sped = (value & CeilingSpeed) >> CeilingSpeedShift;
  int Trig = (value & TriggerType) >> TriggerTypeShift;

  int rtn = 0;

  fixed_t speed;
  // set speed of motion
  switch (Sped)
    {
    case SpeedNormal:
      speed = CEILSPEED*2;
      break;
    case SpeedFast:
      speed = CEILSPEED*4;
      break;
    case SpeedTurbo:
      speed = CEILSPEED*8;
      break;
    case SpeedSlow:
    default:
      speed = CEILSPEED;
      break;
    }

  fixed_t height = 0;
  int type;
  switch (Targ)
    {
    case CtoHnC:
      type = ceiling_t::HnC;
      break;
    case CtoLnC:
      type = ceiling_t::LnC;
      break;
    case CtoNnC:
      type = Dirn ? ceiling_t::UpNnC : ceiling_t::DownNnC;
      break;
    case CtoHnF:
      type = ceiling_t::HnF;
      break;
    case CtoF:
      type = ceiling_t::Floor;
      break;
    case CbyST:
      type = Dirn ? ceiling_t::UpSUT : ceiling_t::DownSUT;
      break;
    case Cby24:
      type = ceiling_t::RelHeight;
      height = (Dirn ? 1 : -1) * 24;
      break;
    case Cby32:
    default:
      type = ceiling_t::RelHeight;
      height = (Dirn ? 1 : -1) * 32;
      break;
    }


  // check if a manual trigger, if so do just the sector on the backside
  bool manual = false;
  if (Trig==PushOnce || Trig==PushMany)
    {
      if (!(sec = line->backsector))
	return rtn;
      secnum = sec-sectors;
      manual = true;
      goto manual_ceiling;
    }

  secnum = -1;
  // if not manual do all sectors tagged the same as the line
  while ((secnum = FindSectorFromTag(line->tag,secnum)) >= 0)
    {
      sec = &sectors[secnum];

    manual_ceiling:                
      // Do not start another function if ceiling already moving
      if (sec->Active(sector_t::ceiling_special))
	{
	  if (!manual)
	    continue;
	  else
	    return rtn;
	}

      // new ceiling thinker
      rtn++;
      ceiling_t *ceiling = new ceiling_t(this, type, sec, speed, Crsh ? 10 : 0, height);

      ceiling->speed = Dirn ? speed : -speed;

      ceiling->texture = sec->ceilingpic;
      ceiling->modelsec = secnum;

      // set texture/type change properties
      if (ChgT)     // if a texture change is indicated
	{
	  if (ChgM)   // if a numeric model change
	    {
	      sector_t *sec2;

	      sec2 = (Targ==CtoHnF || Targ==CtoF) ?         
		sec->FindModelFloorSector(ceiling->destheight) :
		sec->FindModelCeilingSector(ceiling->destheight);
	      if (sec2)
		{
		  ceiling->texture = sec2->ceilingpic;
		  switch (ChgT)
		    {
		    case CChgZero:  // type is zeroed
		      ceiling->modelsec = -1;
		      ceiling->type |= ceiling_t::SetTxTy;
		      break;
		    case CChgTyp:   // type is copied
		      ceiling->modelsec = sec2 - sectors;
		      ceiling->type |= ceiling_t::SetTxTy;
		      break;
		    case CChgTxt:   // type is left alone
		      ceiling->type |= ceiling_t::SetTexture;
		      break;
		    default:
		      break;
		    }
		}
	    }
	  else        // else if a trigger model change
	    {
	      ceiling->texture = line->frontsector->ceilingpic;
	      switch (ChgT)
		{
		case CChgZero:    // type is zeroed
		  ceiling->modelsec = -1;
		  ceiling->type |= ceiling_t::SetTxTy;
		  break;
		case CChgTyp:     // type is copied
		  ceiling->modelsec = line->frontsector - sectors;
		  ceiling->type |= ceiling_t::SetTxTy;
		  break;
		case CChgTxt:     // type is left alone
		  ceiling->type |= ceiling_t::SetTexture;
		  break;
		default:
		  break;
		}
	    }
	}
      AddActiveCeiling(ceiling); // add this ceiling to the active list
      if (manual) return rtn;
    }
  return rtn;
}

//
// Handle generalized lift types
//
// Passed the linedef activating the lift
// Returns true if a thinker is created
//
int Map::EV_DoGenLift(line_t *line)
{
  plat_t*         plat;
  int             rtn;
  sector_t*       sec;
  unsigned        value = (unsigned)line->special - GenLiftBase;

  // parse the bit fields in the line's special type

  int Targ = (value & LiftTarget) >> LiftTargetShift;
  int Dely = (value & LiftDelay) >> LiftDelayShift;
  int Sped = (value & LiftSpeed) >> LiftSpeedShift;
  int Trig = (value & TriggerType) >> TriggerTypeShift;

  int secnum = -1;
  rtn = 0;

  // Activate all <type> plats that are in_stasis

  if (Targ==LnF2HnF)
    ActivateInStasisPlat(line->tag);
        
  // check if a manual trigger, if so do just the sector on the backside
  bool manual = false;
  if (Trig==PushOnce || Trig==PushMany)
    {
      if (!(sec = line->backsector))
	return rtn;
      secnum = sec-sectors;
      manual = true;
      goto manual_lift;
    }

  // if not manual do all sectors tagged the same as the line
  while ((secnum = FindSectorFromTag(line->tag,secnum)) >= 0)
    {
      sec = &sectors[secnum];

    manual_lift:
      // Do not start another function if floor already moving
      if (sec->Active(sector_t::floor_special))
	{
	  if (!manual)
	    continue;
	  else
	    return rtn;
	}
      
      int type;
      // setup the target destination height
      switch(Targ)
	{
	case F2LnF:
	  type = plat_t::LnF;
	  break;
	case F2NnF:
	  type = plat_t::NLnF;
	  break;
	case F2LnC:
	  type = plat_t::LnC;
	  break;
	case LnF2HnF:
	default:
	  type = plat_t::LHF;
	  break;
	}

      fixed_t speed;
      // setup the speed of motion
      switch(Sped)
	{
	case SpeedNormal:
	  speed = PLATSPEED * 4;
	  break;
	case SpeedFast:
	  speed = PLATSPEED * 8;
	  break;
	case SpeedTurbo:
	  speed = PLATSPEED * 16;
	  break;
	case SpeedSlow:
	default:
	  speed = PLATSPEED * 2;
	  break;
	}

      int wait;
      // setup the delay time before the floor returns
      switch(Dely)
	{
	case 0:
	  wait = 1*35;
	  break;
	case 1:
	  wait = PLATWAIT*35;
	  break;
	case 2:
	  wait = 5*35;
	  break;
	case 3:
	default:
	  wait = 10*35;
	  break;
	}

      // Setup the plat thinker
      rtn++;
      plat = new plat_t(this, type, sec, speed, wait, 0);
      AddActivePlat(plat); // add this plat to the list of active plats

      if (manual)
	return rtn;
    }
  return rtn;
}

//
// Handle generalized stair building
//
// Passed the linedef activating the stairs
// Returns true if a thinker is created
//
int Map::EV_DoGenStairs(line_t *line)
{
  int       secnum;
  int       i;
  sector_t *sec;
    
  unsigned value = (unsigned)line->special - GenStairsBase;

  // parse the bit fields in the line's special type

  int Igno = (value & StairIgnore) >> StairIgnoreShift;
  int Dirn = (value & StairDirection) >> StairDirectionShift;
  int Step = (value & StairStep) >> StairStepShift;
  int Sped = (value & StairSpeed) >> StairSpeedShift;
  int Trig = (value & TriggerType) >> TriggerTypeShift;

  int rtn = 0;

  // check if a manual trigger, if so do just the sector on the backside
  bool manual = false;
  if (Trig==PushOnce || Trig==PushMany)
    {
      if (!(sec = line->backsector))
	return rtn;
      secnum = sec-sectors;
      manual = true;
      goto manual_stair;
    }

  secnum = -1;
  // if not manual do all sectors tagged the same as the line
  while ((secnum = FindSectorFromTag(line->tag,secnum)) >= 0)
    {
      sec = &sectors[secnum];

    manual_stair:          
      //Do not start another function if floor already moving
      //Add special lockout condition to wait for entire
      //staircase to build before retriggering
      if (sec->Active(sector_t::floor_special) || sec->stairlock)
	{
	  if (!manual)
	    continue;
	  else
	    return rtn;
	}
      
      fixed_t speed;
      // setup speed of stair building
      switch(Sped)
	{
	default:
	case SpeedSlow:
	  speed = FLOORSPEED/4;
	  break;
	case SpeedNormal:
	  speed = FLOORSPEED/2;
	  break;
	case SpeedFast:
	  speed = FLOORSPEED*2;
	  break;
	case SpeedTurbo:
	  speed = FLOORSPEED*4;
	  break;
	}

      fixed_t stairsize;
      // setup stepsize for stairs
      switch(Step)
	{
	default:
	case 0:
	  stairsize = 4;
	  break;
	case 1:
	  stairsize = 8;
	  break;
	case 2:
	  stairsize = 16;
	  break;
	case 3:
	  stairsize = 24;
	  break;
	}
      if (!Dirn)
	stairsize = -stairsize;

      fixed_t height = sec->floorheight + stairsize;

      // new floor thinker
      rtn++;
      floor_t *floor = new floor_t(this, floor_t::AbsHeight, sec, speed, 0, height);

      Material *texture = sec->floorpic;

      sec->stairlock = -2;
      sec->nextsec = -1;
      sec->prevsec = -1;

      int osecnum = secnum;
      // Find next sector to raise
      // 1.     Find 2-sided line with same sector side[0]
      // 2.     Other side is the next sector to raise
      bool ok;
      do
	{
	  ok = false;
	  for (i = 0;i < sec->linecount;i++)
	    {
	      if ( !((sec->lines[i])->backsector) )
		continue;
                                  
	      sector_t *tsec = (sec->lines[i])->frontsector;
	      int newsecnum = tsec-sectors;
          
	      if (secnum != newsecnum)
		continue;

	      tsec = (sec->lines[i])->backsector;
	      newsecnum = tsec - sectors;

	      if (!Igno && tsec->floorpic != texture)
		continue;

	      if (tsec->Active(sector_t::floor_special) || tsec->stairlock)
		continue;

	      height += stairsize;

	      // link the stair chain in both directions
	      // lock the stair sector until building complete
	      sec->nextsec = newsecnum; // link step to next
	      tsec->prevsec = secnum;   // link next back
	      tsec->nextsec = -1;       // set next forward link as end
	      tsec->stairlock = -2;     // lock the step

	      sec = tsec;
	      secnum = newsecnum;

	      floor = new floor_t(this, floor_t::AbsHeight, sec, speed, 0, height);

	      ok = true;
	      break;
	    }
	} while(ok);
      if (manual)
        return rtn;
      secnum = osecnum;
    }
  // retriggerable generalized stairs build up or down alternately
  if (rtn)
    line->special ^= StairDirection; // alternate dir on succ activations
  return rtn;
}

//
// Handle generalized crusher types
//
// Passed the linedef activating the crusher
// Returns true if a thinker created
//
int Map::EV_DoGenCrusher(line_t *line)
{
  int       secnum;
  sector_t *sec;
  unsigned  value = unsigned(line->special) - GenCrusherBase;

  // parse the bit fields in the line's special type

  int Slnt = (value & CrusherSilent) >> CrusherSilentShift;
  int Sped = (value & CrusherSpeed) >> CrusherSpeedShift;
  int Trig = (value & TriggerType) >> TriggerTypeShift;

  int rtn = ActivateInStasisCeiling(line->tag);

  fixed_t speed;
  // setup ceiling motion speed
  switch (Sped)
    {
    case SpeedNormal:
      speed = CEILSPEED*2;
      break;
    case SpeedFast:
      speed = CEILSPEED*4;
      break;
    case SpeedTurbo:
      speed = CEILSPEED*8;
      break;
    default:
    case SpeedSlow:
      speed = CEILSPEED;
      break;
    }

  // check if a manual trigger, if so do just the sector on the backside
  bool manual = false;
  if (Trig==PushOnce || Trig==PushMany)
    {
      if (!(sec = line->backsector))
	return rtn;
      secnum = sec-sectors;
      manual = true;
      goto manual_crusher;
    }

  secnum = -1;
  // if not manual do all sectors tagged the same as the line
  while ((secnum = FindSectorFromTag(line->tag,secnum)) >= 0)
    {
      sec = &sectors[secnum];

    manual_crusher:                
      // Do not start another function if ceiling already moving
      if (sec->Active(sector_t::ceiling_special))
	{
	  if (!manual)
	    continue;
	  else
	    return rtn;
	}

      // new crusher thinker
      rtn++;
      crusher_t *crusher = new crusher_t(this, Slnt ? ceiling_t::Silent : 0, sec, speed, speed, 10, 8);

      crusher->texture = sec->ceilingpic;
      crusher->modelsec = secnum;

      AddActiveCeiling(crusher);  // add to list of active ceilings
      if (manual) return rtn;
    }
  return rtn;
}

//
// Handle generalized locked door types
//
// Passed the linedef activating the generalized locked door
// Returns true if a thinker created
//
int Map::EV_DoGenLockedDoor(line_t *line)
{
  int   secnum;
  sector_t* sec;
  unsigned  value = unsigned(line->special) - GenLockedBase;

  // parse the bit fields in the line's special type

  int Kind = (value & LockedKind) >> LockedKindShift;
  int Sped = (value & LockedSpeed) >> LockedSpeedShift;
  int Trig = (value & TriggerType) >> TriggerTypeShift;

  int rtn = 0;

  // check if a manual trigger, if so do just the sector on the backside
  bool manual = false;
  if (Trig==PushOnce || Trig==PushMany)
    {
      if (!(sec = line->backsector))
	return rtn;
      secnum = sec-sectors;
      manual = true;
      goto manual_locked;
    }

  secnum = -1;
  rtn = 0;
  
  // if not manual do all sectors tagged the same as the line
  while ((secnum = FindSectorFromTag(line->tag,secnum)) >= 0)
    {
      sec = &sectors[secnum];
    manual_locked:
      // Do not start another function if ceiling already moving
      if (sec->Active(sector_t::ceiling_special))
	{
	  if (!manual)
	    continue;
	  else
	    return rtn;
	}
  
      // new door thinker
      rtn++;

      fixed_t speed;
      int type = Kind ? vdoor_t::Open : vdoor_t::OwC;
      // setup speed of door motion
      switch (Sped)
	{
	default:
	case SpeedSlow:
	  speed = VDOORSPEED;
	  break;
	case SpeedNormal:
	  speed = VDOORSPEED*2;
	  break;
	case SpeedFast:
	  type |= vdoor_t::Blazing;
	  speed = VDOORSPEED*4;
	  break;
	case SpeedTurbo:
	  type |= vdoor_t::Blazing;
	  speed = VDOORSPEED*8;
	  break;
	}

      vdoor_t *door = new vdoor_t(this, type, sec, speed, VDOORWAIT);

      if (manual)
	{
	  door->boomlighttag = line->tag;
	  return rtn;
	}
    }
  return rtn;
}

//
// Handle generalized door types
//
// Passed the linedef activating the generalized door
// Returns true if a thinker created
//
int Map::EV_DoGenDoor(line_t *line)
{
  int   secnum;
  sector_t* sec;
  unsigned  value = unsigned(line->special) - GenDoorBase;

  // parse the bit fields in the line's special type

  int Dely = (value & DoorDelay) >> DoorDelayShift;
  int Kind = (value & DoorKind) >> DoorKindShift;
  int Sped = (value & DoorSpeed) >> DoorSpeedShift;
  int Trig = (value & TriggerType) >> TriggerTypeShift;

  int rtn = 0;

  // check if a manual trigger, if so do just the sector on the backside
  bool manual = false;
  if (Trig==PushOnce || Trig==PushMany)
    {
      if (!(sec = line->backsector))
	return rtn;
      secnum = sec-sectors;
      manual = true;
      goto manual_door;
    }


  secnum = -1;
  rtn = 0;
  
  // if not manual do all sectors tagged the same as the line
  while ((secnum = FindSectorFromTag(line->tag,secnum)) >= 0)
    {
      sec = &sectors[secnum];
    manual_door:
      // Do not start another function if ceiling already moving
      if (sec->Active(sector_t::ceiling_special))
	{
	  if (!manual)
	    continue;
	  else
	    return rtn;
	}
  
      // new door thinker
      rtn++;

      int type;
      // set kind of door, whether it opens then close, opens, closes etc.
      // assign target heights accordingly
      switch (Kind)
	{
	default:
	case OdCDoor:
	  type = vdoor_t::OwC;
	  break;
	case ODoor:
	  type = vdoor_t::Open;
	  break;
	case CdODoor:
	  type = vdoor_t::CwO;
	  break;
	case CDoor:
	  type = vdoor_t::Close;
	  break;
	}

      fixed_t speed;
      // setup speed of door motion
      switch(Sped)
	{
	default:
	case SpeedSlow:
	  speed = VDOORSPEED;
	  break;
	case SpeedNormal:
	  speed = VDOORSPEED*2;
	  break;
	case SpeedFast:
	  type |= vdoor_t::Blazing;
	  speed = VDOORSPEED*4;
	  break;
	case SpeedTurbo:
	  type |= vdoor_t::Blazing;
	  speed = VDOORSPEED*8;
	  break;
	}

      int wait;
      // setup delay for door remaining open/closed
      switch(Dely)
	{
	default:
	case 0:
	  wait = 35;
	  break;
	case 1:
	  wait = VDOORWAIT;
	  break;
	case 2:
	  wait = 2*VDOORWAIT;
	  break;
	case 3:
	  wait = 7*VDOORWAIT;
	  break;
	}

      vdoor_t *door = new vdoor_t(this, type, sec, speed, wait);

      if (manual)
	{
	  door->boomlighttag = line->tag;
	  return rtn;
	}
    }
  return rtn;
}
