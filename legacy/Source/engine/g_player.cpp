// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
//  $Id: g_player.cpp 527 2008-01-27 02:25:14Z smite-meister $
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
/// \brief PlayerInfo class implementation

#include "tnl/tnlNetObject.h"
#include "doomdef.h"
#include "command.h"
#include "cvars.h"

#include "g_player.h"
#include "g_game.h"
#include "g_map.h"
#include "g_mapinfo.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "g_decorate.h"
#include "b_bot.h"

#include "a_functions.h"

#include "n_connection.h"
#include "n_interface.h"

#include "r_sprite.h"
#include "hardware/md3.h"

#include "wi_stuff.h"
#include "tables.h"
#include "z_zone.h"
#include "hud.h"

//============================================================
//           Global (nonstatic) client data
//============================================================

/// Local human players, then local bot player
LocalPlayerInfo LocalPlayers[NUM_LOCALPLAYERS] =
{
  // these constructors are required only define correct control key sets for the human players, otherwise the default constructor is enough
  LocalPlayerInfo("Bruce", 0), LocalPlayerInfo("Dick", 1), LocalPlayerInfo("Alfred", 2), LocalPlayerInfo("Barbara", 3)
};

/// Locally observed players (multiple viewports...)
vector<PlayerInfo *> ViewPlayers;


bool G_RemoveLocalPlayer(PlayerInfo *p)
{
  if (p)
    {
      // remove the given player
      for (int k=0; k < NUM_LOCALPLAYERS; k++)
	if (LocalPlayers[k].info == p)
	  {
	    LocalPlayers[k].info = NULL;
	    return true;
	  }

      return false;
    }

  // remove all players
  for (int k=0; k < NUM_LOCALPLAYERS; k++)
    LocalPlayers[k].info = NULL;

  return true;
}


//============================================================
//   PlayerOptions class
//============================================================

static char default_weaponpref[NUMWEAPONS] =
{
  1,4,5,6,8,7,3,9,2,  // Doom
  1,4,5,7,8,3,9,6,0,  // Heretic
  2,2,2,4,4,4,6,6,6,0,0,0,0 // Hexen
};


PlayerOptions::PlayerOptions(const string &n)
{
  name = n;

  ptype = -1;
  color = 0;
  skin  = 0; // "marine"?

  autoaim = true;
  originalweaponswitch = true;
  for (int i=0; i<NUMWEAPONS; i++)
    weaponpref[i] = default_weaponpref[i];

  messagefilter = 2;
}


/// Sends the player preferences to the server.
void PlayerOptions::Write(BitStream *stream)
{
  stream->writeString(name.c_str());
  stream->write(ptype);
  stream->write(color);
  stream->write(skin);

  stream->write(autoaim);
  stream->write(originalweaponswitch);
  for (int i=0; i<NUMWEAPONS; i++)
    stream->write(weaponpref[i]);

  stream->write(messagefilter);
}

/// server reads remote player preferences sent by client
void PlayerOptions::Read(BitStream *stream)
{
  char temp[256];

  stream->readString(temp);
  temp[32] = '\0'; // limit name length
  name = temp;

  stream->read(&ptype);
  stream->read(&color);
  stream->read(&skin);

  stream->read(&autoaim);
  stream->read(&originalweaponswitch);
  for (int i=0; i<NUMWEAPONS; i++)
    stream->read(&weaponpref[i]);

  stream->read(&messagefilter);
}



//============================================================
//   LocalPlayerInfo class
//============================================================

LocalPlayerInfo::LocalPlayerInfo(const string &n, int keyset)
  : PlayerOptions(n)
{
  controlkeyset = keyset;
  autorun = false;
  crosshair = 0;

  info = NULL;
  ai = NULL;
}


/// Builds the control struct (ticcmd) based on human input or AI decisions
void LocalPlayerInfo::GetInput(int elapsed)
{
  if (!info)
    return;

  // inventory timer (TODO does it belong here?)
  if (info->invTics)
    info->invTics--;

  if (ai)
    ai->BuildInput(info, elapsed);
  else
    info->cmd.Build(this, elapsed);
}


