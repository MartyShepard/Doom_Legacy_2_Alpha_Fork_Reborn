// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: s_sound.cpp 504 2007-12-16 17:58:26Z smite-meister $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2006 by DooM Legacy Team.
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
/// \brief Sound system and cache

#include <stdlib.h> // rand
#include <math.h>

#include "doomdef.h"
#include "doomdata.h"
#include "command.h"
#include "cvars.h"

#include "g_actor.h"
#include "g_game.h"
#include "g_pawn.h"
#include "g_player.h"

#include "m_argv.h"
#include "r_defs.h"

#include "i_sound.h"
#include "s_sound.h"
#include "sounds.h"
#include "tables.h"

#include "w_wad.h"
#include "z_zone.h"
#include "z_cache.h"

#include "m_swap.h"

CV_PossibleValue_t soundvolume_cons_t[]={{0,"MIN"},{31,"MAX"},{0,NULL}};

consvar_t cv_soundvolume = {"soundvolume","15",CV_SAVE,soundvolume_cons_t};
consvar_t cv_musicvolume = {"musicvolume","15",CV_SAVE,soundvolume_cons_t};
consvar_t cd_volume = {"cd_volume","31",CV_SAVE,soundvolume_cons_t};
consvar_t cv_numChannels = {"snd_channels","16",CV_SAVE, CV_Unsigned};
consvar_t cv_stereoreverse = {"stereoreverse","0",CV_SAVE ,CV_OnOff};
consvar_t cv_surround = {"surround", "0", CV_SAVE, CV_OnOff};
consvar_t cv_precachesound = {"precachesound","0",CV_SAVE ,CV_OnOff};


bool  nomusic = false, nosound = false;

SoundSystem S;


/// \brief RIFF/WAVE file header
struct WAV_t
{
  char   riff_magic[4];  ///< "RIFF"
  Uint32 riff_size;      ///< filesize-8
  char   wave_magic[4];  ///< "WAVE"

  // fmt subchunk
  struct
  {
    char   magic[4];      ///< "fmt "
    Uint32 size;          ///< must be 16 
    Uint16 audioformat;   ///< 1 means PCM
    Uint16 numchannels;   ///< mono or stereo?
    Uint32 samplerate;    ///< in Hz
    Uint32 byterate;      ///< == samplerate*numchannels*bitspersample/8
    Uint16 blockalign;    ///< == numchannels*bitspersample/8
    Uint16 bitspersample; ///< 8 or 16
  } __attribute__((packed)) fmt;

  // data subchunk
  struct
  {
    char   magic[4]; ///< "data"
    Uint32 size;     ///< == numsamples*numchannels*bitspersample/8
    byte   data[0];  ///< actual data begins here
  } __attribute__((packed)) data;
} __attribute__((packed));



//===========================================================
//  Sound cache
//===========================================================

sounditem_t::sounditem_t(const char *n)
  : cacheitem_t(n)
{
}

sounditem_t::~sounditem_t()
{
  if (data)
    Z_Free(data);
}



class soundcache_t : public cache_t<sounditem_t>
{
protected:
  // We assume that the sound is in Doom or WAV sound format.
  virtual sounditem_t *Load(const char *p)
  {
  int lump = fc.FindNumForName(p, false);
  if (lump == -1)
    return NULL;

  byte *raw = static_cast<byte *>(fc.CacheLumpNum(lump, PU_SOUND));
  unsigned size = fc.LumpLength(lump);

  sounditem_t *t = new sounditem_t(p);
  t->lumpnum = lump;
  t->data = raw;
  
  WAV_t *h = reinterpret_cast<WAV_t *>(raw);
  if (!strncmp(h->riff_magic, "RIFF", 4))
    {
      // RIFF file
      if (strncmp(h->wave_magic, "WAVE", 4))
	I_Error("RIFF lump '%s' has unknown type!\n", p);

      if (LONG(h->riff_size)+8 != size
	  || strncmp(h->fmt.magic, "fmt ", 4)
	  || strncmp(h->data.magic, "data", 4))
	I_Error("WAVE lump '%s' is corrupted!\n", p);

      t->depth = SHORT(h->fmt.bitspersample) / 8;

      if (LONG(h->fmt.size) != 16
	  || SHORT(h->fmt.audioformat) != 1  // PCM only
	  || SHORT(h->fmt.numchannels) != 1 // TODO support stereo?
	  || (t->depth != 1 && t->depth != 2))
	I_Error("WAVE lump '%s' is in an unsupported format!\n", p);

      t->rate = LONG(h->fmt.samplerate);
      t->length = LONG(h->data.size);
      if (t->length != size-44)
	I_Error("WAVE lump '%s' has wrong length!\n", p);
      t->sdata = &h->data.data;
    }
  else
    {
      // assume Doom SFX
      doomsfx_t *ds = reinterpret_cast<doomsfx_t *>(raw);
      // endianness conversion
      ds->magic   = SHORT(ds->magic);
      ds->rate    = SHORT(ds->rate);
      ds->samples = SHORT(ds->samples);
      ds->zero    = SHORT(ds->zero);

      //CONS_Printf(" Sound '%s', s = %d\n", p, ds->samples);
      //CONS_Printf("m = %d, r = %d, s = %d, z = %d, length = %d\n", ds->magic, ds->rate, ds->samples, ds->zero, size);

      t->rate = ds->rate; // the only piece of header info we use
      t->depth = 1;
      t->length = size - 8;  // 8 byte header
      t->sdata = &ds->data;
    }
  return t;
  }
};


