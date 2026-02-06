// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// $Id: t_parse.cpp 616 2010-03-23 05:15:47Z smite-meister $
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
/// \brief FS parsing
///
/// Takes lines of code, or groups of lines and runs them.
/// The main core of FraggleScript

#include <stdarg.h>
#include <string.h>

#include "doomdef.h"
#include "doomtype.h"
#include "command.h"

#include "g_pawn.h"

#include "sounds.h"
#include "z_zone.h"

#include "t_parse.h"
#include "t_spec.h"
#include "t_vari.h"
#include "t_func.h"

#include "w_wad.h"




int script_debug = false;

script_t *current_script;   // the current script
Map *current_map;           // current_script->mp, just for convenience
Actor *trigger_obj;         // object which triggered script
PlayerInfo *trigger_player;

bool killscript;         // when set to true, stop the script quickly
fs_section_t *prev_section;       // the section from the previous statement

/************ Divide into tokens **************/

char *linestart;        // start of line
char *rover;            // current point reached in script

        // inline for speed
#define isnum(c) ( ((c)>='0' && (c)<='9') || (c)=='.' )
        // isop: is an 'operator' character, eg '=', '%'
#define isop(c)   !( ( (c)<='Z' && (c)>='A') || ( (c)<='z' && (c)>='a') || \
                     ( (c)<='9' && (c)>='0') || ( (c)=='_') )




fs_section_t *current_section; // the section (if any) found in parsing the line
int bracetype;              // bracket_open or bracket_close


// return an escape sequence (prefixed by a '\')
// do not use all C escape sequences
static char escape_sequence(char c)
{
  if(c == 'n') return '\n';
  if(c == '\\') return '\\';
  if(c == '"') return '"';
  if(c == '?') return '?';
  if(c == 'a') return '\a';         // alert beep
  if(c == 't') return '\t';         //tab
//  if(c == 'z') return *FC_TRANS;    // translucent toggle
  
  // font colours
  //Hurdler: fix for Legacy text color
  if(c >= '0' && c <= '9') return /*128 +*/ (c-'0');
  
  return c;
}




//============================================
//               tokenizer
//============================================

token_t tokens[T_MAXTOKENS];
int     num_tokens = 0;

// for simplicity:
#define tt (tokens[num_tokens-1].type)
#define tok (tokens[num_tokens-1].v)




// add_char: add one character to the current token
static void add_char(char c)
{
  char *out = tok + strlen(tok);
  
  *out++ = c;
  *out = 0;
}



// next_token: end this token, go onto the next
void next_token()
{
  if (tok[0] || tt == TO_string)
    {
      num_tokens++;
      tok = tokens[num_tokens-2].v + strlen(tokens[num_tokens-2].v) + 1;
      tok[0] = 0;
    }
  
  // get to the next token, ignoring spaces, newlines,
  // useless chars etc.
  
  // pass whitespace
  while ((*rover == ' ' || *rover < 32) && *rover)
    rover++;

  // end-of-script?
  if (!*rover)
    {
      if (tokens[0].v[0])
	{
	  CONS_Printf("%s %i %i\n", tokens[0].v, rover - current_script->data, current_script->len);
	  // line contains text, but no semicolon: an error
	  script_error("missing ';'\n");
	}
      // empty line, end of command-list
      return;
    }

  // was the previous token a function?
  if (num_tokens > 1 && *rover == '(' && tokens[num_tokens-2].type == TO_name)
    tokens[num_tokens-2].type = TO_function;

  if (*rover == '{' || *rover == '}') // braces
    {
      if (*rover == '{')
        {
          bracetype = bracket_open;
          current_section = current_script->find_section_start(rover);
        }
      else            // closing brace
        {
          bracetype = bracket_close;
          current_section = current_script->find_section_end(rover);
        }

      if (!current_section)
        {
          script_error("section not found!\n");
          return;
        }
    }
  else if (*rover == '\"') // string literal
    {
      tt = TO_string;
      if (tokens[num_tokens-2].type == TO_string)
        num_tokens--;   // join strings
      rover++;
    }
  else
    {
      tt = isop(*rover) ? TO_oper :
        isnum(*rover) ? TO_number : TO_name;
    }
}



// get_tokens.
// Take a string, break it into tokens.

// individual tokens are stored inside the tokens[] array

