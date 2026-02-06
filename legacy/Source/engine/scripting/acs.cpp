// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2003-2008 by DooM Legacy Team.
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
/// \brief ACS interpreter, new implementation.

#include "g_actor.h"
#include "g_pawn.h"
#include "g_mapinfo.h"
#include "g_map.h"
#include "g_player.h"
#include "g_game.h"

#include "t_acs.h"

#include "command.h"
#include "m_random.h"
#include "m_swap.h"
#include "r_defs.h"
#include "r_data.h"
#include "sounds.h"

#include "w_wad.h"
#include "z_zone.h"


// precautions
#define ACS_MAX_SCRIPTS 1024
#define ACS_MAX_STRINGS 1024

Sint32 ACS_world_vars[ACS_WORLD_VARS]; ///< ACS world variables (visible in all maps)


/// Clears all global and hub scripting variables.
void GameInfo::SV_ResetScripting()
{
  // clear the ACS world vars etc. TODO FS too
  memset(ACS_world_vars, 0, sizeof(ACS_world_vars));

  for (mapinfo_iter_t s = mapinfo.begin(); s != mapinfo.end(); s++)
    s->second->ACS_deferred.clear();
}


/// Console command for running ACS scripts
void Command_RunACS_f()
{
  if (COM.Argc() < 2)
    {
      CONS_Printf("Usage: run-acs <script>\n");
      return;
    }

  if (!com_player || !com_player->mp || !com_player->pawn)
    return;
  
  byte args[5] = {0,0,0,0,0};
  int num = atoi(COM.Argv(1));
  int i, n = COM.Argc() - 2;
  for (i=0; i<n; i++)
    args[i] = atoi(COM.Argv(i+2));

  // line_t trigger is always NULL...
  com_player->mp->ACS_StartScript(num, args, com_player->pawn, NULL, 0);
}



//========================================================================
//   ACS functions
//========================================================================


enum
{
  ACS_CONTINUE = 0,
  ACS_PAUSE,
  ACS_HALT
};


int acs_t::NOP()
{
  return ACS_CONTINUE;
}

int acs_t::Terminate()
{
  return ACS_HALT;
}

int acs_t::Suspend()
{
  def->state = ACS_suspended;
  return ACS_PAUSE;
}

int acs_t::PushNumber()
{
  Push(*ip++);
  return ACS_CONTINUE;
}

int acs_t::ExecLineSpecial(int num_args)
{
  union
  {
    byte args[8];
    int  args_int[2];
  };

  // We have extended some linedef types and hence cannot trust standard ACC to generate proper opcodes for them.
  args_int[0] = args_int[1] = 0; // to be safe, pad args with zeroes

  int special = *ip++;
  // pop correct number of args from the stack
  for (int i = num_args-1; i >= 0; i--)
    args[i] = Pop();

  // HACK to force ExecuteLineSpecial to use args[0] as the tag
  if (line)
    {
      unsigned temp = line->tag;
      line->tag = args[0];
      mp->ExecuteLineSpecial(special, args, line, side, triggerer);
      line->tag = temp;
    }
  else
    mp->ExecuteLineSpecial(special, args, line, side, triggerer);

  return ACS_CONTINUE;
}

int acs_t::ExecLineSpecialImm(int num_args)
{
  union
  {
    byte args[8];
    int  args_int[2];
  };

  // We have extended some linedef types and hence cannot trust standard ACC to generate proper opcodes for them.
  args_int[0] = args_int[1] = 0; // to be safe, pad args with zeroes

  int special = *ip++;
  // read correct number of args from the ip
  for (int i = 0; i < num_args; i++)
    args[i] = *ip++;

  // HACK to force ExecuteLineSpecial to use args[0] as the tag
  if (line)
    {
      unsigned temp = line->tag;
      line->tag = args[0];
      mp->ExecuteLineSpecial(special, args, line, side, triggerer);
      line->tag = temp;
    }
  else
    mp->ExecuteLineSpecial(special, args, line, side, triggerer);

  return ACS_CONTINUE;
}

int acs_t::Add()
{
  Push(Pop() + Pop());
  return ACS_CONTINUE;
}

int acs_t::Sub()
{
  int op2 = Pop(); // NOTE: for non-commutative operations we must make sure to Pop the operands in the correct order!
  // C++ does not guarantee a specific order for operations in some expressions!
  Push(Pop() - op2);
  return ACS_CONTINUE;
}

