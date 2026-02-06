// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: screen.cpp 570 2009-11-27 23:57:47Z smite-meister $
//
// Copyright (C) 1998-2009 by DooM Legacy Team.
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
/// \brief Video subsystem. Handles multiple resolutions, 8bpp/16bpp(highcolor) modes,
/// palettes etc.

#include "doomdef.h"

#include "screen.h"
#include "v_video.h"

#include "g_game.h"
#include "console.h"
#include "command.h"
#include "am_map.h"
#include "hud.h"

#include "m_argv.h"
#include "i_video.h"
#include "i_system.h" // Marty
#include "r_data.h"
#include "r_draw.h"
#include "r_main.h"

#include "w_wad.h"
#include "z_zone.h"


// ------------------
// global video state
// ------------------
Video vid;



// =========================================================================
// console variables
// =========================================================================

void CV_Fuzzymode_OnChange();
void CV_Fullscreen_OnChange();
void CV_video_gamma_OnChange();
#ifdef BORDERLESS_WIN32
void CV_Borderless_OnChange();
#endif

static CV_PossibleValue_t scr_depth_cons_t[]={{8,"8 bits"}, {16,"16 bits"}, {24,"24 bits"}, {32,"32 bits"}, {0,NULL}};

//added:03-02-98: default screen mode, as loaded/saved in config
consvar_t   cv_scr_width  = {"scr_width",  "800", CV_SAVE, CV_Unsigned};
consvar_t   cv_scr_height = {"scr_height", "600", CV_SAVE, CV_Unsigned};
consvar_t   cv_scr_depth =  {"scr_depth",  "16 bits",   CV_SAVE, scr_depth_cons_t};
consvar_t   cv_fullscreen = {"fullscreen", "no", CV_SAVE | CV_CALL | CV_NOINIT, CV_YesNo, CV_Fullscreen_OnChange};
#ifdef BORDERLESS_WIN32
consvar_t   cv_borderless = {"borderless", "no", CV_SAVE | CV_CALL | CV_NOINIT,CV_YesNo, CV_Borderless_OnChange};
#endif
// Are invisible things translucent or fuzzy?
consvar_t   cv_fuzzymode = {"fuzzymode", "Off", CV_SAVE | CV_CALL, CV_OnOff, CV_Fuzzymode_OnChange};

// gamma correction
static CV_PossibleValue_t gamma_cons_t[] = {{0,"MIN"},{4,"MAX"},{0,NULL}};
consvar_t  cv_video_gamma = {"gamma","0",CV_SAVE|CV_CALL,gamma_cons_t,CV_video_gamma_OnChange};

const byte *R_BuildGammaTable()
{
  static byte gammatable[256];

  // calculate gammatable anew each time
  float gamma = 1 -0.125*cv_video_gamma.value;
  for (int i=0; i<256; i++)
    gammatable[i] = round(255.0*pow((i+1)/256.0, gamma));

  return gammatable;
}


// reload palette when gamma is changed
void CV_video_gamma_OnChange()
{
  // reload palette
  vid.LoadPalette("PLAYPAL");
  vid.SetPalette(0);
}


// change drawer function when fuzzymode is changed
void CV_Fuzzymode_OnChange()
{
  if (vid.BytesPerPixel == 1)
    fuzzcolfunc = (cv_fuzzymode.value) ? R_DrawFuzzColumn_8 : R_DrawTranslucentColumn_8;
  else if (vid.BytesPerPixel > 1)
    fuzzcolfunc = (cv_fuzzymode.value) ? R_DrawFuzzColumn_16 : R_DrawTranslucentColumn_16;
}


// Change fullscreen on/off when cv_fullscreen is changed
void CV_Fullscreen_OnChange()
{
  vid.setmodeneeded = I_GetVideoModeForSize(cv_scr_width.value, cv_scr_height.value) + 1;
}

// =========================================================================

Video::Video()
{
  buffer = direct = NULL;
  palette = NULL;
  currentpalette = 0;
}

