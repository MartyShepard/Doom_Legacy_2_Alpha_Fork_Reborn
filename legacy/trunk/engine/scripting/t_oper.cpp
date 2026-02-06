// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// $Id: t_oper.cpp 533 2009-06-11 08:31:10Z smite-meister $
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
//--------------------------------------------------------------------------

/// \file
/// \brief FS operators
///
/// Handler code for all the operators. The 'other half' of the parsing.

#include <stdlib.h>
#include <string.h>

#include "command.h"
#include "doomtype.h"
#include "z_zone.h"

#include "t_parse.h"
#include "t_vari.h"
#include "t_script.h"

#define evaluate_leftnright(a, b, c) {\
  left = evaluate_expression((a), (b)-1); \
  right = evaluate_expression((b)+1, (c)); }\

svalue_t OPequals(int, int, int);           // =

svalue_t OPplus(int, int, int);             // +
svalue_t OPminus(int, int, int);            // -
svalue_t OPmultiply(int, int, int);         // *
svalue_t OPdivide(int, int, int);           // /
svalue_t OPremainder(int, int, int);        // %

svalue_t OPor(int, int, int);               // ||
svalue_t OPand(int, int, int);              // &&
svalue_t OPnot(int, int, int);              // !

svalue_t OPor_bin(int, int, int);           // |
svalue_t OPand_bin(int, int, int);          // &
svalue_t OPnot_bin(int, int, int);          // ~

svalue_t OPcmp(int, int, int);              // ==
svalue_t OPnotcmp(int, int, int);           // !=
svalue_t OPlessthan(int, int, int);         // <
svalue_t OPgreaterthan(int, int, int);      // >

svalue_t OPincrement(int, int, int);        // ++
svalue_t OPdecrement(int, int, int);        // --

svalue_t OPlessthanorequal(int, int, int);  // <=
svalue_t OPgreaterthanorequal(int, int, int); // >=

svalue_t OPstructure(int, int, int);    // in t_vari.c

operator_t operators[]=
{
  {"=",   OPequals,               opdir_backward},
  {"||",  OPor,                   opdir_forward},
  {"&&",  OPand,                  opdir_forward},
  {"|",   OPor_bin,               opdir_forward},
  {"&",   OPand_bin,              opdir_forward},
  {"==",  OPcmp,                  opdir_forward},
  {"!=",  OPnotcmp,               opdir_forward},
  {"<",   OPlessthan,             opdir_forward},
  {">",   OPgreaterthan,          opdir_forward},
  {"<=",  OPlessthanorequal,      opdir_forward},
  {">=",  OPgreaterthanorequal,   opdir_forward},
  {"+",   OPplus,                 opdir_forward},
  {"-",   OPminus,                opdir_forward},
  {"*",   OPmultiply,             opdir_forward},
  {"/",   OPdivide,               opdir_forward},
  {"%",   OPremainder,            opdir_forward},
  {"!",   OPnot,                  opdir_forward},
  {"++",  OPincrement,            opdir_forward},
  {"--",  OPdecrement,            opdir_forward},
  {".",   OPstructure,            opdir_forward},
};

int num_operators = sizeof(operators) / sizeof(operator_t);

/***************** logic *********************/

// = operator

svalue_t OPequals(int start, int n, int stop)
{
  svalue_t evaluated;
  svariable_t *var = find_variable(tokens[start].v);
  
  if(var)
    {
      evaluated = evaluate_expression(n+1, stop);
      var->setvalue(evaluated);
    }
  else
    {
      script_error("unknown variable '%s'\n", tokens[start].v);
      return nullvar;
    }
  
  return evaluated;
}


svalue_t OPor(int start, int n, int stop)
{
  svalue_t returnvar;
  int exprtrue = false;
  svalue_t eval;
  
  // if first is true, do not evaluate the second
  
  eval = evaluate_expression(start, n-1);
  
  if(intvalue(eval))
    exprtrue = true;
  else
    {
      eval = evaluate_expression(n+1, stop);
      exprtrue = !!intvalue(eval);
    }
  
  returnvar.type = svt_int;
  returnvar.value.i = exprtrue;
  return returnvar;
  
  return returnvar;
}


svalue_t OPand(int start, int n, int stop)
{
  svalue_t returnvar;
  int exprtrue = true;
  svalue_t eval;

  // if first is false, do not eval second
  
  eval = evaluate_expression(start, n-1);
  
  if(!intvalue(eval) )
    exprtrue = false;
  else
    {
      eval = evaluate_expression(n+1, stop);
      exprtrue = !!intvalue(eval);
    }

  returnvar.type = svt_int;
  returnvar.value.i = exprtrue;

  return returnvar;
}