int acs_t::Mul()
{
  Push(Pop() * Pop());
  return ACS_CONTINUE;
}

int acs_t::Div()
{
  int op2 = Pop();
  Push(Pop() / op2);
  return ACS_CONTINUE;
}

int acs_t::Mod()
{
  int op2 = Pop();
  Push(Pop() % op2);
  return ACS_CONTINUE;
}

int acs_t::EQ()
{
  Push(Pop() == Pop());
  return ACS_CONTINUE;
}

int acs_t::NE()
{
  Push(Pop() != Pop());
  return ACS_CONTINUE;
}

int acs_t::LT()
{
  int op2 = Pop();
  Push(Pop() < op2);
  return ACS_CONTINUE;
}

int acs_t::GT()
{
  int op2 = Pop();
  Push(Pop() > op2);
  return ACS_CONTINUE;
}

int acs_t::LE()
{
  int op2 = Pop();
  Push(Pop() <= op2);
  return ACS_CONTINUE;
}

int acs_t::GE()
{
  int op2 = Pop();
  Push(Pop() >= op2);
  return ACS_CONTINUE;
}

int acs_t::AssignScriptVar()
{
  vars[*ip++] = Pop();
  return ACS_CONTINUE;
}

int acs_t::AssignMapVar()
{
  mp->ACS_map_vars[*ip++] = Pop();
  return ACS_CONTINUE;
}

int acs_t::AssignWorldVar()
{
  ACS_world_vars[*ip++] = Pop();
  return ACS_CONTINUE;
}

int acs_t::PushScriptVar()
{
  Push(vars[*ip++]);
  return ACS_CONTINUE;
}

int acs_t::PushMapVar()
{
  Push(mp->ACS_map_vars[*ip++]);
  return ACS_CONTINUE;
}

int acs_t::PushWorldVar()
{
  Push(ACS_world_vars[*ip++]);
  return ACS_CONTINUE;
}

int acs_t::AddScriptVar()
{
  vars[*ip++] += Pop();
  return ACS_CONTINUE;
}

int acs_t::AddMapVar()
{
  mp->ACS_map_vars[*ip++] += Pop();
  return ACS_CONTINUE;
}

int acs_t::AddWorldVar()
{
  ACS_world_vars[*ip++] += Pop();
  return ACS_CONTINUE;
}

int acs_t::SubScriptVar()
{
  vars[*ip++] -= Pop();
  return ACS_CONTINUE;
}

int acs_t::SubMapVar()
{
  mp->ACS_map_vars[*ip++] -= Pop();
  return ACS_CONTINUE;
}

int acs_t::SubWorldVar()
{
  ACS_world_vars[*ip++] -= Pop();
  return ACS_CONTINUE;
}

int acs_t::MulScriptVar()
{
  vars[*ip++] *= Pop();
  return ACS_CONTINUE;
}

int acs_t::MulMapVar()
{
  mp->ACS_map_vars[*ip++] *= Pop();
  return ACS_CONTINUE;
}

int acs_t::MulWorldVar()
{
  ACS_world_vars[*ip++] *= Pop();
  return ACS_CONTINUE;
}

int acs_t::DivScriptVar()
{
  vars[*ip++] /= Pop();
  return ACS_CONTINUE;
}

int acs_t::DivMapVar()
{
  mp->ACS_map_vars[*ip++] /= Pop();
  return ACS_CONTINUE;
}

int acs_t::DivWorldVar()
{
  ACS_world_vars[*ip++] /= Pop();
  return ACS_CONTINUE;
}

int acs_t::ModScriptVar()
{
  vars[*ip++] %= Pop();
  return ACS_CONTINUE;
}

int acs_t::ModMapVar()
{
  mp->ACS_map_vars[*ip++] %= Pop();
  return ACS_CONTINUE;
}

int acs_t::ModWorldVar()
{
  ACS_world_vars[*ip++] %= Pop();
  return ACS_CONTINUE;
}

int acs_t::IncScriptVar()
{
  vars[*ip++]++;
  return ACS_CONTINUE;
}

int acs_t::IncMapVar()
{
  mp->ACS_map_vars[*ip++]++;
  return ACS_CONTINUE;
}

int acs_t::IncWorldVar()
{
  ACS_world_vars[*ip++]++;
  return ACS_CONTINUE;
}