// Propagates the preferences to the PlayerInfo or sends them to the server.
void LocalPlayerInfo::UpdatePreferences()
{
  if (!info)
    return; // nothing else to do if we have no PlayerInfo yet

  if (game.server)
    {
      info->options = *this;
      info->name = name;
      info->setMaskBits(PlayerInfo::M_IDENTITY);
    }
  else
    game.net->SendPlayerOptions(info->number, *this);
}



//============================================================
//   PlayerInfo class
//============================================================

TNL_IMPLEMENT_NETOBJECT(PlayerInfo);

PlayerInfo::PlayerInfo(const LocalPlayerInfo *p)
{
  if (p)
    {
      options = *p;
      name = options.name;
    }

  number = 0;
  team = 0;

  connection = NULL;
  client_hash = 0;

  playerstate = PST_NEEDMAP;
  spectator = false;

  requestmap = entrypoint = 0;

  mp = NULL;
  pawn = NULL;
  pov = NULL;
  hubsavepawn = NULL;

  viewz = viewheight = deltaviewheight = 0;
  palette = -1;
  damagecount = bonuscount = 0;
  itemuse = 0;

  Reset(false, true);  // clear score, frags...

  cmd.Clear();
  invTics = invSlot = invPos = 0;

  // net stuff
  mNetFlags.set(Ghostable);
};


/// This is called on clients when a new player joins
bool PlayerInfo::onGhostAdd(class GhostConnection *c)
{
  CONS_Printf("%s has joined the game (player %d)\n", name.c_str(), number);
  game.AddPlayer(this);

  if (!LConnection::joining_players.empty())
    {
      list<LocalPlayerInfo *>::iterator t = LConnection::joining_players.begin();
      for ( ; t != LConnection::joining_players.end(); t++)
	{
	  LocalPlayerInfo *p = *t;
	  if (p->pnumber == number)
	    {
	      if (p->info)
		I_Error("Multiple ghostadds!\n");

	      p->info = this;
	      LConnection::joining_players.erase(t); // the PlayerInfo found its owner

	      ViewPlayers.push_back(this); // let's see it too!
	      //hud.ST_Start(this); // TODO
	      break;
	    }
	}
    }
  // NOTE playerstate should not mean anything to the client...yet?

  return true;
}


/// This is called on clients when a player leaves
void PlayerInfo::onGhostRemove()
{
  CONS_Printf("%s has left the game.\n", name.c_str());
  //game.RemovePlayer(number);
  playerstate = PST_REMOVE;
}


/// This is the function the server uses to ghost PlayerInfo data to clients.
U32 PlayerInfo::packUpdate(GhostConnection *c, U32 mask, class BitStream *stream)
{
  if (isInitialUpdate())
    mask = M_IDENTITY; // everything else is at default value at this point


  int ret = 0;
  // check which states need to be updated, and write updates

  if (stream->writeFlag(mask & M_IDENTITY))
    {
      // very rarely
      CONS_Printf("---PI: sent identity\n");
      stream->writeString(name.c_str());
      stream->write(number);
      stream->write(team);
    }

  if (mask & M_PAWN)
    {
      // rarely
      S32 idx = c->getGhostIndex(pawn); // these indices replace pointers to other ghosts!
      S32 idx2 = c->getGhostIndex(pov);

      if (stream->writeFlag(idx != -1 && idx2 != -1))
	{
	  stream->write(idx);
	  stream->write(idx2);
	  CONS_Printf("---PI: sent pawn\n");
	}
      else
	{
	  ret |= M_PAWN; // try again later
	  CONS_Printf("---PI: postponed pawn send\n");
	}
    }
  else
    stream->writeFlag(false);

  if (stream->writeFlag(mask & M_SCORE))
    {
      CONS_Printf("---PI: sent score\n");
      // occasionally
      stream->write(score);
      // kills, items, secrets?
    }

  // feedback (goes only to the owner)
  if (c != connection)
    {
      stream->writeFlag(false); // nothing further for others
      return 0;
    }
  else
    stream->writeFlag(true);


  if (stream->writeFlag(mask & M_PALETTE))
    {
      // often
      stream->write(palette);
      palette = -1;
    }

  if (stream->writeFlag(mask & M_HUDFLASH))
    {
      // often
      // palette change overrides flashes
      if (stream->writeFlag(damagecount))
	stream->write(damagecount);
      if (stream->writeFlag(bonuscount))
	stream->write(bonuscount);

      stream->writeFlag(itemuse);
    }




  // the return value from packUpdate can set which states still
  // need to be updated for this object.
  return ret;
}


