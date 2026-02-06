// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_cdmus.cpp 514 2007-12-21 16:07:36Z smite-meister $
//
// Copyright (C) 1998-2005 by DooM Legacy Team.
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
/// \brief CD music interface


#include <stdlib.h>
#include <string.h>

#include "SDL.h"

#include "doomtype.h"
#include "i_sound.h"
#include "command.h"
#include "cvars.h"
#include "m_argv.h"

#define MAX_CD_TRACKS 256

static bool cdValid = false;
static bool playing = false;
static bool wasPlaying = false;
static bool initialized = false;
static bool enabled = false;
static bool playLooping = false;
static byte playTrack;
static byte maxTrack;
static byte cdRemap[MAX_CD_TRACKS];
static int  cdvolume = -1;


static SDL_CD *cdrom = NULL;
static Uint32 lastchk = 0;
static CDstatus cdStatus;



/// CDAudio_GetAudioDiskInfo
///
/// description:
/// set number of tracks if CD is available

static int CDAudio_GetAudioDiskInfo()
{
  cdValid = false;
  maxTrack = 0;
    
  cdStatus = SDL_CDStatus(cdrom);

  if (!CD_INDRIVE(cdStatus))
    {
      CONS_Printf(" No CD in drive\n");
      return -1;
    }
    
  if (cdStatus == CD_ERROR)
    {
      CONS_Printf(" CD Error: %s\n", SDL_GetError());
      return -1;
    }
    
  cdValid = true;
  maxTrack = cdrom->numtracks;
    
  return 0;
}



/// I_EjectCD
///
/// description:
///

static void I_EjectCD()
{
  if (cdrom == NULL || !enabled)
    return; // no cd init'd
    
  I_StopCD();
    
  if (SDL_CDEject(cdrom))
    {
      CONS_Printf("cdrom eject failed\n");
    }
  return;
}


/// Command_Cd_f
///
/// description:
/// handles all CD commands from the console

static void Command_Cd_f()
{
  const char *command;
  int   ret, n;

  if (!initialized)
    return;

  if (COM.Argc() < 2) {
    CONS_Printf("cd [on] [off] [remap] [reset] [open]\n"
		"   [info] [play <track>] [resume]\n"
		"   [stop] [pause] [loop <track>]\n");
    return;
  }

  command = COM.Argv(1);

  if (!strncmp(command, "on", 2)) {
    enabled = true;
    return;
  }

  if (!strncmp(command, "off", 3)) {
    I_StopCD();
    enabled = false;
    return;
  }
	
  if (!strncmp(command, "remap", 5)) {
    ret = COM.Argc() - 2;
    if (ret <= 0) {
      for (n = 1; n < MAX_CD_TRACKS; n++)
	if (cdRemap[n] != n)
	  CONS_Printf("  %u -> %u\n", n, cdRemap[n]);
      return;
    }
    for (n = 1; n <= ret; n++)
      cdRemap[n] = atoi(COM.Argv(n+1));
    return;
  }
        
  if (!strncmp(command, "reset", 5)) {
    enabled = true;
    I_StopCD();
            
    for (n = 0; n < MAX_CD_TRACKS; n++)
      cdRemap[n] = n;
    CDAudio_GetAudioDiskInfo();
    return;
  }
        
  if (!cdValid)
    {
      CDAudio_GetAudioDiskInfo();
      if (!cdValid)
	return;
    }

  if (!strncmp(command, "open", 4)) {
    I_EjectCD();
    cdValid = false;
    return;
  }

  if (!strncmp(command, "info", 4)) {
    CONS_Printf("%u tracks\n", maxTrack);
    if (playing)
      CONS_Printf("Currently %s track %u\n", playLooping ? "looping" : "playing", playTrack);
    else if (wasPlaying)
      CONS_Printf("Paused %s track %u\n", playLooping ? "looping" : "playing", playTrack);
    CONS_Printf("Volume is %d\n", cdvolume);
    return;
  }

  if (!strncmp(command, "play", 4)) {
    I_PlayCD((byte)atoi(COM.Argv(2)), false);
    return;
  }

  if (!strncmp(command, "loop", 4)) {
    I_PlayCD((byte)atoi(COM.Argv(2)), true);
    return;
  }

  if (!strncmp(command, "stop", 4)) {
    I_StopCD();
    return;
  }
        
  if (!strncmp(command, "pause", 5)) {
    I_PauseCD();
    return;
  }
        
  if (!strncmp(command, "resume", 6)) {
    I_ResumeCD();
    return;
  }
        
  CONS_Printf("Invalid command \"cd %s\"\n", COM.Argv(1));
}


/// StopCD
///
/// description:
///

void I_StopCD()
{
  if (cdrom == NULL || !enabled)
    return;
    
  if (!(playing || wasPlaying))
    return;
    
  if (SDL_CDStop(cdrom))
    {
      CONS_Printf("cdromstop failed\n");
    }
    
  wasPlaying = false;
  playing = false;
}


/// PauseCD
///
/// description:
///

