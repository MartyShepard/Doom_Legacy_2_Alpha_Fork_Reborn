// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: hwr_render.cpp 514 2007-12-21 16:07:36Z smite-meister $
//
// Copyright (C) 1998-2004 by DooM Legacy Team.
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
//
//
//-----------------------------------------------------------------------------

/// \file
/// \brief New hardware renderer, using the standard HardWareRender
/// driver DLL for Doom Legacy

#include <string.h>
#include "SDL.h"

#include "doomdef.h"
#include "command.h"
#include "cvars.h"

#include "g_map.h"
#include "g_actor.h"

#include "hardware/hwr_states.h"
#include "hardware/hwr_geometry.h"
#include "hardware/hwr_render.h"
#include "hardware/hwr_bsp.h"

#include "tables.h"
#include "m_bbox.h"

#include "r_bsp.h"
#include "r_data.h"
#include "r_main.h"
#include "r_draw.h"
#include "v_video.h"

#include "hardware/md3.h"

#ifndef NO_OPENGL

void OglSdlFinishUpdate(bool vidwait);

void show_stackframe();

const float fixedtofloat = (1.0f / 65536.0f);
static const float ORIGINAL_ASPECT = (320.0f/200.0f);
static void CV_filtermode_ONChange();
static void CV_FogDensity_ONChange();
static void CV_grFogColor_OnChange();
static void CV_grFov_OnChange();
static void CV_grPolygonSmooth_OnChange();
static void CV_grMonsterDL_OnChange();
static void CV_Gammaxxx_ONChange();
static void Command_GrStats_f();

CV_PossibleValue_t grcrappymlook_cons_t[]= {{0,"Off"}, {1,"On"},{2,"Full"}, {0,NULL} };
CV_PossibleValue_t grfov_cons_t[]= {{0,"MIN"}, {179,"MAX"}, {0,NULL} };


consvar_t cv_grrounddown       = {"gr_rounddown",       "Off", 0,       CV_OnOff };
consvar_t cv_grcrappymlook     = {"gr_mlook",          "Full", CV_SAVE, grcrappymlook_cons_t };

consvar_t cv_grsky             = {"gr_sky",              "On", 0,       CV_OnOff };

consvar_t cv_grzbuffer         = {"gr_zbuffer",          "On", 0,       CV_OnOff };
consvar_t cv_grcorrecttricks   = {"gr_correcttricks",    "On", 0,       CV_OnOff };
consvar_t cv_grsolvetjoin      = {"gr_solvetjoin",       "On", 0,       CV_OnOff };

// console variables in development
consvar_t cv_grpolygonsmooth   = {"gr_polygonsmooth",   "Off", CV_CALL, CV_OnOff, CV_grPolygonSmooth_OnChange };
consvar_t cv_grmd2             = {"gr_md2",             "Off", 0,       CV_OnOff };
consvar_t cv_grtranswall       = {"gr_transwall",       "Off", 0,       CV_OnOff };

// faB : needs fix : walls are incorrectly clipped one column less
const consvar_t cv_grclipwalls = {"gr_clipwalls",       "Off", 0,       CV_OnOff };

//development variables for diverse uses
consvar_t cv_gralpha = {"gr_alpha", "160", 0, CV_Unsigned };
consvar_t cv_grbeta  = {"gr_beta",  "0",   0, CV_Unsigned };
consvar_t cv_grgamma = {"gr_gamma", "0",   0, CV_Unsigned };


consvar_t cv_grstaticlighting  = {"gr_staticlighting",   "On", CV_SAVE, CV_OnOff };
consvar_t cv_grmblighting      = {"gr_mblighting",       "On", CV_SAVE|CV_CALL,   CV_OnOff, CV_grMonsterDL_OnChange };

//Hurdler: Transform (coords + angles)
//BP: transform order : scale(rotation_x(rotation_y(translation(v))))
struct FTransform
{
  float x, y, z;                 // position if the viewer
  float anglex, angley;          // aimingangle / viewangle
  float scalex, scaley, scalez;
  float fovxangle, fovyangle;
  int   splitscreen;
};


