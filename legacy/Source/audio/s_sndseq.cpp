// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: s_sndseq.cpp 306 2006-02-11 16:26:27Z jussip $
//
// Copyright (C) 2003-2004 by DooM Legacy Team.
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
/// \brief Sound sequences. Incorporates Hexen sequences, Heretic ambient sequences
/// and Legacy-specific sequences.

#include <vector>
#include <string.h>

#include "m_random.h"
#include "parser.h"

#include "g_map.h"
#include "s_sound.h"
#include "sounds.h"
#include "w_wad.h"
#include "z_zone.h"

using namespace std;



void sndseq_t::clear()
{
  number  = 0;
  stopsound = 0;
  name.clear();
  data.clear();
}


map<int, sndseq_t*> SoundSeqs; // this is where the sequence definitions are stored
typedef map<int, sndseq_t*>::iterator sndseq_iter_t;


//===========================================
// This sucks, but is necessary for Hexen
//===========================================

struct hexen_seq_t
{
  const char *tag;
  int seq[3]; // max 3 are needed
};

// flexible sequence mappings
#define PLAT_S(x) (x + SEQ_PLAT)
#define DOOR_S(x) (x + SEQ_DOOR)
#define ENV_S(x) (x + SEQ_ENV)
const int NUMSEQ = 14;
hexen_seq_t HexenSeqs[NUMSEQ] =
{
  { "Platform", {PLAT_S(0), PLAT_S(1), PLAT_S(3)}}, // 'stone', 'heavy' and 'creak' platforms are all alike
  { "PlatformMetal", {PLAT_S(2), -1, -1}},
  { "Silence", {PLAT_S(4), DOOR_S(4), -1}},
  { "Lava",    {PLAT_S(5), DOOR_S(5), -1}},
  { "Water",   {PLAT_S(6), DOOR_S(6), -1}},
  { "Ice",     {PLAT_S(7), DOOR_S(7), -1}},
  { "Earth",   {PLAT_S(8), DOOR_S(8), -1}},
  { "PlatformMetal2", {PLAT_S(9), -1, -1}},
  { "DoorNormal", {DOOR_S(0), -1, -1}},
  { "DoorHeavy",  {DOOR_S(1), -1, -1}},
  { "DoorMetal",  {DOOR_S(2), -1, -1}},
  { "DoorCreak",  {DOOR_S(3), -1, -1}},
  { "DoorMetal2", {DOOR_S(9), -1, -1}},
  { "Wind", {ENV_S(0), -1, -1}}
};



//===========================================
//  SNDSEQ parser
//===========================================

// both an index to SOUNDSEQ_cmds and the actual token used in sequence
enum soundseq_cmd_t
{
  SSEQ_NONE,
  SSEQ_PLAY,
  SSEQ_PLAYUNTILDONE,
  SSEQ_WAITUNTILDONE = SSEQ_PLAYUNTILDONE, // used by PLAYUNTILDONE
  SSEQ_PLAYTIME,
  SSEQ_PLAYREPEAT,
  SSEQ_DELAY,
  SSEQ_DELAYRAND,
  SSEQ_VOLUME,
  SSEQ_STOPSOUND,
  SSEQ_END,

  // Legacy additions
  SSEQ_VOLUMERAND,
  SSEQ_CHVOL,

  /*
  SSEQ_ATTENUATION,
  SSEQ_PLAYLOOP,
  SSEQ_DOOR,
  SSEQ_PLATFORM,
  SSEQ_ENVIRONMENT,
  SSEQ_NOSTOPCUTOFF,
  */
  SSEQ_NUMCMDS
};

static const char *SOUNDSEQ_cmds[SSEQ_NUMCMDS + 1] =
{
  "xxx",
  "play",
  "playuntildone",
  "playtime",
  "playrepeat",
  "delay",
  "delayrand",
  "volume",
  "stopsound",
  "end",

  "volumerand", // like delayrand but for volume
  "chvol",      // additive volume adjustment
  /* // zdoom additions
    "attenuation",
    "playloop",
    "door",
    "platform",
    "environment",
    "nostopcutoff",
  */
  NULL
};


