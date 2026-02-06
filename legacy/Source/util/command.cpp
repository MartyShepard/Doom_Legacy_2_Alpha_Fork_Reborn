// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: command.cpp 527 2008-01-27 02:25:14Z smite-meister $
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
/// \brief Command buffer and console variables
///   
/// Parse and execute commands from console input/scripts/
/// and remote server.
///
/// Handles console variables, which are a simplified version
/// of commands. Each consvar can have a function which is called
/// when it is modified.
///
/// Code shamelessly inspired by the QuakeC sources, thanks Id :)

#include "tnl/tnlBitStream.h"

#include "doomdef.h"
#include "command.h"
#include "console.h"
#include "z_zone.h"

#include "n_interface.h"

#include "m_misc.h"
#include "m_fixed.h"

#include "g_game.h"
#include "g_player.h"



static void COM_Alias_f();
static void COM_Echo_f();
static void COM_Exec_f();
static void COM_Wait_f();
static void COM_Help_f();
static void COM_Toggle_f();


//=========================================================================
//                           COMMAND BUFFER
//=========================================================================

/// console commands
struct xcommand_t
{
  const char  *name;
  xcommand_t  *next;
  com_func_t   function;
};


/// command aliases
struct cmdalias_t
{
  cmdalias_t *next;
  char    *name;
  char    *value;     // the command string to replace the alias
};


command_buffer_t COM;


command_buffer_t::command_buffer_t()
{
  com_commands = NULL;
  com_alias = NULL;

  com_maxsize = 256;

  com_wait = 0;

  com_argc = 0;
  com_args = NULL;
  for (int k=0; k<MAX_ARGS; k++)
    com_argv[k] = NULL;
}


//  Initialise command buffer and add basic commands
void command_buffer_t::Init()
{
  CONS_Printf("Initializing the command buffer.\n");

#define COM_BUF_SIZE    8192   // command buffer size

  // set command buffer maximum size
  com_maxsize = COM_BUF_SIZE;

  // add standard commands
  AddCommand("alias",COM_Alias_f);
  AddCommand("echo", COM_Echo_f);
  AddCommand("exec", COM_Exec_f);
  AddCommand("wait", COM_Wait_f);
  AddCommand("help", COM_Help_f);
  AddCommand("toggle", COM_Toggle_f);
}








//  Add text in the command buffer (for later execution)
//
void command_buffer_t::AppendText(const char *text)
{
  unsigned len = strlen(text);

  if (com_text.length() + len >= com_maxsize)
    {
      CONS_Printf("Command buffer full!\n");
      return;
    }
  com_text.append(text);
}


// Adds command text in front of the command buffer (immediately after the current command)
// Adds a \n to the text
void command_buffer_t::PrependText(const char *text)
{
  com_text.insert(0, text);
}


//  Flush (execute) console commands in buffer
//   does only one if com_wait
//
void command_buffer_t::BufExecute()
{
  int     i;

  if (com_wait)
    {
      com_wait--;
      return;
    }

  while (!com_text.empty())
    {
      // find a '\n' or ; line break
      const char *text = com_text.c_str();
      int n = com_text.length();

      int quotes = 0;
      for (i=0; i < n; i++)
        {
          if (text[i] == '"')
            quotes++;
          if ( !(quotes&1) &&  text[i] == ';')
            break;  // don't break if inside a quoted string
          if (text[i] == '\n' || text[i] == '\r')
            break;
        }

      char line[1024];
      memcpy(line, text, i);
      line[i] = 0;

      // flush the command text from the command buffer, _BEFORE_
      // executing, to avoid that 'recursive' aliases overflow the
      // command text buffer, in that case, new commands are inserted
      // at the beginning, in place of the actual, so it doesn't
      // overflow
      if (i == n)
        // the last command was just flushed
        com_text.clear();
      else
        {
          i++;
	  com_text.erase(0, i);
        }

      // execute the command line
      COM_ExecuteString(line);

      // delay following commands if a wait was encountered
      if (com_wait)
        {
          com_wait--;
          break;
        }
    }
}


// =========================================================================
//                            COMMAND EXECUTION
// =========================================================================



PlayerInfo *com_player; // player associated with the current command. 
// Only "interactive" commands may require this.


