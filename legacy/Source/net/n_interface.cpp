// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: n_interface.cpp 516 2007-12-22 15:57:58Z jussip $
//
// Copyright (C) 2004-2006 by DooM Legacy Team.
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
/// \brief Legacy Network Interface
///
/// Uses OpenTNL.

#include "tnl/tnlAsymmetricKey.h"

#include "doomdef.h"
#include "command.h"
#include "cvars.h"

#include "n_interface.h"
#include "n_connection.h"

#include "g_game.h"
#include "g_type.h"

#include "i_system.h"


#include "m_misc.h"
#include "w_wad.h"
#include "vfile.h"

using namespace TNL;


//===================================================================
//    Server lists
//===================================================================

serverinfo_t::serverinfo_t(const Address &a)
{
  addr = a;
  nextquery = 0;
  version = 0;
  players = maxplayers = 0;
  gt_version = 0;
}

/// reads server information from a packet
void serverinfo_t::Read(BitStream &s)
{
  char temp[256];

  s.read(&version);
  s.readString(temp);
  versionstring = temp;

  s.readString(temp);
  name = temp;

  s.read(&players);
  s.read(&maxplayers);

  s.readString(temp);
  gt_name = temp;
  s.read(&gt_version);
}

/// writes the current server information from to a packet
void serverinfo_t::Write(BitStream &s)
{
  s.write(game.demoversion);
  s.writeString(LEGACY_VERSIONSTRING);
  s.writeString(cv_servername.str);
  s.write(static_cast<unsigned int>(game.Players.size()));
  s.write(game.maxplayers);
  s.writeString(game.gtype->gt_name.c_str());
  s.write(game.gtype->gt_version);
  // TODO more basic info? current mapname? server load?
}


serverinfo_t *LNetInterface::SL_FindServer(const Address &a)
{
  int n = serverlist.size();
  for (int i = 0; i<n; i++)
    if (serverlist[i]->addr == a)
      return serverlist[i];
  
  return NULL;
}


serverinfo_t *LNetInterface::SL_AddServer(const Address &a)
{
  if (serverlist.size() >= 50)
    return NULL; // no more room

  serverinfo_t *s = new serverinfo_t(a);
  serverlist.push_back(s);
  return s;
}


void LNetInterface::SL_Clear()
{
  int n = serverlist.size();
  for (int i = 0; i<n; i++)
    delete serverlist[i];

  serverlist.clear();
}


void LNetInterface::SL_Update()
{
  int n = serverlist.size();
  for (int i = 0; i<n; i++)
    if (serverlist[i]->nextquery <= nowtime)
      SendQuery(serverlist[i]);
}


//===================================================================
//     Network Interface
//===================================================================

static const char *ConnectionState[] =
{
  "Not connected",               ///< Initial state of a NetConnection instance - not connected.
  "Awaiting challenge response", ///< We've sent a challenge request, awaiting the response.
  "Sending punch packets",       ///< The state of a pending arranged connection when both sides haven't heard from the other yet
  "Computing puzzle solution",   ///< We've received a challenge response, and are in the process of computing a solution to its puzzle.
  "Awaiting connect response",   ///< We've received a challenge response and sent a connect request.
  "Connect timeout",             ///< The connection timed out during the connection process.
  "Connection rejected",         ///< The connection was rejected.
  "Connected.",                  ///< We've accepted a connect request, or we've received a connect response accept.
  "Disconnected",                ///< The connection has been disconnected.
  "Connection timed out"         ///< The connection timed out.
};



LNetInterface::LNetInterface(const Address &bind)
  : NetInterface(bind)
{
  master_con = NULL;
  server_con = NULL;
  netstate = SV_Unconnected;

  nowtime = nextpingtime = 0;
  autoconnect = false;
  nodownload = false;

  setAllowsConnections(false);

  // Asymmetric cipher based on elliptic curves
  //setPrivateKey(new AsymmetricKey(32));
  //setRequiresKeyExchange(true);
}



