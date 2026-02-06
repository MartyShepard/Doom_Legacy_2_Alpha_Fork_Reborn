// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2006-2007 by DooM Legacy Team.
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
/// \brief ActorInfo class implementation, ActorInfo dictionary.

#include <stdarg.h>
#include "doomdef.h"
#include "dstrings.h"
#include "g_game.h"
#include "g_actor.h"
#include "g_map.h"
#include "g_decorate.h"
#include "info.h"
#include "p_setup.h"
#include "mnemonics.h" // flags are shared with BEX
#include "sounds.h"
#include "w_wad.h"
#include "z_zone.h"

#include "command.h"
#include "cvars.h"


ActorInfoDictionary aid;

extern char orig_sprnames[NUMSPRITES][5]; // Four chars plus terminating NUL.
vector<const char *> spritenames;


bool Read_DECORATE(int lump);

// Label names for standard state sequences
static const char *StandardLabels[] = {"spawn", "see", "melee", "missile", "pain", "death", "xdeath", "crash", "raise"};


ActorInfo::~ActorInfo()
{
  if (devparm)
    CONS_Printf(" deleting %s.\n", classname);

  int n = labels.size();
  for (int i=0; i<n; i++)
    if (labels[i].dyn_states && labels[i].label_states)
      free(labels[i].label_states);
}


// fill fields with default values
ActorInfo::ActorInfo(const string& n, int en)
{
  strncpy(classname, n.c_str(), CLASSNAME_LEN);
  classname[CLASSNAME_LEN] = '\0';

  mobjtype = MT_NONE;

  game = gm_doom2;
  obituary = string("%s got killed by an instance of ") + classname;
  spawn_always = false;

  doomednum = en;
  spawnhealth = 1000;
  reactiontime = 8;
  painchance = 0;
  speed  = 0;
  radius = 20;
  height = 16;
  mass   = 100;
  damage = 0;

  flags = 0;
  flags2 = 0;

  seesound    = sfx_None;
  attacksound = sfx_None;
  painsound   = sfx_None;
  deathsound  = sfx_None;
  activesound = sfx_None;

  spawnstate   = &states[S_NULL];
  seestate     = NULL;
  meleestate   = NULL;
  missilestate = NULL;
  painstate    = NULL;
  deathstate   = NULL;
  xdeathstate  = NULL;
  crashstate   = NULL;
  raisestate   = NULL;

  touchf = NULL;
}


// copy values from mobjinfo_t
ActorInfo::ActorInfo(const mobjinfo_t& m, int gm)
{
  mobjtype = mobjtype_t(&m - mobjinfo);

  if (m.classname)
    strncpy(classname, m.classname, CLASSNAME_LEN);
  else
    sprintf(classname, "class_%d", mobjtype);
  classname[CLASSNAME_LEN] = '\0';

  game = gm;
  obituary = string("%s got killed by an instance of ") + classname;
  spawn_always = false;

  doomednum = m.doomednum;
  spawnhealth = m.spawnhealth;
  reactiontime = m.reactiontime;
  painchance = m.painchance;
  speed = m.speed;
  radius = m.radius;
  height = m.height;
  mass = m.mass;
  damage = m.damage;

  flags = m.flags;
  flags2 = m.flags2;

  seesound    = m.seesound;
  attacksound = m.attacksound;
  painsound   = m.painsound;
  deathsound  = m.deathsound;
  activesound = m.activesound;

  spawnstate   = m.spawnstate;
  seestate     = m.seestate;
  meleestate   = m.meleestate;
  missilestate = m.missilestate;
  painstate    = m.painstate;
  deathstate   = m.deathstate;
  xdeathstate  = m.xdeathstate;
  crashstate   = m.crashstate;
  raisestate   = m.raisestate;

  touchf = m.touchf;

  // create labels
  for (int i=0; i<9; i++)
    {
      if ((&spawnstate)[i]) // HACK, works if struct is packed...
	{
	  statelabel_t temp;
	  strncpy(temp.label, StandardLabels[i], SL_LEN);

	  temp.dyn_states = false; // until states table is got rid of
	  temp.label_states = (&spawnstate)[i]; // HACK
	  temp.num_states = 10; // TODO guesstimate

	  temp.jumplabel[0] = '\0';
	  temp.jumplabelnum = -1;
	  temp.jumpoffset = 0;

	  labels.push_back(temp);
	}
    }
}


