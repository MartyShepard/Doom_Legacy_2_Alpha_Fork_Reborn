// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: n_connection.cpp 500 2007-11-09 00:37:13Z smite-meister $
//
// Copyright (C) 2004-2005 by DooM Legacy Team.
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
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Network connections

#include "doomdef.h"
#include "command.h"
#include "cvars.h"

#include "n_interface.h"
#include "n_connection.h"

#include "g_game.h"
#include "g_type.h"
#include "g_player.h"
#include "g_pawn.h"

#include "w_wad.h"

#ifndef min
#define min(x,y) ( ((x)<(y)) ? (x) : (y) )
#endif

extern unsigned num_bots;

/*
More serverside stuff:

void objectLocalScopeAlways(o) (for playerinfos)
void objectLocalClearAlways(o)
bool isGhosting()

*/

// netobject:
/*
  virtual void onGhostAvailable (GhostConnection *theConnection) // serverside
  virtual F32  getUpdatePriority (NetObject *scopeObject, U32 updateMask, S32 updateSkips)
*/

list<class LocalPlayerInfo *> LConnection::joining_players;


TNL_IMPLEMENT_NETCONNECTION(LConnection, NetClassGroupGame, true);


LConnection::LConnection()
{
  //setIsAdaptive();
  //setTranslatesStrings();
  //setFixedRateParameters(50, 50, 2000, 2000); // packet rates, sizes (send and receive)
}


// client
void LConnection::writeConnectRequest(BitStream *stream)
{
  Parent::writeConnectRequest(stream);

  stream->write(LEGACY_VERSION);
  stream->writeString(LEGACY_VERSIONSTRING);

  joining_players.clear();

  // send local player data
  unsigned n = 1+cv_splitscreen.value; // number of local human players
  stream->write(n + num_bots);

  LocalPlayerInfo *p;
  // first humans
  for (unsigned i = 0; i < n; i++)
    {
      p = &LocalPlayers[i];
      p->Write(stream);
      joining_players.push_back(p);
    }

  // then bots
  for (unsigned i = 0; i < num_bots; i++)
    {
      p = &LocalPlayers[NUM_LOCALHUMANS + i];
      p->Write(stream);
      joining_players.push_back(p);
    }
}



// server
bool LConnection::readConnectRequest(BitStream *stream, const char **errorString)
{
  if (!Parent::readConnectRequest(stream, errorString))
    return false;

  char temp[256];
  *errorString = temp; // in case we need it

  int version;
  stream->read(&version);
  stream->readString(temp);

  if (version != LEGACY_VERSION || strcmp(temp, LEGACY_VERSIONSTRING))
    {
      sprintf(temp, "Different Legacy versions cannot play a net game! (Server version %d.%d%s)",
	      LEGACY_VERSION/100, LEGACY_VERSION%100, LEGACY_VERSIONSTRING);
      return false;
    }

  if (!cv_allownewplayers.value)
    {
      sprintf(temp, "The server is not accepting new players at the moment.");
      return false;
    }

  // how many players want to get in?
  unsigned n;
  stream->read(&n);

  LNetInterface *net = (LNetInterface *)getInterface();

  if (net->netstate == LNetInterface::SV_Running)
    {
      if (game.Players.size() >= unsigned(cv_maxplayers.value))
	{
	  sprintf(temp, "Maximum number of players reached (%d).", cv_maxplayers.value);
	  return false;
	}

      n = min(n, cv_maxplayers.value - game.Players.size()); // this many fit in

      // read joining players' preferences
      for (unsigned i = 0; i < n; i++)
	{
	  PlayerInfo *p = new PlayerInfo();
	  p->options.Read(stream);
	  p->connection = this;
	  p->client_hash = getNetAddress().hash();

	  // TODO check that name is unique, change if necessary
	  p->name = p->options.name;

	  player.push_back(p);
	  if (!game.AddPlayer(p))
	    I_Error("shouldn't happen! rotten!\n");
	}
    }
  else
    {
      // TODO waiting for specific clients to return (check names and hashes!)
    }

  return true; // server accepts
}


