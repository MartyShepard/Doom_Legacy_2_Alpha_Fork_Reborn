// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: g_map.h 526 2008-01-22 23:28:24Z smite-meister $
//
// Copyright (C) 2002-2007 by DooM Legacy Team.
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
/// \brief Map class definition.

#ifndef g_map_h
#define g_map_h 1

#include <vector>
#include <deque>
#include <string>
#include <list>
#include <map>

#include "doomdef.h"
#include "r_defs.h"
#include "g_think.h"
#include "vect.h"
#include "m_fixed.h"
#include "m_bbox.h"
#include "info.h" // mobjtype_t

using namespace std;

typedef bool (*thing_iterator_t)(class Actor *a);
typedef bool (*thinker_iterator_t)(Thinker *t);


/// \brief A single game map and all the stuff it contains.
/// \nosubgrouping
/// \ingroup g_central
/*!
  This class stores all gameplay-related information about one map,
  including geometry, BSP, blockmap, reject, mapthings, Thinker list,
  player list, ambient sounds, physics...
*/
class Map
{
  friend class GameInfo;
public:
  /// \name Essentials
  //@{
  class MapInfo *info; ///< the controlling MapInfo struct for this map

  string lumpname;   ///< map lumpname
  int    lumpnum;    ///< lumpnum of the separator beginning the map

  tic_t starttic, maptic;      ///< number of tics the map has played
  int   kills, items, secrets; ///< map totals

  bool hexen_format; ///< is this map stored in the Hexen format?
  //@}

  /// \name Geometry
  //@{
  int                 numvertexes;
  struct vertex_t    *vertexes;    ///< normal vertices

  int                 numlines;
  struct line_t      *lines;       ///< linedefs

  int                 numsides;
  struct side_t      *sides;       ///< sidedefs

  int                 numsectors;
  struct sector_t    *sectors;     ///< map sectors

  int                 numsubsectors;
  struct subsector_t *subsectors;  ///< subsectors, aka BSP leaves

  int                 numnodes;
  struct node_t      *nodes;       ///< BSP nodes

  int                 numsegs;
  struct seg_t       *segs;        ///< what linedefs are to sectors, segs are to subsectors

  int                 numglvertexes;
  vertex_t           *glvertexes;  ///< additional vertices from GL nodes

  int                 NumPolyobjs;
  struct polyobj_t   *polyobjs;    ///< polyobjects

  byte *glvis; ///<  Subsector visibility data from glVIS.

  bbox_t              root_bbox;   ///< bounding box for all the vertices in the map
  line_t            **linebuffer;  ///< combining sectors and lines
  //@}

  /// \name Rendering
  /// Map-specific stuff only the renderer uses.
  //@{
  class fadetable_t *fadetable; ///< colormaps the software renderer uses to simulate light levels
  bool R_SetFadetable(const char *name);

  class Material *skytexture; ///< current sky texture
  class Actor    *skybox_pov; ///< default skybox viewpoint
  //@}


  /// Blockmap for this Map
  class blockmap_t *blockmap;
  // MAXRADIUS is for precalculated sector block boxes
  // the spider demon is larger, but we do not have any moving sectors nearby
#define MAXRADIUS       32


  /// \name Reject
  /// Binary sector-to-sector visibility matrix for fast sight rejection.
  /// Speeds up enemy AI by skipping detailed LineOfSight calculation.
  /// Without the "Reject special effects" hacks in some PWADs, this could be used as a PVS lookup as well.
  //@{
  byte *rejectmatrix;
  //@}


  /// \name Scripting
  //@{
  // FS
  struct script_t        *FS_levelscript;    ///< the mother of all FS scripts in this map
  struct runningscript_t *FS_runningscripts; ///< linked list of currently active FS scripts

  // ACS
  byte   *ACS_base;                        ///< the raw BEHAVIOR lump, base for offsets
  typedef map<unsigned, struct acs_script_t>::iterator acs_script_iter_t;
  map<unsigned, acs_script_t> ACS_scripts; ///< mapping from script numbers to script definitions
  vector<char *>              ACS_strings; ///< array of the ACS strings in this map
#define ACS_MAP_VARS 32
  Sint32  ACS_map_vars[ACS_MAP_VARS];      ///< ACS map variables

  bool ACS_LoadScripts(int lump);
  void ACS_StartOpenScript(acs_script_t *s);
  bool ACS_StartScriptInMap(int mapnum, unsigned scriptnum, byte *args);
  class acs_t *ACS_StartScript(unsigned number, byte *args, Actor *activator, line_t *line, int side);
  void ACS_StartDeferredScripts();
  bool ACS_Terminate(unsigned number);
  bool ACS_Suspend(unsigned number);
  void ACS_ScriptFinished(unsigned number);

