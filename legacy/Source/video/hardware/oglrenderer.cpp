// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: oglrenderer.cpp 536 2009-06-29 06:46:13Z smite-meister $
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
/// \brief OpenGL renderer.

/* OpenGL Init --------------------
 * liegt in oglinit.h
   #define GL_GLEXT_PROTOTYPES 1
   #include <GL/gl.h>
 */
#include "Hardware/ogl_init.h"
/* OpenGL Init End ------------- */

#include "doomdef.h"
#include "command.h"
#include "cvars.h"

#include "g_map.h"
#include "p_maputl.h"
#include "g_player.h"
#include "g_pawn.h"
#include "g_actor.h"
#include "g_mapinfo.h"

#include "hardware/oglrenderer.hpp"
#include "hardware/oglhelpers.hpp"
#include "hardware/oglshaders.h"

#include "screen.h"
#include "v_video.h"
#include "tables.h"
#include "r_data.h"
#include "r_main.h"
#include "r_presentation.h"
#include "r_sprite.h"
#include "am_map.h"
#include "w_wad.h" // Need file cache to get playpal.
#include "z_zone.h"


extern trace_t trace;

void MD3_InitNormLookup();


/*!
  \defgroup g_opengl OpenGL renderer

  Definitions:
    s: physical screen dimensions
    p: physical pixel dimensions
    r: screen resolution in pixels
    Screen aspect ratio:     a_s := w_s/h_s
    Pixel aspect ratio:      a_p := w_p/h_p
    Resolution aspect ratio: a_r := w_r/h_r

    Fundamental equalities:  w_s = w_r*w_p, h_s = h_r*h_p

    From these we get   a_s = a_p * a_r


  Doom was originally meant to be rendered on a CRT display with a_s = 4/3 aspect ratio and 320x200 resolution,
  which gives a_r = 320/200 = 1.6 = 16/10 and a_p = 5/6. Hence the pixels are not supposed to be square, but a little
  stretched in the vertical direction.

  Modern display equipment almost invariably has square pixels, i.e. a_p = 1. A "standard" display still has a_s = 4/3,
  whereas a widescreen display has a_s = 16/10.

  And herein we have a problem. If we want to draw a 2D Doom graphic on a modern screen, we'll have to scale it in the
  y direction by a factor of 6/5 or it'll look squashed. However, if we want to use nearest-pixel sampling for a crisp,
  sharp image, the scaling factors in both x and y directions should be integers to avoid visual artefacts.
  The lowest resolution where this can be perfectly accomplished even for full-screen images is 1600x1200 (sx = 5, sy = 6).
  For smaller images, e.g. menu graphics, we could use the perfect scaling but draw the images closer together to fit them on screen?
  Additionally, with a widescreen display 1/6 of the screen area is unused (the sides).
  NOTE: At the moment we do no such y-scaling, so the 2D images ARE squashed. TODO 3D?

  The 3D view does not cause such problems, because all the textures need to be scaled anyway depending on the viewing angle and distance.
*/



OGLRenderer::OGLRenderer()
{
  workinggl = false;
  glversion = 0;
  screen = NULL;
  curssec = NULL;

  palette = static_cast<RGB_t*>(fc.CacheLumpName("PLAYPAL", PU_STATIC));

  mp = NULL;

  x = y = z = 0.0;
  theta = phi = 0.0;
  viewportw = viewporth = 0;

  fov = 90.0;

 

  hudar = 4.0/3.0; // Basic DOOM hud takes the full screen.
  screenar = 1.0;  // Pick a default value, any default value.
  viewportar = 1.0;

  // Initially we are at 2D mode.
  consolemode = true;

  InitLumLut();
  MD3_InitNormLookup();

  CONS_Printf("New OpenGL renderer created.\n");
}

OGLRenderer::~OGLRenderer()
{
  CONS_Printf("Closing OpenGL renderer.\n");
  Z_Free(palette);
  // No need to release screen. SDL does it automatically.
}



/// Sets up those GL states that never change during rendering.
void OGLRenderer::InitGLState()
{
  glShadeModel(GL_SMOOTH);
  glEnable(GL_TEXTURE_2D);

  // GL_ALPHA_TEST gets rid of "black boxes" around sprites. Might also speed up rendering a bit?
  glAlphaFunc(GL_GEQUAL, 0.5); // 0.5 is an optimal value for linear magfiltering
  glEnable(GL_ALPHA_TEST);

  /*glDepthFunc(GL_LESS);*/


  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_CULL_FACE);

  // lighting
  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE); // now we can use glColor4 to do alpha effects
  glColor4f(1.0, 1.0, 1.0, 1.0);

  //GLfloat mat_ad[]  = { 1.0, 1.0, 1.0, 1.0 }; 											//* Marty, Ausgekommentiert
  //glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,mat_ad ); 							//* Marty, Ausgekommentiert, mat_ad hinmzugefügt
  //glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,mat_ad ); 							//* Marty, Ausgekommentiert, mat_ad hinmzugefügt
  //glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mat_ad); 	//* Marty, Ausgekommentiert
  //glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,mat_ad ); 						//* Marty, Ausgekommentiert, mat_ad hinmzugefügt
  //glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0); 								//* Marty, Ausgekommentiert
  //glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,mat_ad ); 						//* Marty, Ausgekommentiert, mat_ad hinmzugefügt

  GLfloat lmodel_ambient[] = { 1.0, 1.0, 1.0, 1.0 };
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
  //glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE); //* Marty, Ausgekommentiert
  //glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE); //* Marty, Ausgekommentiert

  // TEST positional/directional light
  //glEnable(GL_LIGHT0); //* Marty, Ausgekommentiert
  GLfloat light_ambient[]  = { 0.0, 0.0, 0.0, 1.0 };
  GLfloat light_diffuse[]  = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
  glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

  GLfloat light_position[] = { 1.0, -1.0, 0.0, 0.0 }; // infinitely far away
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	#ifdef GL_USE_GLEXT	
		static PFNGLPOINTPARAMETERFPROC  _GLPointParameterf  = NULL;
		static PFNGLPOINTPARAMETERFVPROC _GLPointParameterfv = NULL;		
		_GLPointParameterf  = (PFNGLPOINTPARAMETERFPROC)  wglGetProcAddress("glPointParameterf");
		_GLPointParameterfv = (PFNGLPOINTPARAMETERFVPROC) wglGetProcAddress("glPointParameterfv");
	#else
		#define glPointParameterf  _GLPointParameterf
		#define glPointParameterfv _GLPointParameterfv		
	#endif
	
  // red aiming dot parameters
  glEnable(GL_POINT_SMOOTH);
  glPointSize(8.0);
  _GLPointParameterf/*glPointParameterf*/(GL_POINT_SIZE_MIN, 2.0);
  _GLPointParameterf/*glPointParameterf*/(GL_POINT_SIZE_MAX, 8.0);
  GLfloat point_att[3] = {1, 0, 1e-4};
  _GLPointParameterfv/*glPointParameterfv*/(GL_POINT_DISTANCE_ATTENUATION, point_att);

  // other debugging stuff
  glLineWidth(3.0);
	
}


