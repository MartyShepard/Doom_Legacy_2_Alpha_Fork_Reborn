// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: g_map.cpp 536 2009-06-29 06:46:13Z smite-meister $
//
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
/// \brief Map class implementation

#include "g_map.h"
#include "g_blockmap.h"
#include "g_mapinfo.h"
#include "g_game.h"
#include "g_player.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "g_decorate.h"

#include "command.h"
#include "cvars.h"

#include "b_path.h"

#include "p_effects.h"
#include "p_spec.h"
#include "p_hacks.h"
#include "p_polyobj.h"
#include "p_maputl.h"

#include "r_splats.h"

#include "hud.h"
#include "m_random.h"
#include "wi_stuff.h"

#include "t_acs.h"

#include "sounds.h"
#include "s_sound.h"
#include "z_zone.h"
#include "tables.h"


const fixed_t FLOATRANDZ = fixed_t::FMAX-1;

#define PLAYERSPAWN_HALF_DELAY 10 // 0.5 * min. tics to wait between player spawns

// can handle also -32768 properly
static inline unsigned short Abs(short x)
{
  return (x < 0) ? -x : x;
}


// Map class constructor
Map::Map(MapInfo *i)
{
  info = i;
  lumpname = i->lumpname;
  hexen_format = false;

  vertexes = NULL;
  lines = NULL;
  sides = NULL;
  sectors = NULL;
  subsectors = NULL;
  nodes = NULL;
  segs = NULL;
  polyobjs = NULL;
  linebuffer = NULL;
  glvertexes   = NULL;
  glvis = NULL;

  blockmap = NULL;
  rejectmatrix = NULL;

  fadetable = NULL;
  skytexture = NULL;
  skybox_pov = NULL;

  FS_levelscript = NULL;
  FS_runningscripts = NULL;

  ACS_base = NULL;

  mapthings = NULL;
  force_pointercheck = false;

  ActiveAmbientSeq = NULL;

  braintargeton = 0;

  effects = NULL;
  botnodes = NULL;
};


// destructor
Map::~Map()
{
  if (vertexes) // geometry loaded
    {
      Z_Free(vertexes);
      Z_Free(lines);
      Z_Free(sides);
      Z_Free(sectors);
      Z_Free(subsectors);
      Z_Free(nodes);
      Z_Free(segs);
      if (polyobjs)
	{
	  for (int i=0; i < NumPolyobjs; i++)
	    {
	      if (polyobjs[i].segs)
		Z_Free(polyobjs[i].segs);

	      if (polyobjs[i].base_points)
		Z_Free(polyobjs[i].base_points);

	      if (polyobjs[i].current_points)
		Z_Free(polyobjs[i].current_points);
	    }

	  Z_Free(polyobjs);
	}
      Z_Free(linebuffer);

      // Remove GL nodes if they exist.
      if (glvertexes)
	Z_Free(glvertexes);
      if (glvis)
	Z_Free(glvis);

      delete blockmap;
      Z_Free(rejectmatrix);

      FS_ClearScripts();

      if (ACS_base)
	Z_Free(ACS_base);

      Z_Free(mapthings);
    }

  if (effects)
    delete effects;

  if (botnodes)
    delete botnodes;

  Thinker *t, *next;
  for (t = thinkercap.next; t != &thinkercap; t = next)
    {
      next = t->next;
      delete t;
    }

  int n = DeletionList.size();
  for (int i=0; i<n; i++)
    delete DeletionList[i];

  // TODO sound sequences?

  // clear the splats from deleted map
  R_ClearLevelSplats(); // FIXME find a better way
  // 3D sounds must be stopped when their sources are deleted...
  S.Stop3DSounds(); // TODO not correct, since several maps may run simultaneously.
}



//===================================================
//             GAME SPAWN FUNCTIONS
//===================================================