  acs_script_t *ACS_FindScript(unsigned number);
  const char *ACS_FindString(unsigned number);

  void TagFinished(unsigned tag);
  void PO_Finished(unsigned po);
  //@}


  /// \name Mapthings and Thinkers
  //@{
  int                nummapthings;
  struct mapthing_t *mapthings;    ///< things

  Thinker thinkercap; ///< Linked list of Thinkers in the map. The head and tail of the thinker list.

  bool        force_pointercheck; ///< force a Thinker pointer cleanup
  vector<Thinker *> DeletionList; ///< Thinkers to be deleted are stored here

  deque<mapthing_t *> itemrespawnqueue; ///< for respawning items
  deque<tic_t>        itemrespawntime;  ///< this could be combined to the previous one, but...

  deque<Actor *> bodyqueue; ///< queue for player/monster corpses

  multimap<short, Actor *> TID_map; ///< Thing ID, a system for grouping things.
  typedef multimap<short, Actor *>::iterator TID_map_iter_t;

  /// Currently active dynamic geometry elements, based on BOOM code.
  list<class ceiling_t *> activeceilings;
  list<class plat_t *>    activeplats;
  //@}


  /// \name Players
  //@{
  vector<class PlayerInfo *> players; ///< players currently in the map
  deque<PlayerInfo *>   respawnqueue; ///< players queuing to be respawned

  multimap<int, mapthing_t *> playerstarts; ///< Mapping from player number to playerstart. args[0] holds the entrypoint number.
  static const unsigned MAX_DM_STARTS = 32;
  vector<mapthing_t *> dmstarts; ///< DeathMatch starts
  //@}


  /// \name Sound sequences
  //@{
  list<class ActiveSndSeq*> ActiveSeqs;
  vector<int>   AmbientSeqs;
  ActiveSndSeq *ActiveAmbientSeq;
  //@}


  //-----------------------------------
  class MapEffect *effects; ///< Hexen lightning effect
  class BotNodes *botnodes; // TEST

  //------------------------------------
  // TODO: from this line on it's badly designed stuff to be fixed someday
  vector<mapthing_t *> braintargets; // DoomII demonbrain spawnbox targets
  int braintargeton;

  vector<mapthing_t *> BossSpots;
  vector<mapthing_t *> MaceSpots;

public:
  // constructor and destructor
  Map(MapInfo *i);
  ~Map(); // also destroys all objects that belong to this map

  // not yet implemented
  void Reset(); // resets the map to starting position. Lighter than Setup().

  int  Serialize(class LArchive &a);
  int  Unserialize(LArchive &a);

  // in p_tick.cpp
  void Ticker();

  void InitThinkers();
  void AddThinker(Thinker *thinker);
  void DetachThinker(Thinker *thinker);
  void RemoveThinker(Thinker *thinker);
  void RunThinkers();
  void PointerCleanup();

  // in g_map.cpp
  void AddPlayer(PlayerInfo *p); // adds a new player to the map (and respawnqueue)
  bool RemovePlayer(PlayerInfo *p);
  void RebornPlayer(PlayerInfo *p); // adds a player to respawnqueue, gets rid of corpse
  PlayerInfo *FindPlayer(int number); // returns player 'number' if he is in the map, otherwise NULL
  int  RespawnPlayers();
  int  HandlePlayers();

  bool DeathMatchRespawn(PlayerInfo *p);
  bool CoopRespawn(PlayerInfo *p);
  bool CheckRespawnSpot(PlayerInfo *p, mapthing_t *mthing);
  void QueueBody(Actor *p);

  void BossDeath(const class DActor *mo);
  int  Massacre();
  void ExitMap(Actor *activator, int exit, int entrypoint = 0);

  void RespawnSpecials();
  void RespawnWeapons();

  Actor  *SpawnActor(Actor *a, fixed_t spawnheight);
  DActor *SpawnDActor(fixed_t nx, fixed_t ny, fixed_t nz, const class ActorInfo *ai);
  DActor *SpawnDActor(fixed_t nx, fixed_t ny, fixed_t nz, mobjtype_t t);
  inline DActor *SpawnDActor(const vec_t<fixed_t>& r, mobjtype_t t) { return SpawnDActor(r.x, r.y, r.z, t); }
  inline DActor *SpawnDActor(const vec_t<fixed_t>& r, const ActorInfo *ai) { return SpawnDActor(r.x, r.y, r.z, ai); }
  void SpawnPlayer(PlayerInfo *pi, mapthing_t *mthing);
  DActor *SpawnSplash(const vec_t<fixed_t>& pos, fixed_t z, int sound, mobjtype_t base,
		      mobjtype_t chunk = MT_NONE, bool randtics = true); 
  DActor *SpawnBlood(const vec_t<fixed_t>& r, int damage);
  void SpawnBloodSplats(const vec_t<fixed_t>& r, int damage, fixed_t px, fixed_t py);
  void SpawnPuff(const vec_t<fixed_t>& r, mobjtype_t pufftype, bool hit_thing);
  void SpawnSmoke(const vec_t<fixed_t>& r);