// copy constructor, tricky
ActorInfo::ActorInfo(const ActorInfo& a)
{
  *this = a; // first use auto-generated assignment op
  // then fix dynamically allocated stuff
  int n = labels.size();
  for (int i=0; i<n; i++)
    if (labels[i].dyn_states && labels[i].label_states)
      {
	int size = labels[i].num_states * sizeof(state_t);
	labels[i].label_states = static_cast<state_t*>(malloc(size));
	memcpy(labels[i].label_states, a.labels[i].label_states, size);
      }
}


void ActorInfo::SetName(const char *n)
{
  strncpy(classname, n, CLASSNAME_LEN);
  classname[CLASSNAME_LEN] = '\0';
}


void ActorInfo::SetFlag(const char *flagname, bool on)
{
  flag_mnemonic_t *p = BEX_FlagMnemonics;

  for ( ; p->name; p++)
    if (!strcasecmp(flagname, p->name))
      {
	switch (p->flagword)
	  {
	  case 1:
	    on ? (flags |= p->flag) : (flags &= ~p->flag);
	    break;

	  case 2:
	  default:
	    on ? (flags2 |= p->flag) : (flags2 &= ~p->flag);
	    break;
	  }

	break;
      }

  if (!p->name)
    {
      if (!strcasecmp(flagname, "MONSTER"))
	{
	  int f1 = MF_MONSTER | MF_SOLID | MF_SHOOTABLE | MF_COUNTKILL;
	  int f2 = MF2_PUSHWALL | MF2_MCROSS;
	  on ? (flags |= f1, flags2 |= f2) : (flags &= ~f1, flags2 &= ~f2);
	  // TODO missing MONSTER property: CANPASS 
	}
      else if (!strcasecmp(flagname, "PROJECTILE"))
	{
	  int f1 = MF_MISSILE | MF_DROPOFF | MF_NOBLOCKMAP | MF_NOGRAVITY;
	  int f2 = MF2_NOTELEPORT | MF2_IMPACT | MF2_PCROSS;
	  on ? (flags |= f1, flags2 |= f2) : (flags &= ~f1, flags2 &= ~f2);
	}
      else
	Error("Unknown flag '%s'.\n", flagname);
    }
}


ActorInfo::statelabel_t *ActorInfo::FindLabel(const char *l)
{
  int n = labels.size();
  for (int i=0; i<n; i++)
    if (!strncasecmp(l, labels[i].label, SL_LEN))
      {
	// label found
	return &labels[i];
      }

  return NULL; // not found
}


// used during state construction
static string new_label;
static vector<state_t> new_states;


void ActorInfo::AddLabel(const char *label)
{
  new_states.clear(); // get ready to accept new state definitions for this label
  new_label = label;
}


void ActorInfo::AddStates(const char *spr, const char *frames, int tics, const char *func)
{
  actionf_p1 f;
  if (func)
    {
      dactor_mnemonic_t *m;
      for (m = BEX_DActorMnemonics; m->name && strcasecmp(&func[2], m->name); m++); // ignore the initial "A_"
      if (!m->name)
	Error("Unknown action function mnemonic '%s'.\n", func);

      f = m->ptr;
    }
  else
    f = NULL;

  // find sprite num
  int i, n = spritenames.size();
  for (i=0; i<n; i++)
    if (!strcasecmp(spritenames[i], spr))
      break;

  if (i == n) // not found
    {
      // FIXME if not found, create!
      //spritenames.push_back();
      i = SPR_NONE;
    }

  state_t s = {static_cast<spritenum_t>(i), 0, tics, f, NULL};

  for ( ; *frames; frames++)
    {
      char c = toupper(*frames);
      if (c >= 'A' && c <= ']')
	{
	  s.frame = c - 'A';
	  new_states.push_back(s);
	}
    }
}