Actor *Map::SpawnActor(Actor *a, fixed_t spawnheight)
{
  AddThinker(a);     // AddThinker sets Map *mp
  a->SetPosition();  // set subsector and/or block links

  sector_t *s = a->subsector->sector;
  fixed_t nz = a->pos.z;

  // NOTE: We need a proper z coordinate for CheckPosition (due to fake floors!)
  if (nz == ONFLOORZ || nz == FLOATRANDZ)
    a->pos.z = s->floorheight + spawnheight; // from floor up
  else if (nz == ONCEILINGZ)
    a->pos.z = s->ceilingheight -a->height -spawnheight; // from ceiling down

  position_check_t *ccc = a->CheckPosition(a->pos, Actor::PC_LINES); // for missiles owner is not yet set => collides, so we only check geometry
  a->floorz = ccc->op.bottom;
  a->ceilingz = ccc->op.top;

  if (nz == FLOATRANDZ)
    {
      fixed_t space = a->ceilingz - a->height - a->floorz;
      a->pos.z = (space > 48) ? a->floorz +40 +(((space - 40) * P_Random()) >> 8) : a->floorz;
    }

  if ((a->flags2 & MF2_FOOTCLIP) && (a->subsector->sector->floortype >= FLOOR_LIQUID)
      && (a->floorz == a->subsector->sector->floorheight))
    a->floorclip = FOOTCLIPSIZE;
  else
    a->floorclip = 0;

  return a;
}



DActor *Map::SpawnDActor(fixed_t nx, fixed_t ny, fixed_t nz, mobjtype_t t)
{
  return SpawnDActor(nx, ny, nz, aid[t]);
}


/// Spawns and adds a DActor to a Map.
DActor *Map::SpawnDActor(fixed_t nx, fixed_t ny, fixed_t nz, const ActorInfo *ai)
{
  // NOTE: nz may have the values ONFLOORZ, ONCEILINGZ and FLOATRANDZ in addition to real z coords...
  DActor *a = new DActor(nx, ny, nz, ai);
  SpawnActor(a, 0);
  return a;
}



// Called when a player is spawned on the level.
// Most of the player structure stays unchanged
//  between levels.
void Map::SpawnPlayer(PlayerInfo *pi, mapthing_t *mthing)
{
  vec_t<fixed_t> loc(mthing->x, mthing->y, ONFLOORZ);

  PlayerPawn *p;

  // the player may have his old pawn from the previous level
  if (!pi->pawn)
    {
      p = new PlayerPawn(loc.x, loc.y, loc.z, pi->options.ptype);
      p->player = pi;
      p->team = pi->team;
      pi->pawn  = p;
    }
  else
    {
      p = pi->pawn;
      p->pos = loc;
      p->vel.Set(0, 0, 0);
    }

  SpawnActor(p, mthing->height); // spawn the pawn
  p->TeleportMove(p->pos); // teleport to current location to telefrag bothersome obstacles (read: other players)

  // Boris stuff
  if (!pi->options.originalweaponswitch)
    p->UseFavoriteWeapon();

  // FIXME set skin sprite here
  // set color translations for player sprites

  p->yaw = ANG45 * (mthing->angle/45);

  // setup gun psprite
  p->SetupPsprites();

  // give all cards in death match mode
  if (cv_deathmatch.value)
    p->keycards = it_allkeys;

  p->spawnpoint = mthing;
  mthing->mobj = p;
  mthing->tid = static_cast<short>(maptic) + PLAYERSPAWN_HALF_DELAY; // set the timer

  // TODO spawn a teleport fog?
  /*
    unsigned an = ( ANG45 * (mthing->angle/45) ) >> ANGLETOFINESHIFT;
    Actor *fog = SpawnDActor(nx+20*finecosine[an], ny+20*finesine[an], nz, MT_TFOG);
    S_StartSound(fog, sfx_telept); // don't start sound on first frame?
  */

  pi->pov = p;
  pi->viewheight = cv_viewheight.value;
  pi->viewz = p->pos.z + pi->viewheight;
  pi->playerstate = PST_INMAP;
  pi->setMaskBits(PlayerInfo::M_PAWN); // notify network system
}




