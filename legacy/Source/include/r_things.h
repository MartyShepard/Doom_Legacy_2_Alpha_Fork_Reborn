// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: r_things.h 533 2009-06-11 08:31:10Z smite-meister $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2006 by DooM Legacy Team.
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
/// \brief Rendering of sprites and masked textures

#ifndef r_things_h
#define r_things_h 1

#include "m_fixed.h"
#include "screen.h"

// Constant arrays used for psprite clipping
//  and initializing clipping.
extern short            negonearray[MAXVIDWIDTH];
extern short            screenheightarray[MAXVIDWIDTH];

// vars for R_DrawMaskedColumn
extern short*           mfloorclip;
extern short*           mceilingclip;
extern fixed_t          spryscale;
extern fixed_t          sprtopscreen;
extern fixed_t          sprbotscreen;
extern fixed_t          windowtop;
extern fixed_t          windowbottom;

extern fixed_t          pspritescale;
extern fixed_t          pspriteiscale;
extern fixed_t          pspriteyscale;  //added:02-02-98:for aspect ratio

extern const int PSpriteSY[];

typedef struct post_t column_t;
void R_DrawMaskedColumn(column_t* column);

void R_SortVisSprites();

//SoM: 6/5/2000: Light sprites correctly!
void R_AddSprites(struct sector_t* sec, int lightlevel);
void R_AddPSprites();
//void R_DrawSprite(vissprite_t* spr);
void R_InitSprites();
void R_ClearSprites();
void R_DrawSprites();  //draw all vissprites
//void R_DrawMasked();

void R_ClipVisSprite(struct vissprite_t *vis, int xl, int xh);


void R_InitDrawNodes();

#endif
