// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_camera.cpp 522 2008-01-20 22:42:23Z smite-meister $
//
// Copyright (C) 2003-2006 by DooM Legacy Team.
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
/// \brief Chasecam.

#include <math.h>

#include "command.h"
#include "cvars.h"

#include "g_map.h"
#include "g_player.h"
#include "g_pawn.h"

#include "p_camera.h"
#include "r_defs.h"

#include "m_archive.h"
#include "tables.h"


//consvar_t cv_chasecam   = {"chasecam", "0", CV_CALL | CV_NOINIT, CV_OnOff, Chasecam_OnChange};
consvar_t cv_cam_dist   = {"cam_dist"  ,"128"  ,CV_FLOAT,NULL};
consvar_t cv_cam_height = {"cam_height", "20"   ,CV_FLOAT,NULL};
consvar_t cv_cam_speed  = {"cam_speed" ,  "0.25",CV_FLOAT,NULL};


IMPLEMENT_CLASS(Camera, Actor);

Camera::Camera()
  : Actor()
{
  flags = MF_NOBLOCKMAP | MF_NOSECTOR | MF_NOGRAVITY | MF_NOSPLASH | MF_NOCLIPTHING | MF_PLAYER;
  flags2 = MF2_SLIDE | MF2_DONTDRAW;

  health = 1000;
  mass = 10;
  radius = 20;
  //height = 16;
  height = 5;
}



Camera::Camera(PlayerPawn *o, Actor *t)
  : Actor()
{
  flags = MF_NOBLOCKMAP | MF_NOSECTOR | MF_NOGRAVITY | MF_NOSPLASH | MF_NOCLIPTHING | MF_PLAYER;
  flags2 = MF2_SLIDE | MF2_DONTDRAW;

  health = 1000;
  mass = 10;
  radius = 20;
  height = 5;

  owner = o;

  if (t)
    ResetCamera(t); // target t, spawn cam in map
}


int Camera::Marshal(LArchive &a)
{
  Actor::Marshal(a);
  return 0;
}


void Camera::ClearCamera()
{
  if (mp)
    {
      Actor *temp = owner; // save it (HACK)
      Detach();
      owner = temp;
    }
}



// make sure the camera is not outside the world
// and looks at the thing it is supposed to
void Camera::ResetCamera(Actor *p)
{
  target = p;

  pos = p->pos;
  pos.z += cv_viewheight.Get();

  if (mp != p->mp)
    {
      if (mp != NULL)
	ClearCamera();

      // add cam to the map
      p->mp->SpawnActor(this, 0);
      //cam = p->mp->SpawnActor(x,y,z, MT_CHASECAM);
    }

  yaw = p->yaw;
  pitch = 0;

  // hey we should make sure that the sounds are heard from the camera
  // instead of the marine's head : TODO
}


void Camera::Think()
{
  PlayerPawn *o = reinterpret_cast<PlayerPawn*>(owner);
  if (!o || !o->player || o->player->pov != this)
    {
      // We're no longer pov and thus unnecessary.
      Remove();
      return;
    }

  if (target == NULL)
    return;

  if (mp == NULL)
    I_Error("ChaseCamera: no map set\n");

  // chasecam is reset when 1) target respawns, 2) target teleports, 3) chasecam is first turned on

  angle_t ang = target->yaw;

  // sets ideal cam pos
  float dist = cv_cam_dist.Get().Float();
  vec_t<fixed_t> t = target->pos;
  t.x -= dist * Cos(ang);
  t.y -= dist * Sin(ang);
  t.z += cv_viewheight.Get() + cv_cam_height.Get();

  vec_t<fixed_t> delta = t-pos;
  vec_t<float> temp(delta.x.Float(), delta.y.Float(), delta.z.Float()); 

  // warp to target if it's too far away!
  if (temp.Norm() > 2.5f*dist)
    ResetCamera(target);

  // move camera down to move under lower ceilings
  subsector_t *newsubsec = mp->FindSubsector((target->pos.x + t.x) >> 1, (target->pos.y + t.y) >> 1);
  if (!newsubsec)
    {
      // use player sector
      newsubsec = target->subsector;
    }

  if (newsubsec->sector->ceilingheight < t.z + height)
    t.z = newsubsec->sector->ceilingheight - height;// - 11; // No ticket!
  // don't be blocked by a opened door

  // does the camera fit in its own sector
  newsubsec = mp->GetSubsector(t.x, t.y);
  if (newsubsec->sector->ceilingheight < t.z + height)
    t.z = newsubsec->sector->ceilingheight - height;// - 11;


  // point viewed by the camera
  // this point is just 64 unit forward the player
  dist = 64;
  fixed_t viewpointx, viewpointy;

  viewpointx = target->pos.x + dist * Cos(ang);
  viewpointy = target->pos.y + dist * Sin(ang);

  yaw = R_PointToAngle2(t.x, t.y, viewpointx, viewpointy);

  // follow the player
  vel = (t-pos) * cv_cam_speed.Get();

  // compute aiming to look the viewed point
  float f1 = (viewpointx - pos.x).Float();
  float f2 = (viewpointy - pos.y).Float();
  dist = sqrtf(f1*f1 + f2*f2);

  int dp = pitch - R_PointToAngle2(0, pos.z, dist, target->Center() + Sin(target->pitch) * 64);
  pitch -= (dp >> 3);

  Actor::Think();
}




/*
bool PTR_FindCameraPoint (intercept_t* in)
{
  
  static fixed_t cameraz;
  int         side;
	fixed_t             slope;
	fixed_t             dist;
	line_t*             li;

	li = in->d.line;

	if ( !(li->flags & ML_TWOSIDED) )
        return false;

    // crosses a two sided line
    //added:16-02-98: Fab comments : sets opentop, openbottom, openrange
    //                lowfloor is the height of the lowest floor
    //                         (be it front or back)
    P_LineOpening (li);

    dist = FixedMul (attackrange, in->frac);

    if (li->frontsector->floorheight != li->backsector->floorheight)
    {
    //added:18-02-98: comments :
    // find the slope aiming on the border between the two floors
    slope = FixedDiv (openbottom - cameraz , dist);
    if (slope > aimslope)
    return false;
    }

    if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
    {
    slope = FixedDiv (opentop - shootz , dist);
    if (slope < aimslope)
    goto hitline;
    }

    return true;

    // hit line
    hitline:
  // stop the search
  return false;
}
*/