/// Clean up stuffage so we can start drawing a new frame.
void OGLRenderer::StartFrame()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  ClearDrawColorAndLights();
}


/// Done with drawing. Swap buffers.
void OGLRenderer::FinishFrame()
{
  SDL_GL_SwapBuffers(); // Double buffered OpenGL goodness.
}

// Set default material colors and lights to bright white with full
// intensity.

void OGLRenderer::ClearDrawColorAndLights()
{
  GLfloat lmodel_ambient[] = { 1.0, 1.0, 1.0, 1.0 };
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
  glColor4f(1.0, 1.0, 1.0, 1.0);
}


void OGLRenderer::SetGlobalColor(GLfloat *rgba)
{
}


// Writes a screen shot to the specified file. Writing is done in BMP
// format, as SDL has direct support of that. Adds proper file suffix
// when necessary. Returns true on success.
bool OGLRenderer::WriteScreenshot(const char *fname)
{
  int fnamelength;
  string finalname;
  SDL_Surface *buffer;
  bool success;

  if(screen == NULL)
    return false;

  /*
  if(screen->pixels == NULL) {
    CONS_Printf("Empty SDL surface. Can not take screenshot.\n");
    return false;
  }
  */

  if(fname == NULL)
    finalname = "DOOM000.bmp";
  else {    
    fnamelength = strlen(fname);

    if(!strcmp(fname+fnamelength-4, ".bmp") ||
       !strcmp(fname+fnamelength-4, ".BMP"))
      finalname = fname;
    else {
      finalname = fname;
      finalname += ".bmp";
    }
  }

  // Now we know the file name. Time to start the magic.

  // Potential endianness bug with masks?
  buffer = SDL_CreateRGBSurface(SDL_SWSURFACE, screen->w, 
				screen->h, 32, 0xff000000, 0xff0000, \
				0xff00, 0xff);
  if(!buffer) {
    CONS_Printf("Could not create SDL surface. SDL error: %s\n", SDL_GetError());
    return false;
  }

  SDL_LockSurface(buffer);
  glReadPixels(0, 0, screen->w, screen->h, GL_RGBA, 
	       GL_UNSIGNED_INT_8_8_8_8, buffer->pixels);

  // OpenGL keeps the pixel data "upside down" for some reason. Flip
  // the surface.
  char *templine = new char[buffer->pitch];
  for(int j=0; j<buffer->h/2; j++) {
    char *p1;
    char *p2;
    p1 = static_cast<char*>(buffer->pixels) + j*buffer->pitch;
    p2 = static_cast<char*>(buffer->pixels) + (buffer->h-j-1)*buffer->pitch;
    memcpy(templine, p1, buffer->pitch);
    memcpy(p1, p2, buffer->pitch);
    memcpy(p2, templine, buffer->pitch);
  }
  delete[] templine;

  SDL_UnlockSurface(buffer);
  success = !SDL_SaveBMP(buffer, finalname.c_str());
  SDL_FreeSurface(buffer);
  return success;
}

bool OGLRenderer::InitVideoMode(const int w, const int h, /*const*/ bool fullscreen)
{
  printf("\n [%s][%d]OGLRenderer::InitVideoMode %d\n",__FILE__,__LINE__, fullscreen);
  Uint32 surfaceflags;
  int mindepth = 16;
  int temp;
  bool first_init;

  first_init = screen ? false : true;
	
  // Some platfroms silently destroy OpenGL textures when changing
  // resolution. Unload them all, just in case.
  materials.ClearGLTextures();

  workinggl = false;

  surfaceflags = SDL_OPENGL;
	
  if(fullscreen)
    surfaceflags |= SDL_FULLSCREEN;
  
  if (surfaceflags & SDL_FULLSCREEN)
  {
    // So entfernt man ein Flag wieder (korrekt!)
    surfaceflags &= ~SDL_FULLSCREEN;
  }  

  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16/*8*/);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  // Check that we get hicolor.
  int cbpp = SDL_VideoModeOK(w, h, 24, surfaceflags);
  if(cbpp < mindepth) {
    CONS_Printf(" Hicolor OpenGL mode not available.\n");
    return false;
  }

  screen = SDL_SetVideoMode(w, h, cbpp, surfaceflags);
  if(!screen) {
    CONS_Printf(" Could not obtain requested resolution.\n");
    return false;
  }
	
  // This is the earlies possible point to print these since GL
  // context is not guaranteed to exist until the call to
  // SDL_SetVideoMode.
  if(first_init) {
    CONS_Printf(" OpenGL vendor:   %s\n", glGetString(GL_VENDOR));
    CONS_Printf(" OpenGL renderer: %s\n", glGetString(GL_RENDERER));
    const char *str = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    glversion = str ? strtof(str, NULL) : 0;
    CONS_Printf(" OpenGL version:  %s\n", str);
  }

  CONS_Printf(" Color depth in bits: ");
  SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &temp);
  CONS_Printf("R%d ", temp);
  SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &temp);
  CONS_Printf("G%d ", temp);
  SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &temp);
  CONS_Printf("B%d.\n", temp);
  SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &temp);
  CONS_Printf(" Alpha buffer depth %d bits.\n", temp);
  SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &temp);
  CONS_Printf(" Depth buffer depth %d bits.\n", temp);

  SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &temp);
  if(temp)
    CONS_Printf(" OpenGL mode is double buffered.\n");
  else
    CONS_Printf(" OpenGL mode is NOT double buffered.\n");

	/* Marty */
	
  SDL_GL_GetAttribute(SDL_GL_BUFFER_SIZE, &temp);
  CONS_Printf(" 24/32 Bit Farbe + Alpha: %d bits.\n", temp);
	
  SDL_GL_GetAttribute(SDL_GL_ACCELERATED_VISUAL, &temp);
  if(temp==0)
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL,  1);
  
  SDL_GL_GetAttribute(SDL_GL_ACCELERATED_VISUAL, &temp);
  CONS_Printf(" Hardware-Beschleunigung %s.\n", (temp==1)?"Ja":"Nein");
		
  SDL_GL_GetAttribute(SDL_GL_SWAP_CONTROL, &temp);
  if(temp==-1)
		CONS_Printf(" VSync Modus: Adaptive\n", temp);
	else
		CONS_Printf(" VSync Modus: %s\n", (temp==1)?"An":"Aus");
	
  SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &temp);
	CONS_Printf(" Anti-Aliasing %d.\n", temp);
	if (temp==0)
	{
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,  1);
		SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &temp);
		CONS_Printf(" Anti-Aliasing %d (Geaendert).\n", temp);
	}

  SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &temp);
	CONS_Printf(" Multisampling (MSAA): %d.\n", temp);
	if (temp==0)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,  4);
		SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &temp);
		CONS_Printf(" Multisampling (MSAA): %d (Geaendert)\n", temp);
	}		

  // Print state and debug info.
  CONS_Printf(" Set OpenGL video mode %dx%dx%d", w, h, cbpp);
  CONS_Printf(fullscreen ? " (fullscreen)\n" : " (windowed)\n");

  // Calculate the screen's aspect ratio. Assumes square pixels.
  // Check a couple of exceptions.
  // Marty: Dafür muss eine Routine her. Sonst crash mit fullscreen .....
  if(w == 1280 && h == 1024 && surfaceflags & SDL_FULLSCREEN)
  {
    screenar = 4.0/3.0;
  }
  else if(w == 320 && h == 200 && surfaceflags & SDL_FULLSCREEN)
  {
    screenar = 4.0/3.0;
  }
  else
  {
    if (surfaceflags & SDL_FULLSCREEN)
    {
        surfaceflags |= SDL_FULLSCREEN;
        fullscreen = false;
        printf(" [%s][%d]OGLRenderer::InitVideoMode:         \n"
               "                      Fallback to Window Mode\n"
               "                 dump... Hardcdoded Resolution",__FILE__,__LINE__);
    }
    
    screenar = GLfloat(w)/h;
  }

  CONS_Printf(" Screen aspect ratio %.2f.\n", screenar);
  CONS_Printf(" HUD aspect ratio %.2f.\n", hudar);

  workinggl = true;

  // Reset matrix transformations.
  SetFullScreenViewport();

  InitGLState();

  // Clear any old GL errors.
  while (glGetError() != GL_NO_ERROR)
    ;

  if(GLExtAvailable("GL_ARB_multitexture"))
    CONS_Printf(" GL multitexturing supported.\n");
  else
    CONS_Printf(" GL multitexturing not supported. Expect trouble.\n");

  if(GLExtAvailable("GL_ARB_texture_non_power_of_two"))
    CONS_Printf(" Non power of two textures supported.\n");
  else
    CONS_Printf(" Only power of two textures supported.\n");


  #ifdef BORDERLESS_WIN32
  if(!fullscreen)
  {    
    ToggleBorderless();
		CenterSDL1Window();
  }
  #endif         // Marty
	
  return true;
}