//   TO_name: a piece of text which starts with an alphabet letter.
//         probably a variable name. Some are converted into
//         function types later on in find_brackets
//   TO_number: a number. like '12' or '1337'
//   TO_oper: an operator such as '&&' or '+'. All FraggleScript
//             operators are either one character, or two character
//             (if 2 character, 2 of the same char or ending in '=')
//   TO_string: a text string that was enclosed in quote "" marks in
//           the original text
//   TO_unset: shouldn't ever end up being set really.
//   TO_function: a function name (found in second stage parsing)

char *get_tokens(char *s)
{
  rover = s;

  num_tokens = 1;
  tok[0] = 0;
  tt = TO_name;
  
  current_section = NULL;   // default to no section found
  
  next_token();
  linestart = rover;      // save the start
  
  if (*rover)
    while(1)
      {
        if(killscript) return rover;
        if (current_section)
          {
            // a { or } section brace has been found
            break;        // stop parsing now
          }
        else if (tt != TO_string)
	  if (*rover == ';')
	    break;     // check for end of command ';'
        
        switch (tt)
          {
          case TO_unset:
          case TO_string:
            while(*rover != '\"')     // dedicated loop for speed
              {
                if(*rover == '\\')       // escape sequences
                  {
                    rover++;
                    add_char(escape_sequence(*rover));
                  }
                else
                  add_char(*rover);
                rover++;
              }
            rover++;
            next_token();       // end of this token
            continue;
            
          case TO_oper:
            // all 2-character operators either end in '=' or
            // are 2 of the same character
            // do not allow 2-characters for brackets '(' ')'
            // which are still being considered as operators
            
            // operators are only 2-char max, do not need
            // a seperate loop
            
            if((*tok && *rover != '=' && *rover!=*tok) ||
               *tok == '(' || *tok == ')')
              {
                // end of operator
                next_token();
                continue;
              }
            add_char(*rover);
            break;
            
          case TO_number:
            // add while number chars are read

            while(isnum(*rover))       // dedicated loop
              add_char(*rover++);
            next_token();
            continue;

          case TO_name:
            // add the chars

            while(!isop(*rover))        // dedicated loop
              add_char(*rover++);
            next_token();
            continue;
            
          default: break; // shut up compiler

          }
        rover++;
      }
  
  // check for empty last token

  if (!tok[0])
    num_tokens--;
  
  rover++;
  return rover;
}


void print_tokens()     // DEBUG
{
  for (int i=0; i<num_tokens; i++)
    {
      CONS_Printf("\n'%s' \t\t --", tokens[i].v);
      switch(tokens[i].type)
        {
        case TO_string:   CONS_Printf("string");        break;
        case TO_oper:     CONS_Printf("operator");      break;
        case TO_name:     CONS_Printf("name");          break;
        case TO_number:   CONS_Printf("number");        break;
        case TO_unset :   CONS_Printf("duh");           break;
        case TO_function: CONS_Printf("function name"); break;
        }
    }
  CONS_Printf("\n");
  if(current_section)
    CONS_Printf("current section: offset %i\n", int(current_section->start - current_script->data));
}




//=======================================================


// runs a script
void script_t::run()
{
  // start at the beginning of the script
  rover = data;
  lastiftrue = false;
  
  parse(); // run it
}


void script_t::continue_script(char *continue_point)
{
  // continue from place specified
  rover = continue_point;
  
  parse(); // run 
}


// rover must be set
void script_t::parse()
{
  current_script = this;

  // check for valid rover
  if (rover < data || rover > data+len)
    {
      script_error("parse_script: trying to continue from point"
                   "outside script!\n");
      return;
    }

  // some shortcuts
  trigger_obj = trigger;  // set trigger
  PlayerPawn *p;
  trigger_player = trigger ?
    ((p = trigger->Inherits<PlayerPawn>()) ? p->player : NULL)
    : NULL;
  
  parse_data(data, data+len);
  
  // dont clear global vars!
  if (scriptnum != -1)
    clear_variables(); // free variables TODO why? should we?

  lastiftrue = false;
}


