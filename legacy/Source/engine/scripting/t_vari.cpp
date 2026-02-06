// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// $Id: t_vari.cpp 570 2009-11-27 23:57:47Z smite-meister $
//
// Copyright(C) 2000 Simon Howard
// Copyright(C) 2001-2005 Doom Legacy Team
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

/// \file
/// \brief FS variables
///
/// Variable code: create new variables, look up variables, get value,
/// set value
///
/// variables are stored inside the individual scripts, to allow for
/// 'local' and 'global' variables. This way, individual scripts cannot
/// access variables in other scripts. However, 'global' variables can
/// be made which can be accessed by all scripts. These are stored inside
/// a dedicated script_t which exists only to hold all of these global
/// variables.
///
/// functions are also stored as variables, these are kept in the global
/// script so they can be accessed by all scripts. function variables
/// cannot be set or changed inside the scripts themselves.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "z_zone.h"

#include "t_script.h"
#include "t_parse.h"
#include "t_vari.h"
#include "t_func.h"


svalue_t nullvar = { svt_int, {0} }; // null var for empty return




// sf: string value of an svalue_t
const char *stringvalue(svalue_t v)
{
  static char buffer[256];

  switch(v.type)
   {
      case svt_string:
        return v.value.s;

      case svt_actor:
        return "map object";

      case svt_fixed:
        {
          float val = FIXED_TO_FLOAT(v.value.i);
          sprintf(buffer, "%g", val);
          return buffer;
        }

      case svt_int:
      default:
        sprintf(buffer, "%i", v.value.i);
        return buffer;
    }
}



// find_variable checks through the current script, level script
// and global script to try to find the variable of the name wanted
svariable_t *find_variable(const char *name)
{
  svariable_t *var;
  script_t *current = current_script;
  
  while (current)
    {
      // check this script
      if((var = current->variableforname(name)))
	return var;
      current = current->parent;    // try the parent of this one
    }

  return NULL;    // no variable
}


// create a new variable in a particular script.
// returns a pointer to the new variable.
svariable_t *script_t::new_variable(const char *name, int vtype)
{
  int tagtype = (this == &global_script || this == &hub_script) ? PU_STATIC : PU_LEVEL;
  
  // find an empty slot first
  svariable_t *newvar = (svariable_t *)Z_Malloc(sizeof(svariable_t), tagtype, 0);
  newvar->name = (char *)Z_Strdup(name, tagtype, 0);
  newvar->type = vtype;
  
  if (vtype == svt_string)
    {
      // 256 bytes for string
      newvar->value.s = (char *)Z_Malloc(256, tagtype, 0);
      newvar->value.s[0] = 0;
    }
  else
    newvar->value.i = 0;
  
  // now hook it into the hashchain  
  int n = variable_hash(name);
  newvar->next = variables[n];
  variables[n] = newvar;
  
  return newvar;
}


// search a particular script for a variable, which
// is returned if it exists
svariable_t *script_t::variableforname(const char *name)
{
  svariable_t *current = variables[variable_hash(name)];
  
  while (current)
    {
      if (!strcmp(name, current->name)) // found it?
	return current;         
      current = current->next;        // check next in chain
    }
  
  return NULL;
}


// free all the variables in a given script
void script_t::clear_variables()
{
  svariable_t *current, *next;
  
  for (int i=0; i<VARIABLESLOTS; i++)
    {
      current = variables[i];
      
      // go thru this chain
      while (current)
	{
	  // labels are added before variables, during
	  // preprocessing, so will be at the end of the chain
	  // we can be sure there are no more variables to free
	  if (current->type == svt_label)
	    break;
	  
	  next = current->next; // save for after freeing
	  
	  //FIXME free them too?
	  // if a string, free string data
	  if (current->type == svt_string)
	    Z_Free(current->value.s);
	  
	  current = next; // go to next in chain
	}
      // start of labels or NULL
      variables[i] = current;
    }
}


// returns an svalue_t holding the current
// value of a particular variable.
svalue_t svariable_t::getvalue()
{
  svalue_t returnvar;
  
  if(type == svt_pString)
    {
      returnvar.type = svt_string;
      returnvar.value.s = *value.pS;
    }
  else if(type == svt_pInt)
    {
      returnvar.type = svt_int;
      returnvar.value.i = *value.pI;
    }
  else if(type == svt_pFixed)
    {
      returnvar.type = svt_fixed;
      returnvar.value.i = (*value.pFixed).value();
    }
  else if(type == svt_pActor)
    {
      returnvar.type = svt_actor;
      returnvar.value.mobj = *value.pMobj;
    }
  else
    {
      returnvar.type = type;
      // copy the value
      returnvar.value.i = value.i;
    }
  
  return returnvar;
}


// set a variable to a value from an svalue_t
void svariable_t::setvalue(svalue_t newvalue)
{
  if (killscript) return;  // protect the variables when killing script
  
  if (type == svt_const)
    {
      // const adapts to the value it is set to
      type = newvalue.type;

      // alloc memory for string
      if(type == svt_string)   // static incase a global_script var
	value.s = (char *)Z_Malloc(256, PU_STATIC, 0);
    }

  switch (type)
    {
    case svt_int:
      value.i = intvalue(newvalue);
      break;
    case svt_string:
      strcpy(value.s, stringvalue(newvalue));
      break;
    case svt_fixed:
      value.i = fixedvalue(newvalue).value();
      break;
    case svt_actor:
      value.mobj = MobjForSvalue(newvalue);
      break;
    case svt_pInt:
      *value.pI = intvalue(newvalue);
      break;
    case svt_pString:
      // free old value
      free(*value.pS);
      // dup new string
      *value.pS = strdup(stringvalue(newvalue));
      break;
    case svt_pFixed:
      *value.pFixed = fixedvalue(newvalue);
      break;
    case svt_pActor:
      *value.pMobj = MobjForSvalue(newvalue);
      break;
    case svt_function:
      script_error("attempt to set function to a value\n");
    }
}



