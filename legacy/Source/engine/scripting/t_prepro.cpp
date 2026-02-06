// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// $Id: t_prepro.cpp 570 2009-11-27 23:57:47Z smite-meister $
//
// Copyright(C) 2000 Simon Howard
// Copyright(C) 2001-2004 Doom Legacy Team
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
/// \brief FS preprocessor.
///
/// The preprocessor must be called when the script is first loaded.
/// It performs 2 functions:
///      1: blank out comments (which could be misinterpreted)
///      2: makes a list of all the sections held within {} braces
///      3: 'dry' runs the script: goes thru each statement and
///         sets the types of all the section_t's in the script
///      4: Saves locations of all goto() labels
///
/// the system of section_t's is pretty horrible really, but it works
/// and its probably the only way i can think of of saving scripts
/// half-way thru running

#include <stdio.h>
#include <string.h>
#include "command.h"
#include "w_wad.h"
#include "z_zone.h"

#include "t_parse.h"
#include "t_spec.h"
#include "t_vari.h"
#include "t_func.h"


//=====================================================
//                 {} sections
//=====================================================

// during preprocessing all of the {} sections
// are found. these are stored in a hash table
// according to their offset in the script. 
// functions here deal with creating new section_t's
// and finding them from a given offset.


fs_section_t *script_t::new_section(char *brace)
{
  // create section
  // make level so its cleared at start of new level
  
  fs_section_t *newsec = (fs_section_t *)Z_Malloc(sizeof(fs_section_t), PU_LEVEL, 0);
  newsec->start = brace;
  
  // hook it into the hashchain
  int n = section_hash(brace);
  newsec->next = sections[n];
  sections[n] = newsec;
  
  return newsec;
}

// find a fs_section_t from the location of the starting { brace
fs_section_t *script_t::find_section_start(char *brace)
{
  int n = section_hash(brace);
  fs_section_t *current = sections[n];
  
  // use the hash table: check the appropriate hash chain
  while (current)
    {
      if (current->start == brace)
	return current;
      current = current->next;
    }
  
  return NULL;    // not found
}

// find a fs_section_t from the location of the ending } brace
fs_section_t *script_t::find_section_end(char *brace)
{
  // hash table is no use, they are hashed according to
  // the offset of the starting brace
  
  // we have to go through every entry to find from the
  // ending brace
  
  for(int n=0; n<SECTIONSLOTS; n++)      // check all sections in all chains
    {
      fs_section_t *current = sections[n];
      
      while (current)
	{
	  if (current->end == brace)
	    return current;        // found it
	  current = current->next;
	}
    }
  
  return NULL;    // not found
}


//=====================================================
//                     labels
//=====================================================

// labels are also found during the
// preprocessing. these are of the form
//
//      label_name:
//
// and are used for the goto function.
// goto labels are stored as variables.

#define isop(c) !( ((c)<='Z' && (c)>='A') || ((c)<='z' && (c)>='a') || ((c)<='9' && (c)>='0') || ((c)=='_') )

// create a new label. pass the location inside the script
svariable_t *script_t::new_label(char *labelptr)
{
  // labels are stored as variables
  svariable_t *newlabel = new_variable(labelptr, svt_label); // we replaced the colon : with a NUL
  
  // put neccesary data in the label
  newlabel->value.labelptr = labelptr;
  
  return newlabel;
}


//=====================================================
//          main preprocessing functions
//=====================================================

// This works by recursion. when a { opening
// brace is found, another instance of the
// function is called for the data inside
// the {} section.
// At the same time, the sections are noted
// down and hashed. Goto() labels are noted
// down, and comments are blanked out