// Set up viewport projection matrix, and 2D mode using the new aspect ratio.
void OGLRenderer::SetFullScreenViewport()
{
  viewportw = screen->w;
  viewporth = screen->h;
  viewportar = GLfloat(viewportw)/viewporth;
  glViewport(0, 0, viewportw, viewporth);
  Setup2DMode();
}


void OGLRenderer::SetViewport(unsigned vp)
{
  // Splitscreen.
  unsigned n = min(cv_splitscreen.value + 1, MAX_GLVIEWPORTS) - 1;

  if (vp > n)
    return;

  viewportdef_t *v = &gl_viewports[n][vp];

  viewportw = GLint(screen->w * v->w);
  viewporth = GLint(screen->h * v->h);
  viewportar = GLfloat(viewportw)/viewporth;
  glViewport(GLint(screen->w * v->x), GLint(screen->h * v->y), viewportw, viewporth);
}



/// Set up the GL matrices so that we can draw 2D stuff like menus.
void OGLRenderer::Setup2DMode()
{
  GLfloat extraoffx, extraoffy, extrascalex, extrascaley;
  consolemode = true;
  ClearDrawColorAndLights();

  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0.0, 1.0, 0.0, 1.0);
  
  /* Marty */
  GLfloat Width  = screen->w;
  GLfloat Height = screen->h;
  GLfloat Ratio  = Width/Height;
  
  //printf("OGLRenderer: %d\n", cv_graspectratio.value);
  
  if ((Ratio != hudar) && (cv_graspectratio.value == 0))//Auto
  {    
    //printf("OGLRenderer: Ratio %.3f != hudar %.3f\n", Ratio, hudar);

    if (Ratio >= 1.185f && Ratio < 1.250f)  // 32:27 bis 5:4
        hudar = 32.0f / 27.0f;
        
    else if (Ratio >= 1.250f && Ratio < 1.333f)  // 5:4 bis 4:3
        hudar = 5.0f / 4.0f;
        
    else if (Ratio >= 1.333f && Ratio < 1.600f)  // 4:3 bis 16:10
        hudar = 4.0f / 3.0f;
        
    else if (Ratio >= 1.600f && Ratio < 1.778f)  // 16:10 bis 16:9
        hudar = 32.0f / 27.0f/*16.0f / 10.0f*/;  // oder 4.0f / 3.0f als Fallback
        
    else if (Ratio >= 1.778f)  // 16:9+
        hudar = 32.0f / 27.0f/*16.0f / 9.0f*/;   // oder pillarbox zu 4:3   
  }
  else if  (cv_graspectratio.value == 1) 
    hudar = 4.0f / 3.0f;
  else if  (cv_graspectratio.value == 2) 
    hudar = 5.0f / 4.0f;   
  else if  (cv_graspectratio.value == 3) 
    hudar = 16.0f / 9.0f;     
  else if  (cv_graspectratio.value == 4)
     hudar = 16.0f / 10.0f;
  else if  (cv_graspectratio.value == 5) 
     hudar = 32.0f / 27.0f;
  /* Marty End */
          
  if(viewportar > hudar)
  {
    extraoffx = (viewportar - hudar)/(viewportar*2.0);
    extraoffy = 0.0;
    extrascalex = hudar/viewportar;
    extrascaley = 1.0;
  }
  else if(viewportar < hudar)
  {     
     extraoffx = 0.0;
     extraoffy = (hudar - viewportar)/(hudar*2.0);
     extrascalex = 1.0;
     extrascaley = viewportar/hudar;
      
  }
  else
  {
     extrascalex = extrascaley = 1.0;
     extraoffx = extraoffy = 0.0;
  }

  glTranslatef(extraoffx, extraoffy, 0.0);
  glScalef(extrascalex, extrascaley, 1.0);
   
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}