//=============================================================================

HWRend::HWRend() :
  bsp(0)
{
}

HWRend::~HWRend()
{
  if (bsp)  // check for buggy compilers (TODO: remove this test if it's not necessary on all supported plateform)
    {
      delete bsp;
    }
}

static void SetTransform(FTransform *transform)
{
  static int special_splitscreen;
  float aspect_ratio = 1.0f; // (320.0f / 200.0f)

  //CONS_Printf("SetTransform(): (%f %f) (%f %f %f)\n", transform->anglex, transform->angley, transform->x, transform->y, transform->z);

  glLoadIdentity();
  if (transform)
    {
      glScalef(transform->scalex, transform->scaley, -transform->scalez);
      glRotatef(transform->anglex,        1.0, 0.0, 0.0);
      glRotatef(transform->angley+270.0f, 0.0, 1.0, 0.0);
      glTranslatef(-transform->x, -transform->z, -transform->y);

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      special_splitscreen = (transform->splitscreen && transform->fovxangle==90.0f);
      if (special_splitscreen)
        {
          gluPerspective( 53.13, 2.0f*aspect_ratio, cv_grnearclippingplane.value * fixedtofloat, cv_grfarclippingplane.value * fixedtofloat);  // 53.13 = 2*atan(0.5)
        }
      else
        {
          gluPerspective(transform->fovxangle, aspect_ratio, cv_grnearclippingplane.value * fixedtofloat, cv_grfarclippingplane.value * fixedtofloat);
        }
      //glGetDoublev(GL_PROJECTION_MATRIX, projMatrix); // added for new coronas' code (without depth buffer)
      glMatrixMode(GL_MODELVIEW);
    }
  else
    {
      glScalef(1.0, 1.0f, -1.0f);

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      if (special_splitscreen)
        {
          gluPerspective( 53.13, 2.0f*aspect_ratio, cv_grnearclippingplane.value * fixedtofloat, cv_grfarclippingplane.value * fixedtofloat);  // 53.13 = 2*atan(0.5)
        }
      else
        {
          gluPerspective(90.0f, aspect_ratio, cv_grnearclippingplane.value * fixedtofloat, cv_grfarclippingplane.value * fixedtofloat);
        }
      //glGetDoublev(GL_PROJECTION_MATRIX, projMatrix); // added for new coronas' code (without depth buffer)
      glMatrixMode(GL_MODELVIEW);
    }
  //glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix); // added for new coronas' code (without depth buffer)
}

void HWRend::RenderPlayerView(int viewnumber, PlayerInfo *player)
{
  glClearColor(0.5f, 1.0f, 1.0f, 0.5f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LEQUAL);

  R.R_SetupFrame(player);

  FTransform atransform;
  atransform.anglex = (float)(R.aimingangle>>ANGLETOFINESHIFT)*(360.0f/(float)FINEANGLES);
  atransform.angley = (float)(R.viewangle>>ANGLETOFINESHIFT)*(360.0f/(float)FINEANGLES);
  atransform.x      = R.viewx.Float();
  atransform.y      = R.viewy.Float();
  atransform.z      = R.viewz.Float();
  atransform.scalex = 1;
  atransform.scaley = ORIGINAL_ASPECT;
  atransform.scalez = 1;
  atransform.fovxangle = cv_grfov.value;
  atransform.fovyangle = cv_grfov.value;
  atransform.splitscreen = cv_splitscreen.value;
  SetTransform(&atransform);

  //R.HWR_RenderPlayerView(viewnumber, player);
  bsp->Render(R.numnodes - 1);
  if (cv_grcrappymlook.value && (R.aimingangle || cv_grfov.value>90))
    {
      R.viewangle += ANG90;
      bsp->Render(R.numnodes - 1);     // left view
      R.viewangle += ANG90;
      if (cv_grcrappymlook.value == 2 && ((int)R.aimingangle>ANG45 || (int)R.aimingangle<-ANG45))
        {
          bsp->Render(R.numnodes - 1); // back view
        }
      R.viewangle += ANG90;
      bsp->Render(R.numnodes - 1);     // right view
      R.viewangle += ANG90;
    }
  SetTransform(0);
}