char *script_t::process_find_char(char *p, char find)
{
  while (*p)
    {
      if (*p == find)
	return p;

      if (*p == '\"') // found a quote: ignore stuff in it
	{
	  p++;
	  while(*p && *p != '\"')
	    {
	      // escape sequence ?
	      if (*p=='\\') p++;
	      p++;
	    }
	  // error: end of script in a string constant
	  if (!*p)
	    return NULL;

	  // p points to the closing quote
	}
      // comments: blank out
      else if (*p == '/' && p[1] == '*')  // /* -- */ comment
	{
	  while(*p && (*p != '*' || p[1] != '/') )
	    *p++ = ' ';

	  if (*p)
	    {
	      *p = p[1] = ' ';   // blank the last bit
	      p++;
	    }
	  else
	    {
	      rover = p;
	      // script terminated in comment
	      script_error("script terminated inside comment\n");
	    }

	  // p points to the place where the comment ended
	}
      else if (*p == '/' && p[1] == '/')        // // -- comment
	{
	  while (*p != '\n')
	    *p++ = ' ';    // blank out
	  // p points to the newline
	}
      // labels ':'  store name and location, then blank out
      else if (*p == ':' && scriptnum != -1)   // not levelscript FIXME why?
	{
	  *p = '\0'; // temporary
	  char *labelptr = p-1;
	  while(!isop(*labelptr))
	    labelptr--;

	  new_label(++labelptr);

	  while (labelptr <= p)
	    *labelptr++ = ' '; // blank it out
	  // p points to the place where the label ended
	}
      else if (*p == '{')  // { -- } sections: add 'em
	{
	  fs_section_t *newsec = new_section(p);
	  
	  newsec->type = st_empty;
	  // find the ending } and save
	  newsec->end = process_find_char(p+1, '}');
	  if (!newsec->end)
	    {                // brace not found
	      rover = p;
	      script_error("section error: no ending brace\n");
	      return NULL;
	    }
	  // continue from the end of the section
	  p = newsec->end;
	}

      p++;
    }

  return NULL;
}



/************ includes ******************/

// FraggleScript allows 'including' of other lumps.
// we divert input from the current_script (normally
// levelscript) to a seperate lump. This of course
// first needs to be preprocessed to remove comments
// etc.

// parse an 'include' lump
void parse_include(char *lumpname)
{
  int lumpnum = fc.GetNumForName(lumpname);

  if (lumpnum == -1)
    {
      script_error("include lump '%s' not found!\n", lumpname);
      return;
    }
  
  char *lump = (char *)Z_Malloc(fc.LumpLength(lumpnum) + 1, PU_STATIC, NULL);
  fc.ReadLump(lumpnum, lump);
  
  char *saved_rover = rover;    // save rover during include
  rover = lump;
  char *end = lump + fc.LumpLength(lumpnum);
  *end = 0;
  
  // blank the comments
  // we assume that it does not include sections or labels or 
  // other nasty things
  // FIXME this is awful... if it HAS sections or labels, we're "fucked".
  current_script->process_find_char(lump, 0);
  
  // now parse the lump
  parse_data(lump, end);
  
  // restore rover
  rover = saved_rover;
  
  // free the lump
  Z_Free(lump);
}



/*********** second stage parsing ************/

// second stage preprocessing considers the script
// in terms of tokens rather than as plain data.
//
// we 'dry' run the script: go thru each statement and
// collect types for fs_section_t
//
// this is an important thing to do, it cannot be done
// at runtime for 2 reasons:
//      1. gotos() jumping inside loops will pass thru
//         the end of the loop
//      2. savegames. loading a script saved inside a
//         loop will let it pass thru the loop
//
// this is basically a cut-down version of the normal
// parsing loop.

void script_t::dry_run()
{
  current_script = this;

  // save some stuff
  fs_section_t *old_current_section = current_section;

  killscript = false;

  // allocate space for the tokens
  char *token_alloc = (char *)Z_Malloc(len + T_MAXTOKENS, PU_STATIC, 0);
  char *r = data;
  char *end = data + len;
  
  while(r < end && *r)
    {
      tokens[0].v = token_alloc;
      r = get_tokens(r);
      
      if(killscript) break;
      if(!num_tokens) continue;
      
      if(current_section && tokens[0].type == TO_function)
	{
	  if(!strcmp(tokens[0].v, "if"))
	    {
              current_section->type = st_if;
	      continue;
	    }
          else if(!strcmp(tokens[0].v, "elseif"))
            {
              current_section->type = st_elseif;
              continue;
            }
          else if(!strcmp(tokens[0].v, "else"))
            {
              current_section->type = st_else;
              continue;
            }
	  else if(!strcmp(tokens[0].v, "while") ||
		  !strcmp(tokens[0].v, "for"))
	    {
	      current_section->type = st_loop;
	      current_section->data.data_loop.loopstart = linestart;
	      continue;
	    }
	}
    }
  
  Z_Free(token_alloc);
  
  // restore stuff
  current_section = old_current_section;
}



// clear the script: section and variable slots
void script_t::clear()
{
  int i;
  
  for(i=0; i<SECTIONSLOTS; i++)
    sections[i] = NULL;
  
  for(i=0; i<VARIABLESLOTS; i++)
    variables[i] = NULL;

  // clear child scripts
  for(i=0; i<MAXSCRIPTS; i++)
    children[i] = NULL;
}


// preprocesses a script
void script_t::preprocess()
{
  len = strlen(data);
  
  clear();
  process_find_char(data, 0);  // fill in everything
  dry_run();
}
