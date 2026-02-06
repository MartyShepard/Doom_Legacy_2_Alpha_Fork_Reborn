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
/// \brief Lemon parser for JDS NTEXTURE lumps.


%name NTEXTURE_Parse
%extra_argument {ntexture_driver *d}

%parse_accept {
  fprintf(stderr, "Parsing complete!\n");
}

%parse_failure {
  fprintf(stderr,"Giving up.  Parser is hopelessly lost...\n");
}

%stack_overflow {
  fprintf(stderr,"Giving up.  Parser stack overflow.\n");
}

%token_type {yy_t}
%token_destructor { if (yymajor == STR) Z_Free(const_cast<char*>($$.stype)); }

%type str {const char *}
%type int {int}
%type num {float}

%include {
#include "i_video.h"
#include "z_zone.h"
#include "parser_driver.h"
#include "ntexture.parser.h"
}

//============================================================================

start ::= definitions.

// sequence of 0-N texture definitions
definitions ::= . // empty
definitions ::= definitions def. // left recursion

// shader definition
def ::= shader_name L_BRACE shader_props R_BRACE.
  {
    if (rendermode == render_opengl)
      d->p->Link();
  }

shader_name ::= SHADER str(A).
  {
    if (rendermode == render_opengl)
      d->p = shaderprogs.Get(A);
  }

shader_props ::= .
shader_props ::= shader_props shader_prop. // left recursion

shader_prop ::= shader_sourcetype shader_sources SEMICOLON.

shader_sourcetype ::= V_SOURCE. { d->vertex_src = true; }
shader_sourcetype ::= F_SOURCE. { d->vertex_src = false; }

shader_sources ::= shader_src.
shader_sources ::= shader_sources shader_src. // left recursion

shader_src ::= str(A).
  {
    if (rendermode == render_opengl)
    {
      Shader *s = shaders.Find(A); // cannot use Get because Shader constructor needs type as parameter
      if (!s)
      {
	s = new Shader(A, d->vertex_src); // construct and compile
	shaders.Insert(s);
      }

      d->p->AttachShader(s); // attach a source module to the shader
    }
  }


// TODO: shader default parameters


//======================================================================
// single Material declaration
def ::= mat_type mat_name L_BRACE mat_props R_BRACE.
  {
    // Material definition finished
    if (d->m)
    {
      if (d->m->tex.size() < 1)
	I_Error("No textures given."); // FIXME proper check

      CONS_Printf(" Built material '%s'.\n", d->m->GetName());
      d->m->InitializeMaterial();
    }
  }

mat_type ::= MATERIAL. { d->is_sprite = false; }
mat_type ::= SPRITE.   { d->is_sprite = true; }

mat_name ::= str(A).
  {
    d->m = materials.Edit(A, d->is_sprite ? TEX_sprite : TEX_wall, true);
    d->texeloffsets = false;
    d->tex_unit = 0;
  }
//mat_name ::= str(A) COLON str(P).  // TODO inheritance, copy construction...
def ::= COLON. // HACK

// now we know that t points to a valid Material object, let's just fill in the props
mat_props ::= . // empty
mat_props ::= mat_props mat_prop. // left recursion

mat_prop ::= SHADER_REF str(A) SEMICOLON.
  {
    if (rendermode == render_opengl)
    {
      ShaderProg *p = shaderprogs.Find(A);
      if (p)
        d->m->shader = p;
      else
        CONS_Printf(" Unknown shader '%s'.\n", A);
    }
  }

mat_prop ::= tex_unit L_BRACE tex_props R_BRACE.
  {
    d->tex_unit++;
  }

tex_unit ::= TEX_UNIT.
  {
    int units = d->m->tex.size();
    if (units <= d->tex_unit)
      d->m->tex.resize(d->tex_unit+1);
    d->tr = &d->m->tex[d->tex_unit];
  }

tex_props ::= . // empty
tex_props ::= tex_props tex_prop. // left recursion

tex_prop ::= TEXTURE str(A) SEMICOLON.
  {
    Texture *tex = textures.Get(A);
    d->tr->t = tex;
  }

tex_prop ::= WORLDSIZE num(A) num(B) SEMICOLON.
  {
    if (d->tr->t)
    {
      d->tr->xscale = d->tr->t->width / A;
      d->tr->yscale = d->tr->t->height / B;
    }
  }
tex_prop ::= SCALE num(A) num(B) SEMICOLON.  { d->tr->yscale = 1.0/A; d->tr->xscale = d->tr->yscale/B; } 
tex_prop ::= SCALE num(A) SEMICOLON.         { d->tr->xscale = d->tr->yscale = 1.0/A; }
tex_prop ::= TEXELOFFSETS int(A) SEMICOLON.  { d->texeloffsets = A; }
tex_prop ::= OFFSET num(A) num(B) SEMICOLON.
  {
    if (d->tr->t)
      if (d->texeloffsets)
        {
	  d->tr->t->leftoffs = int(A / d->tr->xscale);
	  d->tr->t->topoffs = int(B / d->tr->yscale);
	}
      else
        {
	  d->tr->t->leftoffs = int(A);
	  d->tr->t->topoffs = int(B);
	}
  }

tex_prop ::= FILTERING str(A) str(B) SEMICOLON.
  {
    d->tr->mag_filter = (toupper(A[0]) == 'L') ? GL_LINEAR : GL_NEAREST;
    int temp;
    bool min_linear = (toupper(B[0]) == 'L');
    switch (toupper(B[1]))
      {
      case 'L':
	temp = min_linear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR; break;
      case 'N':
	temp = min_linear ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_NEAREST; break;
      default:
	temp = min_linear ? GL_LINEAR : GL_NEAREST; break;
      }

    d->tr->min_filter = temp;
  }

tex_prop ::= MAX_ANISOTROPY num(A) SEMICOLON.
  {
    d->tr->max_anisotropy = A;
  }

// string
str(A) ::= STR(B). { A = B.stype; }

// int
int(A) ::= INT(B). { A = B.itype; }

// any real number
num(A) ::= FLOAT(B). { A = B.ftype; }
num(A) ::= INT(B).   { A = B.itype; } // make it a float
