// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: b_bot.h 391 2006-11-28 13:50:16Z smite-meister $
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
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Bot players.

#ifndef b_bot_h
#define b_bot_h 1


/// \brief ABC for PlayerInfo AI (a client-side bot)
class BotAI
{
protected:
  class  PlayerInfo *subject; ///< the player the AI is controlling
  class  PlayerPawn *pawn;    ///< shorthand for subject->pawn
  class  Map        *mp;      ///< shorthand for subject->mp
  struct ticcmd_t   *cmd;     ///< shorthand for *subject->cmd

public:
  BotAI();
  virtual ~BotAI() {}

  virtual void BuildInput(PlayerInfo *p, int elapsed_tics) = 0;
};


extern unsigned num_bots;

void Command_AddBot();

#endif