//
// Returns false if the player p cannot be respawned
// at the given mapthing_t spot
// because something is occupying it
//
bool Map::CheckRespawnSpot(PlayerInfo *p, mapthing_t *mthing)
{
  if (!p || !mthing)
    return false;

  if (mthing->args[0] != p->entrypoint)
    return false;

  // has the spawn spot been used just recently? (less stupid telefrag this way!)
  if (Abs(static_cast<short>(maptic - mthing->tid)) <= PLAYERSPAWN_HALF_DELAY)
    return false;

  fixed_t x = mthing->x;
  fixed_t y = mthing->y;
  subsector_t *ss = GetSubsector(x,y);

  // check for respawn in team-sector
  if (ss->sector->teamstartsec)
    if (cv_teamplay.value)
      if (p->team != ss->sector->teamstartsec)
	return false; // start is meant for another team

  // will it fit there?
  // FIXME! at this point p has no longer a pawn, besides,
  // if the new pawn is larger than the previous, it wouldn't help anyway
  // no size checking right now.
  //if (!p->pawn->TestLocation(x, y)) return false;

  return true;
}



// Spawns a player at one of the random death match spots
// called at level load and each death
bool Map::DeathMatchRespawn(PlayerInfo *p)
{
  int n = dmstarts.size();
  if (n == 0)
    I_Error("No deathmatch start in this map !");

  int i, j;

  // TODO  create a random n-permutation and use it!
  j = i = P_Random() % n;
  do {
    if (CheckRespawnSpot(p, dmstarts[j]))
      {
	SpawnPlayer(p, dmstarts[j]);
	return true;
      }
    j++;
    if (j == n)
      j = 0;
  } while (j != i);

  return false;
}


bool Map::CoopRespawn(PlayerInfo *p)
{
  int i = p->number;
  multimap<int, mapthing_t *>::iterator s, t;

  // let's check his own start first
  s = playerstarts.lower_bound(i);
  t = playerstarts.upper_bound(i);
  for ( ; s != t--; )
    {
      mapthing_t *m = t->second;
      // the pawn is spawned at the last playerstart
      if (CheckRespawnSpot(p, m))
	{
	  SpawnPlayer(p, m);

	  // spawn voodoo dolls in all other playerstarts
	  if (cv_voodoodolls.value)
	    {
	      for ( ; s != t; s++)
		{
		  m = s->second;
		  if (CheckRespawnSpot(p, m))
		    VoodooDoll::Spawn(p, m);
		}
	    }

	  return true;
	}
    }

  // try to spawn at one of the other players' spots, then
  s = playerstarts.begin();
  t = playerstarts.end();
  for ( ; s != t; s++)
    {
      mapthing_t *m = s->second;
      if (CheckRespawnSpot(p, m))
	{
	  SpawnPlayer(p, m);
	  return true;
	}
    }

  return false;
}


// tries to respawn the players waiting in respawnqueue
int Map::RespawnPlayers()
{
  // Let's try to empty the respawnqueue!
  // players in respawnqueue may or may not have pawns

  int count = 0;
  PlayerInfo *p;

  bool ok;
  do {
    //CONS_Printf("RespawnPlayers: %d, count = %d\n", respawnqueue.size(), count);
    p = respawnqueue.front();
    if (p == NULL)
      {
	// player has been removed while in respawn queue
	respawnqueue.pop_front();
	ok = true;
	continue;
      }

    ok = false;

    // spawn at random spot if in death match
    if (cv_deathmatch.value)
      {
	if (DeathMatchRespawn(p))
	  ok = true;
	else
	  ok = CoopRespawn(p);
      }
    else
      ok = CoopRespawn(p);

    if (ok)
      {
	respawnqueue.pop_front();
	count++;
      }
    else
      {
	// check that the requested entrypoint even exists
	multimap<int, mapthing_t *>::iterator s, t;	
	s = playerstarts.begin();
	t = playerstarts.end();
	for ( ; s != t; s++)
	  {
	    mapthing_t *m = s->second;
	    if (m->args[0] == p->entrypoint)
	      break;
	  }

	if (s == t)
	  {
	    CONS_Printf("Nonexistant entrypoint (%d) requested!\n", p->entrypoint);
	    p->entrypoint = 0;
	  }

	if (p->time++ > 10)
	  {
	    // this player has already waited for respawn for 10 ticks!
	    I_Error("Could not respawn player %d to entrypoint %d\n", p->number, p->entrypoint);
	  }
      }
  } while (ok && !respawnqueue.empty());

  return count;
}