// uses rover, current_script
void parse_data(char *data, char *end)
{
  killscript = false;     // dont kill the script straight away
  
  // allocate space for the tokens
  char *token_alloc = (char *)Z_Malloc(current_script->len + T_MAXTOKENS, PU_STATIC, 0);
  
  prev_section = NULL;  // clear it
  
  while (rover <= end && *rover)   // go through the script executing each statement
    {
      // reset the tokens before getting the next line
      tokens[0].v = token_alloc;
      
      prev_section = current_section; // store from prev. statement
      
      // get the line and tokens
      rover = get_tokens(rover);
      
      if(killscript) break;
      
      if(!num_tokens)
        {
          if(current_section)       // no tokens but a brace
            {
              // possible } at end of loop:
              // refer to spec.c
              spec_brace();
            }
          
          continue;  // continue to next statement
        }
      
      if(script_debug) print_tokens();   // debug
      run_statement();         // run the statement
    }
  Z_Free(token_alloc);
}


void run_statement()
{
  // decide what to do with it
  
  // NB this stuff is a bit hardcoded:
  //    it could be nicer really but i'm
  //    aiming for speed
  
  // if() and while() will be mistaken for functions
  // during token processing
  if(tokens[0].type == TO_function)
    {
      if(!strcmp(tokens[0].v, "if"))
        {
          current_script->lastiftrue = spec_if() ? true: false;
          return;
        }
      else if(!strcmp(tokens[0].v, "elseif"))
        {
          if(!prev_section || (prev_section->type != st_if && prev_section->type != st_elseif))
          {
            script_error("elseif without if!\n");
            return;
          }
          current_script->lastiftrue = spec_elseif(current_script->lastiftrue) ? true : false;
          return;
        }
      else if(!strcmp(tokens[0].v, "else"))
        {
          if(!prev_section || (prev_section->type != st_if && prev_section->type != st_elseif))
          {
            script_error("else without if!\n");
            return;
          }
          spec_else(current_script->lastiftrue);
          current_script->lastiftrue = true;
          return;
        }
      else if(!strcmp(tokens[0].v, "while"))
        {
          spec_while();
          return;
        }
      else if(!strcmp(tokens[0].v, "for"))
        {
          spec_for();
          return;
        }
    }
  else if(tokens[0].type == TO_name)
    {
      // NB: goto is a function so is not here

      // if a variable declaration, return now
      if(spec_variable()) return;
    }

  // just a plain expression
  evaluate_expression(0, num_tokens-1);
}

/***************** Evaluating Expressions ************************/

        // find a token, ignoring things in brackets        
int find_operator(int start, int stop, const char *value)
{
  int i;
  int bracketlevel = 0;
  
  for(i=start; i<=stop; i++)
    {
      // only interested in operators
      if(tokens[i].type != TO_oper) continue;
      
      // use bracketlevel to check the number of brackets
      // which we are inside
      bracketlevel += tokens[i].v[0] == '(' ? 1 :
        tokens[i].v[0]==')' ? -1 : 0;
      
      // only check when we are not in brackets
      if(!bracketlevel && !strcmp(value, tokens[i].v))
        return i;
    }
  
  return -1;
}

        // go through tokens the same as find_operator, but backwards
int find_operator_backwards(int start, int stop, const char *value)
{
  int i;
  int bracketlevel = 0;
  
  for(i=stop; i>=start; i--)      // check backwards
    {
      // operators only

      if(tokens[i].type != TO_oper) continue;
      
      // use bracketlevel to check the number of brackets
      // which we are inside
      
      bracketlevel += tokens[i].v[0]=='(' ? -1 :
        tokens[i].v[0]==')' ? 1 : 0;
      
      // only check when we are not in brackets
      // if we find what we want, return it

      if(!bracketlevel && !strcmp(value, tokens[i].v))
        return i;
    }
  
  return -1;
}

// simple_evaluate is used once evalute_expression gets to the level
// where it is evaluating just one token

// converts number tokens into svalue_ts and returns
// the same with string tokens
// name tokens are considered to be variables and
// attempts are made to find the value of that variable
// command tokens are executed (does not return a svalue_t)


static svalue_t simple_evaluate(int n)
{
  svalue_t returnvar;
  svariable_t *var;
  
  switch(tokens[n].type)
    {
    case TO_string:
      returnvar.type = svt_string;
      returnvar.value.s = tokens[n].v;
      return returnvar;

    case TO_number:
      if(strchr(tokens[n].v, '.'))
        {
          returnvar.type = svt_fixed;
          returnvar.value.i = fixed_t(float(atof(tokens[n].v))).value();
        }
      else
        {
          returnvar.type = svt_int;
          returnvar.value.i = atoi(tokens[n].v);
        }
      return returnvar;

    case TO_name:
      var = find_variable(tokens[n].v);
      if(!var)
        {
          script_error("unknown variable '%s'\n", tokens[n].v);
          return nullvar;
        }
      else
        return var->getvalue();

    default: return nullvar;
    }
}

