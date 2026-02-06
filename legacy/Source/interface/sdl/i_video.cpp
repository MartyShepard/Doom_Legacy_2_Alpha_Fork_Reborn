// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_video.cpp 536 2009-06-29 06:46:13Z smite-meister $
//
// Copyright (C) 1998-2007 by DooM Legacy Team.
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
/// \brief SDL video interface

#include <stdlib.h>
#include <string.h>
#include <vector>

#include "SDL.h"

#ifdef WIN32
  #include <windows.h>
#endif

#include "doomdef.h"
#include "command.h"

#include "i_system.h"
#include "i_video.h"

#include "screen.h"
#include "m_argv.h"

#include "m_dll.h"

#include "hardware/oglrenderer.hpp"
#include "hardware/oglhelpers.hpp"



extern consvar_t cv_fullscreen; // for fullscreen support


// globals
OGLRenderer  *oglrenderer = NULL;
rendermode_t  rendermode = render_soft;

bool graphics_started = false;


#ifdef DYNAMIC_LINKAGE
static LegacyDLL OGL_renderer;
#endif

// SDL vars
static const SDL_VideoInfo *vidInfo = NULL;
static SDL_Surface   *vidSurface = NULL;
static SDL_Color      localPalette[256];

const static Uint32 surfaceFlags = SDL_SWSURFACE | SDL_HWPALETTE;



// maximum number of windowed modes (see windowedModes[][])
#if !defined(__MACOS__) && !defined(__APPLE_CC__)
#define MAXWINMODES (34)
#else
#define MAXWINMODES (12)
#endif


struct vidmode_t
{
  int w, h;
  char name[16];
};

static std::vector<vidmode_t> fullscrModes;

// windowed video modes from which to choose from.
static vidmode_t windowedModes[MAXWINMODES] =
{
#ifdef __APPLE_CC__
  {MAXVIDWIDTH /*1600*/, MAXVIDHEIGHT/*1200*/},
  {1440, 900},	/* iMac G5 native res */
  {1280, 1024},
  {1152, 720},	/* iMac G5 native res */
  {1024, 768},
  {1024, 640},
  {800, 600},
  {800, 500},
  {640, 480},
  {512, 384},
  {400, 300},
  {320, 200}
#else
  {MAXVIDWIDTH, MAXVIDHEIGHT},
/*
  {1280, 1024}, // 1.25
  {1024, 768}, // 1.3_
  {800, 600}, // 1.3_
  {640, 480}, // 1.3_
  {640, 400}, // 1.6
  {512, 384}, // 1.3_
  {400, 300}, // 1.3_
  {320, 200}  // original Doom resolution (pixel ar 1.6), meant for 1.3_ aspect ratio monitors (nonsquare pixels!)
*/
    {5793, 3055},
  /*{5461, 2880}, Macht Probleme */
    {5016, 2645},
  /*{4487, 2366}, Macht Probleme */
    {4096, 2160},
    {3840, 2160},
    {2560, 1600},
    {2560, 1440},
    {2048, 1536},
    {1600, 1440},
    {1600, 1024},
    {1920, 1440},
    {1920, 1200},
    {1920, 1080},
    {1680, 1050},
    {1600, 1200},
    {1600, 1024},
    {1600, 900},
    {1440, 1080},
    {1440, 900},
    /*{1366, 768}, Macht Probleme */
    {1360, 768},
    {1280, 1080},
    {1280, 1024},
    {1280, 960},
    {1280, 800},
    {1280, 768},
    {1280, 720},
    {1176, 664},
    {1152, 864},
    {1024, 768},
    {1024, 600}, 
    {800, 600},
    {640, 480},
    /*{512, 384}, Macht Probleme */
    {400, 300},
    {320, 200}
};
#endif