  void InsertIntoTIDmap(Actor *p, int tid);
  void RemoveFromTIDmap(Actor *p);

  // in p_map.cpp
  bool CheckSector(sector_t* sector, int crunch);
  bool ChangeSector(sector_t *sector, int crunch);

  // in p_setup.cpp
  bool Setup(tic_t start, bool spawnthings = true);
  void LoadVertexes(int lump);
  void LoadSegs(int lump);
  void LoadSubsectors(int lump);
  void LoadSectors1(int lump);
  void LoadSectors2(int lump);
  void LoadNodes(int lump);
  void LoadThings(int lump, bool heed_spawnflags);
  void LoadLineDefs(int lump);
  void LoadLineDefs2();
  void LoadSideDefs(int lump);
  void LoadSideDefs2(int lump);
  void LoadBlockMap(int lump);
  void GroupLines();
  void SetupSky();
  void GenerateBlockMap();

  int LoadGLVertexes(const int lump);
  void LoadGLSegs(const int lump, const int glversion);
  void LoadGLSubsectors(const int lump, const int glversion);
  void LoadGLNodes(const int lump, const int glversion);
  void LoadGLVis(const int lump);

  // in p_sight.cpp
  bool CrossSubsector(int num);
  bool CrossBSPNode(int bspnum);
  bool CheckSight(Actor *t1, Actor *t2);
  bool CheckSight2(Actor *t1, Actor *t2, fixed_t nx, fixed_t ny, fixed_t nz);

  // in p_maputl.cpp
  subsector_t *GetSubsector(fixed_t x, fixed_t y);
  subsector_t *FindSubsector(fixed_t x, fixed_t y);
  bool IterateThinkers(thinker_iterator_t func);
  bool IterateActors(thing_iterator_t func);
  void CreateSecNodeList(Actor *thing, fixed_t x, fixed_t y);

  // in p_hpspr.cpp
  void PlaceWeapons();
  void RepositionMace(DActor *mo);

  // in p_telept.cpp
  bool EV_Teleport(unsigned tag, line_t *line, Actor *thing, int type, int flags);
  bool EV_SilentLineTeleport(unsigned lineid, line_t *line, Actor *thing, bool reverse);

  // in p_spec.cpp
  void InitTagLists();
  int  FindSectorFromTag(unsigned tag, int start);
  line_t *FindLineFromID(unsigned lineid, int *start);
  bool TagBusy(unsigned tag);

  void AddFakeFloor(sector_t* sec, sector_t* sec2, line_t* master, int flags);

  bool ActivateLine(line_t *line, Actor *thing, int side, int activationType);
  bool ExecuteLineSpecial(unsigned special, byte *args, line_t *line, int side, Actor *mo);

  void UpdateSpecials();

  int  SpawnSectorSpecial(int sp, sector_t *sec);
  void SpawnLineSpecials();
  void SpawnScroller(line_t *l, unsigned tag, int type, int control);
  void SpawnFriction(line_t *l, unsigned tag);
  void SpawnPusher(line_t *l, unsigned tag, int type);

  // some event functions that fit nowhere else
  int  EV_SectorSoundChange(unsigned tag, int seq);
  bool EV_LineSearchForPuzzleItem(line_t *line, byte *args, Actor *mo);

  // in a_action.cpp
  bool EV_LocalQuake(byte *args);

  // in p_switch.cpp
  void ChangeSwitchTexture(line_t *line, int useAgain);

  // in p_genlin.cpp
  int EV_DoGenFloor(line_t* line);
  int EV_DoGenCeiling(line_t* line);
  int EV_DoGenLift(line_t* line);
  int EV_DoGenStairs(line_t* line);
  int EV_DoGenCrusher(line_t* line);
  int EV_DoGenDoor(line_t* line);
  int EV_DoGenLockedDoor(line_t* line);

  // in p_lights.cpp
  void SpawnPhasedLightSequence(sector_t *sec, int indexStep);
  void SpawnStrobeLight(sector_t *sec, short brighttime, short darktime, bool inSync);
  int  EV_StartLightStrobing(unsigned tag, short brighttime, short darktime);
  int  EV_FadeLight(unsigned tag, int destvalue, int speed);
  int  EV_SpawnLight(unsigned tag, int type, short maxl, short minl = 0, short maxt = 0, short mint = 0);
  int  EV_TurnTagLightsOff(unsigned tag);
  int  EV_LightTurnOn(unsigned tag, int bright);