// called when a dead player pushes USE
void Map::RebornPlayer(PlayerInfo *p)
{
  // at this point p->playerstate should be PST_INMAP or PST_FINISHEDMAP
  // and p->pawn not NULL

  // first dissociate the corpse
  if (p->pawn)
    {
      p->pawn->player = NULL;
      QueueBody(p->pawn);
      p->pawn = NULL;
    }

  p->pov = NULL;

  if (!game.multiplayer)
    {
      // in single player games, the map is reset after death
      info->state = MapInfo::MAP_RESET;
      RemovePlayer(p);
      p->requestmap = info->mapnumber; // get back here after the reset...
      p->playerstate = PST_NEEDMAP;
    }
  else
    {
      respawnqueue.push_back(p);
      p->time = 0; // respawn delay counter
      p->playerstate = PST_RESPAWN;
    }

  // should we load a saved playerpawn?  
  if (info->hub)
    p->LoadPawn();

  p->setMaskBits(PlayerInfo::M_PAWN); // notify network system
}


// Adds a player to a Map. The player is immediately queued for respawn.
// Only called from MapInfo.
void Map::AddPlayer(PlayerInfo *p)
{
  // At this point the player may or may not have a pawn.
  p->mp = this;
  players.push_back(p);

  if (!game.server)
    return;

  respawnqueue.push_back(p);
  p->time = 0; // respawn delay counter
  p->playerstate = PST_RESPAWN;
  p->requestmap = 0;

  // TODO if (p->spectator) spawn PST_SPECTATOR

  if (p->pawn)
    {
      p->pawn->eflags &= ~MFE_REMOVE;

      // save the playerpawns as they enter the map
      if (info->hub)
	p->SavePawn();
    }
}


// removes a player from map (but does not remove the pawn!)
// used by server in sp games, and by client
bool Map::RemovePlayer(PlayerInfo *p)
{
  vector<PlayerInfo *>::iterator i;
  for (i = players.begin(); i != players.end(); i++)
    if (*i == p)
      {
	p->mp = NULL;
	players.erase(i);
	return true;
      }
  return false;
}


