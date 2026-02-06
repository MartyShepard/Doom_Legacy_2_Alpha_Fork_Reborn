// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2007-2009 by DooM Legacy Team.
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
/// \brief DeHackEd/BEX/DECORATE mnemonics

#ifndef mnemonics_h
#define mnemonics_h 1

#include "doomtype.h"


/// BEX/DECORATE DActor action function mnemonics
struct dactor_mnemonic_t
{
  const char *name;
  void (*ptr)(class DActor *actor);
};

extern dactor_mnemonic_t BEX_DActorMnemonics[];


/// BEX/DECORATE weapon action function mnemonics
struct weapon_mnemonic_t
{
  const char *name;
  void (*ptr)(class PlayerPawn *player, struct pspdef_t *psp);
};

extern weapon_mnemonic_t BEX_WeaponMnemonics[];


/// BEX/DECORATE Actor flag mnemonics.
struct flag_mnemonic_t
{
  const char *name;
  int   flag;
  int   flagword;
};

extern flag_mnemonic_t BEX_FlagMnemonics[];


/// Maps old-style numeric DeHackEd flags to internal Legacy flags.
struct old_flag_t
{
  Uint32 old_flag;
  Uint32 new_flag;
};

extern old_flag_t OriginalFlags[26];


/// BEX/DECORATE game string mnemonics
struct string_mnemonic_t
{
  const char *name;
  int   num;
};

extern string_mnemonic_t BEX_StringMnemonics[];


#endif
