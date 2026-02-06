// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_sound.cpp 514 2007-12-21 16:07:36Z smite-meister $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2000-2007 by Doom Legacy team
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief SDL interface for sound and music.

#include <math.h>
#include <unistd.h>
#include <stdlib.h>

#include "SDL.h"
#ifndef NO_MIXER
# include "SDL_mixer.h"
#endif

#include "doomdef.h"
#include "doomtype.h"
#include "command.h"
#include "cvars.h"

#include "z_zone.h"

#include "m_fixed.h"
#include "m_swap.h"
#include "i_system.h"
#include "i_sound.h"
#include "s_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"

#include "s_sound.h"
#include "sounds.h"

#include "qmus2mid.h"


#define MIDBUFFERSIZE   128*1024
#define SAMPLERATE      22050*2   // Hz //Marty
#define SAMPLECOUNT     1024 // requested audio buffer size (512 means about 46 ms at 11 kHz)


// Pitch to stepping lookup in 16.16 fixed point. 64 pitch units = 1 octave
//  0 = 0.25x, 128 = 1x, 256 = 4x
static fixed_t steptable[256];

// Volume lookups.
static int vol_lookup[128][256];

// Buffer for MIDI
static byte *mus2mid_buffer;


// Flags for the -nosound and -nomusic options
extern bool nosound;
extern bool nomusic;

static bool musicStarted = false;
static bool soundStarted = false;

static SDL_AudioSpec audio;

#ifndef NO_MIXER
/// the "registered" piece of music
static struct music_channel_t
{
  Mix_Music *mus;
  SDL_RWops *rwop; ///< must not be freed before music is halted

  music_channel_t() { mus = NULL; rwop = NULL; }
} music;
#endif



static void I_SetChannels()
{
  // Init internal lookups (raw data, mixing buffer, channels).
  // This function sets up internal lookups used during
  //  the mixing process.
  int i, j;

  // This table provides step widths for pitch parameters.
  for (i = 0; i < 256; i++)
    steptable[i] = float(pow(2.0, ((i-128)/64.0)));

  // Generates volume lookup tables
  //  which also turn the U8 samples
  //  into S16 samples.
  for (i=0 ; i<128 ; i++)
    for (j=0 ; j<256 ; j++)
      vol_lookup[i][j] = (i*(j-128)*256)/127;
}

/*
static int FindChannel(int handle)
{
  int i;

  for (i = 0; i < NUM_CHANNELS; i++)
    if (channels[i].handle == handle)
      return i;
  
  // not found
  return -1;
}
*/


//----------------------------------------------


void I_SetSfxVolume(int volume)
{
  // Basically, this should propagate
  //  the menu/config file setting
  //  to the state variable used in
  //  the mixing.
  // Since the variable is cv_soundvolume,
  // nothing needs to be done.
}

// used to (re)calculate channel params
void soundchannel_t::CalculateParams()
{
  // how fast should the sound sample be played?
  step = (double(si->rate) / audio.freq) * steptable[pitch];

  // x^2 separation, that is, orientation/stereo.

  float vol = (volume * cv_soundvolume.value) / 64.0;
  // note: >> 5 would use almost the entire dynamical range, but
  // then there would be no "dynamical room" for other sounds :-/
  float sep = separation;
  leftvol  = vol * (1 - sep*sep);
  sep = 1 - sep;
  rightvol = vol * (1 - sep*sep);

  // Sanity check, clamp volume.
  if (rightvol < 0 || rightvol >= 0.5)
    I_Error("rightvol out of bounds");

  if (leftvol < 0 || leftvol >= 0.5)
    I_Error("leftvol out of bounds");

  // Volume arrives in range 0..1 and it must be in 0..127
  // Get the proper lookup table piece for this volume level
  leftvol_lookup = vol_lookup[int(128*leftvol)];
  rightvol_lookup = vol_lookup[int(128*rightvol)];
}

//----------------------------------------------
// This function adds a sound to the
//  list of currently active sounds,
//  which is maintained as a given number
//  (eight, usually) of internal channels.

int I_StartSound(soundchannel_t *c)
{
  if (nosound)
    return 0;

#ifdef NO_MIXER
  SDL_LockAudio();
#endif

  // Tales from the cryptic.
  c->samplesize = c->si->depth;

  // Set pointer to raw data.
  c->data = (Uint8 *)c->si->sdata;
  // Set pointer to end of raw data.
  c->end = c->data + c->si->length;
  
  c->stepremainder = 0;

  c->playing = true;

#ifdef NO_MIXER
  SDL_UnlockAudio();
#endif

  /*
  // Assign current handle number.
  static unsigned short handlenum = 100;
  c->handle = handlenum--;

  // Reset current handle number, limited to 1..100.
  if (!handlenum)
    handlenum = 100;

  return c->handle;
  */
  return 1;
}


void I_StopSound(soundchannel_t *c)
{
  c->playing = false;
}

/*
bool I_SoundIsPlaying(int handle)
{
  return (FindChannel(handle) < 0);
}
*/

// Not used by SDL version
//void I_SubmitSound() {}