svalue_t OPcmp(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  
  evaluate_leftnright(start, n, stop);
  
  returnvar.type = svt_int;        // always an int returned
  
  if(left.type == svt_string && right.type == svt_string)
    {
      returnvar.value.i = !strcmp(left.value.s, right.value.s);
      return returnvar;
    }

  if(left.type == svt_fixed || right.type == svt_fixed)
    {
      returnvar.value.i = fixedvalue(left) == fixedvalue(right);
      return returnvar;
    }

  if(left.type == svt_actor || right.type == svt_actor)
  {
    if(left.type == svt_actor && right.type == svt_actor)
      returnvar.value.i = left.value.mobj == right.value.mobj;
    else if(left.type == svt_actor)
      returnvar.value.i = (left.value.mobj == MobjForSvalue(right)) ? 1 : 0;
    else if(right.type == svt_actor)
      returnvar.value.i = (MobjForSvalue(left) == right.value.mobj) ? 1 : 0;

    return returnvar;
  }

  returnvar.value.i = intvalue(left) == intvalue(right);

  return returnvar;
}

svalue_t OPnotcmp(int start, int n, int stop)
{
  svalue_t returnvar;
  
  returnvar = OPcmp(start, n, stop);
  returnvar.value.i = !returnvar.value.i;
  
  return returnvar;
}

svalue_t OPlessthan(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  
  evaluate_leftnright(start, n, stop);
  
  returnvar.type = svt_int;

  if(left.type == svt_fixed || right.type == svt_fixed)
    returnvar.value.i = fixedvalue(left) < fixedvalue(right);
  else
    returnvar.value.i = intvalue(left) < intvalue(right);

  return returnvar;
}

svalue_t OPgreaterthan(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  
  evaluate_leftnright(start, n, stop);
  
  returnvar.type = svt_int;

  if(left.type == svt_fixed || right.type == svt_fixed)
    returnvar.value.i = fixedvalue(left) > fixedvalue(right);
  else
    returnvar.value.i = intvalue(left) > intvalue(right);

  return returnvar;
}

svalue_t OPnot(int start, int n, int stop)
{
  svalue_t right, returnvar;
  
  right = evaluate_expression(n+1, stop);
  
  returnvar.type = svt_int;
  returnvar.value.i = !intvalue(right);
  return returnvar;
}

/************** simple math ***************/

svalue_t OPplus(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  
  evaluate_leftnright(start, n, stop);
  
  if (left.type == svt_string)
    {
      char *tmp;
      if (right.type == svt_string)
	{
	  tmp = (char *)Z_Malloc(strlen(left.value.s) + strlen(right.value.s) + 1, PU_LEVEL, 0);
	  sprintf(tmp, "%s%s", left.value.s, right.value.s);
	}
      else if (right.type == svt_fixed)
	{
	  tmp = (char *)Z_Malloc(strlen(left.value.s) + 12, PU_LEVEL, 0);
	  sprintf(tmp, "%s%4.4f", left.value.s, FIXED_TO_FLOAT(right.value.i));
	}
      else
	{
	  tmp = (char *)Z_Malloc(strlen(left.value.s) + 12, PU_LEVEL, 0);
	  sprintf(tmp, "%s%d", left.value.s, intvalue(right));
	}
      returnvar.type = svt_string;
      returnvar.value.s = tmp;
    }
  else if (left.type == svt_fixed || right.type == svt_fixed)
    {
      returnvar.type = svt_fixed;
      returnvar.value.i = (fixedvalue(left) + fixedvalue(right)).value();
    }
  else
    {
      returnvar.type = svt_int;
      returnvar.value.i = intvalue(left) + intvalue(right);
    }

  return returnvar;
}

svalue_t OPminus(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  
  // do they mean minus as in '-1' rather than '2-1'?
  if(start == n)
    {
      // kinda hack, hehe
      left.value.i = 0;
      left.type = svt_int;
      right = evaluate_expression(n+1, stop);
    }
  else
    evaluate_leftnright(start, n, stop);
  
  if(left.type == svt_fixed || right.type == svt_fixed)
    {
      returnvar.type = svt_fixed;
      returnvar.value.i = (fixedvalue(left) - fixedvalue(right)).value();
    }
  else
    {
      returnvar.type = svt_int;
      returnvar.value.i = intvalue(left) - intvalue(right);
    }

  return returnvar;
}