  // in p_plats.cpp
  void AddActivePlat(plat_t* plat);
  int  ActivateInStasisPlat(unsigned tag);
  void RemoveActivePlat(plat_t* plat);
  void RemoveAllActivePlats();
  int  EV_DoPlat(unsigned tag, line_t *line, int type, fixed_t speed, int wait, fixed_t height);
  int  EV_StopPlat(unsigned tag);

  // in p_ceiling.cpp
  void AddActiveCeiling(ceiling_t* ceiling);
  int  ActivateInStasisCeiling(unsigned tag);
  void RemoveActiveCeiling(ceiling_t* ceiling);
  void RemoveAllActiveCeilings();
  int  EV_DoCeiling(unsigned tag, line_t *line, int type, fixed_t speed, int crush, fixed_t height);
  int  EV_DoCrusher(unsigned tag, int type, fixed_t upspeed, fixed_t downspeed, int crush, fixed_t height);
  int  EV_StopCeiling(unsigned tag);

  // in p_doors.cpp
  void EV_OpenDoor(unsigned tag, int speed, int wait_time);
  void EV_CloseDoor(unsigned tag, int speed);
  int  EV_DoDoor(unsigned tag, line_t* line, Actor *mo, byte type, fixed_t speed, int delay);
  void SpawnDoorCloseIn30(sector_t* sec);
  void SpawnDoorRaiseIn5Mins(sector_t* sec);

  // in p_floor.cpp
  int EV_DoFloor(unsigned tag, line_t *line, int type, fixed_t speed, int crush, fixed_t height);
  int EV_DoChange(line_t *line, int changetype);
  int EV_BuildStairs(unsigned tag, int type, fixed_t speed, fixed_t stepsize, int crush);
  int EV_BuildHexenStairs(unsigned tag, int type, fixed_t speed, fixed_t stepdelta, int resetdelay, int stepdelay = 0);
  int EV_DoDonut(unsigned tag, fixed_t pspeed, fixed_t sspeed);
  int EV_DoElevator(unsigned tag, int type, fixed_t speed, fixed_t height_f, fixed_t height_c = 0, int crush = 0);
  int EV_DoFloorWaggle(unsigned tag, fixed_t amp, angle_t freq, angle_t offset, int wait);
  int T_MovePlane(sector_t *sector, fixed_t speed, fixed_t dest, int crush, int floorOrCeiling);

  // in s_sndseq.cpp
  bool SN_StartSequence(struct mappoint_t *m, int seq);
  bool SN_StartSequence(Actor *a, int seq);
  bool SN_StartSequenceName(mappoint_t *m, const char *name);
  bool SN_StopSequence(void *origin, bool quiet = false);
  void UpdateSoundSequences();

  // elsewhere, usually former R_* functions
  void PrecacheMap();

  // in p_polyobj.cpp
  map<unsigned, polyobj_t*> PO_map;
  typedef map<unsigned, polyobj_t*>::iterator PO_iter_t;
  void PO_Init();
  bool PO_FindLines(polyobj_t *p, line_t *line);

  polyobj_t *PO_FindPolyobj(unsigned num);
  bool PO_Busy(unsigned  num);
  bool EV_ActivatePolyobj(unsigned id, int type, float speed, angle_t angle, float dist, int delay, bool override);

  // in p_things.cpp
  bool EV_ThingProjectile(int tid, const ActorInfo *ai, angle_t angle, float hspeed, float vspeed, bool gravity);
  bool EV_ThingSpawn(int tid, const ActorInfo *ai, angle_t angle, bool fog);
  bool EV_ThingActivate(int tid);
  bool EV_ThingDeactivate(int tid);
  bool EV_ThingRemove(int tid);
  bool EV_ThingDestroy(int tid);

  // FS scripting
  void FS_ClearScripts();
  void FS_ClearRunningScripts();
  void FS_LoadScripts(char *data);
  bool FS_RunScript(int n, Actor *trig);
  void FS_DelayedScripts();
  void FS_AddRunningScript(runningscript_t *s);
protected:
  bool FS_wait_finished(runningscript_t *script);

public:
  /// Class for iterating through the TID map.
  class Iterate_TID
  {
  protected:
    Map::TID_map_iter_t low, high;
    int count;

  public:
    Iterate_TID(Map *m, int tid)
    { 
      low = m->TID_map.lower_bound(tid);
      high = m->TID_map.upper_bound(tid);
      count = m->TID_map.count(tid);
    }

    inline int   Count() const { return count; }
    inline Actor *Next() { return (low == high) ? NULL : (low++)->second; }
  };
};


#endif