// reads the SNDINFO lump
int S_Read_SNDSEQ(int lump)
{
  Parser p;
  sndseq_t temp;
  sndseq_t *ss;

  temp.number = -1;
  int i, n, hseq = -1;

  if (!p.Open(lump))
    return -1;

  CONS_Printf("Reading SNDSEQ...\n");

  p.RemoveComments(';');
  while (p.NewLine())
    {
      char line[33];

      n = p.GetString(line, 32);
      if (n > 0)
	{
	  if (line[0] == ':')
	    {
	      if (temp.number >= 0)
		CONS_Printf("Nested sequence in sequence %d.\n", temp.number);
	      else
		{
		  // new sequence
		  temp.clear();
		  temp.name = &line[1]; // omitting the colon

		  if (p.MustGetInt(&i))
		    temp.number = i; // Legacy format, :SeqName <number>
		  else
		    {
		      // old Hexen style kludge
		      for (i=0; i<NUMSEQ; i++)
			if (!strcasecmp(HexenSeqs[i].tag, line+1))
			  break;
		      if (i == NUMSEQ)
			{
			  CONS_Printf("No sequence number given for '%s'.\n", line);
			  temp.number = -2;
			  continue;
			}
		      else
			{
			  temp.number = HexenSeqs[i].seq[0];
			  hseq = i;
			}
		    }
		  //CONS_Printf(" starting seq %d, '%s'\n", temp.number, temp.name.c_str());
		}
	    }
	  else if (temp.number >= 0)
	    switch (P_MatchString(line, SOUNDSEQ_cmds))
	      {
	      case SSEQ_PLAY:
		if (p.GetString(line, 32))
		  {
		    temp.data.push_back(SSEQ_PLAY);
		    temp.data.push_back(S_GetSoundID(line));
		  }
		break;

	      case SSEQ_PLAYUNTILDONE:
		if (p.GetString(line, 32))
		  {
		    temp.data.push_back(SSEQ_PLAY);
		    temp.data.push_back(S_GetSoundID(line));
		    temp.data.push_back(SSEQ_WAITUNTILDONE);
		  }
		break;

	      case SSEQ_PLAYTIME:
		if (p.GetString(line, 32))
		  {
		    temp.data.push_back(SSEQ_PLAY);
		    temp.data.push_back(S_GetSoundID(line));
		    temp.data.push_back(SSEQ_DELAY);
		    temp.data.push_back(p.GetInt());
		  }

	      case SSEQ_PLAYREPEAT:
		if (p.GetString(line, 32))
		  {
		    temp.data.push_back(SSEQ_PLAYREPEAT);
		    temp.data.push_back(S_GetSoundID(line));
		  }
		break;

	      case SSEQ_DELAY:
		temp.data.push_back(SSEQ_DELAY);
		temp.data.push_back(p.GetInt());
		break;

	      case SSEQ_DELAYRAND:
		temp.data.push_back(SSEQ_DELAYRAND);
		temp.data.push_back(p.GetInt());
		temp.data.push_back(p.GetInt());
		break;

	      case SSEQ_VOLUME:
		temp.data.push_back(SSEQ_VOLUME);
		temp.data.push_back(p.GetInt());
		break;

	      case SSEQ_STOPSOUND:
		if (p.GetString(line, 32))
		  {
		    temp.data.push_back(SSEQ_STOPSOUND);
		    temp.stopsound = S_GetSoundID(line);
		  }
		break;

	      case SSEQ_END:
		temp.data.push_back(SSEQ_END);

		// create and store the sequence
		if (SoundSeqs.count(temp.number)) // already there
		  {
		    CONS_Printf("Warning: Sequence %d defined more than once!\n", temp.number);
		    delete SoundSeqs[temp.number]; // later one takes precedence
		  }

		ss = new sndseq_t(temp); // make a copy
		SoundSeqs[ss->number] = ss; // insert into the map

		//CONS_Printf(" seq %d done\n", ss->number);

		if (hseq >= 0)
		  { // other half of the Hexen kludge:
		    // some sequences need to be copied
		    for (n=1; n<3; n++)
		      if (HexenSeqs[hseq].seq[n] != -1)
			{
			  ss = new sndseq_t(temp);
			  ss->number = HexenSeqs[hseq].seq[n];
			  SoundSeqs[ss->number] = ss;
			  //CONS_Printf(" seq %d done\n", ss->number);
			}
		    
		    hseq = -1;
		  }

		temp.number = -1;
		break;

	      case SSEQ_VOLUMERAND:
		temp.data.push_back(SSEQ_VOLUMERAND);
		temp.data.push_back(p.GetInt());
		temp.data.push_back(p.GetInt());
		break;

	      case SSEQ_CHVOL:
		temp.data.push_back(SSEQ_CHVOL);
		temp.data.push_back(p.GetInt());
		break;

	      default:
		CONS_Printf("Unknown command '%s'.\n", line);
		break;
	      }
	}
    }

  CONS_Printf(" %d sequences found.\n", SoundSeqs.size());
  return SoundSeqs.size();
}