void HWRend::DrawViewBorder()
{
  int x, y;
  int top, side;
  int baseviewwidth, baseviewheight;
  int basewindowx, basewindowy;

  int clearlines = BASEVIDHEIGHT; //refresh all

  // calc view size based on original game resolution
  baseviewwidth  = (int)(viewwidth/vid.fdupx);
  baseviewheight = (int)(viewheight/vid.fdupy);
  top  = (int)(viewwindowy/vid.fdupy);
  side = (int)(viewwindowx/vid.fdupx);

  // top
  DrawFill(0, 0, BASEVIDWIDTH, (top<clearlines ? top : clearlines), window_background);
  // left
  if (top<clearlines)
    DrawFill(0, top, side, (clearlines-top < baseviewheight ? clearlines-top : baseviewheight), window_background);
  // right
  if (top<clearlines)
    DrawFill(side + baseviewwidth, top, side, (clearlines-top < baseviewheight ? clearlines-top : baseviewheight), window_background);
  // bottom
  if (top+baseviewheight<clearlines)
    DrawFill(0, top+baseviewheight, BASEVIDWIDTH, BASEVIDHEIGHT, window_background);

  // draw the view borders
  basewindowx = (BASEVIDWIDTH - baseviewwidth)>>1;
  if (baseviewwidth==BASEVIDWIDTH)
    basewindowy = 0;
  else
    basewindowy = top;

  // top edge
  if (clearlines > basewindowy-8)
    for (x=0 ; x<baseviewwidth; x+=8)
      window_border[BRDR_T]->Draw(basewindowx+x,basewindowy-8,0);
  // bottom edge
  if (clearlines > basewindowy+baseviewheight)
    for (x=0 ; x<baseviewwidth ; x+=8)
      window_border[BRDR_B]->Draw(basewindowx+x,basewindowy+baseviewheight,0);
  // left edge
  if (clearlines > basewindowy)
    for (y=0 ; y<baseviewheight && (basewindowy+y < clearlines); y+=8)
      window_border[BRDR_L]->Draw(basewindowx-8,basewindowy+y,0);
  // right edge
  if (clearlines > basewindowy)
    for (y=0 ; y<baseviewheight && (basewindowy+y < clearlines); y+=8)
      window_border[BRDR_R]->Draw(basewindowx+baseviewwidth,basewindowy+y,0);

  // Draw beveled corners.
  if (clearlines > basewindowy-8)
    window_border[BRDR_TL]->Draw(basewindowx-8,basewindowy-8,0);
  if (clearlines > basewindowy-8)
    window_border[BRDR_TR]->Draw(basewindowx+baseviewwidth, basewindowy-8,0);
  if (clearlines > basewindowy+baseviewheight)
    window_border[BRDR_BL]->Draw(basewindowx-8, basewindowy+baseviewheight,0);
  if (clearlines > basewindowy+baseviewheight)
    window_border[BRDR_BR]->Draw(basewindowx+baseviewwidth, basewindowy+baseviewheight,0);
}

int HWRend::GetTextureUsed()
{
  show_stackframe();
  return 0;
}

// --------------------------------------------------------------------------
// Add hardware engine commands & consvars
// --------------------------------------------------------------------------
//added by Hurdler: console varibale that are saved
void HWRend::AddCommands()
{
  cv_grgammablue.Reg();
  cv_grgammagreen.Reg();
  cv_grgammared.Reg();
  // cv_grcontrast.Reg();
  // cv_grpolygonsmooth.Reg(); // moved below
  cv_grmd2.Reg();
  cv_grtranswall.Reg();
  cv_grmblighting.Reg();
  cv_grstaticlighting.Reg();
  cv_grdynamiclighting.Reg();
  cv_grcoronas.Reg();
  cv_grcoronasize.Reg();
  cv_grfov.Reg();
  cv_granisotropy.Reg();
  cv_grfogdensity.Reg();
  cv_grfogcolor.Reg();
  cv_grfog.Reg();
  cv_grcrappymlook.Reg();
  cv_grfiltermode.Reg();
  cv_grcorrecttricks.Reg();
  cv_grsolvetjoin.Reg();
  cv_grnearclippingplane.Reg();
  cv_grfarclippingplane.Reg();
}

