// Emacs style mode select   -*- C++ -*-
//---------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2006-2007 by DooM Legacy Team.
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
/// \brief Parser driver header for Flex/Lemon parsers.

#ifndef driver_h
#define driver_h 1
#include <assert.h>
#include "doomdef.h"
#include "r_data.h"
#include "hardware/oglshaders.h"


#ifndef YYSTYPE
union yy_t
{
  float ftype;
  int   itype;
  const char *stype; // copy of the parsed string, must be destroyed later on
};
# define YYSTYPE yy_t // sizeof(yy_t) == small, no point in using a pointer
# define YYSTYPE_IS_TRIVIAL 1
#endif


/// temp variables for building Textures
struct ntexture_driver
{
  class ShaderProg *p;
  bool vertex_src;

  class Material *m;
  bool is_sprite;
  bool texeloffsets;
  int  tex_unit;
  struct Material::TextureRef *tr;
};


/// temp variables for building ActorInfos
typedef void decorate_driver;


#endif
