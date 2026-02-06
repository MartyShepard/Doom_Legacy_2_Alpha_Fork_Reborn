// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: parser.cpp 437 2007-02-21 19:59:44Z smite-meister $
//
// Copyright (C) 2003-2005 by DooM Legacy Team.
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
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Simple parsing functions for plaintext script lumps.

#include <string.h>
#include <ctype.h>

#include "doomdef.h"
#include "parser.h"
#include "w_wad.h"
#include "z_zone.h"


//===========================================
// Parser class implementation
//===========================================

Parser::Parser()
{
  length = 0;
  ms = me = s = e = NULL;
}


Parser::~Parser()
{
  if (ms)
    Z_Free(ms);
}


// opens a text lump for parsing, removes '\r' chars
int Parser::Open(int lump)
{
  if (lump < 0)
    return 0;

  length = fc.LumpLength(lump);
  if (length == 0)
    return 0;

  char *temp = (char *)fc.CacheLumpNum(lump, PU_STATIC);
  ms = (char *)Z_Malloc(length + 1, PU_STATIC, NULL);
  memcpy(ms, temp, length);
  Z_Free(temp);
  ms[length] = '\0'; // to make searching easy

  me = ms + length; // past-the-end pointer
  s = e = ms;

  RemoveCRs(); // NOTE this is for convenience and usually OK.
  return length;
}


// prepares a buffer for parsing
int Parser::Open(const char *buf, int len)
{
  if (len <= 0)
    return 0;

  length = len;

  ms = (char *)Z_Malloc(length + 1, PU_STATIC, NULL);
  memcpy(ms, buf, length);
  ms[length] = '\0'; // to make searching easy

  me = ms + length; // past-the-end pointer
  s = e = ms;

  return length;
}


// Removes all chars 'c' from the buffer and compactifies it.
void Parser::DeleteChars(char c)
{
  char *q = ms;
  for (char *p = ms; p < me; p++)
    if (*p != c)
      *q++ = *p;

  // and then fix the new limits
  me = q;
  length = me - ms;
  ms[length] = '\0'; // to make searching easy
}


// Clears the parser
void Parser::Clear()
{
  if (ms)
    Z_Free(ms);

  length = 0;
  ms = me = s = e = NULL;
}


// Replace all comments after 's' with whitespace.
// anything between the symbol // or ; and the next newline is a comment.
// TODO right now there is no way to escape these symbols!
void Parser::RemoveComments(char c, bool linestart)
{
  if (linestart)
    {
      // only interpret it as a comment if it is in the beginning of a line
      // (for DeHackEd and the stupid ID # thing!)
      for (char *p = s; p < me; p++)
	{
	  if (p[0] == '\n' && p[1] == c)
	    for (p++ ; p < me && *p != '\n'; p++)
	      *p = ' ';
	}   
      return;
    }

  if (c == '/')
    for (char *p = s; p+1 < me; p++)
      {
	if (p[0] == '/' && p[1] == '/')
	  for ( ; p < me && *p != '\n'; p++)
	    *p = ' ';
      }
  else
    for (char *p = s; p < me; p++)
      {
	if (p[0] == c)
	  for ( ; p < me && *p != '\n'; p++)
	    *p = ' ';
      }    
}


// replace 'from' chars with 'to' chars
int Parser::ReplaceChars(char from, char to)
{
  int ret = 0;
  for (char *p = ms; p < me; p++)
    if (*p == from)
      {
	*p = to;
	ret++;
      }
  return ret;
}



// Reads at most n chars starting from the next line. Updates e.
// Returns the number of chars actually read.
int Parser::ReadChars(char *to, int n)
{
  int i;
  for (i = 0; e < me && i < n; e++, i++)
    to[i] = *e;

  to[i] = '\0';
  return i; 
}


// NOTE you must use this before using the line-oriented Parser methods.
// Seeks the next row ending with a newline.
// Returns false if the lump ends.
bool Parser::NewLine(bool pass_ws)
{
  if (e >= me)
    return false;

  // end passes any whitespace
  if (pass_ws)
    while (e < me && isspace(*e))
      e++;

  s = e; // this is where the next line starts

  // seek the next newline
  while (e < me && (*e != '\n'))
    e++;

  if (e <= me)
    {
      // Gets the last line even if it has no ending newline.
      // me is a past-the-end pointer for the actual data, but it's still valid to write at.
      *e = '\0';  // mark the line end
      e++; // past-the-end
      return true;
    }

  return false; // lump ends
}


// passes any contiguous whitespace
void Parser::PassWS()
{
  while (s < me && isspace(*s))
    s++;
}


// replace 'from' chars with 'to' chars on current line
int Parser::LineReplaceChars(char from, char to)
{
  int ret = 0;
  for (char *p = s; *p; p++)
    if (*p == from)
      {
	*p = to;
	ret++;
      }
  return ret;
}


// Tokenizer. Advances s.
char *Parser::GetToken(const char *delim)
{
  // Damnation! If strtok_r() only was part of ISO C!
  //return strtok_r(s, delim, &s);

  // pass initial delimiters
  int n = strlen(delim);
  for (; s < me && *s; s++)
    {
      int i;
      for (i=0; i<n; i++)
	if (*s == delim[i])
	  break;
      if (i >= n)
	break;
    }

  if (s == me || *s == '\0')
    return NULL;

  char *temp = strtok(s, delim);
  s += strlen(temp) + 1;
  if (s >= e)
    s = e-1; // do not leave the line
  return temp;
}