// post-tick:
// Check if any players are leaving, handle the pawns properly, remove the players from the map.
int Map::HandlePlayers()
{
  if (info->state == MapInfo::MAP_RESET)
    return 0; // single player, special case

  // Clean respawnqueue from removed players
  int n = respawnqueue.size();
  for (int i=0; i<n; i++)
    if (respawnqueue[i]->playerstate == PST_REMOVE)
      respawnqueue[i] = NULL;

  unsigned fin = 0;
  int r = 0;

  // we iterate the vector in reversed order since we may have to delete some items on the way
  vector<PlayerInfo *>::iterator t;
  for (t = players.end(); t != players.begin(); )
    {
      --t; // since t was not equal to begin(), this must be valid

      PlayerInfo *p = *t;
      

      if (p->playerstate == PST_LEAVINGMAP || p->playerstate == PST_REMOVE)
	{
	  //CONS_Printf(" Map:: Removing player %s.\n", p->name.c_str());
	  r++;
	  players.erase(t);
	  p->mp = NULL;

	  // handle the pawn
	  PlayerPawn *w = p->pawn;
	  if (w)
	    switch (p->playerstate) 
	      {
	      case PST_LEAVINGMAP:
		if (w->flags & MF_CORPSE)
		  {
		    // drop the dead pawn
		    w->player = NULL;
		    QueueBody(w);
		    p->pawn = NULL;
		  }
		else
		  {
		    // save pawn for next level
		    w->Detach();
		  }

		// intermission?
		if (game.server)
		  {
		    // Starts an intermission for the player if appropriate.
		    MapInfo *next = game.FindMapInfo(p->requestmap); // do we need an intermission?

		    // only use intermissions if mapchanges are synchronous
		    if (next && cv_intermission.value)
		      {
			if (p->connection)
			  // nonlocal players need intermission data
			  // TODO need we send Map kills, items or secrets?
			  p->s2cStartIntermission(info->mapnumber, next->mapnumber, maptic, kills, items, secrets);
			else
			  {
			    // for locals, the intermission is started
			    wi.Start(info, next, maptic, kills, items, secrets);
			    game.StartIntermission();
			  }

			p->playerstate = PST_INTERMISSION; // c2sIntermissionDone rpc resets this
			return true;
		      }
		    else
		      p->playerstate = PST_NEEDMAP;
		  }
		break;

	      case PST_REMOVE:
		w->Remove();
		p->pawn = NULL;
		break;

	      default:
		// not possible
		break;
	      }
	}
      else if (p->playerstate == PST_FINISHEDMAP)
	fin++;
    }

  if (players.size() != fin)
    return r;

  // TODO handle spectators...

  // all the remaining players are finished, so let's finish the Map.
  if (game.server)
    info->state = MapInfo::MAP_FINISHED;
  else
    info->state = MapInfo::MAP_INSTASIS; // TEST


  return r;
}



// returns player 'number' if he is in the map, otherwise NULL
PlayerInfo *Map::FindPlayer(int num)
{
  int i, n = players.size();
  for (i = 0; i < n; i++)
    if (players[i]->number == num)
      return players[i];

  return NULL;
}



void Map::QueueBody(Actor *p)
{
  p->flags2 &= ~MF2_DONTDRAW;
  bodyqueue.push_back(p);

  // flush an old corpse if needed
  while (bodyqueue.size() > cv_bodyqueue_size.value)
    {
      bodyqueue.front()->Remove();
      bodyqueue.pop_front();
    }
}


/// Kills all monsters.
int Map::Massacre()
{
  int count = 0;

  for (Thinker *th = thinkercap.next; th != &thinkercap; th = th->next)
    {
      Actor *a = th->Inherits<Actor>();
      if (!a)
	continue; // Not an actor
      
      if ((a->flags & MF_MONSTER) && (a->health > 0))
	{
	  a->flags2 &= ~(MF2_NONSHOOTABLE + MF2_INVULNERABLE);
	  a->flags |= MF_SHOOTABLE;
	  a->Damage(NULL, NULL, 10000, dt_always);
	  count++;
	}
    }
  return count;
}


// helper function for Map::BossDeath
static const state_t *P_FinalState(const state_t *state)
{
  while (state->tics != -1)
    state = state->nextstate;

  return state;
}