#ifdef WIN32

  // Struktur für deine Modi-Liste (ähnlich SDL_Rect)
  typedef struct {
      Uint32 w, h;
      Uint8 bpp;           // BitsPerPixel
      Uint8 bpx;
  } DisplayMode;
  
  // Liste aller gefundenen Modi (max. 32 sollte reichen)
  #define MAX_MODES MAXWINMODES
  static DisplayMode custom_modes[MAX_MODES];
  static int num_modes = 0;  
  void I_EnumWindowsDisplayModes(void);
#endif


//
// I_StartFrame
//
void I_StartFrame()
{
  if (render_soft == rendermode)
    {
      if (SDL_MUSTLOCK(vidSurface))
        {
          if (SDL_LockSurface(vidSurface) < 0)
            return;
        }
    }
  else
    {
      oglrenderer->StartFrame();
    }
}

//
// I_OsPolling
//
void I_OsPolling()
{
  if (!graphics_started)
    return;

  I_GetEvent();
}


//
// I_FinishUpdate
//
void I_FinishUpdate()
{
  if (rendermode == render_soft)
    {
      /*
      if (vid.screens[0] != vid.direct)
	memcpy(vid.direct, vid.screens[0], vid.height*vid.rowbytes);
      */
      SDL_Flip(vidSurface);

      if (SDL_MUSTLOCK(vidSurface))
        SDL_UnlockSurface(vidSurface);
    }
  else if(oglrenderer != NULL)
    oglrenderer->FinishFrame();

  I_GetEvent();
}


//
// I_SetPalette
//
void I_SetPalette(RGB_t* palette)
{
  for (int i=0; i<256; i++)
    {
      localPalette[i].r = palette[i].r;
      localPalette[i].g = palette[i].g;
      localPalette[i].b = palette[i].b;
    }

  SDL_SetColors(vidSurface, localPalette, 0, 256);
}



void I_SetGamma(float r, float g, float b)
{
  SDL_SetGamma(r, g, b);
}



// return number of fullscreen or windowed modes
int I_NumVideoModes()
{
  if (cv_fullscreen.value)
    return MAXWINMODES/*fullscrModes.size()*/;
  else
    return MAXWINMODES;
}

const char *I_GetVideoModeName(unsigned n)
{
  if (cv_fullscreen.value)
    {
      if (n >= MAXWINMODES/*fullscrModes.size()*/)
        return NULL;

      return windowedModes/*fullscrModes*/[n].name;
    }

  // windowed modes
  if (n >= MAXWINMODES)
    return NULL;

  return windowedModes[n].name;
}

int I_GetVideoModeForSize(int w, int h)
{
  int matchMode = -1;

  if (cv_fullscreen.value)
    {
      for (unsigned i=0; i<MAXWINMODES/*fullscrModes.size()*/; i++)
        {
          if (windowedModes/*fullscrModes*/[i].w == w && windowedModes/*fullscrModes*/[i].h == h)
            {
              matchMode = i;
              break;
            }
        }

      if (matchMode == -1) // use smallest mode
        matchMode =  MAXWINMODES/*fullscrModes.size()*/ - 1;
    }
  else
    {
      for (unsigned i=0; i<MAXWINMODES; i++)
        {
          if (windowedModes[i].w == w && windowedModes[i].h == h)
            {
              matchMode = i;
              break;
            }
        }

      if (matchMode == -1) // use smallest mode
          matchMode = MAXWINMODES-1;
    }

  return matchMode;
}



