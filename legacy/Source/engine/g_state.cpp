// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: g_state.cpp 533 2009-06-11 08:31:10Z smite-meister $
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
/// \brief Part of GameInfo class implementation.
/// Methods related to game state changes.

#include "g_game.h"
#include "g_player.h"
#include "g_team.h"
#include "g_map.h"
#include "g_mapinfo.h"
#include "g_level.h"
#include "g_pawn.h"

#include "command.h"
#include "cvars.h"

#include "n_connection.h"

#include "am_map.h"
#include "f_finale.h"
#include "hud.h"
#include "wi_stuff.h"



// WARNING : check cv_fraglimit>0 before call this function !
bool GameInfo::CheckScoreLimit()
{
  // only checks score, doesn't count it. score must therefore be
  // updated in real time
  if (cv_teamplay.value)
    {
      int i, n = teams.size();
      for (i=0; i<n; i++)
	if (teams[i]->score >= cv_fraglimit.value)
	  return true;
    }
  else
    {
      player_iter_t i;
      for (i = Players.begin(); i != Players.end(); i++)
	if ((*i).second->score >= cv_fraglimit.value)
	  return true;
    }
  return false;
}



int GameInfo::GetFrags(fragsort_t **fragtab, int type)
{
  // PlayerInfo class holds a raw frags table for each player.
  // It also holds a cumulative, game type sensitive frag field named score
  // if the game type is changed in the middle of a game, score is NOT zeroed,
  // just the scoring system is different from there on.

  // scoring is done in PlayerPawn::Kill

  int i, j;
  int n = Players.size();

  int ret;
  fragsort_t *ft;

  player_iter_t u, w;

  if (cv_teamplay.value)
    {
      int m = teams.size();
      ft = new fragsort_t[m];
	  
      //const int cm = m;
      int teamfrags[m][m];

      for (i=0; i<m; i++)
	{
	  ft[i].num   = i; // team 0 are the unteamed
	  ft[i].color = teams[i]->color;
	  ft[i].name  = teams[i]->name.c_str();
	}

      // calculate teamfrags
      for (u = Players.begin(); u != Players.end(); u++)
	{
	  int team1 = (*u).second->team;

	  for (w = Players.begin(); w != Players.end(); w++)
	    {
	      int team2 = (*w).second->team;

	      teamfrags[team1][team2] +=
		(*u).second->Frags[(*w).first];
	    }
	}

      // type is a magic number telling which fragtable we want
      switch (type)
	{
	case 0: // just normal frags
	  for (i=0; i<m; i++)
	    ft[i].count = teams[i]->score;
	  break;

	case 1: // buchholtz
	  for (i=0; i<m; i++)
	    {
	      ft[i].count = 0;
	      for (j=0; j<m; j++)
		if (i != j)
		  ft[i].count += teamfrags[i][j] * teams[j]->score;
	    }
	  break;

	case 2: // individual
	  for (i=0; i<m; i++)
	    {
	      ft[i].count = 0;
	      for (j=0; j<m; j++)
		if (i != j)
		  {
		    if(teamfrags[i][j] > teamfrags[j][i])
		      ft[i].count += 3;
		    else if(teamfrags[i][j] == teamfrags[j][i])
		      ft[i].count += 1;
		  }
	    }
	  break;

	case 3: // deaths
	  for (i=0; i<m; i++)
	    {
	      ft[i].count = 0;
	      for (j=0; j<m; j++)
		ft[i].count += teamfrags[j][i];
	    }
	  break;
      
	default:
	  break;
	}

      ret = m;
    }
  else
    {
      // not teamgame
      ft = new fragsort_t[n]; 
      for (i = 0, u = Players.begin(); u != Players.end(); i++, u++)
	{
	  PlayerInfo *p = u->second;
	  ft[i].num   = p->number;
	  ft[i].color = p->options.color;
	  ft[i].name  = p->name.c_str();
	}

      // type is a magic number telling which fragtable we want
      switch (type)
	{
	case 0: // just normal frags
	  for (i = 0, u = Players.begin(); u != Players.end(); i++, u++)
	    ft[i].count = (*u).second->score;
	  break;

	case 1: // buchholtz
	  for (i = 0, u = Players.begin(); u != Players.end(); i++, u++)
	    {
	      ft[i].count = 0;
	      for (w = Players.begin(); w != Players.end(); w++)
		if (u != w)
		  {
		    int k = (*w).first;
		    ft[i].count += (*u).second->Frags[k]*((*w).second->score + (*w).second->Frags[k]);
		    // FIXME is this formula correct?
		  }
	    }
	  break;

	case 2: // individual
	  for (i = 0, u = Players.begin(); u != Players.end(); i++, u++)
	    {
	      ft[i].count = 0;
	      for (w = Players.begin(); w != Players.end(); w++)
		if (u != w)
		  {
		    int k = (*u).first;
		    int l = (*w).first;
		    if ((*u).second->Frags[l] > (*w).second->Frags[k])
		      ft[i].count += 3;
		    else if ((*u).second->Frags[l] == (*w).second->Frags[k])
		      ft[i].count += 1;
		  }
	    }
	  break;

	case 3: // deaths
	  for (i = 0, u = Players.begin(); u != Players.end(); i++, u++)
	    {
	      ft[i].count = 0;
	      for (w = Players.begin(); w != Players.end(); w++)
		ft[i].count += (*w).second->Frags[(*u).first];
	    }
	  break;
      
	default:
	  break;
	}
      ret = n;
    }

  *fragtab = ft; 
  return ret;
}