int command_buffer_t::CheckParm(const char *check)
{
  for (int i = 1; i < com_argc; i++)
    {
      if (!strcasecmp(check, com_argv[i]))
        return i;
    }
  return 0;
}


// Parses the given string into command line tokens.
//
// Takes a null terminated string.  Does not need to be /n terminated.
// breaks the string up into arg tokens.
void command_buffer_t::COM_TokenizeString(byte *text)
{
  // clear the args from the last string
  for (int i=0 ; i<com_argc ; i++)
    if (com_argv[i])
      {
	Z_Free(com_argv[i]);
	com_argv[i] = NULL;
      }

  com_argc = 0;
  com_args = NULL;

  while (1)
    {
      // skip whitespace up to a /n
      while (*text && *text <= ' ' && *text != '\n')
        text++;

      if (*text == '\n')
        {   // a newline means end of command in buffer,
          // thus end of this command's args too
          text++;
          break;
        }

      if (!*text)
        return;

      if (com_argc == 1)
        com_args = reinterpret_cast<char *>(text);

      text = COM_Parse(text);
      if (!text)
        return;

      if (com_argc < MAX_ARGS)
        {
          com_argv[com_argc] = (char *)ZZ_Alloc(strlen(com_token) + 1);
          strcpy(com_argv[com_argc], com_token);
          com_argc++;
        }
    }
}


// Add a command before existing ones.
//
void command_buffer_t::AddCommand(const char *name, com_func_t func)
{
  xcommand_t  *cmd;

  // fail if the command is a variable name
  if (consvar_t::FindVar(name))
    {
      CONS_Printf ("%s is a variable name\n", name);
      return;
    }

  // fail if the command already exists
  for (cmd=com_commands ; cmd ; cmd=cmd->next)
    {
      if (!strcasecmp(name, cmd->name))
        {
          CONS_Printf("Command %s already exists\n", name);
          return;
        }
    }

  cmd = static_cast<xcommand_t*>(Z_Malloc(sizeof(xcommand_t), PU_STATIC, NULL));
  cmd->name = name;
  cmd->function = func;
  cmd->next = com_commands;
  com_commands = cmd;
}


//  Returns true if a command by the name given exists
//
bool command_buffer_t::Exists(const char *com_name)
{
  for (xcommand_t *cmd = com_commands ; cmd ; cmd=cmd->next)
    {
      if (!strcasecmp(com_name,cmd->name))
        return true;
    }

  return false;
}


//  Command completion using TAB key like '4dos'
//  Will skip 'skips' commands
//
const char *command_buffer_t::CompleteCommand(char *partial, int skips)
{
  int len = strlen(partial);

  if (!len)
    return NULL;

  // check functions
  for (xcommand_t  *cmd=com_commands ; cmd ; cmd=cmd->next)
    if (!strncasecmp(partial,cmd->name, len))
      if (!skips--)
        return cmd->name;

  return NULL;
}



// Parses a single line of text into arguments and tries to execute it.
// The text can come from the command buffer, a remote client, or stdin.
//
void command_buffer_t::COM_ExecuteString(char *text)
{
  xcommand_t  *cmd;
  cmdalias_t *a;

  COM_TokenizeString(reinterpret_cast<byte*>(text)); // UTF-8 is easier to handle using unsigned chars

  // execute the command line
  if (!Argc())
    return;     // no tokens

  // try to find the player "using" the command buffer
  for (int i=0; i<NUM_LOCALPLAYERS; i++)
    {
      com_player = LocalPlayers[0].info;
      if (com_player)
	break;
    }

  // check functions
  for (cmd=com_commands ; cmd ; cmd=cmd->next)
    {
      if (!strcasecmp(com_argv[0],cmd->name))
        {
          cmd->function();
          return;
        }
    }

  // check aliases
  for (a=com_alias ; a ; a=a->next)
    {
      if (!strcasecmp(com_argv[0], a->name))
        {
          PrependText (a->value);
          return;
        }
    }

  // check cvars
  // Hurdler: added at Ebola's request ;)
  // (don't flood the console in software mode with bad gr_xxx command)
  if (!consvar_t::Command())
    {
      CONS_Printf("Unknown command '%s'\n", Argv(0));
    }
}



//============================================================================
//                            SCRIPT PARSE
//============================================================================