//  The video mode change is delayed until the start of the next refresh
//  by setting the setmodeneeded to a value >0
void Video::SetMode()
{
  byte DebugVerbose = 1;
  
  if (DebugVerbose)
	printf("\n [%s][%d]::SetMode\n",__FILE__,__LINE__);

  if (game.dedicated)
    return;

  if (!setmodeneeded)
    return;   //should never happen

  I_SetVideoMode(--setmodeneeded);
  SetPalette(0);

  if (DebugVerbose)
	printf(" [%s][%d] ::SetMode() var - BytesPerPixel = %d\n",
                           __FILE__,__LINE__, BytesPerPixel);
	
  
  /* BytesPerPixel -> Klasse Video:: - extern var vid.BitsPerPixel */
  if (BytesPerPixel == 0) // Marty: Fallabck    
  {
      if (DebugVerbose)
      printf(" [%s][%d] ::SetMode() BytesPerPixel Fallback%d\n",
                              __FILE__,__LINE__, BytesPerPixel);
                              
		  BytesPerPixel = 1;
  }
		
  //  setup the right draw routines for either 8bpp or 16bpp
  if (BytesPerPixel == 1)
  {
    colfunc = basecolfunc = R_DrawColumn_8;
    skycolfunc = R_DrawColumn_8;

    fuzzcolfunc = (cv_fuzzymode.value) ? R_DrawFuzzColumn_8 : R_DrawTranslucentColumn_8;
    transcolfunc = R_DrawTranslatedColumn_8;
    shadecolfunc = R_DrawShadeColumn_8;  //R_DrawColumn_8;
    spanfunc = basespanfunc = R_DrawSpan_8;
  }
  else if (BytesPerPixel > 1)
  {
    if (DebugVerbose)
    CONS_Printf(" Using Highcolor Mode\n");

    colfunc = basecolfunc = R_DrawColumn_16;
    skycolfunc = R_DrawColumn_16;

    fuzzcolfunc = (cv_fuzzymode.value) ? R_DrawFuzzColumn_16 : R_DrawTranslucentColumn_16;
    transcolfunc = R_DrawTranslatedColumn_16;
    shadecolfunc = NULL;      //detect error if used somewhere..
    spanfunc = basespanfunc = R_DrawSpan_16;
  }
  else
    I_Error(" [%s][%d] ::SetMode() Unknown BytesPerPixel Mode = %d\n", BytesPerPixel);

  setmodeneeded = 0;

  Recalc();
}

void R_Init8to16();

// Starts and initializes the video subsystem
void Video::Startup()
{
  byte DebugVerbose = 1;
    
  if (game.dedicated)
    return;

  if (DebugVerbose)
  CONS_Printf("Initializing the video module...\n");

  if (DebugVerbose)
  printf("\n [%s][%d]::Startup() -> I_StartupGraphics()\n",__FILE__,__LINE__);

  I_StartupGraphics();
  
  if (DebugVerbose)
  printf(" [%s][%d]::Startup() -> I_StartupGraphics() [OK..]\n",__FILE__,__LINE__);

  modenum       = 0; // not exactly true, but doesn't matter here.
  setmodeneeded = 31;// 320x200, windowed
  
  resetpaletteneeded = false;

  LoadPalette("PLAYPAL");
  SetPalette(0);
  
  if (DebugVerbose)
  printf(" [%s][%d]::Startup() -> BytesPerPixel: %d\n",__FILE__,__LINE__,BytesPerPixel);

  //fab highcolor maps
  if (BytesPerPixel > 1)
  {
      //CONS_Printf("\nInit HighColor...");
      if (DebugVerbose)
      printf(" [%s][%d]::Startup() -> Init HighColor\n",__FILE__,__LINE__);      
    
      R_Init8to16();
  }
  
  if (DebugVerbose)
  printf(" [%s][%d]::Startup() -> Create Palette Conversion\n",__FILE__,__LINE__);  

  // create palette conversion colormaps if necessary (palette must be set!)  
  materials.InitPaletteConversion();

  buffer = NULL;

  //Recalc();
  if (DebugVerbose)    
  printf(" [%s][%d]::Startup() -> SetMode()\n",__FILE__,__LINE__);  

  SetMode();
}



//added:27-01-98: tell asm code the new rowbytes value.
void ASMCALL ASM_PatchRowBytes(int rowbytes);


// Called after the video mode has changed
void Video::Recalc()
{
  rowbytes = width * BytesPerPixel;

#ifdef USEASM
  // patch the asm code depending on vid buffer rowbytes
  ASM_PatchRowBytes(rowbytes);
#endif

  // scale 1,2,3 times in x and y the patches for the
  // menus and overlays... calculated once and for all
  // used by routines in v_video.c
  {
    fdupx = (float)width / BASEVIDWIDTH;
    fdupy = (float)height / BASEVIDHEIGHT;
    dupx = (int)fdupx;
    dupy = (int)fdupy;
    //baseratio = FixedDiv(height << FRACBITS, BASEVIDHEIGHT << FRACBITS); //Hurdler: not used anymore
  }

  // calculate centering offset for the scaled menu
  scaledofs = 0;
  centerofs = (((height%BASEVIDHEIGHT)/2) * width) +
    (width%BASEVIDWIDTH)/2;

  int i;

  if (rendermode != render_soft)
    {
      // hardware modes do not use screens[] pointers
      buffer = NULL;
      // be sure to cause a NULL read/write error so we detect it, in case of..
      for (i=0 ; i<NUMSCREENS ; i++)
        screens[i] = NULL;
    }
  else
    {
      // screens[0] points to the actual video surface, other screens to buffers we allocate ourselves.
      screens[0] = direct;

      if (buffer)
        free(buffer);

      int screensize = width * height * BytesPerPixel;
      buffer = static_cast<byte*>(calloc(screensize * (NUMSCREENS-1), 1));

      for (i=1 ; i<NUMSCREENS ; i++)
        screens[i] = buffer + (i-1)*screensize;
    }

  // fuzzoffsets for the 'spectre' effect,... this is a quick hack
  R_RecalcFuzzOffsets();

  // r_plane stuff : visplanes, openings, floorclip, ceilingclip, spanstart,
  //                 spanstop, yslope, distscale, cachedheight, cacheddistance,
  //                 cachedxstep, cachedystep
  //              -> allocated at the maximum vidsize, static.

  // r_main : xtoviewangle, allocated at the maximum size.
  // r_things : negonearray, screenheightarray allocated max. size.

  // scr_viewsize doesn't change, neither detailLevel, but the pixels
  // per screenblock is different now, since we've changed resolution.
  R_SetViewSize();

  con.recalc = true;

  hud.st_palette = -1;

  // update automap because some screensize-dependent values
  // have to be calculated
  automap.Resize();
}