//===========================================
//  Active sequences
//===========================================


/// dynamic data (could also be a Thinker... maybe not)
class ActiveSndSeq
{
  friend class Map;

  const sndseq_t *seq;
  unsigned  ip;
  int       delay;
  float     volume;
  int       channel;
  bool      isactor;
  union
  { // the sound origin. if NULL, the sound is ambient
    mappoint_t *mpt;
    Actor      *act;
  };

public:
  ActiveSndSeq(sndseq_t *s, Actor *orig);
  ActiveSndSeq(sndseq_t *s, mappoint_t *orig);

  bool Update();
  void StartSnd(int snd);
  void Stop(bool quiet);

  void *operator new(size_t size) { return Z_Malloc(size, PU_LEVSPEC, NULL); };
  void  operator delete(void *mem) { Z_Free(mem); };
};


ActiveSndSeq::ActiveSndSeq(sndseq_t *s, Actor *orig)
{
  channel = -1;
  seq = s;
  ip = 0;
  delay = 0;
  volume = 1.0f;
  act = orig;
  isactor = true;
}


ActiveSndSeq::ActiveSndSeq(sndseq_t *s, mappoint_t *orig)
{
  channel = -1;
  seq = s;
  ip = 0;
  delay = 0;
  volume = 1.0f;
  mpt = orig;
  isactor = false;
}


void ActiveSndSeq::StartSnd(int snd)
{
  if (act)
    {
      if (isactor)
	channel = S_StartSound(act, snd, volume);
      else
	channel = S_StartSound(mpt, snd, volume);
    }
  else
    channel = S_StartAmbSound(NULL, snd, volume);
}


void ActiveSndSeq::Stop(bool quiet)
{
  S.StopChannel(channel);

  if (!quiet && seq->stopsound)
    StartSnd(seq->stopsound);
}