//  Parse a token out of a string, handles script files too
//  returns the data pointer after the token
byte *command_buffer_t::COM_Parse(byte *data)
{
  if (!data)
    return NULL;

  int c;
  int len = 0;
  com_token[0] = 0;

  // skip whitespace
 skipwhite:
  while ( (c = *data) <= ' ')
    {
      if (c == 0)
        return NULL;            // end of file;
      data++;
    }

  // skip // comments
  if (c=='/' && data[1] == '/')
    {
      while (*data && *data != '\n')
        data++;
      goto skipwhite;
    }


  // handle quoted strings specially
  if (c == '\"')
    {
      data++;
      while (1)
        {
          c = *data++;
          if (c=='\"' || !c)
            {
              com_token[len] = 0;
              return data;
            }
          com_token[len] = c;
          len++;
        }
    }

  // parse single characters
  if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':')
    {
      com_token[len] = c;
      len++;
      com_token[len] = 0;
      return data+1;
    }

  // parse a regular word
  do
    {
      com_token[len] = c;
      data++;
      len++;
      c = *data;
      if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':')
        break;
    } while (c>32);

  com_token[len] = 0;
  return data;
}


// =========================================================================
//                            SCRIPT COMMANDS
// =========================================================================


// alias command : a command name that replaces another command
//
static void COM_Alias_f()
{
  cmdalias_t  *a;
  char        cmd[1024];
  int         i, c;

  if (COM.Argc()<3)
    {
      CONS_Printf("alias <name> <command>\n");
      return;
    }

  a = (cmdalias_t *)ZZ_Alloc(sizeof(cmdalias_t));
  a->next = COM.com_alias;
  COM.com_alias = a;

  a->name = Z_StrDup (COM.Argv(1));

  // copy the rest of the command line
  cmd[0] = 0;     // start out with a null string
  c = COM.Argc();
  for (i=2 ; i< c ; i++)
    {
      strcat(cmd, COM.Argv(i));
      if (i != c)
        strcat(cmd, " ");
    }
  strcat (cmd, "\n");

  a->value = Z_StrDup(cmd);
}


// Echo a line of text to console
//
static void COM_Echo_f()
{
  for (int i = 1; i < COM.Argc(); i++)
    CONS_Printf("%s ",COM.Argv(i));
  CONS_Printf("\n");
}


// Execute a script file
//
static void COM_Exec_f()
{
  if (COM.Argc() != 2)
    {
      CONS_Printf ("exec <filename> : run a script file\n");
      return;
    }

  byte *buf = NULL;

  // load file
  FIL_ReadFile(COM.Argv(1), &buf);

  if (!buf)
    {
      CONS_Printf ("Couldn't execute file %s\n",COM.Argv(1));
      return;
    }

  CONS_Printf ("Executing %s\n",COM.Argv(1));

  // insert text file into the command buffer
  COM.PrependText((char *)buf);

  // free buffer
  Z_Free(buf);
}


// Delay execution of the rest of the commands to the next frame,
// allows sequences of commands like "jump; fire; backward"
//
static void COM_Wait_f()
{
  if (COM.Argc()>1)
    COM.com_wait = atoi(COM.Argv(1));
  else
    COM.com_wait = 1;   // 1 frame
}

static void COM_Help_f()
{
  consvar_t  *cvar;

  if (COM.Argc() > 1)
    {
      cvar = consvar_t::FindVar(COM.Argv(1));
      if (cvar)
        {
          CONS_Printf("Variable %s:\n", cvar->name);
          CONS_Printf("  flags :");
          if( cvar->flags & CV_SAVE )
            CONS_Printf("AUTOSAVE ");
          if( cvar->flags & CV_FLOAT )
            CONS_Printf("FLOAT ");
          if( cvar->flags & CV_NETVAR )
            CONS_Printf("NETVAR ");
          if( cvar->flags & CV_CALL )
            CONS_Printf("ACTION ");
          CONS_Printf("\n");
          if (cvar->PossibleValue)
            {
              if (!strcmp(cvar->PossibleValue[0].strvalue, "MIN"))
                {
                  CONS_Printf("  range from %d to %d\n",cvar->PossibleValue[0].value, cvar->PossibleValue[1].value);
                }
              else
                {
                  CONS_Printf("  possible values:\n");
                  for (int i=0; cvar->PossibleValue[i].strvalue; i++)
		    CONS_Printf("    %-2d : %s\n",cvar->PossibleValue[i].value, cvar->PossibleValue[i].strvalue);
                }
            }
        }
      else
        CONS_Printf("No Help for this command/variable\n");
    }
  else
    {
      int i=0;

      // commands
      CONS_Printf("\2Commands:\n");
      for (xcommand_t *cmd = COM.com_commands; cmd; cmd=cmd->next)
        {
          CONS_Printf("%s ",cmd->name);
          i++;
        }

      // variables
      CONS_Printf("\2\nVariables:\n");
      for (cvar = consvar_t::cvar_list; cvar; cvar = cvar->next)
        {
          CONS_Printf("%s ",cvar->name);
          i++;
        }

      CONS_Printf("\2\nread docs/console.html for more or type help <command or variable>\n");

      if (devparm)
        CONS_Printf("\2Total : %d\n",i);
    }
}

