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
/// \brief GLSL shader implementation.

#include "hardware/oglshaders.h"

shader_cache_t shaders;
shaderprog_cache_t shaderprogs;

/* OpenGL Init --------------------
   liegt in oglinit.h
	 #ifdef NO_SHADERS
			# warning Shaders not included in the build.
		#elif !defined(GL_VERSION_2_0) // GLSL is introduced in OpenGL 2.0
			# warning Shaders require OpenGL 2.0!
		#else
   OpenGL Init End ------------- */

#ifndef NO_SHADERS // Marty: Geändert

#include "doomdef.h"
#include "g_map.h"
#include "w_wad.h"
#include "z_zone.h"
#include "hardware/oglrenderer.hpp"

static void PrintError()
{
  GLenum err = glGetError();
  while (err != GL_NO_ERROR)
    {
      CONS_Printf("OpenGL error: %s\n", gluErrorString(err));
      err = glGetError();
    }
}

Shader::Shader(const char *n, bool vertex_shader)
  : cacheitem_t(n), shader_id(NOSHADER), ready(false)
{
  type = vertex_shader ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;

  int lump = fc.FindNumForName(name);
  if (lump < 0)
    return;

  int len = fc.LumpLength(lump);
  char *code = static_cast<char*>(fc.CacheLumpNum(lump, PU_DAVE));
  Compile(code, len);
  Z_Free(code);
}

Shader::Shader(const char *n, const char *code, bool vertex_shader)
  : cacheitem_t(n), shader_id(NOSHADER), ready(false)
{
  type = vertex_shader ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
  Compile(code, strlen(code));
}


Shader::~Shader()
{
	#ifdef GL_USE_GLEXT
		static PFNGLDELETESHADERPROC _GLDeleteShader = NULL;
		_GLDeleteShader = (PFNGLDELETESHADERPROC) wglGetProcAddress("glDeleteShader");
	#else
		#define glDeleteShader     _GLDeleteShader
	#endif
	
	if (_GLDeleteShader)
			_GLDeleteShader(shader_id);		
	
  if (shader_id != NOSHADER)
    _GLDeleteShader/*glDeleteShader*/(shader_id);
}


void Shader::Compile(const char *code, int len)
{
	#ifdef GL_USE_GLEXT	
		static PFNGLCREATESHADERPROC  _GLCreateShader  = NULL;
		static PFNGLSHADERSOURCEPROC  _GLShaderSource  = NULL;
		static PFNGLCOMPILESHADERPROC _GLCompileShader = NULL;
		static PFNGLGETSHADERIVPROC   _GLGetShaderiv   = NULL;
			
		_GLCreateShader = (PFNGLCREATESHADERPROC)  wglGetProcAddress("glCreateShader");
		_GLShaderSource = (PFNGLSHADERSOURCEPROC)  wglGetProcAddress("glShaderSource");
		_GLCompileShader= (PFNGLCOMPILESHADERPROC) wglGetProcAddress("glCompileShader");
		_GLGetShaderiv  = (PFNGLGETSHADERIVPROC)   wglGetProcAddress("glGetShaderiv");
	#else
		#define glCreateShader     _GLCreateShader
		#define glShaderSource     _GLShaderSource
		#define glCompileShader    _GLCompileShader
		#define glGetShaderiv      _GLGetShaderiv				
	#endif	
	
  shader_id = _GLCreateShader/*glCreateShader*/(type);
  PrintError();
  _GLShaderSource/*glShaderSource*/(shader_id, 1, &code, &len);
  PrintError();
  _GLCompileShader/*glCompileShader*/(shader_id);
  PrintError();

  PrintInfoLog();

  GLint status = 0;
  _GLGetShaderiv/*glGetShaderiv*/(shader_id, GL_COMPILE_STATUS, &status);
  ready = (status == GL_TRUE);
  if (!ready)
    CONS_Printf("Shader '%s' would not compile.\n", name);
}

void Shader::PrintInfoLog()
{
  int len = 0;
	
	#ifdef GL_USE_GLEXT		
		static PFNGLGETSHADERIVPROC   		_GLGetShaderiv       = NULL;
		static PFNGLGETSHADERINFOLOGPROC  _GLGetShaderInfoLog  = NULL;	
		_GLGetShaderiv      = (PFNGLGETSHADERIVPROC)      wglGetProcAddress("glGetShaderiv");
		_GLGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC) wglGetProcAddress("glGetShaderInfoLog");
	#else
		#define glGetShaderInfoLog _GLGetShaderiv
		#define glGetShaderiv      _GLGetShaderiv				
	#endif
	
  _GLGetShaderiv/*glGetShaderiv*/(shader_id, GL_INFO_LOG_LENGTH, &len);

  if (len > 0)
    {
      char *log = static_cast<char*>(Z_Malloc(len, PU_DAVE, NULL));
      int chars = 0;
      _GLGetShaderInfoLog/*glGetShaderInfoLog*/(shader_id, len, &chars, log);
      CONS_Printf("Shader '%s' InfoLog: %s\n", name, log);
      Z_Free(log);
    }
}




