// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: oglhelpers.cpp 515 2007-12-22 14:51:46Z jussip $
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
/// Utility functions for OpenGL renderer.

#include "command.h"
#include "r_data.h"
#include "doomdef.h"
#include "hardware/oglhelpers.hpp"
#include "i_video.h"
#include "m_bbox.h"
#include "p_maputl.h"
#include "r_defs.h"

static const GLubyte *gl_extensions = NULL;

static byte lightleveltonumlut[256];

// This array has the fractional screen coordinates necessary to render multiple player
// views onto the screen. The first index tells how many total viewports-1
// there are on this screen. The second one tells the viewport number
// and must be less or equal to the first one.
viewportdef_t gl_viewports[MAX_GLVIEWPORTS][MAX_GLVIEWPORTS] = {
  // 1 Player
  {{0.0, 0.0, 1.0, 1.0}},

  // 2 players
  {{0.0, 0.5, 1.0, 0.5},
   {0.0, 0.0, 1.0, 0.5}},

  // 3 players
  {{0.0, 0.5, 1.0, 0.5},
   {0.0, 0.0, 0.5, 0.5},
   {0.5, 0.0, 0.5, 0.5}},

  // 4 players
  {{0.0, 0.5, 0.5, 0.5},
   {0.5, 0.5, 0.5, 0.5},
   {0.0, 0.0, 0.5, 0.5},
   {0.5, 0.0, 0.5, 0.5}}
};



// Only list mipmapping filter modes, since we always use them.
CV_PossibleValue_t grfiltermode_cons_t[]= {{0, "NN"}, {1, "LN"}, {2, "NL"}, {3, "LL"}, {0, NULL} };
consvar_t cv_grfiltermode = {"gr_filtermode", "NN", CV_SAVE, grfiltermode_cons_t, NULL};

CV_PossibleValue_t granisotropy_cons_t[]= {{1,"MIN"}, {16,"MAX"}, {0,NULL}};
consvar_t cv_granisotropy = {"gr_anisotropy", "1", CV_SAVE, granisotropy_cons_t, NULL};

consvar_t cv_grnearclippingplane = {"gr_nearclippingplane",   "0.9", CV_SAVE|CV_FLOAT, NULL};
consvar_t cv_grfarclippingplane  = {"gr_farclippingplane", "9000.0", CV_SAVE|CV_FLOAT, NULL};

consvar_t cv_grdynamiclighting = {"gr_dynamiclighting", "On", CV_SAVE, CV_OnOff};
consvar_t cv_grcoronas         = {"gr_coronas",         "On", CV_SAVE, CV_OnOff};
consvar_t cv_grcoronasize      = {"gr_coronasize",       "1", CV_SAVE|CV_FLOAT, NULL};

static void CV_Gamma_OnChange();
static CV_PossibleValue_t gamma_cons_t[]= {{1,"MIN"}, {20,"MAX"}, {0,NULL}};
consvar_t cv_grgammared   = {"gr_gammared",   "10", CV_SAVE|CV_CALL, gamma_cons_t, CV_Gamma_OnChange};
consvar_t cv_grgammagreen = {"gr_gammagreen", "10", CV_SAVE|CV_CALL, gamma_cons_t, CV_Gamma_OnChange};
consvar_t cv_grgammablue  = {"gr_gammablue",  "10", CV_SAVE|CV_CALL, gamma_cons_t, CV_Gamma_OnChange};

static void CV_Gamma_OnChange()
{
  I_SetGamma(cv_grgammared.value/10.0f, cv_grgammagreen.value/10.0f, cv_grgammablue.value/10.0f);
}


consvar_t cv_grfog        = {"gr_fog",            "On", CV_SAVE, CV_OnOff};
consvar_t cv_grfogcolor   = {"gr_fogcolor",   "000000", CV_SAVE, NULL};
consvar_t cv_grfogdensity = {"gr_fogdensity",    "100", CV_SAVE, CV_Unsigned};


void OGL_AddCommands()
{
  cv_grgammared.Reg();
  cv_grgammagreen.Reg();
  cv_grgammablue.Reg();

  cv_grfiltermode.Reg();
  cv_granisotropy.Reg();

  cv_grnearclippingplane.Reg();
  cv_grfarclippingplane.Reg();

  cv_grdynamiclighting.Reg();
  cv_grcoronas.Reg();
  cv_grcoronasize.Reg();


  cv_grfog.Reg();
  cv_grfogdensity.Reg();
  cv_grfogcolor.Reg();
}