int acs_t::DecScriptVar()
{
  vars[*ip++]--;
  return ACS_CONTINUE;
}

int acs_t::DecMapVar()
{
  mp->ACS_map_vars[*ip++]--;
  return ACS_CONTINUE;
}

int acs_t::DecWorldVar()
{
  ACS_world_vars[*ip++]--;
  return ACS_CONTINUE;
}

int acs_t::JMP()
{
  ip = reinterpret_cast<Sint32 *>(mp->ACS_base + *ip);
  return ACS_CONTINUE;
}

int acs_t::JNZ()
{
  if (Pop())
    ip = reinterpret_cast<Sint32 *>(mp->ACS_base + *ip);
  else
    ip++;

  return ACS_CONTINUE;
}

int acs_t::PopAndDiscard()
{
  Pop();
  return ACS_CONTINUE;
}

int acs_t::Delay()
{
  delay = Pop();
  return ACS_PAUSE;
}

int acs_t::DelayImm()
{
  delay = *ip++;
  return ACS_PAUSE;
}

int acs_t::Random()
{
  int high = Pop();
  int low = Pop();
  
  Push(low + (P_Random() % (high-low+1)));
  return ACS_CONTINUE;
}

int acs_t::RandomImm()
{
  int low = *ip++;
  int high = *ip++;

  Push(low + (P_Random() % (high-low+1)));
  return ACS_CONTINUE;
}

int acs_t::ThingCount()
{
  int tid = Pop();
  CountThings(Pop(), tid);
  return ACS_CONTINUE;
}

int acs_t::ThingCountImm()
{
  int type = *ip++;
  CountThings(type, *ip++);
  return ACS_CONTINUE;
}


static mobjtype_t moType;
static int thingCount;
bool IT_TypeCount(Thinker *th)
{
  DActor *m = th->Inherits<DActor>();
  if (m && m->type == moType)
    {
      if (m->flags & MF_CORPSE) // had && m->flags & MF_COUNTKILL, but why?
	return true; // Don't count dead monsters
      thingCount++;
    }
  return true;
}


void acs_t::CountThings(int type, int tid)
{
  if (!type && !tid)
    {
      Push(0); // Nothing to count (we need to Push something or the stack gets messed up)
      return; 
    }

  extern mobjtype_t TranslateThingType[];
  moType = TranslateThingType[type];
  thingCount = 0;

  CONS_Printf("Looking for tid %d, type %d...", tid, type);
  if (tid)
    {
      // Count TID things
      Map::Iterate_TID iter(mp, tid);
      for (Actor *a = iter.Next(); a; a = iter.Next())
	{
	  if (type == 0)	    
	    thingCount++; // Just count TIDs
	  else
	    {
	      DActor *da = a->Inherits<DActor>();
	      if (da && moType == da->type)
		{
		  if (a->flags & MF_CORPSE) // NOTE: && a->flags & MF_COUNTKILL, but why?
		    continue; // Don't count dead monsters or corpses
		  thingCount++;
		}
	    }
	}
    }
  else
    mp->IterateThinkers(IT_TypeCount); // Count only types

  CONS_Printf(" found %d objects.\n", thingCount);
  Push(thingCount);
}

int acs_t::TagWait()
{
  def->wait_data = Pop();
  def->state = ACS_waitfortag;
  return ACS_PAUSE;
}

int acs_t::TagWaitImm()
{
  def->wait_data = *ip++;
  def->state = ACS_waitfortag;
  return ACS_PAUSE;
}

int acs_t::PolyWait()
{
  def->wait_data = Pop();
  def->state = ACS_waitforpoly;
  return ACS_PAUSE;
}

int acs_t::PolyWaitImm()
{
  def->wait_data = *ip++;
  def->state = ACS_waitforpoly;
  return ACS_PAUSE;
}

int acs_t::ChangeFloor()
{
  const char *texname = mp->ACS_FindString(Pop());
  Material *m = materials.Get(texname, TEX_floor);
  int tag = Pop();
  for (int i = -1; (i = mp->FindSectorFromTag(tag, i)) >= 0; )
    {
      mp->sectors[i].floorpic = m;
      mp->sectors[i].SetFloorType(texname);
    }

  return ACS_CONTINUE;
}