/// Setup GL matrices to render level graphics.
void OGLRenderer::Setup3DMode()
{
  consolemode = false;
  ClearDrawColorAndLights();

  glEnable(GL_LIGHTING);
  glEnable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  // Read projection information from consvars. Currently only fov.
  fov = max(1.0f, min(GLfloat(cv_fov.value), 180.0f));

  // Load clipping planes from consvars.
  gluPerspective(fov*hudar/viewportar, viewportar, cv_grnearclippingplane.Get().Float(), cv_grfarclippingplane.Get().Float());

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Now we can render items using their Doom coordinates directly.

}

// Draws a square defined by the parameters. The square is filled by
// the specified Material.
void OGLRenderer::Draw2DGraphic(GLfloat left, GLfloat bottom,
				GLfloat right, GLfloat top,
				Material *mat,
				GLfloat texleft, GLfloat texbottom,
				GLfloat texright, GLfloat textop)
{
  //  GLfloat scalex, scaley, offsetx, offsety;
  //  GLfloat texleft, texright, textop, texbottom;

  if(!workinggl)
    return;
  
  if(!consolemode)
    I_Error("Attempting to draw a 2D HUD graphic while in 3D mode.\n");

  //  printf("Drawing tex %d at (%.2f, %.2f) (%.2f, %.2f).\n", tex, top, left, bottom, right);

  mat->GLUse();
  glBegin(GL_QUADS);
  glTexCoord2f(texleft, texbottom);
  glVertex2f(left, bottom);
  glTexCoord2f(texright, texbottom);
  glVertex2f(right, bottom);
  glTexCoord2f(texright, textop);
  glVertex2f(right, top);
  glTexCoord2f(texleft, textop);
  glVertex2f(left, top);
  glEnd();
}

// Just like the earlier one, except the coordinates are given in Doom
// units. (screen is 320 wide and 200 high.)

void OGLRenderer::Draw2DGraphic_Doom(GLfloat x, GLfloat y, Material *mat, int flags)
{
  // OpenGL origin is at the bottom left corner. Doom origin is at top left.
  // Texture coordinates follow the Doom convention.

  /*
  if((x+width) > doomscreenw || (y+height) > doomscreenh)
    printf("Tex %d out of bounds: (%.2f, %.2f) (%.2f, %.2f).\n", tex, x, y, x+width, y+height);  
  */
  GLfloat l, r, t, b;

  // location scaling
  if (flags & V_SLOC)
    {
      l = x / BASEVIDWIDTH;
      t = y / BASEVIDHEIGHT;
    }
  else
    {
      l = x / screen->w;
      t = y / screen->h;
    }

  Material::TextureRef &tr = mat->tex[0];

  // size scaling
  // assume offsets in world units
  if (flags & V_SSIZE)
    {
      l -= mat->leftoffs / BASEVIDWIDTH;
      r = l + tr.worldwidth / BASEVIDWIDTH;

      t -= mat->topoffs / BASEVIDHEIGHT;
      b = t + tr.worldheight / BASEVIDHEIGHT;
    }
  else
    {
      l -= mat->leftoffs / screen->w;
      r = l + tr.worldwidth / screen->w;

      t -= mat->topoffs / screen->h;
      b = t + tr.worldheight / screen->h;
    }

  Draw2DGraphic(l, 1-b, r, 1-t, mat);
}

void OGLRenderer::Draw2DGraphicFill_Doom(GLfloat x, GLfloat y, GLfloat width, GLfloat height, Material *mat)
{
  //  CONS_Printf("w: %f, h: %f, texw: %f, texh: %f.\n", width, height, texwidth, texheight);
  //  CONS_Printf("xrepeat %.2f, yrepeat %.2f.\n", width/texwidth, height/texheight);
  // here we may ignore offsets (original Doom behavior)
  Material::TextureRef &tr = mat->tex[0];

  Draw2DGraphic(x/BASEVIDWIDTH, 1-(y+height)/BASEVIDHEIGHT,
		(x+width)/BASEVIDWIDTH, 1-y/BASEVIDHEIGHT,
		mat,
		0.0, height/tr.worldheight, width/tr.worldwidth, 0.0);
}


/// Currently a no-op. Possibly do something in the future.
void OGLRenderer::ClearAutomap()
{
}


/// Draws the specified map line to screen.
void OGLRenderer::DrawAutomapLine(const fline_t *line, const int color)
{
  if (!consolemode)
    I_Error("Trying to draw level map while in 3D mode.\n");

  // Set color.
  glColor3f(palette[color].r/255.0, palette[color].g/255.0, palette[color].b/255.0);

  // Do not use a texture.
  glBindTexture(GL_TEXTURE_2D, 0);

  glBegin(GL_LINES);
  glVertex2f(line->a.x/GLfloat(BASEVIDWIDTH), 
	     1.0-line->a.y/GLfloat(BASEVIDHEIGHT));
  glVertex2f(line->b.x/GLfloat(BASEVIDWIDTH),
	     1.0-line->b.y/GLfloat(BASEVIDHEIGHT));
  glEnd();

}