int I_SetVideoMode(int modeNum)
{
  //printf("\n [%s][%d]I_SetVideoMode\n",__FILE__,__LINE__);	
  
  Uint32 flags = surfaceFlags;
  vid.modenum  = modeNum;
  
  printf(" I_SetVideoMode: Screenmode %d (%dbit)\n",modeNum,vid.BitsPerPixel);	
  
	if (vid.BitsPerPixel == 0) // Marty Fallback
  {
     /* Klasse Video:: */
		  vid.BitsPerPixel = 8;
      //printf(" [%s][%d]I_SetVideoMode: Fallback to %dbit)\n",__FILE__,__LINE__,vid.BitsPerPixel);
  }
	    
  if (cv_fullscreen.value)
  {
      vid.width = windowedModes/*fullscrModes*/[modeNum].w;
      vid.height= windowedModes/*fullscrModes*/[modeNum].h;
      flags |= SDL_FULLSCREEN;
      
      if (flags & SDL_FULLSCREEN)
      {
        // So entfernt man ein Flag wieder (korrekt!)
        flags &= ~SDL_FULLSCREEN;      
        //cv_fullscreen.value = 0;
      }
      

      CONS_Printf (" I_SetVideoMode: fullscreen %d x %d (%d bpp)\n", vid.width, vid.height, vid.BitsPerPixel);
  }
  else
  { // !cv_fullscreen.value
      vid.width = windowedModes[modeNum].w;
      vid.height = windowedModes[modeNum].h;
    
      CONS_Printf(" I_SetVideoMode: windowed %d x %d (%d bpp)\n", vid.width, vid.height, vid.BitsPerPixel);
  }

  if (rendermode == render_soft)
  {
    SDL_FreeSurface(vidSurface);

		//flags |= SDL_SWSURFACE|SDL_HWPALETTE;
  
  	flags = SDL_HWSURFACE|
            SDL_HWPALETTE|
            SDL_DOUBLEBUF;                  
			
    vidSurface = SDL_SetVideoMode(vid.width, vid.height, vid.BitsPerPixel, flags);
    
    if (vidSurface == NULL)
      I_Error("Could not set vidmode\n");

    if (vidSurface->pixels == NULL)
      I_Error("Didn't get a valid pixels pointer (SDL). Exiting.\n");

    vid.direct = static_cast<byte*>(vidSurface->pixels);
    // VB: FIXME this stops execution at the latest
    vid.direct[0] = 1;
      
  }
  else
  {  
    if (!oglrenderer->InitVideoMode(vid.width, vid.height, cv_fullscreen.value))			
      I_Error("Could not set OpenGL vidmode.\n");
  }

  #ifdef BORDERLESS_WIN32 
      ToggleBorderless();
      CenterSDL1Window();      
  #endif
    
  I_StartupMouse(); // grabs mouse and keyboard input if necessary
  
  return 1;
}

bool I_StartupGraphics()
{
  
  // 1. SDL_Init (Video)
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
      printf(" [%s][%d] SDL Init failed: %s\n",__FILE__,__LINE__,SDL_GetError());
      return false;
  }

  if (graphics_started)
    return true;
	
  Uint8 BitMode = 8;
  
  Uint8 p = M_CheckParm("-bpp");  // specific bit per pixel color
  if( p )
  {
    BitMode = atoi(myargv[p + 1]);
    printf(" [%s][%d] Bitmode Requestet: %d\n",__FILE__,__LINE__,BitMode);    
  }

  vidInfo = SDL_GetVideoInfo();
  
  I_EnumWindowsDisplayModes();  

  int n = 0;
  int x = 1;
  fullscrModes.clear();  // vorher leeren, falls schon gefüllt
  while (n < num_modes)  // num_modes aus deiner I_EnumWindowsDisplayModes()
  {
    DisplayMode &mode = custom_modes[n];
    if ( mode.bpp == BitMode )
    {

      if (mode.w <= MAXVIDWIDTH && mode.h <= MAXVIDHEIGHT)
      {
        vidmode_t temp;   
        temp.w = mode.w;      
        temp.h = mode.h;        
        sprintf(temp.name, "%dx%d", temp.w, temp.h);
        fullscrModes.push_back(temp);

        // Optional: Ausgabe zum Debuggen
        printf("   Mode:> [%2d] %10sx%dbit\n", x, temp.name, mode.bpp);
        x++;
      }
    }
    n++;
  }

  CONS_Printf(" Found %d video modes.\n", fullscrModes.size());
  if (fullscrModes.empty())
  {
    CONS_Printf(" No suitable video modes found!\n");
    return false;
  }

  // name the windowed modes
  //printf(" [%s][%d] I_StartupGraphics: Definiere und bennen Window Modes\n",__FILE__,__LINE__);
  for (n=0; n<MAXWINMODES; n++)
  {
    sprintf(windowedModes[n].name, "win %dx%d", windowedModes[n].w, windowedModes[n].h);
  }

  // even if I set vid.bpp and highscreen properly it does seem to
  // support only 8 bit  ...  strange
  // so lets force 8 bit (software mode only)
  // TODO why not use hicolor in sw mode too? it must work...

