// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// $Id: t_spec.cpp 570 2009-11-27 23:57:47Z smite-meister $
//
// Copyright(C) 2000 Simon Howard
// Copyright(C) 2001-2003 Doom Legacy Team
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
//
//--------------------------------------------------------------------------
//
// 'Special' stuff
//
// if(), int statements, etc.
//
// By Simon Howard
//
//----------------------------------------------------------------------------

#include <string.h>

#include "doomdef.h"
#include "command.h"
#include "z_zone.h"

#include "t_parse.h"
#include "t_spec.h"
#include "t_vari.h"

int find_operator(int start, int stop, char *value);

// ending brace found in parsing

void spec_brace()
{
  if(script_debug) CONS_Printf("brace\n");
  
  if(bracetype != bracket_close)  // only deal with closing } braces
    return;
  
  // if() requires nothing to be done
  if(current_section->type == st_if || current_section->type == st_else) return;
  
  // if a loop, jump back to the start of the loop
  if(current_section->type == st_loop)
    {
      rover = current_section->data.data_loop.loopstart;
      return;
    }
}

        // 'if' statement
int spec_if()
{
  int endtoken;
  svalue_t eval;
  
  if( (endtoken = find_operator(0, num_tokens-1, ")")) == -1)
    {
      script_error("parse error in if statement\n");
      return 0;
    }
  
  // 2 to skip past the 'if' and '('
  eval = evaluate_expression(2, endtoken-1);
  
  if(current_section && bracetype == bracket_open
     && endtoken == num_tokens-1)
    {
      // {} braces
      if(!intvalue(eval))       // skip to end of section
	rover = current_section->end+1;
    }
  else    // if() without {} braces
    if(intvalue(eval))
      {
	// nothing to do ?
	if(endtoken == num_tokens-1) return(intvalue(eval));
	evaluate_expression(endtoken+1, num_tokens-1);
      }

  return(intvalue(eval));
}


int spec_elseif(bool lastif)
{
  int endtoken;
  svalue_t eval;

  if( (endtoken = find_operator(0, num_tokens-1, ")")) == -1)
    {
      script_error("parse error in elseif statement\n");
      return 0;
    }

  if(lastif)
  {
    rover = current_section->end+1;
    return true;
  }
  // 2 to skip past the 'elseif' and '('
  eval = evaluate_expression(2, endtoken-1);
  
  if(current_section && bracetype == bracket_open
     && endtoken == num_tokens-1)
    {
      // {} braces
      if(!intvalue(eval))       // skip to end of section
	rover = current_section->end+1;
    }
  else    // elseif() without {} braces
    if(intvalue(eval))
      {
	// nothing to do ?
	if(endtoken == num_tokens-1) return(intvalue(eval));
	evaluate_expression(endtoken+1, num_tokens-1);
      }

  return(intvalue(eval));
}


void spec_else(bool lastif)
{
  if(lastif)
    rover = current_section->end+1;
}


// while() loop

void spec_while()
{
  int endtoken;
  svalue_t eval;

  if(!current_section)
    {
      script_error("no {} section given for loop\n");
      return;
    }
  
  if( (endtoken = find_operator(0, num_tokens-1, ")")) == -1)
    {
      script_error("parse error in loop statement\n");
      return;
    }
  
  eval = evaluate_expression(2, endtoken-1);
  
  // skip if no longer valid
  if(!intvalue(eval)) rover = current_section->end+1;
}

void spec_for()                 // for() loop
{
  svalue_t eval;
  int start;
  int comma1, comma2;     // token numbers of the seperating commas
  
  if(!current_section)
    {
      script_error("need {} delimiters for for()\n");
      return;
    }
  
  // is a valid section
  
  start = 2;     // skip "for" and "(": start on third token(2)
  
  // find the seperating commas first
  
  if( (comma1 = find_operator(start,    num_tokens-1, ",")) == -1
      || (comma2 = find_operator(comma1+1, num_tokens-1, ",")) == -1)
    {
      script_error("incorrect arguments to if()\n");
      return;
    }
  
  // are we looping back from a previous loop?
  if(current_section == prev_section)
    {
      // do the loop 'action' (third argument)
      evaluate_expression(comma2+1, num_tokens-2);
      
      // check if we should run the loop again (second argument)
      eval = evaluate_expression(comma1+1, comma2-1);
      if(!intvalue(eval))
	{
	  // stop looping
	  rover = current_section->end + 1;
	}
    }
  else
    {
      // first time: starting the loop
      // just evaluate the starting expression (first arg)
      evaluate_expression(start, comma1-1);
    }
}



