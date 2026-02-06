// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: oglhelpers.hpp 515 2007-12-22 14:51:46Z jussip $
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
//-----------------------------------------------------------------------------

/// \file
/// This file pair contains all sorts of helper functions and arrays
/// used by the OpenGL renderer.

#ifndef oglhelpers_hpp
#define oglhelpers_hpp 1

#include"doomtype.h"

// Texture coordinates for a single quad.

class Material;
struct vertex_t;

struct quadtexcoord {
  GLfloat l;
  GLfloat r;
  GLfloat t;
  GLfloat b;
};

// Data required to render one single textured quad.

struct quad {
  Material *m;
  struct quadtexcoord t;
  vertex_t *v1;
  vertex_t *v2;
  GLfloat bottom;
  GLfloat top;
};


struct viewportdef_t
{
  float x, y; ///< lower left corner in fractional screen coords
  float w, h; ///< dimensions in fractional screen coords
};

#define MAX_GLVIEWPORTS 4
extern viewportdef_t gl_viewports[MAX_GLVIEWPORTS][MAX_GLVIEWPORTS];

byte LightLevelToLum(int l, int extralight=0);
void InitLumLut();
bool GLExtAvailable(const char *extension);

void GeometryUnitTests();

#endif // oglhelpers_hpp