/// The sound cache.
static soundcache_t sc;



//===========================================================
//  Utilities
//===========================================================


void S_PrecacheSounds()
{
  // Initialize external data (all sounds) at start, keep static.
  CONS_Printf("Precaching sounds... ");
  soundID_iter_t i;
  for (i = SoundID.begin(); i != SoundID.end(); i++)
    sc.Get(i->second->lumpname); // one extra reference => never released

  CONS_Printf("done.\n");
}


//===========================================================
//  Sound system
//===========================================================

void soundsource_t::Update()
{
  if (isactor)
    {
      pos = act->pos;
      vel = act->vel;
    }
  else
    {
      pos.Set(mpt->x, mpt->y, mpt->z);
    }
}

// returns a value in the range [0.0, 1.0]
float soundsource_t::ObservedVolume(Actor *listener)
{
  // when to clip out sounds
  // Does not fit the large outdoor areas.
  const float S_CLIPPING_DIST = 1200;

  // Distance to origin when sounds should be maxed out.
  // This should relate to movement clipping resolution
  // (see BLOCKMAP handling).
  // Originally: 200.
  const float S_CLOSE_DIST = 160;

  if (!listener)
    return 0;

  // calculate the distance to sound origin
  //  and clip it if necessary
  vec_t<fixed_t> d = listener->pos - pos;

  // From _GG1_ p.428. Approx. euclidean distance fast.
  /*
  fixed_t adx = abs(listener->x - x);    
  fixed_t ady = abs(listener->y - y);
  fixed_t adz = abs(listener->z - z);
  fixed_t dist = adx + ady - ((adx < ady ? adx : ady)>>1);
  dist = dist + adz - ((dist < adz ? dist : adz)>>1);
  */

  float dist = sqrt(d.Norm2().Float());

  if (dist > S_CLIPPING_DIST)
    return 0.0f;

  if (dist < S_CLOSE_DIST)
    return 1.0f;

  // linear distance attenuation
  return (S_CLIPPING_DIST - dist) / (S_CLIPPING_DIST - S_CLOSE_DIST);
}



