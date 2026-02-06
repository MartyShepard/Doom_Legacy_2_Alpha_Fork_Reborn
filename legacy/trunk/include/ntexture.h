// Emacs style mode select   -*- C++ -*-
//---------------------------------------------------------------------
//
// $Id: ntexture.h 381 2006-09-10 22:20:52Z smite-meister $
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
/// \brief Parsing driver class for the JDS NTEXTURE lump

#ifndef ntexture_h
#define ntexture_h

#include <string>
#include "r_data.h"

// Forward declarations
// All Bison parser stuff is inside this namespace
namespace yy
{
  class location;
  class ntexture_parser;
}



/// \brief Conducts the scanning and parsing
class ntexture_driver
{
protected:
  std::string lumpname;
  char *buffer;
  int   length;

  bool trace_scanning;
  bool trace_parsing;

public:
  // temp variables for building textures
  std::string texname;
  Texture *t;
  LumpTexture dummy;
  bool texeloffsets;
  bool is_sprite;

public:
  ntexture_driver(int lump);
  virtual ~ntexture_driver();
  
  // Handling the scanner.
  void scan_begin();
  void scan_end();

  // Handling the parser.
  int  parse();

  // Error handling.
  void error(const yy::location& l, const std::string& m);
  void error(const std::string& m);
};


#endif
