// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: g_type.h 352 2006-07-21 00:27:31Z smite-meister $
//
// Copyright (C) 2004 by DooM Legacy Team.
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
/// \brief Game types

#include <string>
#include "tnl/tnlNetObject.h"

using namespace std;
using namespace TNL;


struct engine_export_t
{
  class GameInfo *game;  
};


/// \brief Describes a game type or ruleset, i.e. coop, dm, ctf, domination...
///
/// Each modification DLL subclasses this class and redefines virtual methods, if necessary.
/// If the 
///
/// For security reasons, a mod DLL must never be automatically transferred, let alone executed.
/// It would be easy to write a malware DLL.
/// Maybe we (the Legacy team) should keep a list of authorized DLLs and their md5 hashes?

class GameType : public NetObject
{
  typedef NetObject Parent;

  TNL_DECLARE_CLASS(GameType);
public:
  string     gt_name; ///< name unique to this GameType
  U32     gt_version;

  /// dll-to-engine interface
  engine_export_t e;

public:
  GameType();

  // These polymorphic methods are the engine-to-dll interface

  //----- before game -----

  /// called when server accepts client connection
  virtual void WriteServerInfo(BitStream &s);
  virtual void ReadServerInfo(BitStream &s);

  //----- in game -----

  /// Called when a player frags another. Usually a scoring rule.
  virtual void Frag(class PlayerInfo *killer, PlayerInfo *victim);

  /// GameType is the scope object for _all_ the client connections
  virtual void performScopeQuery(GhostConnection *c);

};