void Map::BossDeath(const DActor *mo)
{
  // TODO! This is how it is supposed to work:
  // A map knows what it should do when, say, the last monster of type X dies.

  // It may be doing something to tags 666 or 667, or maybe even exiting the
  // map (but not necessarily the cluster!)
  // But how to code it generally and efficiently?
  // Maybe just copy existing Doom/Doom2/Heretic 666 and 667 tricks and do everything else by FS?

  // Ways to end level:
  // Baron of Hell, Cyberdemon, Spider Mastermind,
  // Mancubus, Arachnotron, Keen, Brain

  int key = info->BossDeathKey;
  if (key == 0)
    return; // no action taken

  int b = 0;

  // cyborgs, spiders and ironliches have two different ways of acting
  switch (mo->type)
    {
    case MT_BRUISER:
      b = 1; break;
    case MT_CYBORG:
      b = 2+4; break;
    case MT_SPIDER:
      b = 8+16; break;
    case MT_FATSO:
      b = 32; break;
    case MT_BABY:
      b = 64; break;
    case MT_KEEN:
      b = 128; break;
    case MT_BOSSBRAIN:
      b = 256; break;
    case MT_HHEAD:
      b = 0x200+0x400; break;
    case MT_MINOTAUR:
      b = 0x800; break;
    case MT_SORCERER2:
      b = 0x1000; break;
    default:
      return;
    }

  if ((key & b) == 0)
    // wrong boss type for this level
    return;
 
  int i, n = players.size();

  // make sure there is a player alive for victory
  for (i=0 ; i<n ; i++)
    if ((players[i]->playerstate == PST_INMAP ||
	 players[i]->playerstate == PST_FINISHEDMAP) &&
	players[i]->pawn && !(players[i]->pawn->flags & MF_CORPSE))
      break;

  if (i == n)
    return; // no one left alive, so do not end game


  const state_t *finalst = P_FinalState(mo->info->deathstate);

  // scan the remaining thinkers to see
  // if all bosses are dead
  for (Thinker *th = thinkercap.next ; th != &thinkercap ; th=th->next)
    {
      DActor *a = th->Inherits<DActor>();
      if (!a)
	continue;

      if (a != mo && a->type == mo->type
	  // && a->health > 0           // the old one (doom original 1.9)
	  // && !(a->flags & MF_CORPSE) // the Heretic one
	  && a->state != finalst)
	// this is better because a thing becomes MF_CORPSE while still falling down.
	// We want to see the deaths completely.
        {
	  // other boss not dead
	  return;
        }
    }

  // victory!

  switch (mo->type)
    {
    case MT_BRUISER:
      EV_DoFloor(666, NULL, floor_t::LnF, -FLOORSPEED, 0, 0);
      return;

    case MT_CYBORG:
      if ((key & 2) != 0)
	break;
      else
	{
	  // used in ult. Doom, map 6
	  EV_DoDoor(666, NULL, NULL, vdoor_t::Open | vdoor_t::Blazing, 4*VDOORSPEED, VDOORWAIT);
	  return;
	}

    case MT_SPIDER:
      if ((key & 8) != 0)
	break;
      else
	{
	  // ult. Doom, map 8
	  EV_DoFloor(666, NULL, floor_t::LnF, -FLOORSPEED, 0, 0);
	  return;
	}

    case MT_FATSO:
      EV_DoFloor(666, NULL, floor_t::LnF, -FLOORSPEED, 0, 0);
      return;

    case MT_BABY:
      EV_DoFloor(667, NULL,  floor_t::UpSLT, FLOORSPEED, 0, 0); // correct
      return;

    case MT_KEEN:
      EV_DoDoor(666, NULL, NULL, vdoor_t::Open, VDOORSPEED, VDOORWAIT);
      return;

    case MT_BOSSBRAIN:
      break;

    case MT_HHEAD:
      if ((key & 0x400) == 0)
	goto nomassacre;
    case MT_MINOTAUR:
    case MT_SORCERER2:
      // if (gameepisode > 1)
      // Kill any remaining monsters
      Massacre();
    nomassacre:
      EV_DoFloor(666, NULL, floor_t::LnF, -FLOORSPEED, 0, 0);
      return;

    default:
      // no action taken
      return;
    }

  ExitMap(NULL, 0); // to the next level
}