#if defined(__APPLE_CC__) || defined(__MACOS__)

  vid.BytesPerPixel	= vidInfo->vfmt->BytesPerPixel;
  vid.BitsPerPixel	= vidInfo->vfmt->BitsPerPixel;
  if (!M_CheckParm("-opengl"))
  {
	  // software mode
	  vid.BytesPerPixel = 1;
	  vid.BitsPerPixel = 8;
  }

#else
  /* Klasse Video:: */
  vid.BytesPerPixel = BitMode/8/*1*/;  //videoInfo->vfmt->BytesPerPixel
  vid.BitsPerPixel  = BitMode  /*8*/;
#endif

  // default resolution
  vid.width  = BASEVIDWIDTH;
  vid.height = BASEVIDHEIGHT;
  
  printf(" [%s][%d] I_StartupGraphics...\n"
         "          Set [vid.] Width        : %d\n"
         "          Set [vid.] Height       : %d\n"
         "          Set [vid.] BytesPerPixel: %d\n"
         "          Set [vid.] BitsPerPixel : %d\n\n"                              
                                      ,__FILE__,__LINE__,
                                      vid.width,vid.height,
                                      vid.BytesPerPixel,vid.BitsPerPixel);
  
  if (M_CheckParm("-opengl"))
  {
    // OpenGL mode
    printf(" I_StartupGraphics: OpenGL Modus\n",__FILE__,__LINE__);
    rendermode = render_opengl;
    oglrenderer = new OGLRenderer;
  }
  else
  {
    // software mode
    rendermode = render_soft;
    printf(" I_StartupGraphics: Software Modus\n",__FILE__,__LINE__);    
    //CONS_Printf("I_StartupGraphics: Windowed %dx%dx%dbit\n", vid.width, vid.height, vid.BitsPerPixel);
    vidSurface = SDL_SetVideoMode(vid.width, vid.height, vid.BitsPerPixel, surfaceFlags);

    if (vidSurface == NULL)
    {
      CONS_Printf("Could not set video mode!\n");
      return false;
    }
    
    vid.direct = static_cast<byte*>(vidSurface->pixels);
  }

  SDL_ShowCursor(SDL_DISABLE);
  I_StartupMouse(); // grab mouse and keyboard input if needed

  graphics_started = true;

  //printf(" [%s][%d] I_StartupGraphics: Return -> True\n",__FILE__,__LINE__);
  return true;
}

void I_ShutdownGraphics()
{
  // was graphics initialized anyway?
  if (!graphics_started)
    return;

  if (rendermode == render_soft)
    {
      // vidSurface should be automatically freed
    }
  else
    {
      delete oglrenderer;
      oglrenderer = NULL;

#ifdef DYNAMIC_LINKAGE
      if (ogl_handle)
        CloseDLL(ogl_handle);
#endif
    }
  SDL_Quit();
}
#ifdef WIN32