static void COM_Toggle_f()
{
  if (COM.Argc() != 2 && COM.Argc() != 3)
    {
      CONS_Printf("Toggle <cvar_name> [-1]\n"
                  "Toggle the value of a cvar\n");
      return;
    }
  consvar_t *cvar = consvar_t::FindVar(COM.Argv(1));
  if (!cvar)
    {
      CONS_Printf("%s is not a cvar\n",COM.Argv(1));
      return;
    }

  // netcvar don't change imediately
  cvar->flags |= CV_ANNOUNCE_ONCE;
  if (COM.Argc() == 3)
    cvar->AddValue(atol(COM.Argv(2)));
  else
    cvar->AddValue(1);
}


//==========================================================================
//
//                           CONSOLE VARIABLES
//
//   console variables are a simple way of changing variables of the game
//   through the console or code, at run time.
//
//   console vars acts like simplified commands, because a function can be
//   attached to them, and called whenever a console var is modified
//
//==========================================================================

consvar_t *consvar_t::cvar_list = NULL;


CV_PossibleValue_t CV_OnOff[] =    {{0,"Off"}, {1,"On"},    {0,NULL}};
CV_PossibleValue_t CV_YesNo[] =    {{0,"No"} , {1,"Yes"},   {0,NULL}};
CV_PossibleValue_t CV_Unsigned[] = {{0,"MIN"}, {999999999,"MAX"}, {0,NULL}};


//  Search if a variable has been registered
//  returns true if given variable has been registered
consvar_t *consvar_t::FindVar(const char *name)
{
  for (consvar_t *cvar = cvar_list; cvar; cvar = cvar->next)
    if (!strcasecmp(name, cvar->name))
      return cvar;

  return NULL;
}


//  Build a unique Net Variable identifier number, that is used
//  in network packets instead of the fullname
Uint16 consvar_t::ComputeNetid(const char *s)
{
  static int premiers[16] = {2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53};

  Uint16 ret = 0;
  int i = 0;
  while (*s)
    {
      ret += (*s)*premiers[i];
      s++;
      i = (i+1)%16;
    }
  return ret;
}


//  Return the Net Variable, from it's identifier number
consvar_t *consvar_t::FindNetVar(unsigned short netid)
{
  for (consvar_t *cvar = cvar_list; cvar; cvar = cvar->next)
    if (cvar->netid == netid)
      return cvar;

  return NULL;
}