// ticks the game forward in time
void GameInfo::Ticker()
{
  tic++;

  // TODO read/write demo ticcmd's here

  // do main actions
  switch (state)
    {
    case GS_INTRO:
      if (--pagetic <= 0)
	AdvanceIntro();
      break;

    case GS_LEVEL:
      if (!paused && currentcluster)
	currentcluster->Ticker();

      if (!dedicated)
	{
	  hud.Ticker();
	  automap.Ticker();
	}
      break;

    case GS_INTERMISSION:
      wi.Ticker();
      break;

    case GS_FINALE:
      F_Ticker();
      break;

    case GS_NULL:
    default:
      // do nothing
      break;
    }


  MapInfo *m;
  PlayerInfo *p;

  if (state != GS_LEVEL)
    return;

  // manage players
  for (player_iter_t t = Players.begin(); t != Players.end(); )
    {
      p = t->second;
      t++; // because "old t" may be invalidated

      if (p->playerstate == PST_REMOVE)
	{
	  // the player is removed from the game (invalidates "old t")
	  if (!p->mp)
	    RemovePlayer(p->number); // first the maps throw out the removed player, then the game proper.

	  // TODO purge the removed players from the frag maps of other players?
	}
      else
	p->Ticker();
    }

  if (!server)
    return;

  //
  for (player_iter_t t = Players.begin(); t != Players.end(); t++)
    {
      p = t->second;

      if (p->playerstate == PST_NEEDMAP)
	{
	  LConnection *conn = p->connection;
	  if (conn && !conn->isGhostAvailable(p))
	    {
	      // remote player, is it already being ghosted? if not, wait.
	      CONS_Printf(" server waiting for client ghost\n");
	      continue;
	    }

	  // assign the player to a map
	  CONS_Printf("Map request..");

	  if (p->requestmap == 0)
	    {
	      m = initial_map; // first map in game
	      p->entrypoint = initial_ep;
	    }
	  else
	    m = FindMapInfo(p->requestmap);

	  if (!m)
	    {
	      // game ends
	      currentcluster->Finish(p->requestmap, p->entrypoint);
	      StartFinale(NULL);
	      break;
	    }
	  else if (!m->found)
	    m = currentcluster->maps[0]; // TODO or give an error?

	  // cluster change?
	  if (currentcluster->number != m->cluster)
	  {
	      // TODO minor thing: if several players exit maps on the same tick,
	      // and someone besides the first one causes a cluster change, some
	      // maps could be loaded in vain...

	      // cluster change!
	      currentcluster->Finish(p->requestmap, p->entrypoint);

	      MapCluster *next = FindCluster(m->cluster);
	      StartFinale(next);
	      currentcluster = next;

	      break; // this is important! no need to check the other players.
	  }

	  p->Reset(!currentcluster->keepstuff, true);

	  // normal individual mapchange
	  if (!m->Activate(p))
	    I_Error("Darn!\n");

	  if (conn)
	  {
	      CONS_Printf(" server sending rpc\n");
	      // nonlocal player enters a new map, notify client
	      // send the EnterMap rpc only to the owning connection
	      NetEvent *e = TNL_RPC_CONSTRUCT_NETEVENT(p, s2cEnterMap, (m->mapnumber));
	      conn->postNetEvent(e);
	  }  
	}
 }
 //CONS_Printf("[%s][%d]::Ticker -> End Routine\n",__FILE__,__LINE__);
}



// called as a result of the rpc
void GameInfo::StartIntermission()
{
  SetState(GS_INTERMISSION);
}


//
void GameInfo::EndIntermission()
{
  SetState(GS_LEVEL);

  for (int i = 0; i < NUM_LOCALPLAYERS; i++)
    {
      PlayerInfo *p = LocalPlayers[i].info;
      if (p && p->playerstate == PST_INTERMISSION)
	{
	  if (server)
	    p->playerstate = PST_NEEDMAP;
	  else
	    p->c2sIntermissionDone();
	}
    }
}


void GameInfo::StartFinale(MapCluster *next)
{
  // check need for finale
  if (cv_deathmatch.value == 0)
  {
    CONS_Printf("[%s][%d]::StartFinale\n",__FILE__,__LINE__);
      // check winning
    if (!next)
    {
      CONS_Printf("[%s][%d]::StartFinale\n",__FILE__,__LINE__);
      SetState(GS_FINALE);
      F_StartFinale(currentcluster, false, true); // final piece of story is exittext
      CONS_Printf("[%s][%d]::StartFinale\n",__FILE__,__LINE__);      
      return;
    }
      
    CONS_Printf("[%s][%d]::StartFinale\n",__FILE__,__LINE__);      
    int c = currentcluster->number;
    int n = next->number;
    // check "mid-game finale" (story) (requires cluster change)
    if (n != c)
    {
      CONS_Printf("[%s][%d]::StartFinale\n",__FILE__,__LINE__);
      if (!next->entertext.empty())
      {
        CONS_Printf("[%s][%d]::StartFinale\n",__FILE__,__LINE__);
        SetState(GS_FINALE);
        F_StartFinale(next, true, false);
        CONS_Printf("[%s][%d]::StartFinale\n",__FILE__,__LINE__);        
        return;
      }
      else if (!(currentcluster->exittext.empty()))
      {
        CONS_Printf("[%s][%d]::StartFinale\n",__FILE__,__LINE__);
        SetState(GS_FINALE);
        F_StartFinale(currentcluster, false, false);
        CONS_Printf("[%s][%d]::StartFinale\n",__FILE__,__LINE__);        
        return;
      }
    }
  }
  else
  {
    // no finales in deathmatch
    // FIXME end game here, show final frags
    //if (nextcluster == NULL)
    //CL_Reset();
  }
  CONS_Printf("[%s][%d]::StartFinale\n",__FILE__,__LINE__);  
}


void GameInfo::EndFinale()
{
  SetState(GS_LEVEL);
}