void OGLRenderer::RenderPlayerView(PlayerInfo *player)
{
  validcount++;

  // Set up the Map to be rendered. Needs to be done separately for each viewport, since the engine
  // can run several Maps at once.
  mp = player->mp;

  if (mp->glvertexes == NULL)
    I_Error("Trying to render level but level geometry is not set.\n");

  Setup3DMode();

  if (!mp->skybox_pov)
    // This simple sky rendering algorithm uses 2D mode. We should
    // probably do something more fancy in the future.
    DrawSimpleSky();
  else
    {
      // render skybox
      // TODO proper sequental rotation so we can do CTF_Face -style maps!
      phi = Degrees(player->pov->yaw + mp->skybox_pov->yaw);
      theta = Degrees(player->pov->pitch);

      Render3DView(mp->skybox_pov);
      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();

      glClear(GL_DEPTH_BUFFER_BIT);
      validcount++; // prepare to render same sectors again if necessary
    }

  // render main view
  phi = Degrees(player->pov->yaw);
  theta = Degrees(player->pov->pitch);
  Render3DView(player->pov);

  // render crosshair
  GLdouble chx, chy, chz; // Last one is a dummy.
  if (player->pawn)
    {
      GLfloat aimsine = 0.0;
      if(player->options.autoaim) {
	player->pawn->AimLineAttack(player->pawn->yaw, 3000, aimsine);
      } else {
	aimsine = Sin(player->pawn->pitch).Float();
      }
      player->pawn->LineTrace(player->pawn->yaw, 30000, aimsine, false);

      vec_t<fixed_t> target = trace.Point(trace.frac);

      //if (!(player->mp->maptic & 0xF))
      //  CONS_Printf("targ: %f, %f, %f, dist %f\n", target.x.Float(), target.y.Float(), target.z.Float(), trace.frac);

      GLdouble model[16];
      GLdouble proj[16];
      GLint vp[4];
      glGetDoublev(GL_MODELVIEW_MATRIX, model);
      glGetDoublev(GL_PROJECTION_MATRIX, proj);
      glGetIntegerv(GL_VIEWPORT, vp);
      gluProject(target.x.Float(), target.y.Float(), target.z.Float(),
		 model, proj, vp,
		 &chx, &chy, &chz);

      /*
      // Draw a red dot there. Used for testing.
      glDisable(GL_DEPTH_TEST);
      glDisable(GL_LIGHTING);
      glBindTexture(GL_TEXTURE_2D, 0);

      glColor3f(0.8, 0.0, 0.0);
      glBegin(GL_POINTS);
      glVertex3f(target.x.Float(), target.y.Float(), target.z.Float());
      glEnd();
      */
    }

  // Pretty soon we want to draw HUD graphics and stuff.
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  Setup2DMode();

  if (player->pawn && LocalPlayers[0].crosshair) // FIXME
    {
      extern Material *crosshair[];
      int c = LocalPlayers[0].crosshair & 3;
      Material *mat = crosshair[c-1];

      GLfloat top, left, bottom, right;

      left   = chx/viewportw - mat->leftoffs/BASEVIDWIDTH;
      right  = left + mat->worldwidth/BASEVIDWIDTH;
      top    = chy/viewporth + mat->topoffs/BASEVIDHEIGHT;
      bottom = top - mat->worldheight/BASEVIDHEIGHT;

      Draw2DGraphic(left, bottom, right, top, mat);
    }

  // Draw weapon sprites.
  bool drawPsprites = (player->pov == player->pawn);

  if (drawPsprites && cv_psprites.value)
    DrawPSprites(player->pawn);
}


/// Set up state and draw a view of the level from the given viewpoint.
/// It is usually the place where the player is currently located.
/// NOTE: Leaves a modelview matrix on the stack, which _must_ be popped by the caller!
void OGLRenderer::Render3DView(Actor *pov)
{
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  x = pov->pos.x.Float();
  y = pov->pos.y.Float();
  z = pov->GetViewZ().Float();

  // Set up camera to look through pov's eyes.
  glRotatef(-90.0 - theta,  1.0, 0.0, 0.0);
  glRotatef(90.0 - phi, 0.0, 0.0, 1.0);
  glTranslatef(-x, -y, -z);

  curssec = pov->subsector;

  // Build frustum and we are ready to render.
  CalculateFrustum();
  RenderBSPNode(mp->numnodes-1);
}




void OGLRenderer::DrawPSprites(PlayerPawn *p)
{
  // add all active psprites
  pspdef_t *psp = p->psprites;
  for (int i=0; i<NUMPSPRITES; i++, psp++)
    {
      if (!psp->state)
	continue; // not active

      // decide which patch to use
      sprite_t *sprdef = sprites.Get(spritenames[psp->state->sprite]);
      spriteframe_t *sprframe = &sprdef->spriteframes[psp->state->frame & TFF_FRAMEMASK];

#ifdef PARANOIA
      if (!sprframe)
	I_Error("sprframes NULL for state %d\n", psp->state - weaponstates);
#endif

      Material *mat = sprframe->tex[0];

      //added:02-02-98:spriteoffset should be abs coords for psprites, based on 320x200
      GLfloat tx = psp->sx.Float();// - t->leftoffs;
      GLfloat ty = psp->sy.Float();// - t->topoffs;

      // lots of TODOs for psprites
      /*
      if (sprframe->flip[0])

      if (viewplayer->flags & MF_SHADOW)      // invisibility effect
      {
	if (viewplayer->powers[pw_invisibility] > 4*TICRATE
	    || viewplayer->powers[pw_invisibility] & 8)
      }
      else if (fixedcolormap)
      else if (psp->state->frame & TFF_FULLBRIGHT)
      else
      // local light
      */
      // TODO extralight, extra_colormap, planelights
      Draw2DGraphic_Doom(tx, ty, mat, V_SCALE);
    }
}

// Calculates the 2D view frustum of the current player view. Call
// before rendering BSP.

void OGLRenderer::CalculateFrustum() {
  static const GLfloat fsize = 10000.0; // Depth of frustum, also a hack.

  // Build frustum points suitable for analysis.
  fr_cx = x;
  fr_cy = y;

  fr_lx = x + fsize*cos((phi+0.5*viewportar*fov)*(M_PI/180.0));
  fr_ly = y + fsize*sin((phi+0.5*viewportar*fov)*(M_PI/180.0));

  fr_rx = x + fsize*cos((phi-0.5*viewportar*fov)*(M_PI/180.0));
  fr_ry = y + fsize*sin((phi-0.5*viewportar*fov)*(M_PI/180.0));

}

// Checks BSP node/subtree bounding box.
// Returns true if some part of the bbox might be visible.
bool OGLRenderer::BBoxIntersectsFrustum(const bbox_t& bbox) const {
  // If we have intersections, bbox area is visible.
  if(bbox.LineCrossesEdge(fr_cx, fr_cy, fr_lx, fr_ly))
    return true;
  if(bbox.LineCrossesEdge(fr_cx, fr_cy, fr_rx, fr_ry))
    return true;
  if(bbox.LineCrossesEdge(fr_lx, fr_ly, fr_rx, fr_ry)) // Is this really necessary?
    return true;

  // At this point the bbox is either entirely outside or entirely
  // inside the frustum. Find out which.
  vertex_t v1, v2;
  line_t l;
  l.v1 = &v1;
  l.v2 = &v2;
  v1.x = fr_cx;
  v1.y = fr_cy;
  v2.x = fr_lx;
  v2.y = fr_ly;
  l.dx = fr_lx-fr_cx;
  l.dy = fr_ly-fr_cy;
 
  if(P_PointOnLineSide(bbox.box[BOXLEFT], bbox.box[BOXTOP], &l))
    return false;

  v2.x = fr_rx;
  v2.y = fr_ry;
  l.dx = fr_rx-fr_cx;
  l.dy = fr_ry-fr_cy;

  if(!P_PointOnLineSide(bbox.box[BOXLEFT], bbox.box[BOXTOP], &l))
    return false;

  return true;
}