//
// set value to the variable, no checking, only for internal use
//
bool consvar_t::Setvalue(const char *s)
{   
  if (PossibleValue)
  {
    char *tail;
    int v = strtol(s, &tail, 0);
      
    if (!strcmp(PossibleValue[0].strvalue, "MIN"))
    {
     
      // bounded cvar
      if (v < PossibleValue[0].value)
        v = PossibleValue[0].value;
      else if (v > PossibleValue[1].value)
        v = PossibleValue[1].value;

      value = v;
      sprintf(str, "%d", v);
    }
    else
    {
      // array of value/name pairs
      int i;

      /* Marty, "StringHandle"
       * Dies Asunhame gilt für String in dem sich das zeichen ':'
       * befindet wie 4:3,5:4.
       */      
      byte StringHandle = 0;
      const char *szColon = strrchr(s, ':');
      if (szColon != NULL)    
        StringHandle =1;  
  
      if ((tail == s) || (StringHandle == 1))
      {
        // no succesful number conversion, so it's a string
        for (i=0; PossibleValue[i].strvalue; i++)
        {
          if (!strcasecmp(PossibleValue[i].strvalue, s))
            break;
        }
      }
      else
      {     
        // int value then
        for (i=0; PossibleValue[i].strvalue; i++)
        {
          if (v == PossibleValue[i].value)
            break;
        }
      }

      if (!PossibleValue[i].strvalue)
      {
          CONS_Printf("\"%s\" is not a possible value for \"%s\"\n", s, name);
          return false;
      }

      value = PossibleValue[i].value;
      strncpy(str, PossibleValue[i].strvalue, CV_STRLEN);
    }
  }
  else
  {
    strncpy(str, s, CV_STRLEN);

    if (flags & CV_FLOAT)
      value = int(atof(str) * fixed_t::UNIT); // 16.16 fixed point
    else
      value = atoi(str);
  }

  if (flags & CV_ANNOUNCE_ONCE || flags & CV_ANNOUNCE)
  {
    CONS_Printf("%s set to %s\n", name, str);
      flags &= ~CV_ANNOUNCE_ONCE;
  }

  flags |= CV_MODIFIED;
  // raise 'on change' code
  if (flags & CV_CALL)
    func();

  return true;
}


//  Register a variable, that can be used later at the console
bool consvar_t::Reg()
{
  // link the variable in, unless it is for internal use only
  if (!(flags & CV_HIDDEN))
    {
      // first check to see if it has already been defined
      if (FindVar(name))
	{
	  CONS_Printf("Variable %s is already defined\n", name);
	  return false;
	}

      // check for overlap with a command
      if (COM.Exists(name))
	{
	  I_Error("%s is a command name\n", name);
	  return false;
	}

      // check net variables
      if (flags & CV_NETVAR)
	{
	  netid = ComputeNetid(name);
	  if (FindNetVar(netid))
	    {
	      I_Error("Variable %s has same netid\n", name);
	      return false;
	    }
	}

      next = cvar_list;
      cvar_list = this;
    }
  else
    {
      netid = 0;
      next = NULL;
    }

  str[0] = '\0';

  if (flags & CV_HANDLER)
    {
      flags &= ~CV_CALL;
      value = 0;
      return true;
    }

  if ((flags & CV_NOINIT) && !(flags & CV_CALL))
    I_Error("variable %s has CV_NOINIT without CV_CALL\n", name);

  if ((flags & CV_CALL) && !func)
    I_Error("variable %s has CV_CALL without func", name);

  // check possible values list
  // It must either be NULL, a terminated array of value/name combinations, or a MIN/MAX pair.
  if (PossibleValue)
  {
    if (!strcmp(PossibleValue[0].strvalue, "MIN"))
    {
      // MIN/MAX pair
      if (strcmp(PossibleValue[1].strvalue, "MAX"))
        I_Error("Bounded cvar \"%s\" without maximum!", name);
      if (PossibleValue[0].value >= PossibleValue[1].value)
        I_Error("Bounded cvar \"%s\" has no proper range!", name);
    }
  }

  if (flags & CV_NOINIT)
    flags &= ~CV_CALL;

  if (!Setvalue(defaultvalue))
    I_Error("Variable %s default value \"%s\" is not a possible value\n", name, defaultvalue);

  if (flags & CV_NOINIT)
    flags |= CV_CALL;

  // the SetValue will set this bit
  flags &= ~CV_MODIFIED;
  return true;
}



//  Completes the name of a console var
const char *consvar_t::CompleteVar(const char *partial, int skips)
{
  int len = strlen(partial);

  if (!len)
    return NULL;

  // check functions
  for (consvar_t *cvar = cvar_list; cvar; cvar = cvar->next)
    if (!strncasecmp(partial, cvar->name, len))
      if (!skips--)
        return cvar->name;

  return NULL;
}


// set a new value to a netvar
void consvar_t::GotNetVar(unsigned short id, const char *str)
{
  consvar_t *cvar = consvar_t::FindNetVar(id);
  if (!cvar)
    {
      CONS_Printf("\2Netvar not found\n");
      return;
    }
  cvar->Setvalue(str);
}