int acs_t::ChangeFloorImm()
{
  int tag = *ip++;
  const char *texname = mp->ACS_FindString(*ip++);
  Material *m = materials.Get(texname, TEX_floor);
  
  for (int i = -1; (i = mp->FindSectorFromTag(tag, i)) >= 0; )
    {
      mp->sectors[i].floorpic = m;
      mp->sectors[i].SetFloorType(texname);
    }

  return ACS_CONTINUE;
}

int acs_t::ChangeCeiling()
{
  Material *m = materials.Get(mp->ACS_FindString(Pop()), TEX_floor);
  int tag = Pop();

  for (int i = -1; (i = mp->FindSectorFromTag(tag, i)) >= 0; )
    mp->sectors[i].ceilingpic = m;

  return ACS_CONTINUE;
}

int acs_t::ChangeCeilingImm()
{
  int tag = *ip++;
  Material *m = materials.Get(mp->ACS_FindString(*ip++), TEX_floor);
  
  for (int i = -1; (i = mp->FindSectorFromTag(tag, i)) >= 0; )
    mp->sectors[i].ceilingpic = m;

  return ACS_CONTINUE;
}

int acs_t::Restart()
{
  ip = def->code;
  return ACS_CONTINUE;
}

int acs_t::LogicalAND()
{
  Push(Pop() && Pop());
  return ACS_CONTINUE;
}

int acs_t::LogicalOR()
{
  Push(Pop() || Pop());
  return ACS_CONTINUE;
}

int acs_t::BitwiseAND()
{
  Push(Pop() & Pop());
  return ACS_CONTINUE;
}

int acs_t::BitwiseOR()
{
  Push(Pop() | Pop());
  return ACS_CONTINUE;
}

int acs_t::BitwiseXOR()
{
  Push(Pop() ^ Pop());
  return ACS_CONTINUE;
}

int acs_t::LogicalNOT()
{
  Push(!Pop());
  return ACS_CONTINUE;
}

int acs_t::LeftShift()
{
  int op2 = Pop();
  Push(Pop() << op2);
  return ACS_CONTINUE;
}

int acs_t::RightShift()
{
  int op2 = Pop();
  Push(Pop() >> op2);
  return ACS_CONTINUE;
}

int acs_t::Negate()
{
  Push(-Pop());
  return ACS_CONTINUE;
}

int acs_t::JZ()
{
  if (Pop())
    ip++;
  else
    ip = reinterpret_cast<Sint32 *>(mp->ACS_base + *ip);

  return ACS_CONTINUE;
}

int acs_t::LineSide()
{
  Push(side);
  return ACS_CONTINUE;
}

int acs_t::ScriptWait()
{
  def->wait_data = Pop();
  def->state = ACS_waitforscript;
  return ACS_PAUSE;
}

int acs_t::ScriptWaitImm()
{
  def->wait_data = *ip++;
  def->state = ACS_waitforscript;
  return ACS_PAUSE;
}

int acs_t::ClearLineSpecial()
{
  if (line)
    line->special = 0;

  return ACS_CONTINUE;
}

int acs_t::CaseJMP()
{
  if (Top() == *ip++)
    {
      ip = reinterpret_cast<Sint32 *>(mp->ACS_base + *ip);
      Pop();
    }
  else
    ip++;

  return ACS_CONTINUE;
}


static string PrintText;

int acs_t::StartPrint()
{
  PrintText.clear();
  return ACS_CONTINUE;
}

int acs_t::EndPrint()
{
  if (triggerer)
    {
      PlayerPawn *p = triggerer->Inherits<PlayerPawn>();
      if (p && p->player)
	p->player->SetMessage(PrintText.c_str(), 1, PlayerInfo::M_HUD, 150);
    }
  else
    {
      int n = mp->players.size();
      for (int i = 0; i < n; i++)
	mp->players[i]->SetMessage(PrintText.c_str(), 1, PlayerInfo::M_HUD, 150);
    }

  return ACS_CONTINUE;
}


int acs_t::PrintString()
{
  PrintText += mp->ACS_FindString(Pop());
  return ACS_CONTINUE;
}

int acs_t::PrintInt()
{
  char temp[16];
  sprintf(temp, "%12d", Pop());
  PrintText += temp;
  return ACS_CONTINUE;
}

int acs_t::PrintChar()
{
  PrintText += char(Pop());
  return ACS_CONTINUE;
}

int acs_t::NumPlayers()
{
  Push(mp->players.size());
  return ACS_CONTINUE;
}