void HWRend::ClearAutomap()
{
  show_stackframe();
  //R.HWR_ClearAutomap();
}

bool HWRend::Screenshot(char *lbmname)
{
  show_stackframe();
  //return R.HWR_Screenshot(lbmname);
  return true;
}

void HWRend::SetPalette(RGB_t *palette)
{
  show_stackframe();
  //R.HWR_SetPalette(palette);
}

void HWRend::FadeScreenMenuBack(unsigned long color, int height)
{
  show_stackframe();
  //R.HWR_FadeScreenMenuBack(color, height);
}

void HWRend::DrawFill(int x, int y, int w, int h, int color)
{
  show_stackframe();
  //R.HWR_DrawFill(x, y, w, h, color);
}

void HWRend::DrawFill(int x, int y, int w, int h, class Material* t)
{
  show_stackframe();
  //R.HWR_DrawFlatFill(x, y, w, h, t);
}

void HWRend::SetViewSize(int blocks)
{
  show_stackframe();
  //R.HWR_SetViewSize(blocks);
}

void HWRend::Setup(int bspnum)
{
  // Correct missing sidedefs & deep water trick
  // This is independent of the renderer code (it just rearrange the software structures)
  R.HWR_CorrectSWTricks();

  // TODO: reset lights, create static lightmaps, prepare texture cache,...

  if (bsp)  // check for buggy compilers (TODO: remove this test if it's not necessary on all supported plateform)
    {
      delete bsp;
    }
  bsp = new HWBsp(R.numsubsectors, bspnum);  // initialize the data

  //TODO: see if we need a kind of HWR_PrepLevelCache

  //TODO: compute lightmaps and other cool effects here
}

