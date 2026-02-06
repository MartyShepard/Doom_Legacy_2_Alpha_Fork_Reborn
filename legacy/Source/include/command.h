// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: command.h 527 2008-01-27 02:25:14Z smite-meister $
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
/// \brief Command buffer

#ifndef command_h
#define command_h 1

#include <string>
#include <stdio.h>

#include "doomtype.h"

using namespace std;

//===================================
// Command buffer & command execution
//===================================

typedef void (*com_func_t)();
extern class PlayerInfo *com_player;

/// \brief The command buffer.
class command_buffer_t
{
public:
  struct xcommand_t  *com_commands; ///< currently known commands
  struct cmdalias_t  *com_alias; ///< aliases list

  string com_text; ///< actual command buffer
  unsigned com_maxsize;

  int    com_wait; ///< one command per frame (for cmd sequences)

  // parsing
#define MAX_ARGS        80
  int         com_argc;
  char       *com_args; ///< current command args or NULL
  char       *com_argv[MAX_ARGS];

  char com_token[1024];

protected:
  void  COM_ExecuteString(char *text);
  void  COM_TokenizeString(byte *text);
  byte *COM_Parse(byte *data);

public:
  /// Constructor
  command_buffer_t();

  /// Setup command buffer, at game startup.
  void Init();

  void AddCommand(const char *name, com_func_t func);


  /// Add text to the end of the command buffer
  void AppendText(const char *text);
  /// Add text to the beginning of the command buffer
  void PrependText(const char *text);

  /// Execute commands in buffer, flush them.
  void BufExecute();


  /// Returns how many args for last command.
  int Argc() { return com_argc; }

  /// Returns string pointer of all command args.
  char *Args() { return com_args; }

  /// Returns string pointer for given argument number.
  const char *Argv(int arg)
  {
    if (arg >= com_argc || arg < 0)
      return "";
    return com_argv[arg];
  }

  int CheckParm(const char *check);


  const char *CompleteCommand(char *partial, int skips);

  /// Checks if the named command exists.
  bool Exists(const char *com_name);

};

extern command_buffer_t COM;



//==================
// Console variables
//==================

/// flags for console variables
enum cvflags_t
{
  CV_SAVE       = 0x0001,    ///< cvar is saved in the config file when the game exits
  CV_CALL       = 0x0002,    ///< call function on change
  CV_NETVAR     = 0x0004,    ///< sent over net from server to clients
  CV_NOINIT     = 0x0008,    ///< don't call function when cvar is registered (1st set)
  CV_FLOAT      = 0x0010,    ///< the value is 16.16 fixed point
  CV_NOTINNET   = 0x0020,    ///< can't be changed in netgame (NOTE: currently not used)
  CV_MODIFIED   = 0x0040,    ///< set when cvar is modified
  CV_ANNOUNCE   = 0x0080,    ///< print a message to console when modified
  CV_ANNOUNCE_ONCE = 0x0100, ///< same but will be reset when modified, set in toggle
  CV_HIDDEN     = 0x0200,    ///< cannot be accessed by the console (not part of the cvar list) (menu etc.)
  CV_HANDLER    = 0x0400,    ///< Call a handler function in AddValue. Overrides CV_CALL. HACK for menu.
};

/// \brief value restriction for consvar_t objects
struct CV_PossibleValue_t
{
  int   value;
  const char *strvalue;
};

extern CV_PossibleValue_t CV_OnOff[];
extern CV_PossibleValue_t CV_YesNo[];
extern CV_PossibleValue_t CV_Unsigned[];


namespace TNL { class BitStream; };

/// \brief console variables
///
/// Console vars are variables that can be changed through code or console,
/// at RUN TIME. They can also act as simplified commands, because a func-
/// tion can be attached to a console var, which is called whenever the
/// variable is modified (using flag CV_CALL).

struct consvar_t
{
  enum
  {
    CV_STRLEN = 64
  };

  const char*name;
  const char*defaultvalue;
  int        flags;        ///< flags from cvflags_t
  CV_PossibleValue_t *PossibleValue;  ///< array of possible values
  void     (*func)();      ///< called on change, if CV_CALL is set
  int        value;        ///< int/fixed_t value
  char       str[CV_STRLEN];          ///< value in string format
  Uint16     netid;        ///< unique network id for CV_NETVARs

  consvar_t *next;         ///< linked list

  static consvar_t *cvar_list; ///< list of registered console variables

protected:
  /// internal setting method
  bool Setvalue(const char *s);

  static Uint16 ComputeNetid(const char *s);
  static consvar_t *FindNetVar(Uint16 netid);

public:
  /// as if "<varname> <value>" was entered at the console
  void Set(const char *value);

  /// expands value to a string before setting it
  void Set(int value);

  /// it a setvalue but with a modulo at the maximum
  void AddValue(int increment);

  /// returns the value, handles CV_FLOAT properly
  class fixed_t Get() const;

  /// register a variable for use at the console
  bool Reg();

  /// displays or changes variable through the console
  static bool Command();

  /// returns the cvar with the corresponding name or NULL
  static consvar_t *FindVar(const char *name);

  /// returns the name of the nearest console variable name found
  static const char *CompleteVar(const char *partial, int skips);

  /// write all CV_SAVE variables to config file
  static void SaveVariables(FILE *f);

  /// save all CV_NETVAR variables into a buffer
  static void SaveNetVars(TNL::BitStream &s);
  /// load all CV_NETVAR variables from a buffer
  static void LoadNetVars(TNL::BitStream &s);
  /// received a new value for a netvar
  static void GotNetVar(unsigned short id, const char *str);
};

#endif