// Changes observed volume, stereo-separation and pitch variables for a channel.
// Returns sound volume.
float soundchannel_t::Adjust()
{
  if (!source.act)
    return 0; // just in case

  source.Update();

  // check non-local sounds for distance clipping
  float vmax = -1;
  Actor *l = NULL;

  int n = ViewPlayers.size();
  for (int i=0; i<n; i++)
    {
      Actor *p = ViewPlayers[i]->pawn;    
      float v = source.ObservedVolume(p);	      
      if (v > vmax)
	{
	  l = p;
	  vmax = v;
	}
    }

  if (!l)
    return 0;

  volume = b_volume*vmax;
  

  // angle of source to listener
  angle_t angle = R_PointToAngle2(l->pos.x, l->pos.y, source.pos.x, source.pos.y);

  if (angle > l->yaw)
    angle -= l->yaw;
  else
    angle += (ANGLE_MAX - l->yaw);

  const float SURROUND_SEP = 0.5f; // TODO wtf?
  const float STEREO_SWING = 0.375f; 

  // Produce a surround sound for angle from 105 till 255
  if (cv_surround.value && (angle > (ANG90 + (ANG45/3)) && angle < (ANG270 - (ANG45/3))))
    separation = SURROUND_SEP;
  else
    {
      // stereo separation
      separation = 0.5 - Sin(angle).Float()*STEREO_SWING;

      if (cv_stereoreverse.value)
	separation = 1-separation;
    }


  pitch = b_pitch;
  // TODO Doppler effect (approximate)
  /*
  if (s.origin)
    {
      Actor *orig = s.origin;
      float dx, dy, dz;
      dx = float(l->px - orig->px) / FRACUNIT;
      dy = float(l->py - orig->py) / FRACUNIT;
      dz = float(l->pz - orig->pz) / FRACUNIT;
      float v_os = sqrt(dx*dx + dy*dy + dz*dz);
      float cos_th = 
      //  We need a vector class.
      opitch = ...;
    }
  */

  return volume;
}



SoundSystem::SoundSystem()
{
  mus_playing = NULL;
  mus_paused = false;
  soundvolume = musicvolume = -1; // force hardware update
}


// Initializes sound hardware, sets up channels.
// Sound and music volume is set before first update.
// allocates channel buffer, sets S_sfx lookup.
void SoundSystem::Startup()
{
  CONS_Printf("Initializing the sound module...\n");

  cv_soundvolume.Reg();
  cv_musicvolume.Reg();
  cd_volume.Reg();
  cv_numChannels.Reg();
  cv_stereoreverse.Reg();
  cv_surround.Reg();
  cv_precachesound.Reg();

  nosound = M_CheckParm("-nosound");
  nomusic = M_CheckParm("-nomusic");

  if (M_CheckParm("-precachesound"))
    cv_precachesound.Set("1");

  I_StartupSound();
  I_InitMusic();

  // Initialize CD-Audio
  if (!M_CheckParm("-nocd"))
    I_InitCD();

  ResetChannels(16);
  sc.SetDefaultItem("DEF_SND"); // default sound

  nextcleanup = game.tic + 35*100;
}


void SoundSystem::SetMusicVolume(int vol)
{
  if (vol < 0 || vol > 31)
    {
      CONS_Printf("Music volume should be between 0-31\n");
      if (vol < 0)
	vol = 0;
      else
	vol = 31;
    }

  musicvolume = vol;
  I_SetMusicVolume(vol);
}


void SoundSystem::SetSoundVolume(int vol)
{
  if (vol < 0 || vol > 31)
    {
      CONS_Printf("Sound volume should be between 0-31\n");
      if (vol < 0)
	vol = 0;
      else
	vol = 31;
    }

  soundvolume = vol;
#ifdef HW3SOUND
  hws_mode == HWS_DEFAULT_MODE ? I_SetSfxVolume(vol) : HW3S_SetSfxVolume(vol);
#else
  // now hardware volume
  I_SetSfxVolume(vol);
#endif
}



//=================================================================
// Music

musicinfo_t::musicinfo_t()
{
  lumpnum = 0;
  length = 0;
  data = NULL;
  handle = 0;
}

static musicinfo_t mu;


// Stop and resume music, during game PAUSE.
void SoundSystem::PauseMusic()
{
  if (game.dedicated || nomusic)
    return;

  if (mus_playing && !mus_paused)
    {
      I_PauseSong(mus_playing->handle);
      mus_paused = true;
    }

  // pause cd music
  I_PauseCD();
}


void SoundSystem::ResumeMusic()
{
  if (game.dedicated || nomusic)
    return;

  if (mus_playing && mus_paused)
    {
      I_ResumeSong(mus_playing->handle);
      mus_paused = false;
    }

  // resume cd music
  I_ResumeCD();
}