ShaderProg::ShaderProg(const char *n)
  : cacheitem_t(n)
{
	#ifdef GL_USE_GLEXT			
		static PFNGLCREATEPROGRAMPROC _GLCreateProgram = NULL;
		_GLCreateProgram    = (PFNGLCREATEPROGRAMPROC)   wglGetProcAddress("glCreateProgram");
	#else
		#define glCreateProgram _GLCreateProgram			
	#endif	
	
  prog_id = _GLCreateProgram/*glCreateProgram*/();
  PrintError();
}

ShaderProg::~ShaderProg()
{
	#ifdef GL_USE_GLEXT		
		static PFNGLDELETEPROGRAMPROC _GLDeleteProgram = NULL;
		_GLDeleteProgram    = (PFNGLDELETEPROGRAMPROC)   wglGetProcAddress("glDeleteProgram");
	#else
		#define glDeleteProgram _GLDeleteProgram			
	#endif		
  _GLDeleteProgram/*glDeleteProgram*/(prog_id);
}

void ShaderProg::DisableShaders()
{
	#ifdef GL_USE_GLEXT		
		static PFNGLUSEPROGRAMPROC _GLUseProgram = NULL;
		_GLUseProgram    = (PFNGLUSEPROGRAMPROC)   wglGetProcAddress("glUseProgram");
	#else
		#define glUseProgram _GLUseProgram			
	#endif
	
  _GLUseProgram/*glUseProgram*/(0);
}

void ShaderProg::AttachShader(Shader *s)
{
	#ifdef GL_USE_GLEXT	
		static PFNGLATTACHSHADERPROC _GLAttachShader = NULL;
		_GLAttachShader    = (PFNGLATTACHSHADERPROC)   wglGetProcAddress("glAttachShader");	
	#else
		#define glAttachShader _GLAttachShader			
	#endif
	
	_GLAttachShader/*glAttachShader*/(prog_id, s->GetID());
  PrintError();
}

void ShaderProg::Link()
{
	#ifdef GL_USE_GLEXT	
		static PFNGLLINKPROGRAMPROC 				_GLLinkProgram 			 = NULL;
		static PFNGLVALIDATEPROGRAMPROC 		_GLValidateProgram 	 = NULL;	
		static PFNGLGETPROGRAMIVPROC 				_GLGetProgramiv   	 = NULL;	
		static PFNGLGETUNIFORMLOCATIONPROC 	_GLGetUniformLocation= NULL;	
		static PFNGLGETATTRIBLOCATIONPROC 	_GLGetAttribLocation = NULL;	
		
		_GLLinkProgram    		= (PFNGLLINKPROGRAMPROC)   	 		wglGetProcAddress("glLinkProgram");
		_GLValidateProgram		= (PFNGLVALIDATEPROGRAMPROC) 		wglGetProcAddress("glValidateProgram");	
		_GLGetProgramiv				= (PFNGLGETPROGRAMIVPROC) 	 		wglGetProcAddress("glGetProgramiv");
		_GLGetUniformLocation	= (PFNGLGETUNIFORMLOCATIONPROC) wglGetProcAddress("glGetUniformLocation");	
		_GLGetAttribLocation	= (PFNGLGETATTRIBLOCATIONPROC)  wglGetProcAddress("glGetAttribLocation");	
	#else
		#define glLinkProgram 				_GLLinkProgram
		#define glValidateProgram 		_GLValidateProgram
		#define glGetProgramiv 				_GLGetProgramiv
		#define glGetUniformLocation 	_GLGetUniformLocation
		#define glGetAttribLocation 	_GLGetAttribLocation
	#endif
	
  _GLLinkProgram/*glLinkProgram*/(prog_id);
  PrintError();
  if (true)
    {
      // for debugging shaders
      _GLValidateProgram/*glValidateProgram*/(prog_id);
      PrintError();
      PrintInfoLog();
      PrintError();
    }

  GLint status = 0;
  _GLGetProgramiv/*glGetProgramiv*/(prog_id, GL_LINK_STATUS, &status);
  if (status == GL_FALSE)
    CONS_Printf("Shader program '%s' could not be linked.\n", name);

  struct shader_var_t
  {
    GLint *location;
    const char *name;
  };

  shader_var_t uniforms[] = {
    {&loc.tex0, "tex0"},
    {&loc.tex1, "tex1"},
    {&loc.time, "time"},
  };

  shader_var_t attribs[] = {
    {&loc.tangent, "tangent"},
  };

  // TODO the var names need to be defined (they define the Legacy-shader interface!)

  // find locations for shader variables
  for (int k=0; k<3; k++)
    if ((*uniforms[k].location = _GLGetUniformLocation/*glGetUniformLocation*/(prog_id, uniforms[k].name)) == -1)
      CONS_Printf("Uniform shader var '%s' not found!\n", uniforms[k].name);

  for (int k=0; k<1; k++)
    if ((*attribs[k].location = _GLGetAttribLocation/*glGetAttribLocation*/(prog_id, attribs[k].name)) == -1)
      CONS_Printf("Attribute shader var '%s' not found!\n", attribs[k].name);
}