// server
void LConnection::writeConnectAccept(BitStream *stream)
{
  Parent::writeConnectAccept(stream);

  unsigned n = player.size();
  stream->write(n);                   // how many players were accepted?
  for (unsigned i = 0; i < n; i++)
    stream->write(player[i]->number); // send pnums

  serverinfo_t::Write(*stream); // first send basic server info (including the gametype!)
  game.gtype->WriteServerInfo(*stream); // then gametype dependent stuff: netvars, resource file info...
}


// client
bool LConnection::readConnectAccept(BitStream *stream, const char **errorString)
{
  if (!Parent::readConnectAccept(stream, errorString))
    return false;

  unsigned n;
  stream->read(&n); // number of players accepted
  if (n == 0 || n > joining_players.size())
    return false;

  CONS_Printf("Server accepts %d players.\n", n);
  joining_players.resize(n); // humans have precedence over bots

  list<class LocalPlayerInfo *>::iterator t = joining_players.begin();
  for ( ; t != joining_players.end(); t++)
    stream->read(&(*t)->pnumber); // read pnums

  // read server properties
  LNetInterface *net = (LNetInterface *)getInterface();

  serverinfo_t *s = net->SL_FindServer(getNetAddress());
  if (!s)
    s = new serverinfo_t(getNetAddress()); 

  s->Read(*stream);

  // TODO check if we have the correct gametype DLL!
  game.gtype->ReadServerInfo(*stream);
  /*
  if (!game.FindGametypeDLL(*stream))
    return false; // no suitable DLL found, must disconnect
  */

  // FIXME read needed files, check them, download them...

  return true;
}




void LConnection::onConnectTerminated(TerminationReason r, const char *reason)
{
  CONS_Printf("Connect terminated (%d), %s\n", r, reason);
  ConnectionTerminated(false);
}




void LConnection::onConnectionEstablished()
{
  // initiator becomes the "client", the other one "server"
  Parent::onConnectionEstablished();

  // To see how this program performs with 50% packet loss,
  // Try uncommenting the next line :)
  //setSimulatedNetParams(0.5, 0);

  LNetInterface *n = (LNetInterface *)getInterface();
   
  if (isInitiator())
    {
      // client side
      setGhostFrom(false);
      setGhostTo(true);
      n->server_con = this;
      n->netstate = LNetInterface::CL_Connected;
      CONS_Printf("Connected to server at %s.\n", getNetAddressString());

      rpcTest(7467);

      game.CL_SpawnClient();
      game.CL_StartGame();
    }
  else
    {
      // server side
      n->client_con.push_back(this);

      int k = player.size();
      for (int i = 0; i<k; i++)
	{
	  CONS_Printf("%s entered the game (player %d)\n", player[i]->name.c_str(), player[i]->number);
	}
      setScopeObject(game.gtype);
      setGhostFrom(true);
      setGhostTo(false);
      activateGhosting(); // soon, the new PlayerInfos will be ghosted to everyone
    }
}



void LConnection::onConnectionTerminated(TerminationReason r, const char *error)
{
  CONS_Printf("%s - connection to %s: %s\n.", getNetAddressString(), isConnectionToServer()
	      ? "server" : "client", error);
  ConnectionTerminated(true);
}



void LConnection::ConnectionTerminated(bool established)
{
  if (isConnectionToServer())
    {
      ((LNetInterface *)getInterface())->CL_Reset();
      // TODO end/reset game? start intro loop?
    }
  else
    {
      if (established)
	{
	  int n = player.size();
	  for (int i = 0; i<n; i++)
	    {
	      CONS_Printf("Player (%d) dropped.\n", player[i]->number);
	      player[i]->playerstate = PST_REMOVE;
	    }
	  resetGhosting();
	  ((LNetInterface *)getInterface())->SV_RemoveConnection(this);
	}
      else
	{
	  CONS_Printf("Unsuccesful connect attempt\n");
	}
    }
}



void LConnection::onStartGhosting()
{
  CONS_Printf("Ghosting started...\n");
}



void LConnection::onEndGhosting()
{
  CONS_Printf("Ghosting ended.\n");
}