bool ActiveSndSeq::Update()
{
  if (delay > 0)
    {
      delay--;
      return false;
    }

  if (ip >= seq->data.size())
    {
      CONS_Printf("Sound sequence overrun!\n");
      return true;
    }

  bool playing = S.ChannelPlaying(channel);

  switch (seq->data[ip])
    {
    case SSEQ_PLAY:
      if (!playing)
	StartSnd(seq->data[ip + 1]);
      ip += 2;
      break;

    case SSEQ_WAITUNTILDONE:
      if (!playing)
	ip++;
      break;

    case SSEQ_PLAYREPEAT:
      if (!playing)
	StartSnd(seq->data[ip + 1]); // TODO should make looping sound
      break;

    case SSEQ_DELAY:
      delay = seq->data[ip + 1];
      ip += 2;
      break;

    case SSEQ_DELAYRAND:
      delay = seq->data[ip + 1] + P_Random() % (seq->data[ip + 2] - seq->data[ip + 1] + 1);
      ip += 3;
      break;

    case SSEQ_VOLUME:
      // volume is in range 0..100
      volume = float(seq->data[ip + 1])/100;
      ip += 2;
      break;

    case SSEQ_STOPSOUND:
      // Wait until something else stops the sequence
      break;

    case SSEQ_END:
      S.StopChannel(channel);
      return true;

    case SSEQ_VOLUMERAND:
      volume = float(seq->data[ip + 1] + P_Random() % (seq->data[ip + 2] - seq->data[ip + 1] + 1))/100;
      ip += 3;
      break;

    case SSEQ_CHVOL:
      volume += float(seq->data[ip + 1])/100;
      ip += 2;
      break;

    default:	
      break;
    }

  return false;
}





//===========================================
//  Map soundsequence methods
//===========================================

bool Map::SN_StartSequence(Actor *a, int s)
{
  sndseq_iter_t i = SoundSeqs.find(s);

  if (i == SoundSeqs.end())
    return false;

  SN_StopSequence(a); // Stop any previous sequence
  ActiveSndSeq *temp = new ActiveSndSeq((*i).second, a);
  ActiveSeqs.push_back(temp);
  return true;
}

bool Map::SN_StartSequence(mappoint_t *m, int s)
{
  sndseq_iter_t i = SoundSeqs.find(s);

  if (i == SoundSeqs.end())
    return false;

  SN_StopSequence(m, true); // Stop any previous sequence
  ActiveSndSeq *temp = new ActiveSndSeq((*i).second, m);
  ActiveSeqs.push_back(temp);

  return true;
}


bool Map::SN_StartSequenceName(mappoint_t *m, const char *n)
{
  sndseq_iter_t i;
  for (i = SoundSeqs.begin(); i != SoundSeqs.end(); i++)
    {
      if (!(*i).second->name.compare(n))
	{
	  SN_StartSequence(m, (*i).first);
	  return true;
	}
    }
  return false;
}


bool Map::SN_StopSequence(void *origin, bool quiet)
{
  list<ActiveSndSeq*>::iterator i;

  for (i = ActiveSeqs.begin(); i != ActiveSeqs.end(); i++)
    {
      if ((*i)->act == origin)
	{
	  (*i)->Stop(quiet);
	  delete *i;
	  ActiveSeqs.erase(i);
	  return true;
	}
    }
  return false;
}


void Map::UpdateSoundSequences()
{
  list<ActiveSndSeq*>::iterator i, j;

  for (i = ActiveSeqs.begin(); i != ActiveSeqs.end(); )
    {
      if ((*i)->Update())
	{
	  // finished
	  j = i++; // because erasing invalidates it
	  delete *j;
	  ActiveSeqs.erase(j);
	}
      else
	i++;
    }

  // ambient sequence
  int n = AmbientSeqs.size();

  if (n == 0)
    return;

  if (ActiveAmbientSeq)
    {
      if (ActiveAmbientSeq->Update())
	{
	  // ambient sequence finished, pick a new one next tick
	  delete ActiveAmbientSeq;
	  ActiveAmbientSeq = NULL;
	}
    }
  else
    {
      n = P_Random() % n;
      int s = AmbientSeqs[n] + 1000;
      sndseq_iter_t i = SoundSeqs.find(s);
      if (i == SoundSeqs.end())
	{
	  CONS_Printf("WARNING: Ambient sequence %d not defined!\n", s - 1000);
	  AmbientSeqs.erase(AmbientSeqs.begin() + n);
	  return;
	}

      ActiveAmbientSeq = new ActiveSndSeq((*i).second, (Actor *)NULL);
    }
}


