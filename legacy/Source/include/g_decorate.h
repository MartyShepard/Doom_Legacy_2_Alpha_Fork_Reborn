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
/// \brief ActorInfo class definition.

#ifndef g_decorate_h
#define g_decorate_h

#include <string>
#include <map>
#include "dictionary.h"
#include "info.h"

using namespace std;


/// \brief Actor type definition. Each instant defines a DECORATE class.
/// \ingroup g_thing
/*!
  Stores static data that is shared between all Actor instances of the same type.

  The only reason we still have mobjinfo_t is the mobjinfo table, which is converted into
  a pair of std::maps at program start. Later it could (along with the state table) be replaced
  using DECORATE files, but it is a huge effort and I have better things to do.
 */
class ActorInfo
{
  enum
  {
    CLASSNAME_LEN = 63
  };
protected:
  char classname[CLASSNAME_LEN+1]; ///< NUL-terminated name of the DECORATE class, identical to the ZDoom equivalents if possible.
  mobjtype_t   mobjtype; ///< Old mobjtype_t number, TODO maybe remove it eventually and use only classname.

public:
  int              game; ///< To which game does it belong? Uses the gamemode_t enum.
  string       obituary; ///< Obituary message for players killed by "instances" of this DECORATE class.
  string    hitobituary; ///< Same, but for melee attacks.
  string      modelname; ///< Name of MD3 model to be used to represent this class.
  string       skinname; ///< Name of skin to use (either sprite skin or model skin).
  bool     spawn_always; ///< Do not care about mapthing "when-to-spawn" flags.

public:
  // Actor part of mobjinfo_t members. ActorInfo must be semantically equivalent to mobjinfo_t so it can replace it.
  int     doomednum;     ///< Editor number for this thing type or -1 if none.
  int     spawnhealth;   ///< Initial health.
  int     reactiontime;  ///< How soon (in tics) will the thing do something again.
  fixed_t radius;        ///< Thing radius.
  fixed_t height;        ///< Thing height.
  float   mass;          ///< Just take a guess.
  Uint32  flags;         ///< mobjflag_t flags.
  Uint32  flags2;        ///< mobjflag2_t flags.

  // DActor part of mobjinfo_t members.
  int     painchance;    ///< Probability of going into painstate when hurt.  
  float   speed;         ///< Max. movement speed.
  Uint32  damage;        ///< Low 16 bits: damage for missiles, high 16 bits: see damage_t.

  int seesound;          ///< Played when thing sees an enemy.
  int attacksound;       ///< Played when attacking.
  int painsound;         ///< Played when hurt.
  int deathsound;        ///< Played when thing dies.
  int activesound;       ///< Occasionally played when active.

  state_t *spawnstate;   ///< Initial state.
  state_t *seestate;     ///< Used when thing sees an enemy. Active.
  state_t *meleestate;   ///< Used when thing executes a melee attack.
  state_t *missilestate; ///< Same, but for missile attacks.
  state_t *painstate;    ///< Used when thing is hurt.
  state_t *deathstate;   ///< Used when thing is killed.
  state_t *xdeathstate;  ///< Used when thing is killed in a messy way, e.g. exploded.
  state_t *crashstate;   ///< Used when a flying thing has died and crashed on the ground.
  state_t *raisestate;   ///< Used when thing is being raised from dead.

  touchfunc_t touchf;    ///< If MF_TOUCHFUNC is set, this function is to be called when touching another Actor.

public:
  /// State labels are retained indefinitely.
  struct statelabel_t
  {
#define SL_LEN 16
    char label[SL_LEN]; ///< Label string, NUL-terminated.
    int  num_states;    ///< Number of states in the sequence.
    state_t *label_states;  ///< DActor states owned by this label.
    bool dyn_states;    ///< Are the states dynamically allocated?
    char jumplabel[SL_LEN]; ///< Label to jump to after the sequence.
    int  jumplabelnum;  ///< Number of label to jump to after the sequence, or -1 for S_NULL.
    int  jumpoffset;    ///< Additional offset for the jump.
  };

protected:
  vector<statelabel_t> labels; ///< All known state labels for this DActor type.

public:
  /// constructors
  ActorInfo(const string& n, int en = -1);
  /// copy constructor
  ActorInfo(const ActorInfo& a);
  /// Convert mobjinfo_t into an ActorInfo
  ActorInfo(const mobjinfo_t& m, int game);