// Not used by SDL version. Nevertheless, please 
// witness the wit below:
// void I_UpdateSound () {}

/* Pour une raison que j'ignore, la version SDL n'appelle jamais
   ce truc directement. Fonction vide pour garder une compatibilité
   avec le point de vue de legacy... */

// Himmel, Arsch und Zwirn

//
// The SDL audio callback.
//
// This function loops all active (internal) sound
//  channels, retrieves a given number of samples
//  from the raw sound data, modifies it according
//  to the current (internal) channel parameters,
//  mixes the per channel samples into the given
//  mixing buffer, and clamping it to the allowed
//  range.
//
// This function currently supports only 16bit.
//
static void I_UpdateSound_sdl(void *unused, Uint8 *stream, int len)
{
  if (nosound)
    return;

  // stream contains len bytes of outgoing stereo (LR order) music data.
  // Here we mix in current sound data.

  // Data, from raw sound, for right and left.
  register int dl, dr;

  // Left and right channel
  // are in S16 audio stream, alternating.
  Sint16 *leftout = (Sint16 *)stream;
  Sint16 *rightout = ((Sint16 *)stream) + 1;

  // Step in stream, left and right, thus two.
  int step = 2;

  // Determine end, for left channel only
  //  (right channel is implicit).
  Sint16 *leftend = leftout + len/step;


  int n = S.channels.size();
  for (int i = 0; i < n; i++)
    {
      soundchannel_t *c = &S.channels[i];
      // Check if channel is active.
      if (c->playing)
	{
	  // Set current pitch, volume and separation
	  c->CalculateParams();
	}
    }

  // Mix sounds into the mixing buffer.
  // Loop over entire buffer
  while (leftout != leftend)
    {
      // Reset left/right value.
      dl = *leftout;
      dr = *rightout;

      // Love thy L2 chache - made this a loop.
      // Now more channels could be set at compile time
      //  as well. Thus loop those  channels.
      for (int i = 0; i < n; i++)
        {
	  soundchannel_t *c = &S.channels[i];
	  // Check if channel is active.
	  if (c->playing)
            {
	      // handle pitch.
	      c->stepremainder += c->step;

	      register unsigned int sample;
	      // Get the raw data from the channel.
	      if (c->samplesize == 2)
		{
		  // S16 FIXME this still does not work right
		  sample = *reinterpret_cast<Sint16 *>(c->data);
		  dl +=  int(c->leftvol*sample);
		  dr += int(c->rightvol*sample);

		  // 16.16 fixed point: high word is the current stride
		  c->data += 2*c->stepremainder.floor();
		}
	      else
		{
		  // U8
		  sample = *(c->data);
		  // Add left and right part for this channel (sound) to the current data.
		  // Adjust volume accordingly.
		  dl += c->leftvol_lookup[sample];
		  dr += c->rightvol_lookup[sample];

		  // 16.16 fixed point: high word is the current stride
		  c->data += c->stepremainder.floor();
		}

	      // cut away high word
	      c->stepremainder = c->stepremainder.frac();
	      
	      // Check whether data is exhausted.
	      if (c->data >= c->end)
		{
		  c->data = NULL;
		  c->playing = false; // notify SoundSystem
		}
            }
        }

      // Clamp to range. Left hardware channel.
      if (dl > 0x7fff)
	*leftout = 0x7fff;
      else if (dl < -0x8000)
	*leftout = -0x8000;
      else
	*leftout = dl;

      // Same for right hardware channel.
      if (dr > 0x7fff)
	*rightout = 0x7fff;
      else if (dr < -0x8000)
	*rightout = -0x8000;
      else
	*rightout = dr;

      // Increment current pointers in stream
      leftout += step;
      rightout += step;
    }
}

/*
bool I_UpdateSoundParams(int handle, int vol, int sep, int pitch)
{
  // Would be using the handle to identify
  //  on which channel the sound might be active,
  //  and resetting the channel parameters.

  int i = FindChannel(handle);
  if (i < 0)
    return false;

  chan_t *c = &channels[i];
  if (c->data)
    {
      I_SetChannelParams(c, vol, sep, pitch);
      return true;
    }
  return false;
}
*/




void I_StartupSound()
{
  if (nosound)
    return;

  // Configure sound device
  if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
      CONS_Printf(" Couldn't initialize SDL Audio: %s\n", SDL_GetError());
      nosound = true;
      return;
    }

  // audio device parameters
  audio.freq = SAMPLERATE;
  audio.format = AUDIO_S16SYS;
  audio.channels = 2;
  audio.samples = SAMPLECOUNT;
  audio.callback = I_UpdateSound_sdl;

  I_SetChannels();

  // SDL_mixer controls the audio device, see I_InitMusic.
#ifdef NO_MIXER
  SDL_OpenAudio(&audio, NULL);
  CONS_Printf(" Audio device initialized: %d Hz, %d samples/slice.\n", audio.freq, audio.samples);
  SDL_PauseAudio(0);
  soundStarted = true;
#endif
}