void HWRend::Startup()
{
  float aspect_ratio = 1.0f; // (320.0f / 200.0f)
#if 0
  HWD.pfnInitVidMode(width, height, bpp);
#else

  glViewport(0, 0, vid.width, vid.height);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(cv_grfov.value, aspect_ratio, cv_grnearclippingplane.value * fixedtofloat, cv_grfarclippingplane.value * fixedtofloat);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glGetIntegerv(GL_VIEWPORT, viewport);
  glGetFloatv(GL_PROJECTION_MATRIX, projection_matrix);
#endif

  // Now, init structures
  CONS_Printf ("HWRend::Startup()\n");
  static bool startupdone = false;
  if (startupdone == false)
    {
      cv_grpolygonsmooth.Reg();

      // engine state variables
      //cv_grsky.Reg();
      //cv_grzbuffer.Reg();
      //cv_grclipwalls.Reg();
      cv_grrounddown.Reg();

      // engine development mode variables
      // - usage may vary from version to version..
      cv_gralpha.Reg();
      cv_grbeta.Reg();
      cv_grgamma.Reg();

      // engine commands
      COM_AddCommand ("gr_stats", Command_GrStats_f);
      startupdone = true;
    }

  glClearColor(1.0f, 0.5f, 1.0f, 0.5f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  OglSdlFinishUpdate(false);
  glClearColor(1.0f, 0.5f, 1.0f, 0.5f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  OglSdlFinishUpdate(false);
}

void HWRend::DrawAMline(fline_t* fl, int color)
{
  //CONS_Printf("HWR_drawAMline: Not yet implemented\n");
}

#include <stdio.h>
#include <signal.h>
//#include <execinfo.h>

// Hurdler: helper function to show who called who (it's probably not portable and do not work
//          the same with all GCC versions, so it can be safely commented out if necessary)
void show_stackframe()
{
#if 0
  int i;
  void *trace[16];

  int trace_size = backtrace(trace, 16);
  char **messages = backtrace_symbols(trace, trace_size);
  // now format to a more human readable format
  char **format_messages = new (char*)[trace_size];
  for (i=0; i<trace_size; ++i) // i=1 -> do not show "show_stackframe()" itself
    {
      char *pos = strstr(messages[i], "(");
      if (pos) // OK
        {
          pos++;
          if (*pos == '_') // we have a function which is probably not the entry point
            {
              pos++;
              if (*pos == 'Z') // it's a function which is probably written by us (not a glibc one for instance)
                {
                  format_messages[i] = new char[256];
                  format_messages[i][0] = '\0';
                  pos++;
                  char *sep = "";
                  while (*pos && (*pos != '+'))
                    {
                      if (*pos == 'N') // it seems to be a class function
                        {
                          sep = "::";
                        }
                      else if (*pos == 'E') // it seems to be the end of the function name of a class function
                        {
                          strcat(format_messages[i], "()");
                          break;
                        }
                      else if (*pos == 'P') // it seems to the beginning of the function parameters
                        {
                          strcat(format_messages[i], "()");
                          break;
                        }
                      else if (*pos == 'v') // it seems to be the end of the function name
                        {
                          strcat(format_messages[i], "()");
                          break;
                        }
                      else if ((*pos >= '0') && (*pos <= '9'))
                        {
                          int size = 0;
                          while ((*pos >= '0') && (*pos <= '9'))
                            {
                              size *= 10;
                              size += (*pos) - '0';
                              pos++;
                            }
                          strcat(format_messages[i], sep);
                          strncat(format_messages[i], pos, size);
                          pos += size - 1;
                        }
                      else
                        {
                          sep = "";
                        }
                      pos++;
                    }
                }
              else // probably a glibc function (or at least a function not written by us)
                {
                  format_messages[i] = strdup(pos-2);
                }
            }
          else // probably the entry point (main())
            {
              format_messages[i] = strdup(pos-1);
            }
        }
      else // abnomal problem (probably the name of the executable on kernel stack)
        {
          format_messages[i] = strdup(messages[i]);
        }
    }
  unsigned int t = SDL_GetTicks();
  printf("*** Execution path: %02d:%02d:%02d\n", t / 60000, (t / 1000) % 60, (t / 10) % 60);
  for (i=1; i<trace_size; ++i) // i=1 -> do not show "show_stackframe()" itself
    {
      printf("    %s\n", format_messages[i]);
    }
  free(messages);
#endif
}

#if 0
#define __USE_GNU
#include <ucontext.h>
void bt_sighandler(int sig, siginfo_t *info, void *secret)
{
  void *trace[16];
  ucontext_t *uc = (ucontext_t *)secret;
  if (sig == SIGSEGV)
    printf("Segfault: address is %p, from %p\n", info->si_addr, uc->uc_mcontext.gregs[REG_EIP]);
  int trace_size = backtrace(trace, 16);
  trace[1] = (void *) uc->uc_mcontext.gregs[REG_EIP];
  char **messages = backtrace_symbols(trace, trace_size);
  for (int i=1; i<trace_size; ++i)
    printf("%s\n", messages[i]);
  exit(0);
}
#endif


//====================== OLD RENDERER THINGS TO REMOVE ========================
// Those are there only so we can compile and link the whole thing


// BP: change directely the palette to see the change
static void CV_Gammaxxx_ONChange()
{
#ifdef SDL
  extern void OglSdlSetGamma(float, float, float);
  // Hurdler: I add 1 and divide by 128 for backward compatibility (range is ]0, 2])
  OglSdlSetGamma((cv_grgammared.value + 1.0f) / 128.0f, (cv_grgammagreen.value + 1.0f) / 128.0f, (cv_grgammablue.value + 1.0f) / 128.0f);
#endif
}
static void CV_grFov_OnChange ()
{
  // autoset mlook when FOV > 90
  if ((!cv_grcrappymlook.value) && (cv_grfov.value > 90))
    cv_grcrappymlook.Set(1);
}
static void CV_grMonsterDL_OnChange ()
{
#if 0
    if (cv_grmblighting.value)
    {
        t_lspr[SPR_BAL1] = &lspr[REDBALL_L];
        t_lspr[SPR_BAL2] = &lspr[REDBALL_L];
        t_lspr[SPR_MANF] = &lspr[ROCKET2_L];
        t_lspr[SPR_BAL7] = &lspr[GREENBALL_L];
        t_lspr[SPR_APLS] = &lspr[GREENBALL_L];
        t_lspr[SPR_APBX] = &lspr[GREENBALL_L];
        t_lspr[SPR_SKUL] = &lspr[REDBALL_L];
        t_lspr[SPR_FATB] = &lspr[REDBALL_L];
    }
    else
    {
        t_lspr[SPR_BAL1] = &lspr[NOLIGHT];
        t_lspr[SPR_BAL2] = &lspr[NOLIGHT];
        t_lspr[SPR_MANF] = &lspr[NOLIGHT];
        t_lspr[SPR_BAL7] = &lspr[NOLIGHT];
        t_lspr[SPR_APLS] = &lspr[NOLIGHT];
        t_lspr[SPR_APBX] = &lspr[NOLIGHT];
        t_lspr[SPR_SKUL] = &lspr[NOLIGHT];
        t_lspr[SPR_FATB] = &lspr[NOLIGHT];
    }
#endif
}
static void Command_GrStats_f()
{
  /*
  CONS_Printf("Patch info headers : %7d kb\n", Z_TagUsage(PU_HWRPATCHINFO)>>10);
  CONS_Printf("3D Texture cache   : %7d kb\n", Z_TagUsage(PU_HWRCACHE)>>10);
  CONS_Printf("Plane polygone     : %7d kb\n", Z_TagUsage(PU_OPENGL_GEOMETRY)>>10);
  */
}
static void CV_grPolygonSmooth_OnChange()
{
  // Not used anymore in the old renderer (TODO: write a good one, if possible, for the new renderer)
  //HWD.pfnSetSpecialState (HWD_SET_POLYGON_SMOOTH, cv_grpolygonsmooth.value);
}
static void CV_grFogColor_OnChange()
{
  //HWD.pfnSetSpecialState (HWD_SET_FOG_COLOR, atohex(cv_grfogcolor.string));
}
static void CV_FogDensity_ONChange()
{
  //HWD.pfnSetSpecialState(HWD_SET_FOG_DENSITY, cv_grfogdensity.value );
}
static void CV_filtermode_ONChange()
{
  //HWD.pfnSetSpecialState(HWD_SET_TEXTUREFILTERMODE, cv_grfiltermode.value);
}
//FIXME: Hurdler: I've discovered that too much CONS_Printf (probably before a flush) generate segfault :(
/*
void PatchTexture::HWR_Draw(float x, float y, int flags)
{
  static Geometry *geo = 0;
  static State *state = 0;
  //V_SLOC     =  0x10000,   // scale starting location
  //V_SSIZE    =  0x20000,   // scale size
  //V_SCALE = V_SLOC | V_SSIZE,
  float scale = (flags & V_SSIZE) ? 1.0f : 320.0f/(float)cv_scr_width.value;
  float fx = (flags & V_SLOC) ? scale * (x - 160.f) / 160.0f : -1 + scale * x / 160.0f;
  float fy = (flags & V_SLOC) ? scale * (y - 100.f) / 100.0f : -1 + scale * y / 100.0f;
  float fw = scale * width / 160.0f;
  float fh = scale * height / 100.0f;
  if (!geo)
  {
    geo = new Geometry();
    geo->CreateTexturedRectangle(false, fx, -fy, fx+fw, -fy-fh, 1.0f);
    state = new State();
    state->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
  }
  else
  {
    geo->CreateTexturedRectangle(true, fx, -fy, fx+fw, -fy-fh, 1.0f);
  }
  state->Apply();
  geo->Draw();
}
*/

#endif // NO_OPENGL
