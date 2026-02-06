// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: n_interface.h 343 2006-07-14 19:17:10Z smite-meister $
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

#ifndef n_interface_h
#define n_interface_h 1

#include <vector>
#include <string>
#include "tnl/tnlAssert.h"
#include "tnl/tnlNetInterface.h"

using namespace std;
using namespace TNL;

/// \brief TNL NetInterface class for Legacy
///
/// Wrapper for a socket and more.
/// Takes care of server pinging, server lists, connections,
/// authenthication and basic network housekeeping.

class LNetInterface : public NetInterface
{
  friend class LConnection;
  typedef NetInterface Parent;

public:

  enum netstate_t
  {
    SV_Unconnected,    ///< uninitialized or no network connections
    SV_WaitingClients, ///< server ready and waiting for players to join in
    SV_Running,        ///< server running the game
    CL_PingingServers, ///< client looking for servers
    CL_Connecting,     ///< client trying to connect to a server
    CL_Connected,      ///< client connected to a server
  };

  /// network state
  netstate_t netstate;

protected:
  /// local constants
  enum
  {
    PingDelay  = 5000, ///< ms to wait between sending server pings
    QueryDelay = 8000, ///< ms to wait between sending server queries

    /// Different types of info packets
    PT_ServerPing = FirstValidInfoPacketId,  ///< Client pinging for servers
    PT_PingResponse,   ///< Server answering a ping
    PT_ServerQuery,    ///< Client querying server info
    PT_QueryResponse,  ///< Server answering a query
  };


  U32          nowtime; ///< time of current/last update in ms

  /// server pinging
  U32     nextpingtime;
  Nonce      pingnonce;
public:
  Address ping_address; ///< Host or a LAN broadcast address used in server search
  bool     autoconnect;

  /// information about currently known servers
  vector<class serverinfo_t *> serverlist;

protected:
  /// server list manipulation
  serverinfo_t *SL_FindServer(const Address &a);
  serverinfo_t *SL_AddServer(const Address &a);
  void SL_Update();
  void SL_Clear();


  /// overrides the method in the NetInterface class to handle ping and info packets
  virtual void handleInfoPacket(const Address &a, U8 packetType, BitStream *stream);

  /// Sends out a server ping
  void SendPing(const Address &a, const Nonce &cn);

  /// Sends out a server query
  void SendQuery(serverinfo_t *s);


public:

  // active connections
  class MasterConnection *master_con;   ///< connection to master server
  class LConnection      *server_con;   ///< Current connection to the server, if this is a client.
  std::vector<LConnection *> client_con;   ///< Current client connections, if this is a server.

  bool nodownload;  ///< CheckParm of -nodownload

public:

  /// The constructor initializes and binds the network interface
  LNetInterface(const Address &bind);

  /// Checks for incoming packets, sends out packets if needed, processes connections.
  void Update();

  /// Starts searching for LAN servers
  void CL_StartPinging(bool connectany = false);

  /// Tries to connect to a server
  void CL_Connect(const Address &a);

  /// Closes server connection
  void CL_Reset();

  /// Opens a server to the world
  void SV_Open(bool waitforplayers = false);

  /// Removes the given connection from the client_con vector
  bool SV_RemoveConnection(LConnection *c);

  /// Closes all connections, disallows new connections
  void SV_Reset();

  //================================================
  //  RPC callers
  //================================================

  void SendChat(int from, int to, const char *msg);
  void Pause(int pnum, bool on);
  void SendNetVar(U16 netid, const char *str);
  void SendPlayerOptions(int pnum, class LocalPlayerInfo &p);
  void RequestSuicide(int pnum);
  void Kick(class PlayerInfo *p);
};



/// \brief Vital server properties visible to prospective clients
///
/// A client makes one of these for each server found during server search.
class serverinfo_t
{
public:
  Address         addr; ///< server address
  Nonce             cn; ///< client nonce sent to server
  U32            token; ///< id token the server returned
  unsigned   nextquery; ///< when should the next server query be sent?

  unsigned        ping; ///< computed ping

  int          version; ///< server version
  string versionstring; ///< take a guess
  string          name; ///< server name
  int          players; ///< current # of players
  int       maxplayers; ///< maximum # of players
  string       gt_name; ///< name of the gametype DLL
  U32       gt_version; ///< DLL version

  serverinfo_t(const Address &a);
  void Draw(int x, int y);
  void Read(TNL::BitStream &s);
  static void Write(TNL::BitStream &s);
};


#endif