// The fields of the mapthing are already in host byte order.
Actor *ActorInfo::Spawn(Map *m, mapthing_t *mt, bool initial) const
{
  // don't spawn keycards in deathmatch
  if (cv_deathmatch.value && (flags & MF_NOTDMATCH))
    return NULL;

  // don't spawn any monsters if -nomonsters
  if (cv_nomonsters.value && (flags & MF_MONSTER))
    return NULL;

  // spawn it
  fixed_t nx = mt->x;
  fixed_t ny = mt->y;
  fixed_t nz;

  if (flags & MF_SPAWNCEILING)
    nz = ONCEILINGZ;
  else if (flags & MF_SPAWNFLOAT)
    nz = FLOATRANDZ;
  else
    nz = ONFLOORZ;

  DActor *p = new DActor(nx, ny, nz, this);
  m->SpawnActor(p, mt->height); // set pos.z and floorz, among other things...

  // Seed random starting index for bobbing motion
  if (flags2 & MF2_FLOATBOB)
    {
      p->reactiontime = P_Random(); // randomize phase
      p->special1 = (p->pos.z - p->floorz).floor(); // floating height above floor (integer, NOT fixed_t) 
    }

  mt->mobj = p;
  p->spawnpoint = mt;
  p->special = mt->special;
  p->args[0] = mt->args[0];
  p->args[1] = mt->args[1];
  p->args[2] = mt->args[2];
  p->args[3] = mt->args[3];
  p->args[4] = mt->args[4];

  // yaw
  if (flags & MF_MONSTER)
    p->yaw = ANG45 * (mt->angle/45);
  else
    p->yaw = ((mt->angle << 8)/360) << 24; // full angle resolution

  // do the flags
  if (mt->flags & MTF_AMBUSH)
    p->flags |= MF_AMBUSH;

  if (m->hexen_format && (mt->flags & MTF_DORMANT))
    {
      p->flags2 |= MF2_DORMANT;
      if (GetMobjType() == MT_ICEGUY)
	p->SetState(S_ICEGUY_DORMANT, true);
      p->tics = -1;
    }

  // randomize initial tics
  if (p->tics > 0)
    p->tics = 1 + (P_Random () % p->tics);


  // the rest could be done within Map
  if (initial)
    {
      // NOTE: To make Hexen maps with TID counting scripts playable, respawned monsters get no TID.
      p->tid = mt->tid;
      if (p->tid)
	m->InsertIntoTIDmap(p, p->tid);

      if (flags & MF_COUNTKILL)
	m->kills++;
      if (flags & MF_COUNTITEM)
	m->items++;
    }
  else
    {
      // spawn teleport fog (not in Heretic?)
      Actor *fog = m->SpawnDActor(nx, ny, p->pos.z + (game >= gm_heretic ? TELEFOGHEIGHT : 0), MT_IFOG);
      S_StartSound(fog, sfx_itemrespawn);
    }

  return p;
}



// try respawning items from the itemrespawnqueue
void Map::RespawnSpecials()
{
  // nothing left to respawn?
  if (itemrespawnqueue.empty())
    return;

  // the first item in the queue is the first to respawn
  if (maptic - itemrespawntime.front() < tic_t(cv_itemrespawntime.value)*TICRATE)
    return;

  mapthing_t *mthing = itemrespawnqueue.front();
  if (mthing != NULL)
    mthing->ai->Spawn(this, mthing, false);

  // pull it from the queue anyway
  itemrespawnqueue.pop_front();
  itemrespawntime.pop_front();
}