// caches music lump "name", starts playing it
// returns true if succesful
bool SoundSystem::StartMusic(const char *name, bool loop)
{
  if (mus_playing && mus_playing->name == name)
    return true;

  if (name[0] == '-')
    {
      StopMusic();
      return true;
    }

  CONS_Printf("StartMusic: %s\n", name);
  // so music needs to be changed.

  // shutdown old music
  // TODO: add several music channels, crossfade;)
  StopMusic();

  // TODO: no 2nd level music cache, just one music plays at a time.
  // here we would check if the music 'name' already is in the 2nd level cache.
  // if not, cache it:
  int musiclump = fc.FindNumForName(name);

  if (musiclump < 0)
    {
      CONS_Printf("Music lump '%s' not found!\n", name);
      return false;
    }

  musicinfo_t *m = &mu;
  m->name = name;
  m->lumpnum = musiclump;
  m->data = fc.CacheLumpNum(musiclump, PU_MUSIC);
  m->length = fc.LumpLength(musiclump);

  mus_playing = m;

  if (game.dedicated || nomusic)
    return true;

#if defined(__APPLE_CC__) || defined(__MACOS__)
  // FIXME make Mac interface similar to the other interfaces
  // m->handle = I_RegisterSong(music_num);
  // FIXME /Mac OS X/ sound part of Legacy should be taken from version 1.42
  // but it will take some time to get and rewrite the old code
  // now playing music is disabled
#warning Later!
  return false;
#else
  m->handle = I_RegisterSong(m->data, m->length);
#endif

  // play it
  I_PlaySong(m->handle, loop);
  mus_paused = false;

  return true;
}


void SoundSystem::StopMusic()
{
  if (game.dedicated || nomusic)
    return;

  if (mus_playing)
    {
      if (mus_paused)
	I_ResumeSong(mus_playing->handle);

      I_StopSong(mus_playing->handle);
      I_UnRegisterSong(mus_playing->handle);
      Z_Free(mus_playing->data);
      mus_playing->data = NULL;
      mus_playing = NULL;
    }
}


//=================================================================
// Sound effects


// change number of sound channels
void SoundSystem::ResetChannels(int tot)
{
  int i, n = channels.size();
  for (i=tot; i<n; i++)
    StopChannel(i);

  channels.resize(tot);

  for (i=n; i<tot; i++)
    {
      channels[i].si = NULL;
      channels[i].playing = false;
    }
}


// Tries to make a sound channel available.
int SoundSystem::GetChannel(int pri)
{
  // channel number to use
  int i, n = channels.size();

  int min = 1000, chan = -1;

  // Find a free channel
  for (i=0; i<n; i++)
    {
      if (channels[i].si == NULL)
	break;
      else if (channels[i].priority < min)
	{
	  min = channels[i].priority;
	  chan = i;
	}
    }

  // None available?
  if (i == n)
    {
      // kick out minimum priority sound?
      if (pri > min)
	{
	  StopChannel(chan);
	  i = chan;
        }
      else
        {
	  // FUCK!  No lower priority.  Sorry, Charlie.
	  return -1;
        }
    }

  // channel i is chosen
  return i;
}


// is a channel still playing?
bool SoundSystem::ChannelPlaying(unsigned cnum)
{
  if (cnum >= channels.size())
    return false;

  return channels[cnum].playing;
}


void soundchannel_t::Init(sfxinfo_t *s, float vol, bool rand_pitch)
{
  priority = s->priority;

  // 64 pitch units = 1 octave
  // TODO variable pitch shifts as per sound?
  if (rand_pitch)
    {
      b_pitch = s->pitch + 16 - (rand() & 31);

      if (b_pitch < 0)
	b_pitch = 0;
      if (b_pitch > 255)
	b_pitch = 255;
    }
  else
    b_pitch = s->pitch;
  pitch = b_pitch;

  if (vol > 1 || vol < 0)
    vol = 1;

  b_volume = volume = vol;

  si = sc.Get(s->lumpname);
}

// Starts a normal mono(stereo) sound
int SoundSystem::StartAmbSound(sfxinfo_t *s, float volume, float sep)
{
  if (nosound)
    return -1;

  // try to find a channel
  int i = GetChannel(s->priority);
  if (i == -1)
    return -1;

  soundchannel_t *c = &channels[i];
  c->source.isactor = false;
  c->source.act = NULL;

  // copy source data
  c->Init(s, volume, false);

  c->separation = sep;

  I_StartSound(c);
  return i;
}


