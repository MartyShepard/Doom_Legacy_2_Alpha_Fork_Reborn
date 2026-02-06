// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// 
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
/* Marty */

#ifndef oglinit
#define oglinit 1

#define GL_GLEXT_PROTOTYPES 1
#define GL_USE_GLEXT

#if defined(__APPLE_CC__) || defined(__MACOS__)

	#include <OpenGL/gl.h>
	#include <OpenGL/glu.h>
#else
	/*Marty*/
	#ifndef GL_USE_GLEXT				// ← MUSS ALS ERSTES vor GL.h kommen!
		#include "GL/glew.h"     	// Damit wird aber auch die Glew32.dll benötigt																	
	#endif
	
	#include "GL/gl.h"          // oder <SDL/SDL_opengl.h>, aber glew.h zuerst!
	
	#ifdef GL_USE_GLEXT	
		#include "GL/glext.h"     // ← MUSS immer nach GL.h kommen
	//#include "GL/glcorearb.h"	
	#endif
	
	#include "GL/glu.h"					// nur wenn man glu* braucht
	
	//# warning OpengGL Headers Included.
#endif


#ifdef NO_SHADERS
	# warning Shaders not included in the build.	
	
#elif !defined(GL_VERSION_2_0) // GLSL is introduced in OpenGL 2.0
	# warning Shaders require OpenGL 2.0!	

#endif

#endif