svalue_t OPmultiply(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  
  evaluate_leftnright(start, n, stop);
  
  if(left.type == svt_fixed || right.type == svt_fixed)
    {
      returnvar.type = svt_fixed;
      returnvar.value.i = (fixedvalue(left) * fixedvalue(right)).value();
    }
  else
    {
      returnvar.type = svt_int;
      returnvar.value.i = intvalue(left) * intvalue(right);
    }

  return returnvar;
}

svalue_t OPdivide(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  
  evaluate_leftnright(start, n, stop);
  
//  if(left.type == svt_fixed || right.type == svt_fixed)
    {
      fixed_t fr;

      if((fr = fixedvalue(right)) == 0)
        script_error("divide by zero\n");
      else
	{
          returnvar.type = svt_fixed;
          returnvar.value.i = (fixedvalue(left) / fr).value();
	}
    }
/*  else
    {
      int ir;

      if(!(ir = intvalue(right)))
        script_error("divide by zero\n");
      else
	{
          returnvar.type = svt_int;
          returnvar.value.i = intvalue(left) / ir;
	}
    }*/

  return returnvar;
}

svalue_t OPremainder(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  int ir;
  
  evaluate_leftnright(start, n, stop);
  
  if(!(ir = intvalue(right)))
    script_error("divide by zero\n");
  else
    {
      returnvar.type = svt_int;
      returnvar.value.i = intvalue(left) % ir;
    }
  return returnvar;
}

        /********** binary operators **************/

// binary or |

svalue_t OPor_bin(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  
  evaluate_leftnright(start, n, stop);
  
  returnvar.type = svt_int;
  returnvar.value.i = intvalue(left) | intvalue(right);
  return returnvar;
}


// binary and &

svalue_t OPand_bin(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  
  evaluate_leftnright(start, n, stop);
  
  returnvar.type = svt_int;
  returnvar.value.i = intvalue(left) & intvalue(right);
  return returnvar;
}



        // ++
svalue_t OPincrement(int start, int n, int stop)
{
  if(start == n)          // ++n
    {
      svariable_t *var = find_variable(tokens[stop].v);
      if(!var)
	{
	  script_error("unknown variable '%s'\n", tokens[stop].v);
	  return nullvar;
	}
      svalue_t value = var->getvalue();
      
      value.value.i = intvalue(value) + 1;
      value.type = svt_int;
      var->setvalue(value);
      
      return value;
    }
  else if(stop == n)     // n++
    {
      svalue_t origvalue, value;
      svariable_t *var = find_variable(tokens[start].v);
      if(!var)
	{
	  script_error("unknown variable '%s'\n", tokens[start].v);
	  return nullvar;
	}
      origvalue = var->getvalue();
      
      value.type = svt_int;
      value.value.i = intvalue(origvalue) + 1;
      var->setvalue(value);
      
      return origvalue;
    }
  
  script_error("incorrect arguments to ++ operator\n");
  return nullvar;
}

        // --
svalue_t OPdecrement(int start, int n, int stop)
{
  if(start == n)          // ++n
    {
      svariable_t *var = find_variable(tokens[stop].v);
      if(!var)
	{
	  script_error("unknown variable '%s'\n", tokens[stop].v);
	  return nullvar;
	}
      svalue_t value = var->getvalue();
      
      value.value.i = intvalue(value) - 1;
      value.type = svt_int;
      var->setvalue(value);
      
      return value;
    }
  else if(stop == n)   // n++
    {
      svalue_t origvalue, value;
      svariable_t *var = find_variable(tokens[start].v);
      if(!var)
	{
          script_error("unknown variable '%s'\n", tokens[start].v);
	  return nullvar;
	}
      origvalue = var->getvalue();
      
      value.type = svt_int;
      value.value.i = intvalue(origvalue) - 1;
      var->setvalue(value);
      
      return origvalue;
    }
  
  script_error("incorrect arguments to ++ operator\n");
  return nullvar;
}


// Thank you Quasar!
svalue_t OPlessthanorequal(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  evaluate_leftnright(start, n, stop);
  returnvar.type = svt_int;
  returnvar.value.i = intvalue(left) <= intvalue(right);
  return returnvar;
}


svalue_t OPgreaterthanorequal(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  evaluate_leftnright(start, n, stop);
  returnvar.type = svt_int;
  returnvar.value.i = intvalue(left) >= intvalue(right);
  return returnvar;
}