// Check for screen cmd-line parms : to force a resolution.
// Set the video mode to set at the 1st display loop (setmodeneeded)

void Video::CheckDefaultMode()
{
  // 0 means not set at the cmd-line
  int scr_forcex = 0;
  int scr_forcey = 0;

  int p = M_CheckParm("-width");
  if (p && p < myargc-1)
    scr_forcex = atoi(myargv[p+1]);

  p = M_CheckParm("-height");
  if (p && p < myargc-1)
    scr_forcey = atoi(myargv[p+1]);

  if (scr_forcex && scr_forcey)
    {
      CONS_Printf("Using resolution: %d x %d\n", scr_forcex, scr_forcey);
      // returns -1 if not found, thus will be 0 (no mode change) if not found
      setmodeneeded = I_GetVideoModeForSize(scr_forcex, scr_forcey) + 1;
    }
  else
    {
      CONS_Printf("Default resolution: %d x %d (%d bpp)\n", cv_scr_width.value,
                  cv_scr_height.value, cv_scr_depth.value);
      // see note above
      setmodeneeded = I_GetVideoModeForSize(cv_scr_width.value, cv_scr_height.value) + 1;
    }
}


// Make the current video mode the new default to be saved in the config file.
void Video::SetDefaultMode()
{
  // remember the default screen size
  cv_scr_width.Set(width);
  cv_scr_height.Set(height);
  cv_scr_depth.Set(BitsPerPixel);
  // CV_SetValue (&cv_fullscreen, !windowed); metzgermeister: unnecessary?
}




// keep a copy of the palette so that we can get the RGB
// value for a color index at any time.
void Video::LoadPalette(const char *lumpname)
{
  int i = fc.GetNumForName(lumpname);
  int palsize = fc.LumpLength(i)/3;
  if (palette)
    Z_Free(palette);
  palette = static_cast<RGB_t*>(Z_Malloc(sizeof(RGB_t)*palsize, PU_STATIC, NULL));

  RGB_t *pal = static_cast<RGB_t*>(fc.CacheLumpNum(i, PU_DAVE));
  const byte *gamma_table = R_BuildGammaTable();
  for (i=0; i<palsize; i++)
    {
      palette[i].r = gamma_table[pal[i].r];
      palette[i].g = gamma_table[pal[i].g];
      palette[i].b = gamma_table[pal[i].b];
    }

  Z_Free(pal);
}


// Set the current palette to use for palettized graphics
// (that is, most if not all of Doom's original graphics)
void Video::SetPalette(int palettenum)
{
  if (!palette)
    LoadPalette("PLAYPAL");

  currentpalette = palettenum;

  // PLAYPAL lump contains 14 different 256 color RGB palettes (28 for Hexen)

  if (rendermode == render_soft)
    I_SetPalette(&palette[palettenum*256]);

  resetpaletteneeded = false;
}



// equivalent to LoadPalette(pal); SetPalette(0);
void Video::SetPaletteLump(const char *pal)
{
  LoadPalette(pal);
  currentpalette = 0;
  if (rendermode == render_soft)
    I_SetPalette(palette);
}


// returns the current palette
RGB_t *Video::GetCurrentPalette()
{
  if (!palette)
    return NULL;

  return &palette[currentpalette << 8];
}

#ifdef BORDERLESS_WIN32
void CV_Borderless_OnChange(void)
{
  //CONS_Printf("[%s][%d] cv_borderless.value...%d\n",__FILE__,__LINE__,cv_borderless.value);   
  ToggleBorderless();
}
#endif

void Video::FillScreen(byte color)
{
    if (!buffer) return;
    memset(buffer, color, width * height * (BytesPerPixel));
}

void Video::FillRect(int x, int y, int w, int h, byte color)
{
    if (!buffer) return;
    // Einfache Rechteck-Füllung (kann später optimiert werden)
    for (int yy = y; yy < y + h; yy++)
    {
        if (yy >= height) break;
        byte* line = buffer + yy * rowbytes + x * BytesPerPixel;
        memset(line, color, w * BytesPerPixel);
    }
}