svariable_t *add_game_int(const char *name, int *var)
{
  svariable_t* newvar = global_script.new_variable(name, svt_pInt);
  newvar->value.pI = var;

  return newvar;
}


svariable_t *add_game_fixed(const char *name, fixed_t *fixed)
{
  svariable_t *newvar = global_script.new_variable(name, svt_pFixed);
  newvar->value.pFixed = fixed;

  return newvar;
}

svariable_t *add_game_string(char *name, char **var)
{
  svariable_t* newvar = global_script.new_variable(name, svt_pString);
  newvar->value.pS = var;

  return newvar;
}


svariable_t *add_game_mobj(const char *name, Actor **mo)
{
  svariable_t* newvar = global_script.new_variable(name, svt_pActor);
  newvar->value.pMobj = mo;

  return newvar;
}


// create a new function. returns the function number
svariable_t *new_function(const char *name, void (*handler)())
{
  // create the new variable for the function
  // add to the global script
  svariable_t *newvar = global_script.new_variable(name, svt_function);
  newvar->value.handler = handler;

  return newvar;
}



/********************************
                     FUNCTIONS
 ********************************/
// functions are really just variables
// of type svt_function. there are two
// functions to control functions (heh)

// new_function: just creates a new variable
//      of type svt_function. give it the
//      handler function to be called, and it
//      will be stored as a pointer appropriately.

// evaluate_function: once parse.c is pretty
//      sure it has a function to run it calls
//      this. evaluate_function makes sure that
//      it is a function call first, then evaluates all
//      the arguments given to the function.
//      these are built into an argc/argv-style
//      list. the function 'handler' is then called.

// the basic handler functions are in func.c

int t_argc;                     // number of arguments
svalue_t *t_argv;               // arguments
svalue_t  t_return;             // returned value


svalue_t evaluate_function(int start, int stop)
{
  svariable_t *func = NULL;
  int startpoint, endpoint;

  // the arguments need to be built locally in case of
  // function returns as function arguments eg
  // print("here is a random number: ", rnd() );
  
  int argc;
  svalue_t argv[MAXARGS];

  if (tokens[start].type != TO_function || tokens[stop].type != TO_oper || tokens[stop].v[0] != ')')
    script_error("misplaced closing bracket\n");
  // all the functions are stored in the global script
  else if (!(func = global_script.variableforname(tokens[start].v)))
    script_error("no such function: '%s'\n", tokens[start].v);
  else if (func->type != svt_function)
    script_error("'%s' not a function\n", tokens[start].v);

  if (killscript) return nullvar; // one of the above errors occurred

  // build the argument list
  // use a C command-line style system rather than
  // a system using a fixed length list

  argc = 0;
  endpoint = start + 2;   // ignore the function name and first bracket
  
  while (endpoint < stop)
    {
      startpoint = endpoint;
      endpoint = find_operator(startpoint, stop-1, ",");
      
      // check for -1: no more ','s 
      if (endpoint == -1)
	endpoint = stop; // evaluate the last expression

      if (endpoint-1 < startpoint)
	break;
      
      argv[argc] = evaluate_expression(startpoint, endpoint-1);
      endpoint++;    // skip the ','
      argc++;
    }

  // store the arguments in the global arglist
  t_argc = argc;
  t_argv = argv;

  if(killscript) return nullvar;
  
  // now run the function
  func->value.handler();
  
  // return the returned value
  return t_return;
}

// structure dot (.) operator
// there are not really any structs in FraggleScript, it's
// just a different way of calling a function that looks
// nicer. ie
//      a.b()  = a.b   =  b(a)
//      a.b(c) = b(a,c)

// this function is just based on the one above

svalue_t OPstructure(int start, int n, int stop)
{
  svariable_t *func = NULL;
  
  // the arguments need to be built locally in case of
  // function returns as function arguments eg
  // print("here is a random number: ", rnd() );
  
  int argc;
  svalue_t argv[MAXARGS];

  // all the functions are stored in the global script
  if (!(func = global_script.variableforname(tokens[n+1].v)))
    script_error("no such function: '%s'\n", tokens[n+1].v);
  else if(func->type != svt_function)
    script_error("'%s' not a function\n", tokens[n+1].v);

  if(killscript) return nullvar; // one of the above errors occurred
  
  // build the argument list
  // add the left part as first arg

  argv[0] = evaluate_expression(start, n-1);
  argc = 1; // start on second argv

  if(stop != n+1)         // can be a.b not a.b()
    {
      int startpoint, endpoint;

      // ignore the function name and first bracket
      endpoint = n + 3;
      
      while(endpoint < stop)
	{
	  startpoint = endpoint;
	  endpoint = find_operator(startpoint, stop-1, ",");
	  
	  // check for -1: no more ','s 
	  if(endpoint == -1)
	    {               // evaluate the last expression
	      endpoint = stop;
	    }
	  if(endpoint-1 < startpoint)
	    break;
	  
	  argv[argc] = evaluate_expression(startpoint, endpoint-1);
	  endpoint++;    // skip the ','
	  argc++;
	}
    }

  // store the arguments in the global arglist
  t_argc = argc;
  t_argv = argv;
  
  if(killscript) return nullvar;
  
  // now run the function
  func->value.handler();
  
  // return the returned value
  return t_return;
}