/// Walk through the BSP tree and render the level in back-to-front
/// order. Assumes that the OpenGL state is properly set elsewhere.
void OGLRenderer::RenderBSPNode(int nodenum)
{
  // Found a subsector (leaf node)?
  if (nodenum & NF_SUBSECTOR)
    {
      int nt = nodenum & ~NF_SUBSECTOR;
      if(curssec == NULL || CheckVis(curssec-mp->subsectors, nt))
	 RenderGLSubsector(nt);
      return;
    }

  // otherwise keep traversing the tree
  node_t *node = &mp->nodes[nodenum];

  // Decide which side the view point is on.
  int side = node->PointOnSide(x, y);

  // OpenGL requires back-to-front drawing for translucency effects to
  // work properly. Thus we first check the back.
  if (BBoxIntersectsFrustum(node->bbox[side^1])) // sort of frustum culling
    RenderBSPNode(node->children[side^1]);

  // Now we draw the front side.
  RenderBSPNode(node->children[side]);
}

/// Draws one single GL subsector polygon that is either a floor or a ceiling. 
void OGLRenderer::RenderGlSsecPolygon(subsector_t *ss, GLfloat height, Material *mat, bool isFloor, GLfloat xoff, GLfloat yoff)
{
  int loopstart, loopend, loopinc;
  
  int firstseg = ss->first_seg;
  int segcount = ss->num_segs;

  // GL subsector polygons are clockwise when viewed from above.
  // OpenGL polygons are defined counterclockwise. Thus we need to go
  // "backwards" when drawing the floor.
  if(isFloor) {
    loopstart = firstseg+segcount-1;
    loopend = firstseg-1;
    loopinc = -1;
  } else {
    loopstart = firstseg;
    loopend = firstseg+segcount;
    loopinc = 1;
  }

  glNormal3f(0.0, 0.0, -loopinc);
  mat->GLUse();
  glBegin(GL_POLYGON);
  for (int curseg = loopstart; curseg != loopend; curseg += loopinc) {
    seg_t *seg = &mp->segs[curseg];
    GLfloat x, y, tx, ty;
    vertex_t *v;
    
    if(isFloor)
      v = seg->v2;
    else
      v = seg->v1;
    x = v->x.Float();
    y = v->y.Float();
    
    tx = (x+xoff)/mat->worldwidth;
    ty = 1.0 - (y-yoff)/mat->worldheight;

    glTexCoord2f(tx, ty);
    glVertex3f(x, y, height);
    
    //    printf("(%.2f, %.2f)\n", x, y);
  }
  glEnd();

#if 0
  {
    // draw subsector boundaries (for debugging)
    glDisable(GL_TEXTURE_2D);

    float c = 0.2+fabs(cos(mp->maptic / 35.0f));
    glColor4f(0.7, 0.7, 1.0, c);
    glBegin(GL_LINE_LOOP);
    for (int curseg = loopstart; curseg != loopend; curseg += loopinc)
      {
	vertex_t *v = mp->segs[curseg].v1;
	glVertex3f(v->x.Float(), v->y.Float(), height);
      }
    glEnd();

    glColor3f(0.8, 0.0, 0.0);
    glBegin(GL_POINTS);
    for (int curseg = loopstart; curseg != loopend; curseg += loopinc)
      {
	vertex_t *v = mp->segs[curseg].v1;
	glVertex3f(v->x.Float(), v->y.Float(), height);
      }
    glEnd();

    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0, 1.0, 1.0, 1.0);
  }
#endif
}

// Draw the floor and ceiling of a single GL subsector. The rendering
// order is flats->wall segs->items, so there is no occlusion. In the
// future render also 3D floors and polyobjs.
void OGLRenderer::RenderGLSubsector(int num)
{
  int curseg;

  ffloor_t *ff;
  Material *fmat;
  if (num < 0 || num > mp->numsubsectors)
    return;

  subsector_t *ss = &mp->subsectors[num];
  int firstseg = ss->first_seg;
  int segcount = ss->num_segs;
  sector_t *s = ss->sector;

  // Set up sector lighting.
  GLfloat light = LightLevelToLum(s->lightlevel) / 255.0;
  GLfloat lmodel_ambient[] = {light, light, light, 1.0};
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);


  // Draw ceiling texture, skip sky flats.
  fmat = s->ceilingpic;
  if (fmat && !s->SkyCeiling())
    RenderGlSsecPolygon(ss, s->ceilingheight.Float(), fmat, false, s->ceiling_xoffs.Float(), s->ceiling_yoffs.Float());
 
  // Then the floor.
  fmat = s->floorpic;
  if (fmat && !s->SkyFloor())
    RenderGlSsecPolygon(ss, s->floorheight.Float(), fmat, true, s->floor_xoffs.Float(), s->floor_yoffs.Float());

  // Draw the walls of this subsector.
  for (curseg=firstseg; curseg<firstseg+segcount; curseg++)
    RenderGLSeg(curseg);

  // A quick hack: draw 3D floors here. To do it Right, they (and
  // things in this subsector) need to be depth-sorted.
  for (ff = s->ffloors; ff; ff = ff->next) {

    GLfloat textop, texbottom, texleft, texright;
    fmat = *ff->toppic;
    RenderGlSsecPolygon(ss, ff->topheight->Float(), fmat, true);
    fmat = *ff->bottompic;
    RenderGlSsecPolygon(ss, ff->bottomheight->Float(), fmat, false);

    // Draw "edges" of 3D floor.

    // Sanity check.
    line_t *mline = ff->master;
    if (mline == NULL)
      continue;
    side_t *mside = mline->sideptr[0];
    if (mside == NULL)
      continue;
    fmat = mside->midtexture;
    if (fmat == NULL)
      continue;

    // Calculate texture offsets. (They are the same for every edge.)
    GLfloat fsheight = mside->sector->ceilingheight.Float() -mside->sector->floorheight.Float();
    if(mline->flags & ML_DONTPEGBOTTOM) {
      texbottom = mside->rowoffset.Float();
      textop = texbottom - fsheight;
    } else {
      textop = mside->rowoffset.Float();
      texbottom = textop + fsheight;
    }
    texleft = mside->textureoffset.Float();

    for (curseg=firstseg; curseg<firstseg+segcount; curseg++) {

      fixed_t nx, ny;
      seg_t *s = &(mp->segs[curseg]);
      texright = texleft + s->length;
      vertex_t *v1 = s->v1;
      vertex_t *v2 = s->v2;

      // Surface normal points to the opposite direction than if we
      // were drawing a wall, so swap endpoints (and texcoords!)
      DrawSingleQuad(fmat, v2, v1, 
		     mside->sector->floorheight.Float(), 
		     mside->sector->ceilingheight.Float(),
		     texright/fmat->worldwidth,
		     texleft/fmat->worldwidth,
		     textop/fmat->worldheight,
		     texbottom/fmat->worldheight);

    }


  }

  // finally the actors
  RenderActors(ss);
}