int acs_t::GameType()
{
  extern consvar_t cv_deathmatch;

  enum
  {
    GT_SINGLE = 0,
    GT_COOP = 1,
    GT_DM = 2
  };

  if (game.multiplayer == false)
    Push(GT_SINGLE);
  else if (cv_deathmatch.value)
    Push(GT_DM);
  else
    Push(GT_COOP);

  return ACS_CONTINUE;
}

int acs_t::GameSkill()
{
  Push(game.skill);
  return ACS_CONTINUE;
}

int acs_t::Timer()
{
  Push(game.tic);
  return ACS_CONTINUE;
}

int acs_t::SectorSound()
{
  mappoint_t *orig = line ? &line->frontsector->soundorg : NULL;
  int volume = Pop();
  S_StartSound(orig, S_GetSoundID(mp->ACS_FindString(Pop())), volume/127.0);
  return ACS_CONTINUE;
}

int acs_t::AmbientSound()
{
  int volume = Pop();
  S_StartAmbSound(NULL, S_GetSoundID(mp->ACS_FindString(Pop())), volume/127.0);
  return ACS_CONTINUE;
}

int acs_t::SoundSequence()
{
  mappoint_t *orig = line ? &line->frontsector->soundorg : NULL;
  mp->SN_StartSequenceName(orig, mp->ACS_FindString(Pop()));
  return ACS_CONTINUE;
}


int acs_t::SetLineTexture()
{
  /// ACS texture position numbers
  enum
  {
    ACS_TEX_UPPER = 0,
    ACS_TEX_MIDDLE = 1,
    ACS_TEX_LOWER = 2,
  };

  line_t *line;

  Material *texture = materials.Get(mp->ACS_FindString(Pop()), TEX_wall);
  int position = Pop();
  int side = Pop();
  int lineid = Pop();

  for (int s = -1; (line = mp->FindLineFromID(lineid, &s)) != NULL; )
    {
      if (position == ACS_TEX_MIDDLE)
	line->sideptr[side]->midtexture = texture;
      else if (position == ACS_TEX_LOWER)
	line->sideptr[side]->bottomtexture = texture;
      else // ACS_TEX_UPPER
	line->sideptr[side]->toptexture = texture;
    }

  return ACS_CONTINUE;
}

int acs_t::SetLineBlocking()
{
  int blocking = Pop() ? ML_BLOCKING : 0;
  int lineid = Pop();
  line_t *line;
  for (int s = -1; (line = mp->FindLineFromID(lineid, &s)) != NULL; )
    line->flags = (line->flags & ~ML_BLOCKING) | blocking;

  return ACS_CONTINUE;
}

int acs_t::SetLineSpecial()
{
  int args[5];
  for (int i=4; i>=0; i--)
    args[i] = Pop();

  int special = Pop();
  int lineid = Pop();

  line_t *line;
  for (int s = -1; (line = mp->FindLineFromID(lineid, &s)) != NULL; )
    {
      line->special = special;
      for (int i=0; i<5; i++)
	line->args[i] = args[i];
    }

  return ACS_CONTINUE;
}

int acs_t::ThingSound()
{
  int volume = Pop();
  int sound = S_GetSoundID(mp->ACS_FindString(Pop()));
  int tid = Pop();

  Map::Iterate_TID iter(mp, tid);
  for (Actor *a = iter.Next(); a; a = iter.Next())
    S_StartSound(a, sound, volume/127.0);

  return ACS_CONTINUE;
}


int acs_t::EndPrintBold()
{
  int n = mp->players.size();

  for (int i = 0; i < n; i++)
    mp->players[i]->SetMessage(PrintText.c_str(), 1, PlayerInfo::M_HUD, 150);

  return ACS_CONTINUE;
}