// starts a locational sound
int SoundSystem::Start3DSound(sfxinfo_t *s, soundsource_t *source, float volume)
{
  if (nosound)
    return -1;

  source->Update();

  Actor *listener = NULL;
  float vmax = -1;

  int n = ViewPlayers.size();
  for (int i=0; i<n; i++)
    {
      Actor *temp = ViewPlayers[i]->pawn;    
      float v = source->ObservedVolume(temp);

      if (v > vmax)
	{
	  listener = temp;
	  vmax = v;
	}
    }

  if (vmax <= 0.0f)
    return -1;

  // kill old sound if any (max. one sound per source)
  Stop3DSound(source->act); // pointers are just pointers

  // try to find a channel
  int i = GetChannel(s->priority);
  if (i == -1)
    return -1;

  soundchannel_t *c = &channels[i];
  c->source = *source;

  c->Init(s, volume, true);

  // Set volume, pitch and separation
  c->Adjust();

  I_StartSound(c);
  //CONS_Printf("3D sound started, %d, %f\n", c->ovol, v1);
  return i;
}


// Kills all positional sounds
void SoundSystem::Stop3DSounds()
{

#ifdef HW3SOUND
  if (hws_mode != HWS_DEFAULT_MODE)
    {
      HW3S_StopSounds();
      return;
    }
#endif

  // kill all playing sounds at start of level
  //  (trust me - a good idea)
  int cnum, n = channels.size();

  for (cnum = 0; cnum < n; cnum++)
    if (channels[cnum].source.act && channels[cnum].si)
      StopChannel(cnum);
}


// Stops the positional sound coming from 'origin'
void SoundSystem::Stop3DSound(void *origin)
{
  // SoM: Sounds without origin can have multiple sources, they shouldn't
  // be stopped by new sounds.
  if (!origin)
    return;

#ifdef HW3SOUND
  if (hws_mode != HWS_DEFAULT_MODE)
    {
      HW3S_StopSound(origin);
      return;
    }
#endif

  int cnum, n = channels.size();
  for (cnum=0; cnum<n; cnum++)
    {
      if (channels[cnum].si && channels[cnum].source.act == origin)
	{
	  StopChannel(cnum);
	  break; // what about several sounds from one origin?
	}
    }
}


// shuts down a sound channel
void SoundSystem::StopChannel(unsigned cnum)
{
  if (cnum >= channels.size())
    return;

  soundchannel_t *c = &channels[cnum];

  if (c->si)
    {
      I_StopSound(c);

      // degrade reference count of sound data
      c->si->Release();
      c->si = NULL;
    }
  c->playing = false;
}



// Is the origin playing a sound?
bool SoundSystem::PlayingSound(Actor *origin)
{
  if (!origin)
    return false;

  int n = channels.size();
  for (int cnum=0; cnum<n; cnum++)
    {
      if (channels[cnum].si && channels[cnum].source.act == origin)
	return true;
    }
  return false;
}


// Updates music & sounds, called once a gametic
void SoundSystem::UpdateSounds()
{
  // check if relevant consvars have been changed
  if (soundvolume != cv_soundvolume.value)
    SetSoundVolume(cv_soundvolume.value);
  if (musicvolume != cv_musicvolume.value)
    SetMusicVolume(cv_musicvolume.value);

  if ((int)channels.size() != cv_numChannels.value)
    {
#ifdef HW3SOUND
      if (hws_mode != HWS_DEFAULT_MODE)
	HW3S_SetSourcesNum();
#endif
      S.ResetChannels(cv_numChannels.value);
    }

#ifdef HW3SOUND
  if (hws_mode != HWS_DEFAULT_MODE)
    {
      HW3S_UpdateSources();
      return;
    }
#endif

  // Go through cache,
  // clean up unused data.
  if (game.tic >= nextcleanup)
    {
      CONS_Printf("Sound cache cleanup...\n");

      // FIXME decide if regular cleanups are even needed. i think not.
      // rather we could do a cleanup when memory is low based on usefulness
      int i = 0; //sc.Cleanup();
      if (i > 0)
	CONS_Printf ("Flushed %d sounds\n", i);
      //sc.Inventory();
      nextcleanup = game.tic + 35*500;
    }


  // sound channels
  int nchan = channels.size();
  for (int cnum = 0; cnum < nchan; cnum++)
    {
      soundchannel_t *c = &channels[cnum];

      if (c->playing)
	{
	  if (!c->source.act)
	    continue; // unchanging

	  if (c->Adjust() <= 0)
	    StopChannel(cnum);
	}
      else
	// if channel is allocated but sound has stopped, free it
	StopChannel(cnum);
    }
}

