// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// $Id: t_parse.h 570 2009-11-27 23:57:47Z smite-meister $
//
// Copyright(C) 2000 Simon Howard
// Copyright(C) 2001-2006 Doom Legacy Team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------

/// \file
/// \brief FS parsing

#ifndef t_parse_h
#define t_parse_h 1

#include "m_fixed.h"
#include "t_vari.h"


#define T_MAXTOKENS 128
#define TOKENLENGTH 128


/// \brief FS {} sections
struct fs_section_t
{
  char *start;    // offset of starting brace {
  char *end;      // offset of ending brace   }
  int type;       // section type: for() loop, while() loop etc
  
  union
  {
    struct
    {
      char *loopstart;  // positioned before the while()
    } data_loop;
  } data; // data for section
  
  fs_section_t *next;        // for hashing
};


/// \brief FS {} section types
enum fs_section_e
{
  st_empty,       // none: empty {} braces
  st_if,          // if() statement
  st_elseif,      // elseif() statement
  st_else,        // else() statement
  st_loop,        // loop
};


#define VARIABLESLOTS 16
#define SECTIONSLOTS 17
#define MAXSCRIPTS 256

/// \brief FS script definition
struct script_t
{
  char *data;     ///< script data 
  int scriptnum;  ///< this script's number
  int len;
  
  /// {} sections
  fs_section_t *sections[SECTIONSLOTS];
  
  /// variables, goto labels
  struct svariable_t *variables[VARIABLESLOTS];
  
  // ptr to the parent script
  // the parent script is the script above this level
  // eg. individual linetrigger scripts are children
  // of the levelscript, which is a child of the
  // global_script
  script_t *parent;

  // child scripts.
  // levelscript holds ptrs to all of the level's scripts
  // here.
  script_t *children[MAXSCRIPTS];
  
  class Actor      *trigger;  // object which triggered this script

  //SoM: Used for if/elseif/else statements
  bool  lastiftrue;

protected:

  void clear_variables();

  inline int section_hash(char *b) { return int((b - data) % SECTIONSLOTS); }
  fs_section_t *new_section(char *brace);
  svariable_t *new_label(char *labelptr);

  void parse();

public: 
  void clear(); // nukes sections, variables, children (does not free them!)

  void preprocess();
  void dry_run();
  void run();
  void save(char *r, int wt, int wdata);
  void continue_script(char *continue_point);

  // variables are hashed for speed. this is the hashkey.
  inline static int variable_hash(const char *n)
  {
    return ((n[0] + n[1] + (n[1] ? n[2] + (n[2] ? n[3] : 0) : 0)) % VARIABLESLOTS);
  }

  fs_section_t *find_section_start(char *brace);
  fs_section_t *find_section_end(char *brace);
  char *process_find_char(char *data, char find);
  svariable_t *new_variable(const char *name, int vtype);
  svariable_t *variableforname(const char *name);

  /// Saving and loading
  int  Serialize(class LArchive &a);
  int  Unserialize(LArchive &a);
};





/// \brief FS operator definition
struct operator_t
{
  const char *str;
  svalue_t (*handler)(int, int, int); // left, mid, right
  int direction;
};

extern operator_t operators[];
extern int num_operators;

enum operator_dir_e
{
  opdir_forward,
  opdir_backward
};

void parse_data(char *data, char *end);
void parse_include(char *lumpname);
void run_statement();
void script_error(const char *s, ...);

svalue_t evaluate_expression(int start, int stop);
int find_operator(int start, int stop, const char *value);
int find_operator_backwards(int start, int stop, const char *value);

char *get_tokens(char *r);

/******* tokens **********/

enum tokentype_t
{
  TO_name,   // a name, eg 'count1' or 'frag'
  TO_number,
  TO_oper,
  TO_string,
  TO_unset,
  TO_function          // function name
};


struct token_t
{
  char *v;
  tokentype_t type;
};


enum    // brace types: where current_section is a { or }
{
  bracket_open,
  bracket_close
};

extern svalue_t nullvar;
extern int script_debug;

extern script_t *current_script;
extern class Map *current_map;
extern class Actor *trigger_obj;
extern class PlayerInfo *trigger_player;
extern bool killscript;

extern token_t tokens[T_MAXTOKENS];
extern int num_tokens;

extern char *linestart; // start of the current expression
extern char *rover;     // current point reached in script

extern fs_section_t *current_section;
extern fs_section_t *prev_section;
extern int bracetype;

// the global_script is the root
// script and contains only built-in
// FraggleScript variables/functions

extern script_t global_script; 
extern script_t hub_script;

#endif