///
void PlayerInfo::unpackUpdate(GhostConnection *c, BitStream *stream)
{
  char temp[256];

  // NOTE: the unpackUpdate function must be symmetrical to packUpdate
  if (stream->readFlag()) // M_IDENTITY
    {
      CONS_Printf("---PI: got identity\n");
      stream->readString(temp);
      name = temp;
      stream->read(&number);
      stream->read(&team);
    }

  if (stream->readFlag()) // M_PAWN
    {
      CONS_Printf("---PI: got pawn\n");
      S32 idx;
      stream->read(&idx);
      pawn = reinterpret_cast<PlayerPawn *>(c->resolveGhost(idx)); // these indices replace pointers to other ghosts!

      stream->read(&idx);
      pov = reinterpret_cast<Actor *>(c->resolveGhost(idx));
    }

  if (stream->readFlag()) // M_SCORE
    {
      CONS_Printf("---PI: got score\n");
      stream->read(&score);
    }

  if (!stream->readFlag())
    return; // nothing more

  // feedback

  if (stream->readFlag()) // M_PALETTE
    stream->read(&palette);

  if (stream->readFlag()) // M_HUDFLASH
    {
      // palette change overrides flashes
      if (stream->readFlag())
	stream->read(&damagecount);
      if (stream->readFlag())
	stream->read(&bonuscount);

      itemuse = stream->readFlag();
    }


}


/// server notifies the client that this player has entered a new map
PLAYERINFO_RPC_S2C(s2cEnterMap, (U8 mapnum), (mapnum))
{
  CONS_Printf("player sent to map %d\n", mapnum);

  MapInfo *m = game.FindMapInfo(mapnum);
  if (!m)
    I_Error("Server sent a player to an unknown map %d!", mapnum);

  //requestmap = mapnum;
  /*
  if (mp)
    playerstate = PST_LEAVINGMAP;
  else
    playerstate = PST_NEEDMAP;
  */

  if (mp)
    mp->RemovePlayer(this);

  if (!m->Activate(this)) // clientside map activation
    I_Error("Crap!\n");

  // TEST FIXME
  game.currentcluster = game.FindCluster(m->cluster);
}


PLAYERINFO_RPC_S2C(s2cStartIntermission, (U8 finished, U8 next, U32 maptic, U32 kills, U32 items, U32 secrets), (finished, next, maptic, kills, items, secrets))
{
  CONS_Printf("server ordered intermission for player %d\n", number);

  MapInfo *f = game.FindMapInfo(finished);
  MapInfo *n = game.FindMapInfo(next);

  if (f && n)
    {
      wi.Start(f, n, maptic, kills, items, secrets);
      game.StartIntermission();
    }
  else
    game.EndIntermission();
}


PLAYERINFO_RPC_C2S(c2sIntermissionDone, (), ())
{
  CONS_Printf("client player %d has finished intermission\n", number);
  playerstate = PST_NEEDMAP; 
}



void PlayerInfo::Ticker()
{
  if (bonuscount > 0)
    bonuscount--;

  if (damagecount > 100)
    damagecount = 100;  // teleport stomp does 10k points...
  else if (damagecount > 0)
    damagecount--;

  // inventory
  if (itemuse > 0)
    itemuse--;
}

// send a message to the player
void PlayerInfo::SetMessage(const char *msg, int priority, int type, int extradata)
{
  if (priority > options.messagefilter)
    return;  // not interested (lesser is more important!)

  if (connection)
    connection->rpcMessage_s2c(number, msg, priority, type); // send it over network (usually server does this)
  else if (messages.size() < 20)
    {
      // the player is local and has room in her message queue
      // TODO high priority overrides low priority messages?
      message_t temp;
      temp.msg = msg; // makes a copy of msg, so we can use va() etc.
      temp.priority = priority;
      temp.type = type;
      temp.extradata = extradata;

      messages.push_back(temp);
    }
}


// Sets the new destination Map for the player. Does _not_ finish the current Map.
void PlayerInfo::ExitLevel(int nextmap, int ep)
{
  // if a player is already going somewhere, let him keep his destination:
  if (!requestmap)
    {
      requestmap = nextmap;
      entrypoint = ep;
    }

  playerstate = PST_LEAVINGMAP;
}