void LNetInterface::CL_StartPinging(bool connectany)
{
  autoconnect = connectany;
  nextpingtime = I_GetTime();
  pingnonce.getRandom();
  SL_Clear();
  netstate = CL_PingingServers;
}


/// send out a server ping
void LNetInterface::SendPing(const Address &a, const Nonce &cn)
{
  CONS_Printf("Sending out server ping to %s...\n", a.toString());

  PacketStream out;

  out.write(U8(PT_ServerPing));
  cn.write(&out);           // client nonce
  out.write(LEGACY_VERSION);       // version information
  out.writeString(LEGACY_VERSIONSTRING);
  out.write(nowtime);  // this is used to calculate the ping value

  out.sendto(mSocket, a);
  nextpingtime = nowtime + PingDelay;
}


// send out server query
void LNetInterface::SendQuery(serverinfo_t *s)
{
  CONS_Printf("Querying server %s...\n", s->addr.toString());

  PacketStream out;

  out.write(U8(PT_ServerQuery));
  s->cn.write(&out);
  out.write(s->token);

  out.sendto(mSocket, s->addr);
  s->nextquery = nowtime + QueryDelay;
}



/// Handles received info packets
void LNetInterface::handleInfoPacket(const Address &address, U8 packetType, BitStream *stream)
{
  // first 8 bits denote the packet type
  switch (packetType)
    {
    case PT_ServerPing:
      // ping packet only contains a client nonce and the Legacy ID string(s)
      if (game.server && mAllowConnections)
	{
	  CONS_Printf("received ping from %s\n", address.toString());

	  // read nonce
	  Nonce cn;
	  cn.read(stream);

	  // check version TODO different versioning for protocol? no?
	  int version;
	  stream->read(&version);
	  if (version != LEGACY_VERSION)
	    {
	      CONS_Printf("Wrong version (%d.%d)\n", version/100, version%100);
	      break;
	    }
	  char temp[256];
	  stream->readString(temp);
	  //if (strcmp(temp, LEGACY_VERSIONSTRING)) break;
	  CONS_Printf(" versionstring '%s'\n", temp);

	  // local sending time
	  unsigned time;
	  stream->read(&time);

	  // TODO should we answer back or ignore?

	  // two-part server query protocol for defending agains DOS attacks ;)
	  U32 token = computeClientIdentityToken(address, cn);
	  // this client is now identified with this token

	  PacketStream out;
	  out.write(U8(PT_PingResponse));
	  cn.write(&out);   // write same nonce
	  out.write(token); // and the token
	  out.write(time); // return back the time value received so client can compute ping
	  out.sendto(mSocket, address);
	}
      break;

    case PT_PingResponse:
      if (netstate == CL_PingingServers)
	{
	  CONS_Printf("received ping response from %s\n", address.toString());

	  Nonce cn;
	  cn.read(stream);

	  if (cn != pingnonce)
	    return; // wrong nonce. kind of marginal concern.

	  if (SL_FindServer(address))
	    return; // already known, no need to requery

	  serverinfo_t *s = SL_AddServer(address);
	  if (!s)
	    return;

	  s->cn = cn;
	  stream->read(&s->token); // our identity with this server

	  unsigned time;
	  stream->read(&time);
	  s->ping = nowtime - time;
	  CONS_Printf("ping: %d ms\n", s->ping);

	  SendQuery(s);
	}
      break;

    case PT_ServerQuery:
      // packet contains the client nonce and the id token
      if (game.server && mAllowConnections)
	{
	  CONS_Printf("Got server query from %s\n", address.toString());

	  Nonce cn;
	  cn.read(stream);

	  U32 token;
	  stream->read(&token);

	  if (token == computeClientIdentityToken(address, cn))
            {
	      PacketStream out;
	      out.write(U8(PT_QueryResponse));
	      cn.write(&out);

	      // send over the server info
	      serverinfo_t::Write(out);
	      out.sendto(mSocket, address);
	    }	  
	}
      break;

    case PT_QueryResponse:
      if (netstate == CL_PingingServers) //netstate == CL_QueryingServer
	{
	  CONS_Printf("Got query response from %s\n", address.toString());

	  serverinfo_t *s = SL_FindServer(address);
	  if (!s)
	    return; // "Are you talking to me? You must be, 'cos there's nobody else here."

	  Nonce cn;
	  cn.read(stream);
	  if (cn != s->cn)
	    return; // wrong nonce. kind of marginal concern.

	  s->Read(*stream); // read the server info

	  if (autoconnect)
	    CL_Connect(address); // this will stop the pinging
	}
      break;

    default:
      CONS_Printf("unknown packet %d from %s", packetType, address.toString());
      break;
    }
}