// pointless_brackets checks to see if there are brackets surrounding
// an expression. eg. "(2+4)" is the same as just "2+4"
//
// because of the recursive nature of evaluate_expression, this function is
// neccesary as evaluating expressions such as "2*(2+4)" will inevitably
// lead to evaluating "(2+4)"

static void pointless_brackets(int *start, int *stop)
{
  int bracket_level, i;
  
  // check that the start and end are brackets
  
  while(tokens[*start].v[0] == '(' && tokens[*stop].v[0] == ')')
    {
      
      bracket_level = 0;
      
      // confirm there are pointless brackets..
      // if they are, bracket_level will only get to 0
      // at the last token
      // check up to <*stop rather than <=*stop to ignore
      // the last token
      
      for (i = *start; i<*stop; i++)
        {
          if (tokens[i].type != TO_oper) continue; // ops only
          bracket_level += (tokens[i].v[0] == '(');
          bracket_level -= (tokens[i].v[0] == ')');
          if (bracket_level == 0) return; // stop if braces stop before end
        }
      
      // move both brackets in
      
      *start = *start + 1;
      *stop = *stop - 1;
    }
}

// evaluate_expresion is the basic function used to evaluate
// a FraggleScript expression.
// start and stop denote the tokens which are to be evaluated.
//
// works by recursion: it finds operators in the expression
// (checking for each in turn), then splits the expression into
// 2 parts, left and right of the operator found.
// The handler function for that particular operator is then
// called, which in turn calls evaluate_expression again to
// evaluate each side. When it reaches the level of being asked
// to evaluate just 1 token, it calls simple_evaluate

svalue_t evaluate_expression(int start, int stop)
{
  int i, n;

  if(killscript) return nullvar;  // killing the script
  
  // possible pointless brackets
  if (tokens[start].type == TO_oper && tokens[stop].type == TO_oper)
    pointless_brackets(&start, &stop);
  
  if(start == stop)       // only 1 thing to evaluate
    {
      return simple_evaluate(start);
    }
  
  // go through each operator in order of precedence
  
  for(i=0; i<num_operators; i++)
    {
      // check backwards for the token. it has to be
      // done backwards for left-to-right reading: eg so
      // 5-3-2 is (5-3)-2 not 5-(3-2)

      if( -1 != (n = (operators[i].direction == opdir_forward ?
                find_operator_backwards : find_operator)
                 (start, stop, operators[i].str)) )
        {
          // CONS_Printf("operator %s, %i-%i-%i\n", operators[count].str, start, n, stop);

          // call the operator function and evaluate this chunk of tokens

          return operators[i].handler(start, n, stop);
        }
    }
  
  if(tokens[start].type == TO_function)
    return evaluate_function(start, stop);
  
  // error ?
  {        
    char tempstr[1024]="";
    
    for(i=start; i<=stop; i++)
      sprintf(tempstr,"%s %s", tempstr, tokens[i].v);

    script_error("couldnt evaluate expression: %s\n",tempstr);
    return nullvar;
  }
  
}


void script_error(const char *s, ...)
{
  if(killscript) return;  //already killing script
  
  if(current_script->scriptnum == -1)
    CONS_Printf("global");
  else
    CONS_Printf("%i", current_script->scriptnum);
  
  // find the line number
  
  if(rover >= current_script->data &&
     rover <= current_script->data+current_script->len)
    {
      int linenum = 1;
      char *temp;
      for(temp = current_script->data; temp<linestart; temp++)
        if(*temp == '\n') linenum++;    // count EOLs
      CONS_Printf(", %i", linenum);
    }

  CONS_Printf(": ");
 
#define BUF_SIZE 1024
  char        txt[BUF_SIZE];
  va_list     ap;

  // print the error
  va_start(ap, s);
  vsnprintf(txt, BUF_SIZE, s, ap);
  va_end(ap);
  CONS_Printf(txt);
 
  // make a noise
  S_StartLocalAmbSound(sfx_pldeth);
  
  killscript = true;
}