  /// destructor
  virtual ~ActorInfo();

  /// Creates an Actor of the type described by this ActorInfo instance.
  virtual class Actor *Spawn(class Map *m, struct mapthing_t *mt, bool initial = true) const;

  /// Sets the DECORATE class name.
  void SetName(const char *n);
  /// Returns the DECORATE class name.
  inline const char *GetName() const { return classname; }

  /// Sets the mobjtype.
  inline void SetMobjType(mobjtype_t t) { mobjtype = t; }
  /// Returns the mobjtype number.
  inline mobjtype_t GetMobjType() const { return mobjtype; }

  /// Sets or resets one or more Actor flags depending on the mnemonic 'flag'.
  void SetFlag(const char *flag, bool on);

  /// \name Functions for generating new AI states.
  //@{
  /// Defines a new state label, which will point to the next state to be added.
  static void AddLabel(const char *l);
  /// Adds a sequence of AI states, which only differ in the frame field.
  static void AddStates(const char *spr, const char *frames, int tics, const char *func);
  /// Finishes an AI state sequence with a jump to a label, or if 'label' is NULL, to the beginning of the sequence.
  /// Allocates space for the new AI states.
  void FinishSequence(const char *jumplabel, int offset);

protected:
  /// If 'label' has been defined return the corresponding state label, else return NULL.
  statelabel_t *FindLabel(const char *label);

public:
  /// Associates the labels with the various state_t pointers.
  bool UpdateSequences();
  //@}
  
  /// Prints the DECORATE definition for this class to stdout.
  void PrintDECORATEclass();

  /// Utility for printing error messages during DECORATE parsing.
  static void Error(const char *format, ...);
};



/// \brief Stores all known DActor type definitions.
/// \ingroup g_thing
/*!
  The stored ActorInfo records are stored in three maps according to three different keys:
  - DECORATE class name (primary mapping)
  - mobjtype_t number
  - DoomEd number (not every class has one)
 */
class ActorInfoDictionary : public HashDictionary<ActorInfo>
{
protected:
  typedef HashDictionary<ActorInfo> parent;

  typedef map<mobjtype_t, ActorInfo*> mt_map_t;
  typedef mt_map_t::iterator mt_iter_t;
  mt_map_t mt_map;

  typedef map<int, ActorInfo*> doomed_map_t;
  typedef doomed_map_t::iterator doomed_iter_t;
  doomed_map_t doomed_map;

public:
  /// The safe way of inserting stuff into the dictionary.
  /// The main point is that 'classname' is stored within the ActorInfo structure itself.
  inline bool Insert(ActorInfo *p)
  {
    if (parent::Insert(p))
      {
	// do not replace possible original in mt_map (should not make any difference)
	mt_map.insert(mt_map_t::value_type(p->GetMobjType(), p));
	return true;
      }
    else
      return false;
  }

  /// Returns the first unused mobjtype_t number.
  inline mobjtype_t GetFreeMT() const
  {
    // Map is ordered according to key.
    return static_cast<mobjtype_t>(mt_map.rbegin()->first + 1);
  }

  /// Find the ActorInfo corresponding to mobjtype mt.
  ActorInfo *operator[](mobjtype_t mt)
  {
    mt_iter_t t = mt_map.find(mt);
    return (t == mt_map.end()) ? NULL : t->second;
  }

  /// Inserts p into DoomEd map, possibly replacing original.
  inline bool InsertDoomEd(ActorInfo *p, bool replace)
  {
    if (p->doomednum >= 0)
      {
	if (replace)
	  doomed_map.erase(p->doomednum);

	return doomed_map.insert(doomed_map_t::value_type(p->doomednum, p)).second;
      }
    return false;
  }

  /// Find the ActorInfo corresponding to doomednum n.
  ActorInfo *FindDoomEdNum(int n)
  {
    doomed_iter_t t = doomed_map.find(n);
    if (t == doomed_map.end())
      return NULL;
    else
      return t->second;
  }


  int  Clear();
};


extern ActorInfoDictionary aid;

#endif
