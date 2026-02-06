// Emacs style mode select   -*- C++ -*-
//---------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2005-2007 by DooM Legacy Team.
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
//---------------------------------------------------------------------

/// \file
/// \brief Parser driver for Flex/Lemon parsers.

#include "w_wad.h"
#include "z_zone.h"
#include "parser_driver.h"

// Flex prototypes
typedef struct yy_buffer_state *YY_BUFFER_STATE;

void DECORATE_lexer_reset();
YY_BUFFER_STATE DECORATE__scan_bytes(const char *bytes, int len);
int  DECORATE_lex(yy_t& yylval);
void DECORATE__delete_buffer(YY_BUFFER_STATE);

void *DECORATE_ParseAlloc(void *(*mallocProc)(size_t));
void  DECORATE_Parse(void *yyp, int yymajor, yy_t tokenval, decorate_driver *d);
void  DECORATE_ParseTrace(FILE *TraceFILE, char *zTracePrompt);
void  DECORATE_ParseFree(void *p, void (*freeProc)(void*));



void NTEXTURE_lexer_reset();
YY_BUFFER_STATE NTEXTURE__scan_bytes(const char *bytes, int len);
int  NTEXTURE_lex(yy_t& yylval);
void NTEXTURE__delete_buffer(YY_BUFFER_STATE);

void *NTEXTURE_ParseAlloc(void *(*mallocProc)(size_t));
void  NTEXTURE_Parse(void *yyp, int yymajor, yy_t tokenval, ntexture_driver *d);
void  NTEXTURE_ParseTrace(FILE *TraceFILE, char *zTracePrompt);
void  NTEXTURE_ParseFree(void *p, void (*freeProc)(void*));



bool Read_DECORATE(int lump)
{
  if (lump == -1)
    return false;

  // cache the lump
  char *buffer = static_cast<char *>(fc.CacheLumpNum(lump, PU_STATIC));
  int length = fc.LumpLength(lump);

  // parse the lump
  int  tokentype;
  yy_t tokenvalue;
  void *p = DECORATE_ParseAlloc(malloc);

  DECORATE_lexer_reset();

  //DECORATE_ParseTrace(stderr, "DECO: ");

  // attach the scanner to the buffer
  YY_BUFFER_STATE bufstate = DECORATE__scan_bytes(buffer, length);

  // on EOF yylex will return 0
  while ((tokentype = DECORATE_lex(tokenvalue)))
    { 
      //std::cout << " yylex() " << tokentype << " yylval.dval " << yylval.dval << std::endl;
      DECORATE_Parse(p, tokentype, tokenvalue, NULL);
    }

  // free the scanner buffer
  DECORATE__delete_buffer(bufstate);

  DECORATE_Parse(p, tokentype, tokenvalue, NULL);
  DECORATE_ParseFree(p, free);

  Z_Free(buffer);
  return true;
}


bool Read_NTEXTURE(int lump)
{
  if (lump == -1)
    return false;

  // cache the lump
  char *buffer = static_cast<char *>(fc.CacheLumpNum(lump, PU_STATIC));
  int length = fc.LumpLength(lump);

  // parse the lump
  int  tokentype;
  yy_t tokenvalue;
  void *p = NTEXTURE_ParseAlloc(malloc);

  ntexture_driver d;
  NTEXTURE_lexer_reset();

  //NTEXTURE_ParseTrace(stderr, "NTEX: ");

  // attach the scanner to the buffer
  YY_BUFFER_STATE bufstate = NTEXTURE__scan_bytes(buffer, length);

  // on EOF yylex will return 0
  while ((tokentype = NTEXTURE_lex(tokenvalue)))
    { 
      //std::cout << " yylex() " << tokentype << " yylval.dval " << yylval.dval << std::endl;
      NTEXTURE_Parse(p, tokentype, tokenvalue, &d);
    }

  // free the scanner buffer
  NTEXTURE__delete_buffer(bufstate);

  NTEXTURE_Parse(p, tokentype, tokenvalue, &d);
  NTEXTURE_ParseFree(p, free);

  Z_Free(buffer);
  return true;
}