// used when we are going from deathmatch 2 to deathmatch 1
// picks out all weapons from itemrespawnqueue and respawns them
void Map::RespawnWeapons()
{
  deque<mapthing_t *>::iterator i = itemrespawnqueue.begin();

  for ( ; i != itemrespawnqueue.end(); i++)
    {
      mapthing_t *mthing = *i;
      if (!mthing)
	continue;

      switch (mthing->ai->GetMobjType())
	{
	case MT_SHAINSAW:
	case MT_SHOTGUN:
	case MT_SUPERSHOTGUN:
	case MT_CHAINGUN:
	case MT_ROCKETLAUNCH:
	case MT_PLASMAGUN:
	case MT_BFG9000:
	case MT_WGAUNTLETS:
	case MT_WCROSSBOW:
	case MT_WBLASTER:
	case MT_WPHOENIXROD:
	case MT_WSKULLROD:
	case MT_WMACE:
	case MT_FW_AXE:
	case MT_CW_SERPSTAFF:
	case MT_MW_CONE:
	case MT_FW_HAMMER:
	case MT_CW_FLAME:
	case MT_MW_LIGHTNING:
	case MT_FW_SWORD1:
	case MT_FW_SWORD2:
	case MT_FW_SWORD3:
	case MT_CW_HOLY1:
	case MT_CW_HOLY2:
	case MT_CW_HOLY3:
	case MT_MW_STAFF1:
	case MT_MW_STAFF2:
	case MT_MW_STAFF3:
	  // it's a weapon, remove it from queue and respawn it!
	  *i = NULL; // stays in sync with itemrespawntime...
	  mthing->ai->Spawn(this, mthing, false);
	  break;

	default:
	  // not a weapon, continue search
	  break;
	}
    }
}


// Called when a Map exit method is activated, such as:
// exit teleporter, endgame teleporter, E1M8 sectordamage, bossdeath, timelimit
void Map::ExitMap(Actor *activator, int next, int ep)
{
  //CONS_Printf("ExitMap => %d, %d\n", next, ep);

  if (!game.server || !cv_exitmode.value)
    return; // exit not allowed

  if (next == 0)
    next = info->nextlevel; // zero means "normal exit"

  if (next == 100)
    next = info->secretlevel; // 100 means "secret exit"

  // HACK...

  PlayerPawn *p;
  PlayerInfo *quitter = (activator && (p = activator->Inherits<PlayerPawn>())) ? p->player : NULL;

  int mode = quitter ? cv_exitmode.value : 1; // timed exit, bossdeath => everyone leaves at once.

  // laitetaan joko m_fin tai p_leavmap, aina reqmap

  switch (mode)
    {
    case 1: // first: all players exit the map when someone activates an exit
      {
	int n = players.size();
	for (int i = 0; i<n; i++)
	  players[i]->ExitLevel(next, ep); // save the pawn if needed
      }
      break;

    case 2: // ind: players may exit individually
      // the new maps are launched immediately, the old map keeps running until it is empty.
      quitter->ExitLevel(next, ep);
      break;

    case 3: // last: all players need to reach the exit, others have to wait for the last one
      quitter->playerstate = PST_FINISHEDMAP;
      quitter->requestmap = next;
      quitter->entrypoint = ep;

    default:
      break;
    }
}


void Map::UpdateSpecials()
{
  //  LEVEL TIMER
  if (cv_timelimit.value && maptic > unsigned(cv_timelimit.value))
    ExitMap(NULL, 0);

  if (info->lightning)
    effects->LightningFlash();
}


//==========================================================================
// TID system (Hexen)
//==========================================================================


void Map::InsertIntoTIDmap(Actor *p, int tid)
{
  if (TID_map.size() >= 300)
    I_Error("Map::InsertIntoTIDmap: MAX_TID_COUNT (%d) exceeded.", 300);

  // TODO multiple inserts possible
  TID_map.insert(pair<const short, Actor*>(tid, p));
}

void Map::RemoveFromTIDmap(Actor *p)
{
  if (p->tid == 0)
    return;

  int tid = p->tid;
  p->tid = 0;

  multimap<short, Actor*>::iterator i, j;
  i = TID_map.lower_bound(tid);
  if (i == TID_map.end())
    return; // not found (early out)

  j = TID_map.upper_bound(tid);

  for ( ; i != j; ++i)
    if (i->second == p)
      {
	TID_map.erase(i);
	return;
      }
  // not found
}