// Reset players between levels
void PlayerInfo::Reset(bool rpawn, bool rfrags)
{
  kills = items = secrets = time = 0;

  if (pawn)
    {
      if (rpawn)
	pawn->Reset();
      pawn->powers[pw_allmap] = 0; // automap never carries over to the next map 
    }

  pov = NULL;
  setMaskBits(PlayerInfo::M_PAWN);

  // Initial height of PointOfView
  // will be set by player think.
  viewz = 1;

  if (rfrags)
    {
      score = 0;
      Frags.clear();
      setMaskBits(PlayerInfo::M_SCORE);
    }

  return;
}




/// Client: Calculate the walking / running viewpoint bobbing and weapon swing
void PlayerInfo::CalcViewHeight()
{
  if (!pawn)
    return;

  bool onground = (pawn->Feet() <= pawn->floorz);

  const fixed_t MAXBOB = 16; // 16 pixels of bob
  const fixed_t FLYBOB = 0.5f;

  // Regular movement bobbing
  // basically pawn speed squared, affects weapon swing
  fixed_t bob_amplitude; 

  if ((pawn->eflags & MFE_FLY) && !onground)
    bob_amplitude = FLYBOB;
  else
    {
      bob_amplitude = pawn->vel.XYNorm2() >> 2;

      if (bob_amplitude > MAXBOB)
	bob_amplitude = MAXBOB;
    }

  fixed_t eyes = (pawn->height * cv_viewheight.value) / 56; // default eye view height

  if ((pawn->cheats & CF_NOMOMENTUM) || !onground)
    {
      viewheight = eyes;
      viewz = pawn->Feet() + eyes;
    }
  else
    {
      int phase = (FINEANGLES/20 * game.tic) & FINEMASK;
      fixed_t bob = (bob_amplitude/2) * finesine[phase];

      if (!(pawn->flags & MF_CORPSE))
	{
	  viewheight += deltaviewheight;

	  if (viewheight > eyes)
	    {
	      viewheight = eyes;
	      deltaviewheight = 0;
	    }

	  if (viewheight < eyes/2)
	    {
	      viewheight = eyes/2;
	      if (deltaviewheight <= 0)
		deltaviewheight = 1;
	    }

	  if (deltaviewheight != 0)
	    {
	      deltaviewheight += 0.25f;
	      if (!deltaviewheight)
		deltaviewheight = 1;
	    }
	}

      viewz = pawn->Feet() + viewheight + bob - pawn->floorclip;

      if (viewz < pawn->floorz + 4)
	viewz = pawn->floorz + 4;
    }

  if (viewz > pawn->ceilingz - 4)
    viewz = pawn->ceilingz - 4;


  // server decides the rising/lowering of weapons (sy coord),
  // but client does the bobbing independently

  pspdef_t *psp = pawn->psprites;

  if (psp[ps_weapon].state && psp[ps_weapon].state->action == A_WeaponReady)
    {
      // bob the weapon based on movement speed
      int angle = (128 * game.tic) & FINEMASK;
      psp[ps_weapon].sx = 1 + bob_amplitude*finecosine[angle];
      angle &= FINEANGLES/2-1;
      psp[ps_weapon].sy = WEAPONTOP + bob_amplitude*finesine[angle];

      psp[ps_flash].sx = psp[ps_weapon].sx;
      psp[ps_flash].sy = psp[ps_weapon].sy;
    }
}




/// Makes a copy of the pawn (when the player enters a map)
void PlayerInfo::SavePawn()
{
  if (hubsavepawn)
    delete hubsavepawn;

  if (pawn)
    {
      hubsavepawn = new PlayerPawn(*pawn);
      hubsavepawn->player = NULL; // to avoid conflicts with the real pawn... ok, it's bad design:)
      hubsavepawn->pres = NULL;
    }
  else
    hubsavepawn = NULL;
}


/// Copies the saved pawn as the actual pawn
void PlayerInfo::LoadPawn()
{
  if (!hubsavepawn)
    return;

  pawn = new PlayerPawn(*hubsavepawn);
  pawn->player = this;

  // and a new presentation
  if (!pawn->info->modelname.empty())
    pawn->pres = new modelpres_t(pawn->info->modelname.c_str());
  else
    pawn->pres = new spritepres_t(pawn->info, 0);
}