// Converts Doom sector light values to suitable background pixel
// color. extralight is for temporary brightening of the screen due to
// muzzle flashes etc.
byte LightLevelToLum(int l, int extralight)
{
  /* 
  if (fixedcolormap)
    return 255;
  */
  l = lightleveltonumlut[l];
  l += (extralight << 4);
  if (l > 255)
    l = 255;
  return l;
}



// Hurdler's magical mystery mapping function initializer.
void InitLumLut()
{
  for (int i = 0; i < 256; i++)
    {
      // this polygone is the solution of equ : f(0)=0, f(1)=1
      // f(.5)=.5, f'(0)=0, f'(1)=0), f'(.5)=K
#define K   2
#define A  (-24+16*K)
#define B  ( 60-40*K)
#define C  (32*K-50)
#define D  (-8*K+15)
      float x = (float) i / 255;
      float xx, xxx;
      xx = x * x;
      xxx = x * xx;
      float k = 255 * (A * xx * xxx + B * xx * xx + C * xxx + D * xx);
      lightleveltonumlut[i] = 255 < k ? 255 : int(k); //min(255, k);
    }
}

/// Tells whether the spesified extension is supported by the current
/// OpenGL implementation.

bool GLExtAvailable(const char *extension)
{
    const GLubyte *start;
    GLubyte *where, *terminator;

    where = (GLubyte *) strchr(extension, ' ');
    if (where || *extension == '\0')
        return false;

    if(!gl_extensions)
      gl_extensions = glGetString(GL_EXTENSIONS);

    start = gl_extensions;
    for (;;)
    {
        where = (GLubyte *) strstr((const char *) start, extension);
        if (!where)
            break;
        terminator = where + strlen(extension);
        if (where == start || *(where - 1) == ' ')
            if (*terminator == ' ' || *terminator == '\0')
                return true;
        start = terminator;
    }
    return false;
}

// This function runs some unit and sanity tests to the plane geometry
// & other routines.

void GeometryUnitTests()
{
  CONS_Printf("Starting geometry unit tests.\n");

  bbox_t bb;
  bb.Set(0.5, 0.5, 0.5);

  divline_t l;
  l.x = 0.5;
  l.y = -0.5;
  l.dx = 1.0;
  l.dy = 1.5;

  // Point on line side.
  if(l.PointOnSide(0, 0) ==
     l.PointOnSide(1, 0))
    CONS_Printf("Point on line side test #1 failed.\n");

  if(l.PointOnSide(1, 0) ==
     l.PointOnSide(1, 1))
    CONS_Printf("Point on line side test #2 failed.\n");

  // Inspect line crossings.
  /*
  if(!P_LinesegsCross(0, 0, 1, 1, 0, 1, 1, 0))
    CONS_Printf("Crossing test #1 failed.\n");

  if(!P_LinesegsCross(0, 0, 0, 1, -0.5, 0.5, 0.5, 0.5))
    CONS_Printf("Crossing test #2 failed.\n");

  if(P_LinesegsCross(0, 0, 1, 1, 2, 0, 2, 1))
    CONS_Printf("Crossing test #3 failed.\n");

  if(!P_LinesegsCross(0, 0, 1, 0, 0.5, -0.5, 1.5, 1))
    CONS_Printf("Crossing test #4 failed.\n");

  if(!P_LinesegsCross(1, 0, 1, 1, 0.5, -0.5, 1.5, 1))
    CONS_Printf("Crossing test #5 failed.\n");
  */

  // Bounding boxes.
  if(!bb.LineCrossesEdge(-2, 0, 1000, 0.5))
    CONS_Printf("Bounding box test #1 failed.\n");

  if(!bb.LineCrossesEdge(-1, 0.5, 10, 0.5))
    CONS_Printf("Bounding box test #2 failed.\n");

  if(!bb.LineCrossesEdge(0.5, -1, 0.5, 100))
    CONS_Printf("Bounding box test #3 failed.\n");

  if(bb.LineCrossesEdge(0.2, 0.2, 0.7, 0.7))
    CONS_Printf("Bounding box test #4 failed.\n");

  if(!bb.LineCrossesEdge(0.5, -0.5, 1.5, 1))
    CONS_Printf("Bounding box test #5 failed.\n");

  CONS_Printf("Geometry unit tests finished.\n");
}
