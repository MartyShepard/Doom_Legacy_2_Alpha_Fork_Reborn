// Emacs style mode select   -*- C++ -*-
//---------------------------------------------------------------------
//
// $Id: ntexture.cpp 390 2006-11-04 18:42:45Z smite-meister $
//
// Copyright (C) 2005-2006 by DooM Legacy Team.
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
/// \brief Parser for the NTEXTURE lump.

#include "ntexture.h"
#include "ntexture.tab.h"
#include "w_wad.h"

     
ntexture_driver::ntexture_driver(int lump)
  : dummy("crap", 0, 0, 0)
{
  trace_scanning = false;
  trace_parsing  = false;

  lumpname = fc.FindNameForNum(lump);
  buffer = static_cast<char *>(fc.CacheLumpNum(lump, PU_STATIC));
  length = fc.LumpLength(lump);
}


ntexture_driver::~ntexture_driver ()
{
  Z_Free(buffer);
}
     

int ntexture_driver::parse()
{
  scan_begin();
  yy::ntexture_parser parser(*this);
  parser.set_debug_level(trace_parsing);
  int res = parser.parse();
  scan_end();
  return res;
}
     
void ntexture_driver::error(const yy::location& l, const std::string& m)
{
  CONS_Printf("NTEXTURE lump '%s': (loc) %s\n", lumpname.c_str(), m.c_str()); 
}
     
void ntexture_driver::error(const std::string& m)
{
  CONS_Printf("NTEXTURE lump '%s': %s\n", lumpname.c_str(), m.c_str()); 
}



bool Read_NTEXTURE(const char *lumpname)
{
  int lump = fc.FindNumForName(lumpname);
  if (lump == -1)
    return false;

  // caches the lump
  ntexture_driver ddd(lump);

  // parse the lump
  int res = ddd.parse();

  return (res == 0);
}
