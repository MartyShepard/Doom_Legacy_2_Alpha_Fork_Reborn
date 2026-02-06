// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: p_camera.h 394 2006-12-01 00:35:25Z smite-meister $
//
// Copyright (C) 2002-2006 by DooM Legacy Team.
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

#ifndef p_camera_h
#define p_camera_h

#include "g_actor.h"


/// \brief Chasecam
///
/// target is the Actor the camera is following ("chasing").
/// owner  is the PlayerPawn (sic!) whose pov this camera is.
/// This is a HACK which allows us to make use of Actor data members.
class Camera : public Actor
{
  DECLARE_CLASS(Camera);

public:
  Camera(PlayerPawn *o, Actor *t);

  void ClearCamera();
  void ResetCamera(Actor *p);

  virtual void Think();
};


#endif