// opcode-to-function mapping
typedef int (acs_t::*acs_func_t)();
static acs_func_t ACS_opcode_map[] =
{
  &acs_t::NOP, // 0
  &acs_t::Terminate,
  &acs_t::Suspend,
  &acs_t::PushNumber,
  NULL, // 4: ExecLineSpecial1
  NULL,
  NULL,
  NULL,
  NULL,
  NULL, // 9: ExecLineSpecialImm1
  NULL,
  NULL,
  NULL,
  NULL,
  &acs_t::Add,
  &acs_t::Sub,
  &acs_t::Mul,
  &acs_t::Div,
  &acs_t::Mod,
  &acs_t::EQ,
  &acs_t::NE,
  &acs_t::LT,
  &acs_t::GT,
  &acs_t::LE,
  &acs_t::GE,
  &acs_t::AssignScriptVar,
  &acs_t::AssignMapVar,
  &acs_t::AssignWorldVar,
  &acs_t::PushScriptVar,
  &acs_t::PushMapVar,
  &acs_t::PushWorldVar,
  &acs_t::AddScriptVar,
  &acs_t::AddMapVar,
  &acs_t::AddWorldVar,
  &acs_t::SubScriptVar,
  &acs_t::SubMapVar,
  &acs_t::SubWorldVar,
  &acs_t::MulScriptVar,
  &acs_t::MulMapVar,
  &acs_t::MulWorldVar,
  &acs_t::DivScriptVar,
  &acs_t::DivMapVar,
  &acs_t::DivWorldVar,
  &acs_t::ModScriptVar,
  &acs_t::ModMapVar,
  &acs_t::ModWorldVar,
  &acs_t::IncScriptVar,
  &acs_t::IncMapVar,
  &acs_t::IncWorldVar,
  &acs_t::DecScriptVar,
  &acs_t::DecMapVar,
  &acs_t::DecWorldVar,
  &acs_t::JMP,
  &acs_t::JNZ,
  &acs_t::PopAndDiscard,
  &acs_t::Delay,
  &acs_t::DelayImm,
  &acs_t::Random,
  &acs_t::RandomImm,
  &acs_t::ThingCount,
  &acs_t::ThingCountImm,
  &acs_t::TagWait,
  &acs_t::TagWaitImm,
  &acs_t::PolyWait,
  &acs_t::PolyWaitImm,
  &acs_t::ChangeFloor,
  &acs_t::ChangeFloorImm,
  &acs_t::ChangeCeiling,
  &acs_t::ChangeCeilingImm,
  &acs_t::Restart,
  &acs_t::LogicalAND,
  &acs_t::LogicalOR,
  &acs_t::BitwiseAND,
  &acs_t::BitwiseOR,
  &acs_t::BitwiseXOR,
  &acs_t::LogicalNOT,
  &acs_t::LeftShift,
  &acs_t::RightShift,
  &acs_t::Negate,
  &acs_t::JZ,
  &acs_t::LineSide,
  &acs_t::ScriptWait,
  &acs_t::ScriptWaitImm,
  &acs_t::ClearLineSpecial,
  &acs_t::CaseJMP,
  &acs_t::StartPrint,
  &acs_t::EndPrint,
  &acs_t::PrintString,
  &acs_t::PrintInt,
  &acs_t::PrintChar,
  &acs_t::NumPlayers,
  &acs_t::GameType,
  &acs_t::GameSkill,
  &acs_t::Timer,
  &acs_t::SectorSound,
  &acs_t::AmbientSound,
  &acs_t::SoundSequence,
  &acs_t::SetLineTexture,
  &acs_t::SetLineBlocking,
  &acs_t::SetLineSpecial,
  &acs_t::ThingSound,
  &acs_t::EndPrintBold
};

static unsigned num_acsfuncs = sizeof(ACS_opcode_map)/sizeof(acs_func_t);



//=================================================
//  Map class methods
//=================================================

