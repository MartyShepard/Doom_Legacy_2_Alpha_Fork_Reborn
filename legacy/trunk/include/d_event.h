// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: d_event.h 504 2007-12-16 17:58:26Z smite-meister $
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Input event system

#ifndef d_event_h
#define d_event_h 1

/// \brief Input event types.
enum evtype_t
{
  ev_keydown,
  ev_keyup,
  ev_mouse,
  ev_mouse2
};

/// \brief Event structure.
struct event_t
{
  evtype_t    type;
  int         data1;  ///< keys / mouse/joystick buttons
  int         data2;  ///< mouse x move
  int         data3;  ///< mouse y move
};

//
// GLOBAL VARIABLES
//
#define MAXEVENTS               64

extern  event_t         events[MAXEVENTS];
extern  int             eventhead;
extern  int             eventtail;


// current modifier key status
extern bool shiftdown;
extern bool altdown;


// finale
bool F_Responder(event_t* ev);

// cheat
bool cht_Responder(event_t* ev);

/// Called by IO functions when input is detected.
void D_PostEvent(const event_t* ev);
void D_ProcessEvents();

#endif