// client making a connection
void LNetInterface::CL_Connect(const Address &a)
{
  game.server = false;
  game.netgame = true;
  game.multiplayer = true;

  server_con = new LConnection();

  // Set player info

  // If we had separate server and client progs, we could use  'if (local) s->connectLocal(net, net)';
  server_con->connect(this, a);
  netstate = CL_Connecting;
}



void LNetInterface::CL_Reset()
{
  if (server_con)
    disconnect(server_con, NetConnection::ReasonSelfDisconnect, "Client quits.\n");

  server_con = NULL;
  netstate = SV_Unconnected;
}





void LNetInterface::SV_Open(bool wait)
{
  setAllowsConnections(true);

  if (wait)
    netstate = SV_WaitingClients;
  else
    netstate = SV_Running;

  //if (cv_internetserver.value) RegisterServer(0, 0);
}


/*
void LNetInterface::SV_Close()
{
  setAllowsConnections(false);
  netstate = SV_WaitingClients;
}
*/


bool LNetInterface::SV_RemoveConnection(LConnection *c)
{
  vector<LConnection *>::iterator t = client_con.begin();
  for ( ; t != client_con.end(); t++)
    if (*t == c)
      {
	client_con.erase(t);
	return true;
      }

  return false; // not found
}



class MasterConnection : public LConnection {};


void LNetInterface::SV_Reset()
{
  setAllowsConnections(false);

  vector<LConnection *>::iterator t = client_con.begin();
  for (; t != client_con.end(); t++)
    disconnect(*t, NetConnection::ReasonSelfDisconnect, "Server shutdown!\n");

  client_con.clear();


  if (master_con)
    disconnect(master_con, NetConnection::ReasonSelfDisconnect, "Server shutdown.\n");

  master_con = NULL;

  CL_Reset();
  netstate = SV_Unconnected;
}




void LNetInterface::Update()
{
  nowtime = I_GetTime();

  switch (netstate)
    {
    case CL_PingingServers:
      if (nextpingtime <= nowtime)
	SendPing(ping_address, pingnonce);

      SL_Update(); // refresh the server list by sending new queries
      break;

    case CL_Connecting:
      if (server_con && !(nowtime & 0x1ff))
	CONS_Printf("%s\n", ConnectionState[server_con->getConnectionState()]);
      break;

    default:
      break;
    }

  //Local_Maketic(realtics);    // make local tic, and call menu ?!
  //CL_SendClientCmd();   // send tic cmd

  //if( cv_internetserver.value ) SendHeartbeatMasterServer();

  checkIncomingPackets();
  processConnections();

  // file transfers
}






/// \brief xxx
/*
class NetFileInfo
{
  string filename;
  int        size;
  byte    md5[16];

  // current size, transfer status
};
*/


void FileCache::WriteNetInfo(BitStream &s)
{
  S32 n = vfiles.size();
  s.write(n); // number of files

  for (int i=0; i<n; i++)
    {
      S32 size;
      byte md5[16];

      s.write(vfiles[i]->GetNetworkInfo(&size, md5)); // downloadable?
      s.writeString(FIL_StripPath(vfiles[i]->filename.c_str()));
      s.write(size);
      s.write(16, md5);
    }
}