//========================================================
//            Remote Procedure Calls
//
// Functions that are called at one end of the
// connection and executed at the other. Neat.
//========================================================

  /*
TODO RPC's from server to client:
 - map change/load

Player ticcmd contains both guaranteed_ordered and unguaranteed elements.
Shooting and artifact use should be guaranteed...

    XD_MAP,
    XD_EXITLEVEL,
    XD_LOADGAME,
    XD_SAVEGAME,
    XD_ADDPLAYER,
    XD_USEARTEFACT,
  */




LCONNECTION_RPC(rpcTest, (U8 num), (num), RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
  CONS_Printf("client sent this: %d\n", num);
};


// the chat message "router"
void LNetInterface::SendChat(int from, int to, const char *msg)
{
  if (!game.netgame)
    return;

  PlayerInfo *sender = game.FindPlayer(from);
  if (!sender)
    return;

  if (game.server)
    {
      int n = client_con.size();
      for (int i = 0; i < n; i++)
	{
	  LConnection *c = client_con[i];
	  int np = c->player.size();
	  for (int j = 0; j < np; j++)
	    {
	      PlayerInfo *p = c->player[j];
	      if (to == 0 || p->number == to || p->team == -to)
		{
		  // We could also use PlayerInfo::SetMessage to trasmit chat messages from server to client,
		  // but it is mainly for messages which can easily use TNL::StringTableEntry.
		  c->rpcChat(from, to, msg);
		  if (p->number == to)
		    return; // nobody else should get it

		  break; // one rpc per connection
		}
	    }
	}

      // TODO how to handle messages for local (server) players?
      CONS_Printf("\a%s: %s\n", sender->name.c_str(), msg);
    }
  else if (server_con)
    server_con->rpcChat(from, to, msg);
}

// to: 0 means everyone, positive numbers are players, negative numbers are teams
LCONNECTION_RPC(rpcChat, (S8 from, S8 to, StringPtr msg), (from, to, msg),
		RPCGuaranteedOrdered, RPCDirAny, 0)
{
  PlayerInfo *p = game.FindPlayer(from);

  if (isConnectionToServer())
    {
      // client
      if (p)
	CONS_Printf("%s: %s\n", p->name.c_str(), msg.getString());
      else
	CONS_Printf("Unknown player %d: %s\n", from, msg.getString());
    }
  else
    {
      if (!p || p->connection != this)
	{
	  CONS_Printf("counterfeit message!\n"); // false sender
	  return;
	}

      static_cast<LNetInterface *>(getInterface())->SendChat(from, to, msg);
    }
};



static void PauseMsg(bool on, int pnum)
{
  const char *guilty = "server";
  if (pnum > 0)
    {
      PlayerInfo *p = game.FindPlayer(pnum);
      if (p)
	guilty = p->name.c_str();
    }

  if (on)
    CONS_Printf("Game paused by %s.\n", guilty);
  else
    CONS_Printf("Game unpaused by %s.\n", guilty);
}

/// pauses or unpauses the game
void LNetInterface::Pause(int pnum, bool on)
{
  if (game.server)
    {
      // server can pause the game anytime
      game.Pause(on);

      // send rpc event to all clients
      if (game.netgame && client_con.size())
	{
	  int n = client_con.size();
	  NetEvent *e = TNL_RPC_CONSTRUCT_NETEVENT(client_con[0], rpcPause, (pnum, on));

	  for (int i = 0; i < n; i++)
	    client_con[i]->postNetEvent(e);
	}

      PauseMsg(on, pnum);
    }
  else if (server_con)
    {
      // client must request a pause from the server
      server_con->rpcPause(0, on);

      if (on)
	CONS_Printf("Pause request sent.\n");
      else
	CONS_Printf("Unpause request sent.\n");
    }
}

LCONNECTION_RPC(rpcPause, (U8 pnum, bool on), (pnum, on),
		RPCGuaranteedOrdered, RPCDirAny, 0)
{
  if (isConnectionToServer())
    {
      // server orders a pause
      game.Pause(on);
      PauseMsg(on, pnum);
    }
  else if (cv_allowpause.value)
    {
      // got a pause request from a client
      static_cast<LNetInterface *>(getInterface())->Pause(player[0]->number, on);
    }
}