void I_PauseCD()
{
  if (cdrom == NULL || !enabled)
    return;
    
  if (!playing)
    return;
    
  if (SDL_CDPause(cdrom))
    {
      CONS_Printf("cdrompause failed\n");
    }
    
  wasPlaying = playing;
  playing = false;
}


/// ResumeCD
///
/// description:
/// continue after a pause

void I_ResumeCD()
{
  if (cdrom == NULL || !enabled)
    return;
    
  if (!cdValid)
    return;
    
  if (!wasPlaying)
    return;
	
  if (cd_volume.value == 0)
    return;
    
  if (SDL_CDResume(cdrom))
    {
      CONS_Printf("cdromresume failed\n");
    }
    
  playing = true;
  wasPlaying = false;
 
  return;
}



/// ShutdownCD
///
/// description:
///
///

void I_ShutdownCD()
{
  if (!initialized)
    return;

  I_StopCD();

  SDL_CDClose(cdrom);
    
  cdrom = NULL;

  initialized = false;
  enabled = false;
}


/// InitCD
///
/// description:
/// Initialize the first CD drive SDL detects and add console command 'cd'

void I_InitCD()
{
  int i;
  const char *cdName;
    
  // Don't start music on a dedicated server
  if (M_CheckParm("-dedicated"))
    return ;
    
  // Has been checked in d_main.c, but doesn't hurt here
  if (M_CheckParm("-nocd"))
    return ;
    
  CONS_Printf(" Initializing CD audio...\n");

  // Initialize SDL first
  if (SDL_Init(SDL_INIT_CDROM) < 0) {
    CONS_Printf(" Couldn't initialize SDL CD: %s\n", SDL_GetError());
    return;
  }

  // Open drive
  cdrom = SDL_CDOpen(0);
  cdName = SDL_CDName(0);
    
  if (cdrom == NULL) {
    if (cdName == NULL)
      {
	    
	CONS_Printf(" Couldn't open default CD-ROM drive: %s\n",
		    SDL_GetError());
      }
    else
      {
	CONS_Printf(" Couldn't open default CD-ROM drive %s: %s\n",
		    cdName, SDL_GetError());
      }
	
    return;
  }
    
  for (i = 0; i < MAX_CD_TRACKS; i++)
    cdRemap[i] = i;
    
  initialized = true;
  enabled = true;

  CDAudio_GetAudioDiskInfo();

  COM.AddCommand("cd", Command_Cd_f);
    
  CONS_Printf(" CD audio initialized.\n");
    
  return ;
}



/// UpdateCD
///
/// description:
/// sets CD volume (may have changed) and initiates play evey 2 seconds
/// in case the song has elapsed

void I_UpdateCD()
{
  if (!enabled)
    return;
    
  I_SetVolumeCD(cd_volume.value);
	
  if (playing && lastchk < SDL_GetTicks()) 
    {
      lastchk = SDL_GetTicks() + 2000; //two seconds between chks
	
      if (CDAudio_GetAudioDiskInfo())
	{
	  playing = false;
	  return;
	}

      if (cdStatus != CD_PLAYING &&
	  cdStatus != CD_PAUSED)
	{
	  playing = false;
	  if (playLooping)
	    I_PlayCD(playTrack, true);
	}
    }
  return;
}




/// PlayCD
///
/// description:
/// play the requested track and set the looping flag
/// pauses the CD if volume is 0

void I_PlayCD(int track, bool looping)
{
  if (cdrom == NULL || !enabled)
    return;
    
  if (!cdValid)
    {
      CDAudio_GetAudioDiskInfo();
      if (!cdValid)
	return;
    }
    
  track = cdRemap[track];
    
  if (track < 1 || track > maxTrack)
    {
      CONS_Printf("I_PlayCD: Bad track number %u.\n", track);
      return;
    }
    
  // don't try to play a non-audio track
  if (cdrom->track[track].type == SDL_DATA_TRACK)
    {
      CONS_Printf("I_PlayCD: track %i is not audio\n", track);
      return;
    }
	
  if (playing)
    {
      if (playTrack == track)
	return;
      I_StopCD();
    }
    
  if (SDL_CDPlayTracks(cdrom, track, 0, 1, 0))
    {
      CONS_Printf("Error playing track %d: %s\n",
		  track, SDL_GetError());
      return;
    }
    
  playLooping = looping;
  playTrack = track;
  playing = true;

  if (cd_volume.value == 0)
    {
      I_PauseCD();
    }
    
}



/// SetVolumeCD
///
/// description:
/// SDL does not support setting the CD volume
/// use pause instead and toggle between full and no music

int I_SetVolumeCD(int volume)
{
  if (volume != cdvolume)
    {
      if (volume > 0 && volume < 16)
	{
	  cd_volume.Set(31);
	  cdvolume = 31;
	  
	  I_ResumeCD();
	}
      else if (volume > 15 && volume < 31)
	{
	  cd_volume.Set(0);
	  cdvolume = 0;
	    
	  I_PauseCD();
	}
    }
    
  return 0;
}