/// Initialization, called during Map setup. \return true if succesful.
bool Map::ACS_LoadScripts(int lump)
{
  int length = fc.LumpLength(lump);

  if (hexen_format == false || length == 0)
    return false;

  ACS_base = static_cast<byte *>(fc.CacheLumpNum(lump, PU_LEVEL));

  struct acs_header_t
  {
    Sint32 magic;
    Sint32 info_offset;
  };
  acs_header_t *header = reinterpret_cast<acs_header_t *>(ACS_base);
  if (header->magic != *reinterpret_cast<const Sint32 *>("ACS\0"))
    {
      Z_Free(ACS_base);
      ACS_base = NULL;
      return false; // Unknown magic number
    }

  Sint32 *rover = reinterpret_cast<Sint32 *>(ACS_base + LONG(header->info_offset));
  int ACS_num_scripts = LONG(*rover++);
  if (ACS_num_scripts == 0 || ACS_num_scripts > ACS_MAX_SCRIPTS)
    {
      Z_Free(ACS_base);
      ACS_base = NULL;
      return false; // No scripts defined
    }

  // convert the script definitions

#define ACS_AUTOSTART_BASE 1000 // script numbers starting from this are automatically started when map begins
  for (int i = 0; i < ACS_num_scripts; i++)
    {
      int num = LONG(*rover++);
      acs_script_t &temp = ACS_scripts[num]; // into the map

      // fill in the properties
      temp.number = num;
      temp.code = reinterpret_cast<Sint32 *>(ACS_base + LONG(*rover++));
      temp.num_args = LONG(*rover++);
      temp.wait_data = 0;

      if (temp.number >= ACS_AUTOSTART_BASE)
	{
	  temp.number -= ACS_AUTOSTART_BASE; // TODO overlap with normal scripts?
	  ACS_StartOpenScript(&temp);
	  temp.state = ACS_running;
	}
      else
	temp.state = ACS_stopped;
    }

  // now read and convert the string table
  int ACS_num_strings = LONG(*rover++);
  if (ACS_num_strings > ACS_MAX_STRINGS)
    {
      CONS_Printf("Map::ACS_LoadScripts: Too many (%d) strings defined.\n", ACS_num_strings);
      ACS_num_strings = ACS_MAX_STRINGS; // truncate
    }

  if (ACS_num_strings > 0)
    {
      ACS_strings.resize(ACS_num_strings);
      for (int i = 0; i < ACS_num_strings; i++)
	ACS_strings[i] = reinterpret_cast<char *>(ACS_base + LONG(rover[i]));
    }

  // zero Map vars
  memset(ACS_map_vars, 0, sizeof(ACS_map_vars));
  return true;
}



// Scripts that always start immediately at level load.
void Map::ACS_StartOpenScript(acs_script_t *s)
{
  CONS_Printf("Starting an opening ACS (script %d)\n", s->number);

  acs_t *script = new acs_t(s);
  script->delay = TICRATE; // Give a second for the Map to get spawned.

  AddThinker(script);
}



/// Starts a new script running in the given Map, or queues it if it's intended for a currently nonrunning Map.
/// NOTE: Triggering data is not passed across Maps.
bool Map::ACS_StartScriptInMap(int mapnum, unsigned scriptnum, byte *args)
{
  MapInfo *m = game.FindMapInfo(mapnum);
  if (!m)
    {
      CONS_Printf("ACS script %d was requested to be run in an unknown map %d\n", scriptnum, mapnum);
      return false; // no such map
    }

  if (m->me)
    return m->me->ACS_StartScript(scriptnum, args, NULL, NULL, 0); // running map, run script immediately

  // map currently not running, queue the script
  int n = m->ACS_deferred.size();
  for (int i=0; i<n; i++)
    if (m->ACS_deferred[i].script == scriptnum)
      return false; // no duplicates

  acs_deferred_t temp;
  temp.script = scriptnum;
  temp.args[0] = args[0];
  temp.args[1] = args[1];
  temp.args[2] = args[2];
  temp.args[3] = 0;

  m->ACS_deferred.push_back(temp);
  return true;
}



/// Start (or unpause) an ACS script in this map. Returns the script instance if succesful.
acs_t *Map::ACS_StartScript(unsigned scriptnum, byte *args, Actor *triggerer, line_t *line, int side)
{
  acs_script_t *s = ACS_FindScript(scriptnum);
  if (!s)
    {
      CONS_Printf("Map::StartACS: Unknown script number %d\n", scriptnum);
      return NULL; // not found
    }

  CONS_Printf("Starting ACS script %d\n", scriptnum);

  if (s->state == ACS_suspended)
    {
      s->state = ACS_running; // resume execution
      return s->instance; // FIXME problem?
    }

  if (s->state != ACS_stopped)
    return NULL; // already running or waiting for an event

  acs_t *script = new acs_t(s);
  script->triggerer = triggerer;
  script->line = line;
  script->side = side;

  for (unsigned i = 0; i < s->num_args; i++)
    script->vars[i] = args[i];

  s->state = ACS_running;
  AddThinker(script);
  return script;
}



/// Stops a running script
bool Map::ACS_Terminate(unsigned number)
{
  acs_script_t *s = ACS_FindScript(number);
  if (!s)
    return false; // not found

  if (s->state == ACS_stopped || s->state == ACS_terminating)
    return false; // already terminated

  s->state = ACS_terminating;
  return true;
}