void OGLRenderer::RenderActors(subsector_t *ssec)
{
  sector_t *sec = ssec->sector;

  /*
  if (!sec->numlights)
    {
      if (sec->heightsec == -1)
        lightlevel = sec->lightlevel;

      int lightnum = (lightlevel >> LIGHTSEGSHIFT)+extralight;

      if (lightnum < 0)
        spritelights = scalelight[0];
      else if (lightnum >= LIGHTLEVELS)
        spritelights = scalelight[LIGHTLEVELS-1];
      else
        spritelights = scalelight[lightnum];
    }
  */

  // Handle all things in this subsector.
  for (Actor *thing = sec->thinglist; thing; thing = thing->snext)
    if (!(thing->flags2 & MF2_DONTDRAW) && thing->pres &&
            thing->subsector == ssec)
      {
        thing->pres->Draw(thing); // does both sprites and 3d models
      }
}

/// Calculates the necessary numbers to render upper, middle, and
/// lower wall segments. Any quad whose Material pointer is not NULL
/// after this function is to be rendered.

void OGLRenderer::GetSegQuads(int num, quad &u, quad &m, quad &l) const {
  side_t *lsd; // Local sidedef.
  side_t *rsd; // Remote sidedef.
  sector_t *ls; // Local sector.
  sector_t *rs; // Remote sector.

  GLfloat rs_floor, rs_ceil, ls_floor, ls_ceil; // Floor and ceiling heights.
  GLfloat textop, texbottom, texleft, texright;
  GLfloat utexhei, ltexhei;
  GLfloat ls_height, tex_yoff;
  Material *uppertex, *middletex, *lowertex;

  u.m = m.m = l.m = NULL;

  if(num < 0 || num > mp->numsegs)
    return;

  seg_t *s = &(mp->segs[num]);
  line_t *ld = s->linedef;

  // Minisegs don't have wall textures so no point in drawing them.
  if (ld == NULL)
    return;

  if (ld->v1->x.Float() == 704 && ld->v1->y.Float() == -3552)
    utexhei = 0;

  // Mark this linedef as visited so it gets drawn on the automap.
  ld->flags |= ML_MAPPED;

  u.v1 = m.v1 = l.v1 = s->v1;
  u.v2 = m.v2 = l.v2 = s->v2;

  if(s->side == 0) {
    lsd = ld->sideptr[0];
    rsd = ld->sideptr[1];
  } else {
    lsd = ld->sideptr[1];
    rsd = ld->sideptr[0];
  }

  // This should never happen, but we are paranoid.
  if(lsd == NULL)
    return;

  ls = lsd->sector;
  ls_floor = ls->floorheight.Float();
  ls_ceil = ls->ceilingheight.Float();
  ls_height = ls_ceil - ls_floor;

  if(rsd) {
    rs = rsd->sector;
    rs_floor = rs->floorheight.Float();
    rs_ceil = rs->ceilingheight.Float();
    utexhei = ls_ceil - rs_ceil;
    ltexhei = rs_floor - ls_floor;
  } else {
    rs = NULL;
    rs_floor = 0.0;
    rs_ceil = 0.0;
  }

  tex_yoff = lsd->rowoffset.Float();
  uppertex = lsd->toptexture;
  middletex = lsd->midtexture;
  lowertex = lsd->bottomtexture;

  // Texture X offsets.
  texleft = lsd->textureoffset.Float() + s->offset.Float();
  texright = texleft + s->length;

  // Ready to draw textures.
  if(rs != NULL) {
    // Upper texture.
    if(rs_ceil < ls_ceil && uppertex) {
      if(ld->flags & ML_DONTPEGTOP) {
	textop = tex_yoff;
	texbottom = textop + utexhei;
      } else {
	texbottom = tex_yoff;
	textop = texbottom - utexhei;
      }

      // If the front and back ceilings are sky, do not draw this
      // upper texture.
      if (!ls->SkyCeiling() || !rs->SkyCeiling()) {
	u.m = uppertex;
	u.bottom = rs_ceil;
	u.top = ls_ceil;
	u.t.l = texleft/uppertex->worldwidth;
	u.t.r = texright/uppertex->worldwidth;
	u.t.t = textop/uppertex->worldheight;
	u.t.b = texbottom/uppertex->worldheight;
      }
    }
    
    if(rs_floor > ls_floor && lowertex) {
      // Lower textures are a major PITA.
      if(ld->flags & ML_DONTPEGBOTTOM) {
	texbottom = tex_yoff + ls_height - lowertex->worldheight;
	textop = texbottom - ltexhei;
      } else {
	textop = tex_yoff;
	texbottom = textop + ltexhei;
      }
      l.m = lowertex;
      l.bottom = ls_floor;
      l.top = rs_floor;
      l.t.l = texleft/lowertex->worldwidth;
      l.t.r = texright/lowertex->worldwidth;
      l.t.t = textop/lowertex->worldheight;
      l.t.b = texbottom/lowertex->worldheight;
    }

    // Double sided middle textures do not repeat, so we need some
    // trickery.
    if(middletex != 0 && ls_floor < rs_ceil && ls_ceil > rs_floor) {
      GLfloat top, bottom;
      if(ld->flags & ML_DONTPEGBOTTOM) {
	if(ls_floor < rs_floor)
	  bottom = rs_floor;
	else
	  bottom = ls_floor;
	top = bottom + middletex->worldheight; // FIXME, should be at most
					  // the height of the remote
					  // sector?
      } else {
	if(ls_ceil < rs_ceil)
	  top = ls_ceil;
	else
	  top = rs_ceil;
	bottom = top - middletex->worldheight;
      }
      m.m = middletex;
      m.bottom = bottom;
      m.top = top;
      m.t.l = texleft/middletex->worldwidth;
      m.t.r = texright/middletex->worldwidth;
      m.t.t = 0.0;
      m.t.b = 1.0;
    }
  } else if(middletex) {
    // Single sided middle texture.
    if(ld->flags & ML_DONTPEGBOTTOM) {
      texbottom = tex_yoff;
      textop = texbottom - ls_height;
    } else {
      textop = tex_yoff;
      texbottom = textop + ls_height;
    }
    m.m = middletex;
    m.bottom = ls_floor;
    m.top = ls_ceil;
    m.t.l = texleft/middletex->worldwidth;
    m.t.r = texright/middletex->worldwidth;
    m.t.t = textop/middletex->worldheight;
    m.t.b = texbottom/middletex->worldheight;
  }
}