void ShaderProg::Use()
{
	#ifdef GL_USE_GLEXT	
		static PFNGLUSEPROGRAMPROC _GLUseProgram = NULL;
		_GLUseProgram    = (PFNGLUSEPROGRAMPROC)   wglGetProcAddress("glUseProgram");
	#else
		#define glUseProgram 				_GLUseProgram
	#endif
		
  _GLUseProgram/*glUseProgram*/(prog_id);
}

void ShaderProg::SetUniforms()
{
	#ifdef GL_USE_GLEXT		
		static PFNGLUNIFORM1IPROC	_GLUniform1i = NULL;
		static PFNGLUNIFORM1FPROC _GLUniform1f = NULL;			
		_GLUniform1i = (PFNGLUNIFORM1IPROC) wglGetProcAddress("glUniform1i");
		_GLUniform1f = (PFNGLUNIFORM1FPROC) wglGetProcAddress("glUniform1f");		
	#else
		#define glUniform1i 				_GLUniform1i
		#define glUniform1f 				_GLUniform1f	
	#endif	
  // only call after linking!
  // set uniform vars (per-primitive vars, ie. only changed outside glBegin()..glEnd())
  _GLUniform1i/*glUniform1i*/(loc.tex0, 0);
  _GLUniform1i/*glUniform1i*/(loc.tex1, 1);
  _GLUniform1f/*glUniform1f*/(loc.time, oglrenderer->mp->maptic/60.0);
}

void ShaderProg::SetAttributes(shader_attribs_t *a)
{
	#ifdef GL_USE_GLEXT			
		static PFNGLVERTEXATTRIB3FVPROC _GLVertexAttrib3fv = NULL;			
		_GLVertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC) wglGetProcAddress("glVertexAttrib3fv");
	#else
		#define glVertexAttrib3fv 				_GLVertexAttrib3fv
	#endif			
	
  // then vertex attribute vars (TODO vertex attribute arrays) (can be changed anywhere)
  _GLVertexAttrib3fv/*glVertexAttrib3fv*/(loc.tangent, a->tangent);
}

void ShaderProg::PrintInfoLog()
{
  int len = 0;

	#ifdef GL_USE_GLEXT			
		static PFNGLGETPROGRAMIVPROC 			_GLGetProgramiv 		= NULL;
		static PFNGLGETPROGRAMINFOLOGPROC _GLGetProgramInfoLog= NULL;				
		_GLGetProgramiv 		 = (PFNGLGETPROGRAMIVPROC) 			wglGetProcAddress("glGetProgramiv");
		_GLGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC) wglGetProcAddress("glGetProgramInfoLog");
	#else
		#define glGetProgramiv 			_GLGetProgramiv
		#define glGetProgramInfoLog _GLGetProgramInfoLog		
	#endif	
	
  _GLGetProgramiv/*glGetProgramiv*/(prog_id, GL_INFO_LOG_LENGTH, &len);

  if (len > 0)
    {
      char *log = static_cast<char*>(Z_Malloc(len, PU_DAVE, NULL));
      int chars = 0;
      _GLGetProgramInfoLog/*glGetProgramInfoLog*/(prog_id, len, &chars, log);
      CONS_Printf("Shader program '%s' InfoLog: %s\n", name, log);
      Z_Free(log);
    }
}

#endif // GL_VERSION_2_0