void ActorInfo::FinishSequence(const char *jumplab, int offset)
{
  statelabel_t *s = FindLabel(new_label.c_str());
  if (s)
    {
      // replace existing sequence
      // discard old states
      if (s->dyn_states && s->label_states)
	free(s->label_states);
    }
  else
    {
      // add a new sequence
      labels.resize(labels.size() + 1);
      s = &labels.back();
      strncpy(s->label, new_label.c_str(), SL_LEN);
    }

  int n = s->num_states = new_states.size();
  s->dyn_states = true;

  // set label_states
  if (n)
    {
      // allocate label_states
      s->label_states = static_cast<state_t*>(malloc(n * sizeof(state_t)));
      state_t *st = s->label_states;

      for (int j=0; j < n; j++, st++)
	{
	  *st = new_states[j];
	  st->nextstate = st + 1;
	}
    }
  else
    {
      s->label_states = NULL;
    }

  // fill in jump data
  if (jumplab)
    {
      strncpy(s->jumplabel, jumplab, SL_LEN); // either a label, or "" denoting S_NULL
    }
  else
    {
      // loop to first state in sequence (equivalent to "goto current_label", but save some effort here)
      s->jumplabel[0] = 1; // HACK
    }

  s->jumplabelnum = -2; // means "needs to be set"
  s->jumpoffset = offset;

  // So far so good, now we only need to fix nextstate pointer for last state in sequence
  // and check that jumplabel is good, but for that we need _all_ the state definitions...
  // This happens in UpdateSequences().
}


bool ActorInfo::UpdateSequences()
{
  int n = labels.size();

  // set jumplabelnum
  for (int i=0; i<n; i++)
    {
      statelabel_t *p, *s = &labels[i];
      if (s->jumplabelnum != -2)
	continue; // already ok (never changes even if new labels are added or old ones redefined!)

      const char *temp = s->jumplabel;

      if (!temp[0]) // empty string denotes S_NULL
	{
	  s->jumplabelnum = -1;
	}
      else if (temp[0] == 1) // HACK, loop
	{
	  s->jumplabelnum = s - &labels.front(); // TODO is this certain to work? would iterators be better?
	}
      else if ((p = FindLabel(temp))) 
	{
	  s->jumplabelnum = p - &labels.front();
	}
      else
	{
	  // label not found
	  Error("Unknown state label '%s'.\n", temp);
	  s->jumplabelnum = -1; // go to S_NULL
	}

      if (s->jumplabelnum == -1 && s->num_states == 0)
	{
	  // handle "xxx: stop"
	  s->dyn_states = false;
	  s->label_states = &states[S_NULL];
	  s->num_states = 1;
	}
    }

  // now re-set nextstate pointer for the last state in _every_ sequence using jump* info
  for (int i=0; i<n; i++)
    {
      statelabel_t *s = &labels[i];
      if (!s->dyn_states)
	continue; // don't mess with static statetable

      int j = s->jumplabelnum;
      if (j < 0)
	{
	  if (s->num_states == 0)
	    {
	      I_Error("FIXME, unexpected\n");
	    }
	  else
	    s->label_states[s->num_states-1].nextstate = &states[S_NULL]; // offset is ignored
	}
      else
	{
	  // check redirection
#define MAX_REDIRECTS 10 // allow max. 10 redirects
	  int k;
	  for (k=0; !labels[j].num_states && k < MAX_REDIRECTS; k++)
	    {
	      // follow redirect
	      j = labels[j].jumplabelnum;
	      if (j < 0)
		I_Error("DECORATE: Redirect to bad label.\n");
	    }

	  if (k >= MAX_REDIRECTS)
	    I_Error("DECORATE: Too many redirects.\n"); // or a cyclic redirect, "a: goto b; b: goto a;"

	  if (s->num_states) // for redirects, do nothing
	    {
	      // update last state
	      if (s->jumpoffset < labels[j].num_states)
		s->label_states[s->num_states-1].nextstate = &labels[j].label_states[s->jumpoffset];
	      else
		I_Error("DECORATE: State offset too large.\n"); // TODO wrap offsets to next seqs?
	    }
	}
    }

  // fill in shorthand state pointers
  for (int j=0; j<9; j++)
    (&spawnstate)[j] = NULL; // HACK

  for (int i=0; i<n; i++)
    {
      const char *temp = labels[i].label;
      for (int j=0; j<9; j++)
	{
	  if (!strcasecmp(StandardLabels[j], temp))
	    {
	      // found a match for this label
	      (&spawnstate)[j] = labels[i].label_states; // HACK
	      break;
	    }
	}
    }

  if (!spawnstate)
    {
      Error("Actor '%d' has no spawnstate!\n", classname);
      spawnstate = &states[S_NULL];
      return false;
    }

  return true;
}