// initializes both sound and music
void I_InitMusic()
{
	//printf(" [%s][%d] I_InitMusic(): (nosound arg = %s\n",__FILE__,__LINE__,(nosound==true)?"Ja":"Nein");
  if (nosound)
    {
      nomusic = false;
      //return;
    }

#ifdef NO_MIXER
  printf(" [%s][%d] I_InitMusic(): Keine Music. SDL Mixer wurde nicht mit kompiliert\n",__FILE__,__LINE__); 
  nomusic = true; 
#else

  // because we use SDL_mixer, audio is opened here.
  if (Mix_OpenAudio(audio.freq, audio.format, audio.channels, audio.samples) < 0)
    {
      CONS_Printf(" Unable to open audio: %s\n", Mix_GetError());
      nosound = nomusic = true;
      return;
    }

  int temp; // aargh!
  if (!Mix_QuerySpec(&audio.freq, &audio.format, &temp))
    {
      CONS_Printf(" Mix_QuerySpec: %s\n", Mix_GetError());
      nosound = nomusic = true;
      return;
    }

  Mix_SetPostMix(audio.callback, NULL);  // after mixing music, add sound effects
  
  CONS_Printf(" Audio device initialized: %d Hz, %d samples/slice.\n", audio.freq, audio.samples);
  
  Mix_Resume(-1); // start all sound channels (although they are not used)

  soundStarted = true;


  if (nomusic)
    return;

  Mix_ResumeMusic();  // start music playback
	
  mus2mid_buffer = (byte *)Z_Malloc(MIDBUFFERSIZE, PU_MUSIC, NULL); // FIXME: catch return value

  musicStarted = true;
	
  CONS_Printf(" [%s][%d] I_InitMusic(): var - nomusic      = %s\n"
         "                         var - soundStarted = %s\n"
         "                         var - musicStarted = %s\n"
         "                         aud - frequenz(hz) = %d\n"
         "                         aud - samples/slice= %d\n"       
                                  ,__FILE__,__LINE__,
                                  (nomusic     ==true)?"Ja":"Nein",
                                  (soundStarted==true)?"Ja":"Nein",
                                  (musicStarted==true)?"Ja":"Nein",
                                  audio.freq, audio.samples);
  
  CONS_Printf(" Music initialized.\n");    
#endif
}



// finish sound and music
void I_ShutdownSound()
{
  if (!soundStarted)
    return;
    
  CONS_Printf("I_ShutdownSound: ");
    
#ifdef NO_MIXER
  SDL_CloseAudio();
#else
  Mix_CloseAudio();
#endif

  CONS_Printf("shut down\n");
  soundStarted = false;

  if (!musicStarted)
    return;

  Z_Free(mus2mid_buffer);
  CONS_Printf("I_ShutdownMusic: shut down\n");
  musicStarted = false;  
   
}



/// starts playing the "registered" music
void I_PlaySong(int handle, int looping)
{
#ifndef NO_MIXER
  if (nomusic)
    return;

  if (music.mus)
    {
      Mix_FadeInMusic(music.mus, looping ? -1 : 0, 500);
    }
#endif
}

void I_PauseSong(int handle)
{
  if (nomusic)
    return;

  I_StopSong(handle);
}

void I_ResumeSong(int handle)
{
  if (nomusic)
    return;

  I_PlaySong(handle, true);
}

void I_StopSong(int handle)
{
#ifndef NO_MIXER
  if (nomusic)
    return;

  Mix_FadeOutMusic(500);
#endif
}


void I_UnRegisterSong(int handle)
{
#ifndef NO_MIXER
  if (nomusic)
    return;

  if (music.mus)
    {
      Mix_FreeMusic(music.mus);
      music.mus = NULL;
      music.rwop = NULL;
    }
#endif
}


int I_RegisterSong(void* data, int len)
{
#ifndef NO_MIXER
  if (nomusic)
    return 0;

  if (music.mus)
    {
      I_Error("Two registered pieces of music simultaneously!\n");
    }

  byte *bdata = static_cast<byte *>(data);

  if (memcmp(data, MUSMAGIC, 4) == 0)
    {
      int err;
      Uint32 midlength;
      // convert mus to mid in memory with a wonderful function
      // thanks to S.Bacquet for the source of qmus2mid
      if ((err = qmus2mid(bdata, mus2mid_buffer, 89, 64, 0, len, MIDBUFFERSIZE, &midlength)) != 0)
	{
	  CONS_Printf("Cannot convert MUS to MIDI: error %d.\n", err);
	  return 0;
	}

      music.rwop = SDL_RWFromConstMem(mus2mid_buffer, midlength);
    }
  else
    {
      // MIDI, MP3, Ogg Vorbis, various module formats
      music.rwop = SDL_RWFromConstMem(data, len);
    }
  
  // SDL_mixer automatically frees the rwop when the music is stopped.
  music.mus = Mix_LoadMUS_RW(music.rwop);
  if (!music.mus)
    {
      CONS_Printf("Couldn't load music lump: %s\n", Mix_GetError());
      music.rwop = NULL;
    }

#endif

  return 0;
}

void I_SetMusicVolume(int volume)
{
#ifndef NO_MIXER
  if (nomusic)
    return;

  // acceptable volume range : 0-128
  Mix_VolumeMusic(volume*2);
#endif
}
