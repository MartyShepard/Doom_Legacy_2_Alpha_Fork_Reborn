// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2007 by DooM Legacy Team.
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
/// \brief GLSL shaders.

#ifndef oglshaders_h
#define oglshaders_h 1

#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>

#include "z_cache.h"

struct shader_attribs_t
{
  float tangent[3]; ///< surface tangent, points towards increasing s texture coord.
};


#if defined(GL_VERSION_2_0) && !defined(NO_SHADERS) // GLSL is introduced in OpenGL 2.0

/// GLSL shader object.
class Shader : public cacheitem_t
{
protected:
  static const GLuint NOSHADER = 0;
  GLuint shader_id; ///< OpenGL handle for the shader.
  GLenum type;      ///< Vertex or fragment shader?
  bool   ready;     ///< Compiled and functional?

  void Compile(const char *code, int len);

public:
  Shader(const char *name, bool vertex_shader = true);
  Shader(const char *name, const char *code, bool vertex_shader = true);
  ~Shader();

  inline GLuint GetID() const { return shader_id; }
  inline bool IsReady() const { return ready; }

  void PrintInfoLog();
};


/// GLSL Program object.
class ShaderProg : public cacheitem_t
{
protected:
  GLuint prog_id; ///< OpenGL handle for the program.
  /// Variable locations in the linked program.
  struct
  {
    GLint tex0, tex1; ///< Texture units
    GLint time;
    GLint tangent;
  } loc;

public:
  ShaderProg(const char *name);
  ~ShaderProg();

  static void DisableShaders();

  void AttachShader(Shader *s);
  void Link();
  void Use();
  void SetUniforms();
  void SetAttributes(shader_attribs_t *a);

  void PrintInfoLog();
};

#else // GL_VERSION_2_0

// Inert dummy implementation
class Shader : public cacheitem_t
{
public:
  Shader(const char *name, bool vertex_shader = true) : cacheitem_t(name) {}
};


// Inert dummy implementation
class ShaderProg : public cacheitem_t
{
public:
  ShaderProg(const char *name) : cacheitem_t(name) {}
  static void DisableShaders() {}
  void AttachShader(Shader *s) {};
  void Link() {};
  void Use() {}
  void SetUniforms() {}
  void SetAttributes(shader_attribs_t *a) {}
};


#endif // GL_VERSION_2_0


/// \brief Cache for Shaders
class shader_cache_t : public cache_t<Shader>
{
protected:
  virtual Shader *Load(const char *name) { return NULL; };
};

extern shader_cache_t shaders;


/// \brief Cache for ShaderProgs
class shaderprog_cache_t : public cache_t<ShaderProg>
{
protected:
  virtual ShaderProg *Load(const char *name) { return new ShaderProg(name); };
};

extern shaderprog_cache_t shaderprogs;



#endif