/// Pauses a running script
bool Map::ACS_Suspend(unsigned number)
{
  acs_script_t *s = ACS_FindScript(number);
  if (!s)
    return false; // not found

  if (s->state == ACS_stopped || s->state == ACS_suspended || s->state == ACS_terminating)
    return false; // cannot be suspended

  s->state = ACS_suspended;
  return true;
}



/// Unpauses scripts that are waiting for a particular script to finish
void Map::ACS_ScriptFinished(unsigned number)
{
  for (acs_script_iter_t i = ACS_scripts.begin(); i != ACS_scripts.end(); i++)
    {
      acs_script_t &s = i->second;
      if (s.state == ACS_waitforscript && s.wait_data == number)
	s.state = ACS_running;
    }
}



/// Returns the script with the given number, or NULL if none is found.
acs_script_t *Map::ACS_FindScript(unsigned number)
{
  acs_script_iter_t i = ACS_scripts.find(number);
  if (i == ACS_scripts.end())
    return NULL;

  return &i->second;
}



/// Safe way of querying ACS strings.
const char *Map::ACS_FindString(unsigned num)
{
  if (num < ACS_strings.size())
    return ACS_strings[num];

  return "UNDEFINED_STRING";
}



/// Starts all scripts that are waiting for this Map to start. Called at Map setup.
void Map::ACS_StartDeferredScripts()
{
  int n = info->ACS_deferred.size();
  for (int i=0; i<n; i++)
    {
      acs_deferred_t &s = info->ACS_deferred[i];
      acs_t *script = ACS_StartScript(s.script, s.args, NULL, NULL, 0);
      if (script)
	script->delay = 35;
    }

  info->ACS_deferred.clear();
}



/// Unpauses scripts that are waiting for a particular sector tag to finish
void Map::TagFinished(unsigned tag)
{
  if (TagBusy(tag) == true)
    return;

  for (acs_script_iter_t i = ACS_scripts.begin(); i != ACS_scripts.end(); i++)
    {
      acs_script_t &s = i->second;
      if (s.state == ACS_waitfortag && s.wait_data == tag)
	s.state = ACS_running;
    }
}


/// Unpauses scripts that are waiting for a particular polyobj to finish
void Map::PO_Finished(unsigned po)
{
  if (PO_Busy(po) == true)
    return;

  for (acs_script_iter_t i = ACS_scripts.begin(); i != ACS_scripts.end(); i++)
    {
      acs_script_t &s = i->second;
      if (s.state == ACS_waitforpoly && s.wait_data == po)
      s.state = ACS_running;
    }
}


//=================================================
//  Thinker class for running ACS scripts
//=================================================

IMPLEMENT_CLASS(acs_t, Thinker);
acs_t::acs_t() {}


acs_t::acs_t(acs_script_t *s)
{
  def = s;
  ip = s->code;
  sp = 0;
  memset(stack, 0, sizeof(stack));
  memset(vars, 0, sizeof(vars));

  triggerer = NULL;
  line = NULL;
  side = 0;
  delay = 0;

  s->instance = this;
}



void acs_t::Think()
{
  if (def->state == ACS_terminating)
    {
      def->state = ACS_stopped;
      def->instance = NULL;
      mp->ACS_ScriptFinished(def->number);
      mp->RemoveThinker(this);
      return;
    }

  if (def->state != ACS_running)
    return;

  if (delay)
    {
      delay--;
      return;
    }

  // run opcodes
  int result;
  for (int n = 0; n < 50000; n++) // do not get caught in infinite loops
    {
      Uint32 opcode = *ip++;

      if (opcode >= num_acsfuncs)
	{
	  CONS_Printf("ACS script %d: unknown opcode %d.\n", def->number, opcode);
	  result = ACS_HALT;
	  break;
	}
      else if (opcode >= 4 && opcode <= 13)
	{
	  // HACK: to make linespec funcs simpler
	  if (opcode >= 9)
	    result = ExecLineSpecialImm(opcode - 8);
	  else
	    result = ExecLineSpecial(opcode - 3);
	}
      else
	result = (this->*ACS_opcode_map[opcode])();

      if (result != ACS_CONTINUE)
	break;
    }

  if (result == ACS_HALT)
    {
      def->state = ACS_stopped;
      def->instance = NULL;
      mp->ACS_ScriptFinished(def->number);
      mp->RemoveThinker(this);
    }
}



