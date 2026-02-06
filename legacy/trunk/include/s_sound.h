// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: s_sound.h 361 2006-08-03 22:39:20Z smite-meister $
//
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
/// \brief The not so system specific sound interface.

#ifndef s_sound_h
#define s_sound_h 1

#include <vector>
#include <string>
#include <map>
#include "vect.h"
#include "m_fixed.h"
#include "z_cache.h"

using namespace std;


/// \brief A playing piece of music
struct musicinfo_t
{
  string name;      ///< music lumpname
  int    lumpnum;   ///< lump number of music
  int    length;    ///< lump length in bytes
  void  *data;      ///< music data
  int    handle;    ///< music handle once registered

public:
  musicinfo_t();
};



/// \brief A sound effect definition, generated from the SNDINFO lump.
class sfxinfo_t
{
public:
#define S_TAGLEN 32 // change with care
  char  tag[S_TAGLEN + 1]; ///< SNDINFO tag (+ \0)
  int   number;       ///< internal sound number
  char  lumpname[9];  ///< up to 8-character name (+ \0)
  byte  multiplicity; ///< how many instances of the sound can be heard simultaneously. 0 means not limited.
  byte  priority;     ///< bigger is better
  byte  pitch;        ///< 128 means unchanged, 64 pitch units = 1 octave

public:
  sfxinfo_t(const char *t, int n);
};



/// \brief A sound sequence definition, generated from the SNDSEQ lump.
struct sndseq_t
{
  int    number;
  int    stopsound;
  string name;
  vector<int> data; ///< the sequence script

public:
  void clear();
};



/// \brief Sound cache item
class sounditem_t : public cacheitem_t
{
  friend class soundcache_t;
protected:
  int   lumpnum; ///< lump number of data

public:
  sounditem_t(const char *name);
  virtual ~sounditem_t();

  unsigned  rate;   ///< sample rate in Hz
  unsigned  depth;  ///< sample size in bytes (1 or 2)
  void     *data;   ///< unconverted data
  unsigned  length; ///< length in bytes
  void     *sdata;  ///< raw converted sound data (for now assumed always to be U8 depth)
};




/// \brief A 3D sound source.
///
/// We can't just use an Actor* because of
/// sounds originating from mappoint_t's. A bit clumsy but works.
struct soundsource_t
{
  vec_t<fixed_t> pos;
  vec_t<fixed_t> vel;

  bool     isactor; ///< is origin an Actor (or a mappoint_t)?
  union
  { // the sound origin
    struct mappoint_t *mpt;
    class  Actor      *act;
  };

public:
  void  Update();
  float ObservedVolume(Actor *listener);
};



/// \brief Normal "static" mono(stereo) sound channel.
class soundchannel_t
{
public:
  sounditem_t *si;      ///< data source for the sound
  soundsource_t source; ///< origin of sound (or NULL)

  /// sfx base parameters
  int   priority;
  float b_volume;
  int   b_pitch;

  // observed variables (affected by distance and relative velocity)
  float separation; ///< left/right x^2 separation. 0 is totally left, 0.5 is front, 1 is totally right
  float volume;
  int   pitch;

  bool playing;

  //============================================================
  // Stuff below this line is used by the sound interface only.
  //============================================================

  // The channel data pointers, start and end.
  Uint8 *data;
  Uint8 *end;

  int samplesize; ///< 1 or 2 (bytes)

  // pitch and samplerate fused together
  fixed_t step;          // The channel step amount...
  fixed_t stepremainder; // ... and a 0.16 bit remainder of last step.

  float leftvol, rightvol;

  // Hardware left and right channel volume lookup.
  int *leftvol_lookup;
  int *rightvol_lookup;

  // calculate sound parameters using ch
  void CalculateParams();


public:
  void  Init(sfxinfo_t *s, float vol, bool rand_pitch);
  float Adjust();  ///< returns volume heard by present observers
};





/// \brief Sound and music subsystem
///
/// Manages sound channels for both ambient (normal stereo) and 3D sound effects.
/// Also takes care of playing music.
/// There is only one global instance in use, called "S".
class SoundSystem
{
private:
  // sound
  int soundvolume;
public:
  vector<soundchannel_t> channels; ///< the set of channels available

private:
  // music
  int  musicvolume;
  bool mus_paused;            ///< whether music is paused
  musicinfo_t *mus_playing;   ///< music currently being played

  unsigned nextcleanup;       ///< gametic when to do cleanup

  // these are only used internally, user can change them using consvars
  void SetSoundVolume(int volume);
  void SetMusicVolume(int volume);
  void ResetChannels(int num);

  int  GetChannel(int pri);

public:
  SoundSystem();

  /// initialization when game starts
  void Startup(); 

  // --------- sound ---------

  /// normal mono/stereo sound
  int StartAmbSound(sfxinfo_t *s, float volume = 1.0f, float separation = 0.5f);
  /// positional 3D sound
  int Start3DSound(sfxinfo_t *s, soundsource_t *source, float volume = 1.0f);

  void Stop3DSounds();
  void Stop3DSound(void *origin);

  bool PlayingSound(Actor *origin);
  void StopChannel(unsigned cnum);
  bool ChannelPlaying(unsigned cnum);

  // --------- music ---------
  void PauseMusic();
  void ResumeMusic();

  /// Caches music lump "name", starts playing it.
  bool StartMusic(const char *name, bool looping = false);
  /// Stops the music fer sure.
  void StopMusic();
  /// Returns the lumpname of the currently playing piece of music (or NULL if none).
  const char *GetMusic() const { return mus_playing ? mus_playing->name.c_str() : NULL; }


  /// Updates music & sounds.
  void UpdateSounds();
};

extern SoundSystem S;

extern  map<int, sfxinfo_t*> SoundID;  // ID-number => sound
typedef map<int, sfxinfo_t*>::iterator soundID_iter_t;


#endif