LCONNECTION_RPC(rpcMessage_s2c, (S8 pnum, StringPtr msg, S8 priority, S8 type), (pnum, msg, priority, type),
		RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
  for (int i=0; i<NUM_LOCALPLAYERS; i++)
    if (LocalPlayers[i].info && LocalPlayers[i].info->number == pnum)
      {
	LocalPlayers[i].info->SetMessage(msg, priority, type);
	return;
      }

  CONS_Printf("Received someone else's message!\n");
}



void LNetInterface::SendNetVar(U16 netid, const char *str)
{
  // send netvar as an rpc event to all clients
  int n = client_con.size();
  for (int i = 0; i < n; i++)
    client_con[i]->rpcSendNetVar_s2c(netid, str);
}

LCONNECTION_RPC(rpcSendNetVar_s2c, (U16 netid, StringPtr s), (netid, s),
		RPCGuaranteed, RPCDirServerToClient, 0)
{
  consvar_t::GotNetVar(netid, s);
}



/// only called on a server
void LNetInterface::Kick(PlayerInfo *p)
{
  LConnection *c = p->connection;
  if (c)
    {
      c->rpcKick_s2c(p->number, "Get lost!");

      if (c->player.size() == 1)
	{
	  disconnect(c, NetConnection::ReasonSelfDisconnect, "You were kicked by the server.\n");
	}
      else
	{
	  // just kick the one
	}
    }

  CONS_Printf("\2%s kicked from the game.\n", p->name.c_str());
}

LCONNECTION_RPC(rpcKick_s2c, (U8 pnum, StringPtr str), (pnum, str),
		RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
  // TODO
  const char *s = str;
  CONS_Printf("\2You were kicked from the game: %s.\n", s);
/*
  if(pnum == consoleplayer->number - 1)
    {
      CL_Reset();
      game.StartIntro();
      M_StartMessage("You have been kicked by the server\n\nPress ESC\n",NULL,MM_NOTHING);
    }
  else
    CL_RemovePlayer(pnum);
*/
}



void LNetInterface::SendPlayerOptions(int pnum, LocalPlayerInfo &p)
{
  if (!server_con)
    return;

  BitStream s;
  p.Write(&s);
  server_con->rpcSendOptions_c2s(pnum, &s);
}

LCONNECTION_RPC(rpcSendOptions_c2s, (U8 pnum, ByteBufferPtr buf), (pnum, buf),
		RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
  PlayerInfo *p = game.FindPlayer(pnum);

  if (!p || p->connection != this || buf->getBufferSize() > 100)
    {
      // since we receive a variable-length bytebuffer, we better be careful...
      CONS_Printf("Bogus PlayerOptions RPC!\n");
      return;
    }

  BitStream s(buf->getBuffer(), buf->getBufferSize());
  p->options.Read(&s);

  // put a limit on the name length
  if (p->options.name.size() > MAXPLAYERNAME)
    p->options.name.resize(MAXPLAYERNAME);

  p->name = p->options.name;
}



void LNetInterface::RequestSuicide(int pnum)
{
  if (server_con)
    server_con->rpcSuicide_c2s(pnum);
}

LCONNECTION_RPC(rpcSuicide_c2s, (U8 pnum), (pnum),
		RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
  void Kill_pawn(Actor *v, Actor *k);

  PlayerInfo *p = game.FindPlayer(pnum);
  if (p->connection == this)
    Kill_pawn(p->pawn, p->pawn);
}



LCONNECTION_RPC(rpcRequestPOVchange_c2s, (S32 pnum), (pnum),
		  RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
  // spy mode
  if (game.state == GameInfo::GS_LEVEL && !cv_hiddenplayers.value)
    {
      PlayerInfo *p = NULL;

      /* FIXME NOW
      if (pnum <= 0)
	{
	  // simply "next available POV"
	  player_iter_t i = Players.upper_bound(player[0]->povnum + 1);
	  if (i == Players.end())
	    i = Players.begin();
	  
	  p = i->second;
	}
      else
	p = game.FindPlayer(pnum);
      */

      if (p)
	player[0]->pov = p->pawn;
      else
	player[0]->pov = player[0]->pawn;

      // tell who's the view
      player[0]->SetMessage(va("Viewpoint: %s\n", p->name.c_str()));
      player[0]->setMaskBits(PlayerInfo::M_PAWN); // notify network system
    }

  // TODO client should start HUD on the new pov...
}
