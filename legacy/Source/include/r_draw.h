// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: r_draw.h 498 2007-10-06 21:30:23Z smite-meister $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
/// \brief Software renderer: low level span/column drawer functions.

#ifndef r_draw_h
#define r_draw_h 1

#include "doomdef.h"
#include "m_fixed.h"
#include "screen.h"  // MAXVIDWIDTH, MAXVIDHEIGHT


typedef byte lighttable_t;

// -------------------------------
// COMMON STUFF FOR 8bpp AND 16bpp
// -------------------------------

#define FUZZTABLE     50

extern int fuzzoffset[FUZZTABLE];
extern int fuzzpos;

extern byte**           ylookup;
extern byte*            ylookup1[MAXVIDHEIGHT];
extern byte*            ylookup2[MAXVIDHEIGHT];
extern int              columnofs[MAXVIDWIDTH];

void    R_InitViewBuffer(int width, int height);

// -------------------------
// COLUMN DRAWING CODE STUFF
// -------------------------

extern int dc_x, dc_yl, dc_yh;
extern byte    *dc_source;
extern int      dc_texheight;
extern fixed_t  dc_iscale;
extern fixed_t  dc_texturemid;
extern lighttable_t *dc_colormap;
extern byte         *dc_transmap;
extern byte         *dc_translation;
extern struct r_lightlist_t *dc_lightlist;
extern int dc_numlights, dc_maxlights;


// -----------------------
// SPAN DRAWING CODE STUFF
// -----------------------

extern int ds_y, ds_x1, ds_x2;
extern int ds_xbits, ds_ybits;
extern fixed_t ds_xfrac, ds_yfrac;
extern fixed_t ds_xstep, ds_ystep;
extern byte*            ds_source;
extern lighttable_t*    ds_colormap;
extern byte*            ds_transmap;


// -----------------------
//   Translucency stuff
// -----------------------

#define  MAXTRANSTABLES  20  // how many translucency tables may be used
extern byte  *transtables[MAXTRANSTABLES]; // translucency tables


// TODO: add another asm routine which use the fg and bg indexes in the
//       inverse order so the 20-80 becomes 80-20 translucency, no need
//       for other tables (thus 1090,2080,5050,8020,9010, and fire special)

/// Translucency tables
enum transnum_t
{
  tr_transmed = 1,    //sprite 50 backg 50  most shots
  tr_transmor = 2,    //       20       80  puffs
  tr_transhi  = 3,    //       10       90  blur effect
  tr_transfir = 4,    // 50 50 but brighter for fireballs, shots..
  tr_transfx1 = 5,    // 50 50 brighter some colors, else opaque for torches
  tr_size     = 0x10000,  // one transtable is 256*256 bytes in size
};

// Initialize translucency tables
void    R_InitTranslucencyTables();

// Initialize color translation tables, for player rendering etc.
void    R_InitTranslationTables();




//-----------------------
//   Window borders
//-----------------------

/// windowborder textures
enum windowborder_e
{
  BRDR_T = 0,
  BRDR_B,
  BRDR_L,
  BRDR_R,
  BRDR_TL,
  BRDR_TR,
  BRDR_BL,
  BRDR_BR
};

// textures for window borders and background
extern class Material *window_border[8];
extern Material *window_background;

/// Load view window border textures.
void    R_InitViewBorder();

/// Fills screen 1 with background texture and view window borders.
void    R_FillBackScreen();

/// If the view size is not fullscreen, blits the border around it from screen 1.
void    R_DrawViewBorder();


//added:26-01-98: called by SCR_Recalc() when video mode changes
void    R_RecalcFuzzOffsets();

/// Blits a linear range from screen 1 to screen 0.
void    R_VideoErase(unsigned ofs, int count);

/// Screen wipe/melt special effects.
bool wipe_StartScreen();
bool wipe_EndScreen();
bool wipe_ScreenWipe(int ticks);


// ---------------------------------------------
// color mode dependent drawer function pointers
// ---------------------------------------------

extern void     (*skycolfunc)();
extern void     (*colfunc)();
extern void     (*basecolfunc)();
extern void     (*fuzzcolfunc)();
extern void     (*transcolfunc)();
extern void     (*shadecolfunc)();
extern void     (*spanfunc)();
extern void     (*basespanfunc)();



// -----------------
// 8bpp DRAWING CODE
// -----------------

void    ASMCALL R_DrawColumn_8();
void    ASMCALL R_DrawShadeColumn_8();             //smokie test..
void    ASMCALL R_DrawFuzzColumn_8();
void    ASMCALL R_DrawTranslucentColumn_8();
void    ASMCALL R_DrawTranslatedColumn_8();
void    ASMCALL R_DrawSpan_8();

void    R_DrawTranslucentSpan_8();
void    R_DrawFogSpan_8();
void    R_DrawFogColumn_8(); //SoM: Test
void    R_DrawColumnShadowed_8();
void    R_DrawPortalColumn_8();

// ------------------
// 16bpp DRAWING CODE
// ------------------

void    ASMCALL R_DrawColumn_16();
void    ASMCALL R_DrawFuzzColumn_16();
void    ASMCALL R_DrawTranslucentColumn_16();
void    ASMCALL R_DrawTranslatedColumn_16();
void    ASMCALL R_DrawSpan_16();


#endif