// write the netvars into a packet
void consvar_t::SaveNetVars(TNL::BitStream &s)
{
  for (consvar_t *cvar = cvar_list; cvar; cvar = cvar->next)
    if (cvar->flags & CV_NETVAR)
      {
	s.write(cvar->netid);
        s.writeString(cvar->str);
      }
}

// read the netvars from a packet
void consvar_t::LoadNetVars(TNL::BitStream &s)
{
  for (consvar_t *cvar = cvar_list; cvar; cvar = cvar->next)
    if (cvar->flags & CV_NETVAR)
      {
	// the for loop is just used for count
	unsigned short id;
	s.read(&id);
	char temp[256];
        s.readString(temp);
	GotNetVar(id, temp);
      }
}


// as if "<varname> <value>" is entered at the console
void consvar_t::Set(const char *s)
{
  if (!(flags & CV_HIDDEN))
    {
      consvar_t *cv;
      // am i registered?
      for (cv = cvar_list; cv; cv = cv->next)
	if (cv == this)
	  break;

      if (!cv)
	Reg();

      if (flags & CV_NOTINNET && game.netgame)
	{
	  CONS_Printf("This variable cannot be changed while in a netgame.\n");
	  return;
	}
    }

#ifdef PARANOIA
  if (!str)
    I_Error("CV_Set : %s no string set ?!\n",name);
#endif

  if (!strcmp(str, s))
    return; // no changes

  if (flags & CV_NETVAR && game.netgame)
    {
      // send the value of the variable
      if (!game.server)
        {
          CONS_Printf("Only the server can change this variable\n");
          return;
        }

      if (Setvalue(s))
	game.net->SendNetVar(netid, str);

      return;
    }

  Setvalue(s);
}


//  Expands value to string before calling CV_Set()
void consvar_t::Set(int newval)
{
  char val[32];
  sprintf(val, "%d", newval);
  Set(val);
}

// increments the cvar
void consvar_t::AddValue(int increment)
{
  if (flags & CV_HANDLER)
    {
      reinterpret_cast<void (*)(consvar_t*, int)>(func)(this, increment);
      return;
    }

  int newvalue = value + increment;

  if (PossibleValue)
    {
      if (!strcmp(PossibleValue[0].strvalue, "MIN"))
        {
	  // bounded cvar
	  int minval = PossibleValue[0].value;
	  int maxval = PossibleValue[1].value;

	  // wrap
          if (newvalue < minval)
            newvalue = maxval;
          else if (newvalue > maxval)
            newvalue = minval;
        }
      else
        {
	  int n; // count all possible values
          int i = -1;

          // values must not be repeated in possiblevalues array
          for (n=0; PossibleValue[n].strvalue; n++)
            if (PossibleValue[n].value == value)
              i = n;

#ifdef PARANOIA
          if (i == -1)
            I_Error("consvar_t::AddValue: current value %d not found in possible values!\n", value);
#endif
          i = (i + increment + n) % n;
	  if (i < 0)
	    i = 0; // if increment is -1000 or something
          Set(PossibleValue[i].strvalue);
	  return;
        }
    }

  Set(newvalue);
}


/// Returns value as fixed_t, handles CV_FLOAT correctly.
fixed_t consvar_t::Get() const
{
  fixed_t res;
  if (flags & CV_FLOAT)
    res.setvalue(value);
  else
    res = value;

  return res;
}


//  Displays or changes variable from the console
//
//  Returns false if the passed command was not recognised as
//  console variable.
bool consvar_t::Command()
{
  // check variables
  consvar_t *v = FindVar(COM.Argv(0));
  if (!v)
    return false;

  // perform a variable print or set
  if (COM.Argc() == 1)
    {
      CONS_Printf("\"%s\" is \"%s\" default is \"%s\"\n", v->name, v->str, v->defaultvalue);
      return true;
    }

  v->Set(COM.Argv(1));
  return true;
}


//  Save console variables that have the CV_SAVE flag set
void consvar_t::SaveVariables(FILE *f)
{
  for (consvar_t *cvar = cvar_list; cvar; cvar=cvar->next)
    if (cvar->flags & CV_SAVE)
      fprintf(f, "%s \"%s\"\n", cvar->name, cvar->str);
}