/*********************
            ADD SCRIPT
 *********************/

// when the level is first loaded, all the
// scripts are simply stored in the levelscript.
// before the level starts, this script is
// preprocessed and run like any other. This allows
// the individual scripts to be derived from the
// levelscript. When the interpreter detects the
// 'script' keyword this function is called

void spec_script()
{
  if(!current_section)
    {
      script_error("need seperators for script\n");
      return;
    }
  
  // presume that the first token is "script"
  
  if(num_tokens < 2)
    {
      script_error("need script number\n");
      return;
    }

  int scriptnum = intvalue(evaluate_expression(1, num_tokens-1));
  
  if(scriptnum < 0)
    {
      script_error("invalid script number\n");
      return;
    }

  script_t *script = (script_t *)Z_Malloc(sizeof(script_t), PU_LEVEL, 0);

  // add to scripts list of parent
  current_script->children[scriptnum] = script;
  
  // copy script data
  // workout script size: -2 to ignore { and }
  int datasize = current_section->end - current_section->start - 2;
  script->data = (char *)Z_Malloc(datasize+1, PU_LEVEL, 0);
 
  // copy from parent script (levelscript) 
  // ignore first char which is {
  memcpy(script->data, current_section->start+1, datasize);

  // tack on a 0 to end the string
  script->data[datasize] = '\0';
  
  script->scriptnum = scriptnum;
  script->parent = current_script; // remember parent

  // preprocess the script now
  script->preprocess();
    
  // restore current_script: usefully stored in new script
  current_script = script->parent;

  // rover may also be changed, but is changed below anyway
  
  // we dont want to run the script, only add it
  // jump past the script in parsing
  
  rover = current_section->end + 1;
}



/**************************** Variable Creation ****************************/

int newvar_type;
script_t *newvar_script;

// called for each individual variable in a statement
//  newvar_type must be set

static void create_variable(int start, int stop)
{
  if(killscript) return;
  
  if(tokens[start].type != TO_name)
    {
      script_error("invalid name for variable: '%s'\n",
		   tokens[start+1].v);
      return;
    }
  
  // check if already exists, only checking
  // the current script
  if (newvar_script->variableforname(tokens[start].v))
    return;  // already one
  
  newvar_script->new_variable(tokens[start].v, newvar_type);
  
  if(stop != start) evaluate_expression(start, stop);
}

// divide a statement (without type prefix) into individual
// variables to be create them using create_variable

static void parse_var_line(int start)
{
  int starttoken = start, endtoken;
  
  while(1)
    {
      if(killscript) return;
      endtoken = find_operator(starttoken, num_tokens-1, ",");
      if(endtoken == -1) break;
      create_variable(starttoken, endtoken-1);
      starttoken = endtoken+1;  //start next after end of this one
    }
  // dont forget the last one
  create_variable(starttoken, num_tokens-1);
}

bool spec_variable()
{
  int start = 0;

  newvar_type = -1;                 // init to -1
  newvar_script = current_script;   // use current script

  // check for 'hub' keyword to make a hub variable
  if(!strcmp(tokens[start].v, "hub"))
    {
      newvar_script = &hub_script;
      start++;  // skip first token
    }

  // now find variable type
  if(!strcmp(tokens[start].v, "const"))
    {
      newvar_type = svt_const;
      start++;
    }
  else if(!strcmp(tokens[start].v, "string"))
    {
      newvar_type = svt_string;
      start++;
    }
  else if(!strcmp(tokens[start].v, "int"))
    {
      newvar_type = svt_int;
      start++;
    }
  else if(!strcmp(tokens[start].v, "mobj"))
    {
      newvar_type = svt_actor;
      start++;
    }
  else if(!strcmp(tokens[start].v, "script"))     // check for script creation
    {
      spec_script();
      return true;       // used tokens
    }
  else if(!strcmp(tokens[start].v, "float") || !strcmp(tokens[start].v, "fixed"))
    {
      newvar_type = svt_fixed;
      start++;
    }

  // other variable types could be added: eg float

  // are we creating a new variable?

  if(newvar_type != -1)
    {
      parse_var_line(start);
      return true;       // used tokens
    }

  return false; // not used: try normal parsing
}
