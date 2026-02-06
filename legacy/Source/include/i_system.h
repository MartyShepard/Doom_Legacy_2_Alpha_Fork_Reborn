// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_system.h 516 2007-12-22 15:57:58Z jussip $
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
/// \brief System specific interface stuff.

#ifndef i_system_h
#define i_system_h 1

#include "doomtype.h"


/// basic system initialization
void I_SysInit();

/// returns current time in 1/TICRATE second tics
tic_t I_GetTics();

/// returns current time in ms
unsigned int I_GetTime();

/// sleeps for a given amount of ms (accurate to about 10 ms)
void I_Sleep(unsigned int ms);

/// quits the game
void I_Quit();

/// writes a message to stdout
void I_OutputMsg(const char *error, ...);

/// creates a new directory
int  I_mkdir(const char *dirname, int unixright);

/// returns the free space on disk in bytes
void I_GetDiskFreeSpace(Sint64 *freespace);

/// return free and total physical memory in the system
Uint32 I_GetFreeMem(Uint32 *total);

// called before processing any tics in a frame
// (just after displaying a frame).
// Time consuming syncronous operations
// are performed here (joystick reading).
// Can call D_PostEvent.
void I_StartFrame();

// called before processing each tic in a frame.
// Quick syncronous operations are performed here.
// Can call D_PostEvent.
void I_OsPolling ();

// Asynchronous interrupt functions should maintain private queues
// that are read by the synchronous functions
// to be converted into events.

/// returns the username of the current user (or NULL)
char *I_GetUserName();

/// returns the path to the default wadfile location (usually the current working directory)
const char *I_GetWadPath();



//===========================================
//               User input
//===========================================

/// Polls input events and pushes them to the Legacy event queue.
void I_GetEvent();

/// Joystick init and cleanup.
void I_JoystickInit();
void I_ShutdownJoystick();

void I_StartupMouse();
void I_StartupMouse2();

/// Grabs mouse and keyboard input from the window manager.
void I_GrabMouse();
void I_UngrabMouse();

// Either returns a null ticcmd,
// or calls a loadable driver to build it.
// This ticcmd will then be modified by the gameloop
// for normal input.
struct ticcmd_t *I_BaseTiccmd();


extern int Console_Requested;// Marty

void DL_CaptureMouse(void);
//=================================================================
// Some old stuff I like to keep here for the nostalgia value:)
//=================================================================

/*
// Allocates from low memory under dos, just mallocs under unix
byte* I_AllocLow (int length);

//added:22-02-98: force feedback ??? electro-shock???
void I_Tactile(int on, int off, int total);

// setup timer irq and user timer routine.
void I_TimerISR();      //timer callback routine.
void I_StartupTimer();

// list of functions to call at program cleanup
void I_AddExitFunc(void (*func)());
void I_RemoveExitFunc(void (*func)());
*/

#endif