// Get a max. 'n' character string, ignoring starting whitespace, ending with whitespace or NUL.
// NOTE that 'to' must have space for the terminating NUL char.
// Advances the 's' pointer.
int Parser::GetString(char *to, int n)
{
  PassWS();

  int i = 0;
  while (*s && !isspace(*s) && i < n)
    {
      to[i] = *s;
      s++, i++;
    }

  to[i] = '\0';
  return i;
}


// Reads at most 'n' chars from current location on, but does not go to the next line.
// NOTE that 'to' must have space for the terminating NUL char.
int Parser::GetStringN(char *to, int n)
{
  int i;
  for (i = 0; *s && i < n; s++, i++)
    to[i] = *s;

  to[i] = '\0';
  return i;
}


// Gets one char, ignoring starting whitespace.
bool Parser::GetChar(char *to)
{
  PassWS();

  if (*s)
    {
      *to = *s;
      s++;
      return true;
    }

  return false;
}


// Get an integer, advance the 's' pointer
int Parser::GetInt()
{
  char *tail = NULL;
  int val = strtol(s, &tail, 0);
  if (tail == s)
    {
      CONS_Printf("Expected an integer, got '%s'.\n", s);
      return 0;
    }

  s = tail;
  return val;
}


bool Parser::MustGetInt(int *to)
{
  char *tail = NULL;
  int val = strtol(s, &tail, 0);
  if (tail == s)
    return false;

  s = tail;
  *to = val;
  return true;
}


// Jumps to the char following the next instance of 'str' in the lump
void Parser::GoToNext(const char *str)
{
  // find the first occurrence of str
  char *res = strstr(e, str);
  if (res)
    s = e = res + strlen(str); // after the label
  else
    CONS_Printf("Parser: Label '%s' not found.\n", str);
}


// Parses one command line. cmd contains the command name itself.
// Returns true if succesful. Modifies appropriate fields in var.
bool Parser::ParseCmd(const parsercmd_t *commands, char *base)
{
  char cmd[31];
  GetString(cmd, 30);

  for ( ; commands->name != NULL; commands++)
    if (!strcasecmp(cmd, commands->name))
      break;

  if (commands->name == NULL)
    {
      CONS_Printf("Unknown command '%s' before char %d!\n", cmd, s - ms);
      return false; // not found
    }

  PassWS();
  if (s >= me)
    return false; // lump ends

  void *var  = base + commands->offset1; // this is the location of the first field
  void *var2 = base + commands->offset2; // this is the location of the second field
  int i, j, k;
  float f;

  switch (commands->type)
    {
    case P_ITEM_BOOL:
      *(bool *)var = true;
      break;

    case P_ITEM_INT:
      i = atoi(s);
      *(int *)var = i;
      break;

    case P_ITEM_INT_INT:
      k = sscanf(s, "%d %d", &i, &j);
      *(int *)var = i;
      if (k == 2)
	*(int *)var2 = j;
      break;

    case P_ITEM_PERCENT_FLOAT:
      if (strchr(s, '.'))
	f = atof(s); // floating point number
      else
	f = atoi(s) / 100.0f; // percentile
      *(float *)var = f;
      break;

    case P_ITEM_FLOAT:
      f = atof(s);
      *(float *)var = f;
      break;

    case P_ITEM_STR:
      {
	char *temp = s;

	string& result = *(string *)var;
	result = "";
	bool quoted = false;
	if (*temp == '"') // handle quotes
	  {
	    quoted = true;
	    temp++;
	  }
	  
	while (*temp)
	  {
	    char c = *temp;

	    if (c == '"' || (isspace(c) && !quoted)) // handle quotes
	      break;
	    else if (c == '\\') // handle escape sequences
	      {
		temp++;
		char r = 0;
		switch (*temp)
		  {
		  case '\\': r = '\\'; break;
		  case 'n':  r = '\n'; break;
		  case '"':  r = '"'; break;
		  }

		if (r)
		  result += r;
	      }
	    else
	      result += c;

	    temp++;
	  }
      }
      break;

    case P_ITEM_STR16:
      sscanf(s, "%16s", (char *)var);
      break;

    case P_ITEM_STR16_FLOAT:
      {
	char temp[17];
	i = sscanf(s, "%16s %f", temp, &f);
	*(string *)var = temp;
	if (i == 2)
	  *(float *)var2 = f;
      }
      break;

    default:
      break;
    }

  return true;
}



//===========================================
//  Helper functions for string handling
//===========================================

// string contains only digits?
bool IsNumeric(const char *p)
{
  for ( ; *p; p++)
    if (*p < '0' || *p > '9')
      return false;
  return true;
}


// replaces tailing spaces in a string with NUL chars
void P_StripSpaces(char *p)
{
  char *temp = p + strlen(p) - 1;

  while (*temp == ' ')
    {
      *temp = '\0';
      temp--;
    }
}


// Tries to match a string 'p' to a NULL-terminated array of strings.
// Returns the index of the first matching string, or -1 if there is no match.
int P_MatchString(const char *p, const char *strings[])
{
  for (int i=0; strings[i]; i++)
    if (!strcasecmp(p, strings[i]))
      return i;

  return -1;
}


char *strupr(char *s)
{
  for (int i=0; s[i]; i++)
    s[i] = toupper(s[i]);

  return s;
}


char *strlwr(char *s)
{
  for (int i=0; s[i]; i++)
    s[i] = tolower(s[i]);

  return s;
}


char *strnupr(char *s, int n)
{
  for (int i=0; s[i] && i<n; i++)
    s[i] = toupper(s[i]);

  return s;
}