void ActorInfo::Error(const char *format, ...)
{
  va_list p;

  va_start(p, format);
  fprintf(stderr, "DECORATE: ");
  fprintf(stderr, format, p);
  va_end(p);
}



void ActorInfo::PrintDECORATEclass()
{
  printf("actor %s %d\n{\n", classname, doomednum);
  if (!obituary.empty())
    printf("  obituary \"%s\"\n", obituary.c_str());
  if (!hitobituary.empty())
    printf("  hitobituary \"%s\"\n", hitobituary.c_str());
  if (!modelname.empty())
    printf("  model \"%s\"\n", modelname.c_str());

  printf("  health %d\n", spawnhealth);
  printf("  reactiontime %d\n", reactiontime);
  printf("  painchance %d\n", painchance);
  printf("  speed %g\n", speed);
  printf("  damage %d\n", damage);
  printf("  radius %g\n", radius.Float());
  printf("  height %g\n", height.Float());
  printf("  mass %g\n", mass);

  // TODO flags

  if (seesound != sfx_None)
    printf("  seesound \"%s\"\n", S_GetSoundTag(seesound));
  if (attacksound != sfx_None)
    printf("  attacksound \"%s\"\n", S_GetSoundTag(attacksound));
  if (painsound != sfx_None)
    printf("  painsound \"%s\"\n", S_GetSoundTag(painsound));
  if (deathsound != sfx_None)
    printf("  deathsound \"%s\"\n", S_GetSoundTag(deathsound));
  if (activesound != sfx_None)
    printf("  activesound \"%s\"\n", S_GetSoundTag(activesound));

  // TODO states
  /*
  printf("  states\n  {\n");
  char temp[64];
  if (spawnstate)
    {
      printf("  Spawn:\n");
      printf("    %s %s %d %s\n", spritenames[s->sprite], temp, s->tics, BEX_DActorMnemonics[i].name);
    }
  printf("  }\n");
  */
  printf("}\n\n");
}





class SkyboxCameraAI : public ActorInfo
{
public:
  SkyboxCameraAI(const string& n, int en)
    : ActorInfo(n, en)
  {
    spawn_always = true;
    flags  |= MF_NOGRAVITY|MF_NOSECTOR|MF_NOBLOCKMAP;
    flags2 |= MF2_DONTDRAW;
  }

  virtual Actor *Spawn(Map *m, mapthing_t *mt, bool initial = true)
  {
    Actor *a = ActorInfo::Spawn(m, mt, initial);
    m->skybox_pov = a;
    return a;
  }
};


class TeamStartSecAI : public ActorInfo
{
public:
  TeamStartSecAI(const string& n, int en) : ActorInfo(n, en) {}

  virtual Actor *Spawn(Map *m, mapthing_t *mt, bool initial = true)
  {
    subsector_t *ss = m->GetSubsector(mt->x, mt->y);
    if (ss)
      ss->sector->teamstartsec = mt->angle & 0xff; // high byte is free

    return NULL; // not spawned
  }
};









static void DoomPatchEngine()
{
  cv_jumpspeed.Set("6.0");
  cv_fallingdamage.Set("0");

  // hacks: teleport fog, blood, gibs
  mobjinfo[MT_TFOG].spawnstate = &states[S_TFOG];
  mobjinfo[MT_IFOG].spawnstate = &states[S_IFOG];
  strcpy(orig_sprnames[SPR_BLUD], "BLUD");
  states[S_GIBS].sprite = SPR_POL5;

  // linedef conversions
  int lump = fc.GetNumForName("XDOOM");
  linedef_xtable = (xtable_t *)fc.CacheLumpNum(lump, PU_STATIC);
  linedef_xtable_size = fc.LumpLength(lump) / sizeof(xtable_t);
}