/// Renders one single GL seg. Minisegs and invalid parameters are
/// silently ignored.
void OGLRenderer::RenderGLSeg(int num)
{

  quad u, m, l;

  GetSegQuads(num, u, m, l);

  // For now, just render the quads. In the future, split each quad
  // along 3D floors and change light levels accordingly.
  if(u.m)
    DrawSingleQuad(&u);
  if(m.m)
    DrawSingleQuad(&m);
  if(l.m)
    DrawSingleQuad(&l);
}


void OGLRenderer::DrawSingleQuad(const quad *q) const {
  DrawSingleQuad(q->m, q->v1, q->v2, q->bottom, q->top, 
		 q->t.l, q->t.r, q->t.t, q->t.b);
}

/// Draw a single textured wall segment.
void OGLRenderer::DrawSingleQuad(Material *m, vertex_t *v1, vertex_t *v2, GLfloat lower, GLfloat upper, GLfloat texleft, GLfloat texright, GLfloat textop, GLfloat texbottom) const
{
  m->GLUse();

  // Calculate surface normamp-> Should we account for degenerate
  // linedefs?

  shader_attribs_t sa; // TEST
  sa.tangent[0] = (v2->x - v1->x).Float();
  sa.tangent[1] = (v2->y - v1->y).Float();
  sa.tangent[2] = 0;
    
  glNormal3f(sa.tangent[1], -sa.tangent[0], 0.0);

  if (m->shader)
    {
      m->shader->SetAttributes(&sa);
    }

  glBegin(GL_QUADS);
  
  glTexCoord2f(texleft, texbottom);
  glVertex3f(v1->x.Float(), v1->y.Float(), lower);
  glTexCoord2f(texright, texbottom);
  glVertex3f(v2->x.Float(), v2->y.Float(), lower);
  glTexCoord2f(texright, textop);
  glVertex3f(v2->x.Float(), v2->y.Float(), upper);
  glTexCoord2f(texleft, textop);
  glVertex3f(v1->x.Float(), v1->y.Float(), upper);

  glEnd();
}

// Draws the specified item (weapon, monster, flying rocket etc) in
// the 3D view. Translations and rotations are done with OpenGL
// matrices.

void OGLRenderer::DrawSpriteItem(const vec_t<fixed_t>& pos, Material *mat, int flags, GLfloat alpha)
{
  // You can't draw the invisible.
  if (!mat)
    return;

  if (alpha < 1.0)
    {
      glColor4f(1.0, 1.0, 1.0, alpha); // set material params
      //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); // on by default
    }

  GLfloat top, bottom, left, right;
  GLfloat texleft, texright, textop, texbottom;

  if (flags & FLIP_X) {
    texleft = 1.0;
    texright = 0.0;
  } else {
    texleft = 0.0;
    texright = 1.0;
  }

  // Shouldn't these be the other way around?
  texbottom = 1.0;
  textop = 0.0;

  left = mat->leftoffs;
  right = left - mat->worldwidth;
 
  /* HACK, too high:
   * Marty: Setzt Sprites bündig auf Boden – funktioniert bei modernen Treibern besser als korrekte Variante
	 */
  top = mat->worldheight;
	bottom = 0; // HACK, too high
	/*
   top = mat->topoffs; // this is correct but causes the sprite to penetrate the floor, hence the depth test trick
   bottom = top - mat->worldheight;
	*/


  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  glTranslatef(pos.x.Float(), pos.y.Float(), pos.z.Float());
  glRotatef(phi, 0.0, 0.0, 1.0);
  glNormal3f(-1.0, 0.0, 0.0);

  mat->GLUse();

  // TEST FIXME
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBegin(GL_QUADS);
  glTexCoord2f(texleft, texbottom);
  glVertex3f(0.0, left, bottom);
  glTexCoord2f(texright, texbottom);
  glVertex3f(0.0, right, bottom);
  glTexCoord2f(texright, textop);
  glVertex3f(0.0, right, top);
  glTexCoord2f(texleft, textop);
  glVertex3f(0.0, left, top);
  glEnd();

  /* The world famous black triangles look.
  glBindTexture(GL_TEXTURE_2D, 0);
  glColor4f(0.0, 0.0, 0.0, 1.0);
  glBegin(GL_TRIANGLES);
  glVertex3f(0.0, 0.0, 128.0);
  glVertex3f(0.0, 32.0, 0.0);
  glVertex3f(0.0, -32.0, 0.0);
  glEnd();
  */

  // Leave the matrix stack as it was.
  glPopMatrix();
  
  glColor4f(1.0, 1.0, 1.0, 1.0); // back to normal material params
	
	
}


/// Check for visibility between the given glsubsectors. Returns true
/// if you can see from one to the other and false if not.
bool OGLRenderer::CheckVis(int fromss, int toss)
{
  byte *vis;
  if(mp->glvis == NULL)
    return true;

  vis = mp->glvis + (((mp->numsubsectors + 7) / 8) * fromss);
  if (vis[toss >> 3] & (1 << (toss & 7)))
    return true;
  return false;
}


/// Draws the background sky texture. Very simple, ignores looking up/down.
void OGLRenderer::DrawSimpleSky()
{
  GLfloat left, right, top, bottom, texleft, texright, textop, texbottom;

  GLboolean isdepth = 0;
  glGetBooleanv(GL_DEPTH_TEST, &isdepth);
  if(isdepth)
    glDisable(GL_DEPTH_TEST);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0.0, 1.0, 0.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  textop = 0.0;   // And yet again these should be the other way around.
  texbottom = 1.0;
  top = 1.0;
  bottom = 0.0;
  left = 0.0;
  right = 1.0;

  GLfloat fovx = fov*viewportar;
  fovx *= 2.0; // I just love magic numbers. Don't you?

  texleft = (2.0*phi + fovx)/720.0;
  texright = (2.0*phi - fovx)/720.0;

  mp->skytexture->GLUse();
  glBegin(GL_QUADS);
  glTexCoord2f(texleft, texbottom);
  glVertex2f(left, bottom);
  glTexCoord2f(texright, texbottom);
  glVertex2f(right, bottom);
  glTexCoord2f(texright, textop);
  glVertex2f(right, top);
  glTexCoord2f(texleft, textop);
  glVertex2f(left, top);
  glEnd();

  if(isdepth)
    glEnable(GL_DEPTH_TEST);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
}