void I_EnumWindowsDisplayModes(void)
{
  DEVMODE dm = {0};
  dm.dmSize = sizeof(DEVMODE);
  dm.dmDriverExtra = 0;

  num_modes = 0;

  // 1. Aktuelle Desktop-Auflösung holen (ENUM_CURRENT_SETTINGS)
  if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm))
  {
    custom_modes[num_modes].w   = dm.dmPelsWidth;
    custom_modes[num_modes].h   = dm.dmPelsHeight;
    custom_modes[num_modes].bpp = dm.dmBitsPerPel;
    num_modes++;
    printf(" Aktueller Desktop: %ldx%ld @ %ld bpp\n", dm.dmPelsWidth, dm.dmPelsHeight, dm.dmBitsPerPel);
  }

  // 2. Alle verfügbaren Modi durchlaufen (ENUM_REGISTRY_SETTINGS + dynamisch)
  int i;
  for ( i=0; EnumDisplaySettings(NULL, i, &dm); ++i )
  {
    // Nur sinnvolle Modi (≥ 640x480, 8/16/24/32 bpp)
        
    if (dm.dmPelsWidth  >= 320 && dm.dmPelsHeight >= 200 &&
       (dm.dmBitsPerPel ==   8 || dm.dmBitsPerPel ==  16 ||
        dm.dmBitsPerPel ==  24 || dm.dmBitsPerPel ==  32 ))   
       /* 
        printf(" Modus %d: %lux%lu @ %lu bpp\n", num_modes-1, dm.dmPelsWidth,
                                                              dm.dmPelsHeight,
                                                              dm.dmBitsPerPel);
        */
    {
      // Duplikat vermeiden
      int duplicate = 0;
      for (int j = 0; j < num_modes; j++)
      {
        if (custom_modes[j].w   == dm.dmPelsWidth  &&
            custom_modes[j].h   == dm.dmPelsHeight &&
            custom_modes[j].bpp == dm.dmBitsPerPel &&
            custom_modes[j].bpx == dm.dmBitsPerPel/8)
            {
                    duplicate = 1;
                    break;
            }
      }
       
      if (!duplicate)
      {
        custom_modes[num_modes].w   = dm.dmPelsWidth;
        custom_modes[num_modes].h   = dm.dmPelsHeight;
        custom_modes[num_modes].bpp = dm.dmBitsPerPel;
        custom_modes[num_modes].bpx = dm.dmBitsPerPel/8;                
        num_modes++;
                                
        /* Zum debuggen */
        /*
          printf(" Modus %d: %lux%lu @ %lu bpp [Pixelformat = %lu]\n", num_modes-1,
                                                                    dm.dmPelsWidth,
                                                                    dm.dmPelsHeight,
                                                                    dm.dmBitsPerPel,
                                                                    dm.dmBitsPerPel/8);                                                                    
        */
      }
    }
  }

  if (num_modes == 0)
  {
      printf("Keine Modi gefunden – Fallback auf Standard\n");
      custom_modes[0].w   = 640;
      custom_modes[0].h   = 480;
      custom_modes[0].bpp = 32;
      num_modes           = 1;
      return;
  }
  
}

  #ifdef BORDERLESS_WIN32 
  /*
   * Borderless Mode mit SDL 1.2.16. Keine ahnung
   * ob das auch  mit alten versionen funktioniert
   * Ist ein bissel Tricky. Aber unter Windows geht.
   * Sollte unter Linux eigentlich auch laufen?
   * Junge. Das ist 2025 sowas von Standard
   */
  #include <SDL/SDL_syswm.h>

  #define STYLE_DO_NORMAL (WS_OVERLAPPEDWINDOW | WS_VISIBLE)  // Standard: Rahmen, Titel, Min/Max/Close

  #define STYLE_NO_BORDER (WS_POPUP | WS_VISIBLE)  
        
  void ToggleBorderless(void)        
  {
    static HWND sdl_hwnd = NULL;
    
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWMInfo(&info))
    {
       sdl_hwnd = info.window;
       if (!sdl_hwnd) return; // Kein Fenster Gefunden! Sollte nicht passieren.
    }
   
    // Alte Auflösung vom SDL_Surface merken 
    static int old_width = 0, old_height = 0;    
    old_width = SDL_GetVideoSurface()->w;
    old_height= SDL_GetVideoSurface()->h;   
    
    // Aktuellen Stil holen
    LONG current_style = GetWindowLong(sdl_hwnd, GWL_STYLE);
    LONG new_style;

    if ((current_style & WS_CAPTION) && 
        (cv_borderless.value == 1 ))      // Aktuell mit Rahmen ? zu borderless             
       new_style = STYLE_NO_BORDER;
       
    else if ((current_style | WS_CAPTION) &&
            (cv_borderless.value == 0  )) // Aktuell borderless ? zurück zu normal               
        new_style = STYLE_DO_NORMAL;

    if ( current_style != new_style )
    {
        // Stil ändern
        SetWindowLong(sdl_hwnd, GWL_STYLE, new_style);       
        
        // Neu Zeichnen
        SetWindowPos(sdl_hwnd, NULL, 0, 0, old_width, old_height, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        
        // Fenster wieder Zentrieren
        CenterSDL1Window();
    }

    /*
     *printf("[%s][%d] Fensterstil geändert: %s\n",__FILE__,__LINE__,
		 *						(new_style == STYLE_DO_NORMAL)?"mit Rahmen" : "borderless");
     */
     
    return;  
  }
  #endif
  /*
   * Auto Zentrierung mit SDL 1.2.16. Geht! Also
   * Linux User. Das müsste auch mit euerer API
   * zu schaffen sein?
   * Alter?. Sowas ist 2025 sowas von Standard
   */    
  void CenterSDL1Window(void)
  {    
      static HWND hwnd = NULL;
    
      SDL_SysWMinfo info;
      SDL_VERSION(&info.version);
      if (SDL_GetWMInfo(&info))
      {
        hwnd = info.window;
        if (!hwnd) return; // Kein Fenster Gefunden! Sollte nicht passieren.
      }
    
      // HWND des SDL-Fensters holen
      if (hwnd)
      {               
          //printf(" [%s][%d] CenterSDL1Window(): SDL Surface eingefangen\n",__FILE__,__LINE__);  
          #ifdef BORDERLESS_WIN32 
          //  ToggleBorderless(hwnd);               
          #endif
          // 1. Bildschirmgröße holen
          int Screen_W = GetSystemMetrics(SM_CXSCREEN);
          int Screen_H = GetSystemMetrics(SM_CYSCREEN);

          // 2. Client-Größe (der eigentliche Inhalt) holen – NICHT GetWindowRect!
          RECT clientRect;
          GetClientRect(hwnd, &clientRect);
          int client_w = clientRect.right - clientRect.left;
          int client_h = clientRect.bottom - clientRect.top;

          // 3. Äußere Fenstergröße holen (für die Positionierung)
          RECT windowRect;
          GetWindowRect(hwnd, &windowRect);
          int window_w = windowRect.right - windowRect.left;
          int window_h = windowRect.bottom - windowRect.top;

          // 4. Berechne die gewünschte obere linke Ecke des **gesamten Fensters**
          // so dass die **Client-Area** genau in der Mitte liegt
          int x = (Screen_W - client_w) / 2;
          int y = (Screen_H - client_h) / 2;

          // 5. Korrigiere um den Rahmen/Titelbalken (damit Client zentriert ist)
          int border_offset_x = (window_w - client_w) / 2;
          int border_offset_y = (window_h - client_h) - border_offset_x;  // Titelbalken ist meist oben dicker

          x -= border_offset_x;
          y -= border_offset_y;

          // Sicherheitsgrenzen (damit Fenster nicht außerhalb des Bildschirms liegt)
          if (x < 0) x = 0;
          if (y < 0) y = 0;
          
          // 6. Fenster positionieren
          SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
          /*
          printf(" [%s][%d] CenterSDL1Window(): Client %dx%d ? Position (%d, %d)\n", __FILE__,
                                                                                    __LINE__,
                                                                                    client_w,
                                                                                    client_h,
                                                                                     x, y   );*/
                                                                                    
      }
  }
  #endif