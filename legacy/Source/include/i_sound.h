// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_sound.h 346 2006-07-17 18:16:19Z smite-meister $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
/// \brief Sound system interface

#ifndef i_sound_h
#define i_sound_h 1

#include "doomdef.h"

// Init at program start...
void I_StartupSound();
// ... shut down and relase at program termination.
void I_ShutdownSound();

// ... update sound buffer and audio device at runtime...
void I_UpdateSound();
void I_SubmitSound();


//
//  SFX I/O
//

void I_SetSfxVolume(int volume);

// Starts a sound in a particular sound channel, returns a handle
int I_StartSound(class soundchannel_t *c);

// Stops a sound channel.
void I_StopSound(soundchannel_t *c);

//
//  MUSIC I/O
//

void I_InitMusic();
void I_ShutdownMusic();

void I_SetMusicVolume(int volume);

// PAUSE game handling.
void I_PauseSong(int handle);
void I_ResumeSong(int handle);

// Registers a song handle to song data.
#ifdef __MACOS__
int I_RegisterSong(int song);
#else
int I_RegisterSong(void* data, int len);
#endif

// Called by anything that wishes to start music.
//  plays a song, and when the song is done,
//  starts playing it again in an endless loop.
// Horrible thing to do, considering.
void I_PlaySong(int handle, int looping);

// Stops a song over 3 seconds.
void I_StopSong(int handle);

// See above (register), then think backwards
void I_UnRegisterSong(int handle);


// i_cdmus.h : cd music interface
//
extern byte    cdaudio_started;

void   I_InitCD ();
void   I_StopCD ();
void   I_PauseCD ();
void   I_ResumeCD ();
void   I_ShutdownCD ();
void   I_UpdateCD ();
void   I_PlayCD (int track, bool looping);
int    I_SetVolumeCD (int volume);  // return 0 on failure

#endif