static void HereticPatchEngine()
{
  cv_jumpspeed.Set("6.0");
  cv_fallingdamage.Set("0");

  // hacks
  mobjinfo[MT_TFOG].spawnstate = &states[S_HTFOG1];
  mobjinfo[MT_IFOG].spawnstate = &states[S_HTFOG1];
  strcpy(orig_sprnames[SPR_BLUD], "BLOD");
  states[S_GIBS].sprite = SPR_BLOD;

  int lump = fc.GetNumForName("XHERETIC");
  linedef_xtable = (xtable_t *)fc.CacheLumpNum(lump, PU_STATIC);
  linedef_xtable_size = fc.LumpLength(lump) / sizeof(xtable_t);

  // Above, good. Below, bad.
  text[TXT_PD_REDK] = "YOU NEED A GREEN KEY TO OPEN THIS DOOR";

  text[TXT_GOTBLUECARD] = "BLUE KEY";
  text[TXT_GOTYELWCARD] = "YELLOW KEY";
  text[TXT_GOTREDCARD] = "GREEN KEY";
}


static void HexenPatchEngine()
{
  cv_jumpspeed.Set("9.0");
  cv_fallingdamage.Set("23");

  // hacks
  mobjinfo[MT_TFOG].spawnstate = &states[S_XTFOG1];
  mobjinfo[MT_IFOG].spawnstate = &states[S_XTFOG1];
  strcpy(orig_sprnames[SPR_BLUD], "BLOD");
  states[S_GIBS].sprite = SPR_GIBS;
}


/// Convert static game data structures into dynamic ones. DECORATE scripts are processed after the conversion.
void PrepareGameData()
{
  switch (game.mode)
    {
    case gm_hexen:
      HexenPatchEngine();
      break;
    case gm_heretic:
      HereticPatchEngine();
      break;
    default:
      DoomPatchEngine();
    }

  int i;
  ActorInfo *ai;

  // prepare spritename mapping
  spritenames.resize(NUMSPRITES);
  for (i=0; i<NUMSPRITES; i++)
    spritenames[i] = orig_sprnames[i];

  if (devparm)
    {
      printf("Named DECORATE classes:\n");
      for (i=0; i<NUMMOBJTYPES; i++)
	{
	  if (mobjinfo[i].classname)
	    printf(" %s\n", mobjinfo[i].classname);
	}
    }

  // convert mobjinfo table to DECORATE class dictionary
  // TODO: after this mobjinfo should not be used at all! fix

  for (i = MT_LEGACY; i <= MT_LEGACY_S_END; i++)
    {
      ai = new ActorInfo(mobjinfo[i], gm_none);
      aid.Insert(ai);
      aid.InsertDoomEd(ai, true);

      if (i >= MT_LEGACY_S && i <= MT_LEGACY_S_END)
	ai->spawn_always = true;
    }

  for (i = MT_DOOM; i <= MT_DOOM_END; i++)
    {
      ai = new ActorInfo(mobjinfo[i], gm_doom2);
      aid.Insert(ai);
      aid.InsertDoomEd(ai, game.mode <= gm_doom2);
    }

  for (i = MT_HERETIC; i <= MT_HERETIC_END; i++)
    {
      ai = new ActorInfo(mobjinfo[i], gm_heretic);
      aid.Insert(ai);
      aid.InsertDoomEd(ai, game.mode == gm_heretic);
    }

  for (i = MT_HEXEN; i <= MT_HEXEN_END; i++)
    {
      ai = new ActorInfo(mobjinfo[i], gm_hexen);
      aid.Insert(ai);
      aid.InsertDoomEd(ai, game.mode == gm_hexen);
    }


  static ActorInfo *NativeAIs[3] =
  {
    new SkyboxCameraAI("SkyViewpoint", 9080),
    new ActorInfo("SkyPicker", 9081),
    new TeamStartSecAI("TeamStartSec", 5005), // TEST teamstartsec thing
  };

  for (i=0; i<3; i++)
    {
      ai = NativeAIs[i];
      aid.Insert(ai);
      aid.InsertDoomEd(ai, true);
    }

  CONS_Printf("Reading DECORATE definitions...\n");

  int n = fc.Size();
  for (int i = 0; i < n; i++)
    {
      // cumulative reading
      int lump = -1;
      while ((lump = fc.FindNumForNameFile("DECORATE", i, lump+1)) >= 0)
	Read_DECORATE(lump);
    }

  CONS_Printf(" %d Actor types defined.\n", aid.Size());

  extern void MakePawnAIs();
  MakePawnAIs();
}
