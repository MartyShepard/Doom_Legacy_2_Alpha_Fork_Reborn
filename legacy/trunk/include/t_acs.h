// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2003-2007 by DooM Legacy Team.
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
/// \brief Scripting: ACS interpreter, new implementation.

#ifndef t_acs_h
#define t_acs_h 1

#include <map>
#include "doomtype.h"

#define ACS_WORLD_VARS 64
#define ACS_LOCAL_VARS 10
#define ACS_STACKSIZE 32


extern Sint32 ACS_world_vars[ACS_WORLD_VARS];


/// ACS script states
enum acs_state_t
{
  ACS_stopped,       ///< not running at all
  ACS_running,       ///< running
  ACS_suspended,     ///< paused
  ACS_waitforscript, ///< waiting for another script to finish
  ACS_waitfortag,    ///< waiting for a tag to finish
  ACS_waitforpoly,   ///< waiting for a poly to finish
  ACS_terminating    ///< will stop on next Think
};


/// \brief ACS script definition
struct acs_script_t
{
  unsigned   number; ///< external script number (given by the author)
  Sint32      *code; ///< pointer to the first opcode
  unsigned num_args; ///< number of arguments the script requires
  acs_state_t state; ///< state of the script
  Uint32  wait_data;
  class acs_t *instance; ///< running instance of the script, or NULL (NOTE: ACS only allows one instance per script at a time!)
};


/// \brief Thinker class for running AC scripts
class acs_t : public Thinker
{
  DECLARE_CLASS(acs_t);
protected:
  acs_script_t *def; ///< Script definition.
  Sint32       *ip;  ///< Instruction Pointer into the raw BEHAVIOR lump.
  int           sp;  ///< Stack Pointer, past-the-end (points to the first unused cell).
  Sint32 stack[ACS_STACKSIZE]; ///< Stack for this script instance.

  /// Stack manipulation
  void   Push(Sint32 value) { stack[sp++] = value; }
  Sint32 Pop() { return stack[--sp]; }
  Sint32 Top() { return stack[sp - 1]; }

public:
  Sint32 vars[ACS_LOCAL_VARS]; ///< Local variables for the script instance.

  Actor  *triggerer; ///< Actor who triggered the script, or NULL
  line_t *line;      ///< line that triggered the script, or NULL
  int     side;      ///< side from which the triggering happened

  int delay; ///< tics to wait before execution continues

public:
  acs_t(acs_script_t *s);

  virtual void Think();

  /// Opcode functions
  int NOP();
  int Terminate();
  int Suspend();
  int PushNumber();
  int ExecLineSpecial(int num_args);
  int ExecLineSpecialImm(int num_args);
  int Add();
  int Sub();
  int Mul();
  int Div();
  int Mod();
  int EQ();
  int NE();
  int LT();
  int GT();
  int LE();
  int GE();
  int AssignScriptVar();
  int AssignMapVar();
  int AssignWorldVar();
  int PushScriptVar();
  int PushMapVar();
  int PushWorldVar();
  int AddScriptVar();
  int AddMapVar();
  int AddWorldVar();
  int SubScriptVar();
  int SubMapVar();
  int SubWorldVar();
  int MulScriptVar();
  int MulMapVar();
  int MulWorldVar();
  int DivScriptVar();
  int DivMapVar();
  int DivWorldVar();
  int ModScriptVar();
  int ModMapVar();
  int ModWorldVar();
  int IncScriptVar();
  int IncMapVar();
  int IncWorldVar();
  int DecScriptVar();
  int DecMapVar();
  int DecWorldVar();
  int JMP();
  int JNZ();
  int PopAndDiscard();
  int Delay();
  int DelayImm();
  int Random();
  int RandomImm();
  int ThingCount();
  int ThingCountImm();
  void CountThings(int type, int tid);
  int TagWait();
  int TagWaitImm();
  int PolyWait();
  int PolyWaitImm();
  int ChangeFloor();
  int ChangeFloorImm();
  int ChangeCeiling();
  int ChangeCeilingImm();
  int Restart();
  int LogicalAND();
  int LogicalOR();
  int BitwiseAND();
  int BitwiseOR();
  int BitwiseXOR();
  int LogicalNOT();
  int LeftShift();
  int RightShift();
  int Negate();
  int JZ();
  int LineSide();
  int ScriptWait();
  int ScriptWaitImm();
  int ClearLineSpecial();
  int CaseJMP();
  int StartPrint();
  int EndPrint();
  int PrintString();
  int PrintInt();
  int PrintChar();
  int NumPlayers();
  int GameType();
  int GameSkill();
  int Timer();
  int SectorSound();
  int AmbientSound();
  int SoundSequence();
  int SetLineTexture();
  int SetLineBlocking();
  int SetLineSpecial();
  int ThingSound();
  int EndPrintBold();
};

#endif
