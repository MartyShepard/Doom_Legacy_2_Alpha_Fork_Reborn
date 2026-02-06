// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_draw8.cpp 427 2007-01-31 12:46:04Z smite-meister $
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
/// \brief 8bpp span/column drawer functions.

#include "doomtype.h"

#include "r_draw.h"
#include "r_render.h"

#define USEHIRES 1


// We use the improved Boom versions of the drawers.
// The original code is left here for comparison.
#ifdef ORIGINAL_DRAWERS
void R_DrawColumn_8()
{
  register int count = dc_yh - dc_yl + 1;

  // Zero length, column does not exceed a pixel.
  if (count <= 0)
    return;

#ifdef RANGECHECK
  if (unsigned(dc_x) >= vid.width
      || dc_yl < 0
      || dc_yh >= vid.height)
    I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

  // Framebuffer destination address.
  // Use ylookup LUT to avoid multiply with ScreenWidth.
  // Use columnofs LUT for subwindows?
  register byte *dest = ylookup[dc_yl] + columnofs[dc_x];

  // Determine scaling, which is the only mapping to be done.
  register fixed_t fracstep = dc_iscale;
  register fixed_t frac = dc_texturemid + (dc_yl-centery)*fracstep;

  // Inner loop that does the actual texture mapping,
  //  e.g. a DDA-lile scaling.
  // This is as fast as it gets.
  do
    {
      // Re-map color indices from wall texture column
      //  using a lighting/special effects LUT.
      *dest = dc_colormap[dc_source[(frac.floor())&127]];
      // R_DrawSkyColumn_8: *dest = dc_colormap[dc_source[(frac.floor())&255]];
      // R_DrawTranslucentColumn_8: *dest = dc_colormap[dc_transmap[(dc_source[frac.floor()] << 8) + *dest]];

      dest += vid.width;
      frac += fracstep;

    } while (--count);
}


void R_DrawSpan_8()
{
  register Uint32 xfrac;
  register Uint32 yfrac;
  register byte  *dest;
  register int    count;

#ifdef RANGECHECK
  if (ds_x2 < ds_x1
      || ds_x1<0
      || ds_x2>=vid.width
      || unsigned(ds_y) > vid.height)
    {
      I_Error( "R_DrawSpan: %i to %i at %i", ds_x1,ds_x2,ds_y);
    }
#endif

  xfrac = ds_xfrac.value() & 0x3fFFff; // this does the % 64
  yfrac = ds_yfrac.value();

  dest = ylookup[ds_y] + columnofs[ds_x1];

  // We do not check for zero spans here?
  count = ds_x2 - ds_x1;

  do
    {
      // Lookup pixel from flat texture tile,
      //  re-index using light/colormap.
      *dest = ds_colormap[ds_source[((yfrac>>(16-6))&(0x3f<<6)) | (xfrac>>16)]];
      dest++;

      // Next step in u,v.
      xfrac += ds_xstep;
      yfrac += ds_ystep;
      xfrac &= 0x3fFFff;
    } while (count--);
}
#endif



//==========================================================================
// COLUMNS
//==========================================================================

// A column is a vertical slice/span of a wall texture that uses
// a has a constant z depth from top to bottom.


// SoM: Experiment to make software go faster. Taken from the Boom source
void R_DrawColumn_8()
{
  int count = dc_yh - dc_yl + 1; 

  if (count <= 0)    // Zero length, column does not exceed a pixel.
    return;
                                 
#ifdef RANGECHECK 
  if (dc_x >= vid.width
      || dc_yl < 0
      || dc_yh >= vid.height) 
    I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x); 
#endif 

  // Framebuffer destination address.
  // Use ylookup LUT to avoid multiply with ScreenWidth.
  // Use columnofs LUT for subwindows?
  register byte *dest = ylookup[dc_yl] + columnofs[dc_x];

  // Determine scaling, which is the only mapping to be done.
  fixed_t fracstep = dc_iscale; 
  register fixed_t frac = dc_texturemid + (dc_yl-centery)*fracstep; 

  // Inner loop that does the actual texture mapping,
  //  e.g. a DDA-lile scaling.
  // This is as fast as it gets.

  {
    register const byte *source = dc_source;            
    register const lighttable_t *colormap = dc_colormap; 
    register int heightmask = dc_texheight-1;
    if (dc_texheight & heightmask)
      {
        heightmask++;
        fixed_t fheightmask = heightmask;
          
        if (frac < 0)
          while ((frac += fheightmask) <  0);
        else
          while (frac >= fheightmask)
            frac -= fheightmask;
          
        do
          {
            // Re-map color indices from wall texture column
            //  using a lighting/special effects LUT.
            // fheightmask is the Tutti-Frutti fix -- killough
            
            *dest = colormap[source[frac.floor()]];
            dest += vid.width;
            if ((frac += fracstep) >= fheightmask)
              frac -= fheightmask;
          } 
        while (--count);
      }
    else
      {
        while ((count-=2)>=0)   // texture height is a power of 2 -- killough
          {
            *dest = colormap[source[frac.floor() & heightmask]];
            dest += vid.width;
            frac += fracstep;
            *dest = colormap[source[frac.floor() & heightmask]];
            dest += vid.width;
            frac += fracstep;
          }
        if (count & 1)
          *dest = colormap[source[frac.floor() & heightmask]];
      }
  }
}


/*
void R_DrawSkyColumn_8()
{
  int count = dc_yh - dc_yl + 1; 

  if (count <= 0)    // Zero length, column does not exceed a pixel.
    return; 
                                 
#ifdef RANGECHECK 
  if ((unsigned)dc_x >= vid.width
      || dc_yl < 0
      || dc_yh >= vid.height) 
    I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x); 
#endif 

  // Framebuffer destination address.
  // Use ylookup LUT to avoid multiply with ScreenWidth.
  // Use columnofs LUT for subwindows?
  register byte *dest = ylookup[dc_yl] + columnofs[dc_x];  

  // Determine scaling, which is the only mapping to be done.
  fixed_t fracstep = dc_iscale; 
  register fixed_t frac = dc_texturemid + (dc_yl-centery)*fracstep; 

  // Inner loop that does the actual texture mapping,
  //  e.g. a DDA-lile scaling.
  // This is as fast as it gets.

  {
    register const byte *source = dc_source;            
    register const lighttable_t *colormap = dc_colormap; 
    register int heightmask = 255;
    if (dc_texheight & heightmask)
      {
        heightmask++;
        fixed_t fheightmask = heightmask;
          
        if (frac < 0)
          while ((frac += fheightmask) <  0);
        else
          while (frac >= fheightmask)
            frac -= fheightmask;
          
        do
          {
            // Re-map color indices from wall texture column
            //  using a lighting/special effects LUT.
            // fheightmask is the Tutti-Frutti fix -- killough
            
            *dest = colormap[source[frac.floor()]];
            dest += vid.width;
            if ((frac += fracstep) >= fheightmask)
              frac -= fheightmask;
          } 
        while (--count);
      }
    else
      {
        while ((count-=2)>=0)   // texture height is a power of 2 -- killough
          {
            *dest = colormap[source[(frac.floor()) & heightmask]];
            dest += vid.width; 
            frac += fracstep;
            *dest = colormap[source[(frac.floor()) & heightmask]];
            dest += vid.width; 
            frac += fracstep;
          }
        if (count & 1)
          *dest = colormap[source[(frac.floor()) & heightmask]];
      }
  }
}
*/


//  The standard Doom 'fuzzy' (blur, shadow) effect
//  originally used for spectres and when picking up the blur sphere
void R_DrawFuzzColumn_8()
{
  // Adjust borders. Low...
  if (!dc_yl)
    dc_yl = 1;

  // .. and high.
  if (dc_yh == viewheight-1)
    dc_yh = viewheight - 2;

  register int count = dc_yh - dc_yl;

  // Zero length.
  if (count < 0)
    return;

#ifdef RANGECHECK
  if (unsigned(dc_x) >= vid.width
      || dc_yl < 0 || dc_yh >= vid.height)
    {
      I_Error ("R_DrawFuzzColumn: %i to %i at %i",
	       dc_yl, dc_yh, dc_x);
    }
#endif

  // Does not work with blocky mode.
  register byte *dest = ylookup[dc_yl] + columnofs[dc_x];

  do
    {
      // Lookup framebuffer, and retrieve
      //  a pixel that is either one column
      //  left or right of the current one.
      // Add index from colormap to index.
      *dest = R.base_colormap[6*256 + dest[fuzzoffset[fuzzpos]]];

      // Clamp table lookup index.
      if (++fuzzpos == FUZZTABLE)
	fuzzpos = 0;

      dest += vid.width;
    } while (count--);
}




void R_DrawShadeColumn_8()
{
  // check out coords for src*
  if((dc_yl<0)||(dc_x>=vid.width))
    return;

  register int count = dc_yh - dc_yl;
  if (count < 0)
    return;

#ifdef RANGECHECK
  if (unsigned(dc_x) >= vid.width
      || dc_yl < 0
      || dc_yh >= vid.height)
    {
      I_Error ( "R_DrawColumn: %i to %i at %i",
		dc_yl, dc_yh, dc_x);
    }
#endif

  // FIXME. As above.
  //src  = ylookup[dc_yl] + columnofs[dc_x+2];
  register byte *dest = ylookup[dc_yl] + columnofs[dc_x];

  // Looks familiar.
  register fixed_t fracstep = dc_iscale;
  register fixed_t frac = dc_texturemid + (dc_yl-centery)*fracstep;

  // Here we do an additional index re-mapping.
  do
    {
      *dest = R.base_colormap[(dc_source[frac.floor()] << 8) + *dest];
      dest += vid.width;
      frac += fracstep;
    } while (count--);
}


//
// I've made an asm routine for the transparency, because it slows down
// a lot in 640x480 with big sprites (bfg on all screen, or transparent
// walls on fullscreen)
//
void R_DrawTranslucentColumn_8()
{
  register int count = dc_yh - dc_yl + 1; 

  if (count <= 0)    // Zero length, column does not exceed a pixel.
    return; 
                                 
#ifdef RANGECHECK 
  if (unsigned(dc_x) >= vid.width
      || dc_yl < 0
      || dc_yh >= vid.height) 
    I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x); 
#endif 

  // Framebuffer destination address.
  // Use ylookup LUT to avoid multiply with ScreenWidth.
  // Use columnofs LUT for subwindows? 
  register byte *dest = ylookup[dc_yl] + columnofs[dc_x];  
  
  // Determine scaling, which is the only mapping to be done.
  register fixed_t fracstep = dc_iscale; 
  register fixed_t frac = dc_texturemid + (dc_yl-centery)*fracstep; 

  // Inner loop that does the actual texture mapping,
  //  e.g. a DDA-lile scaling.
  // This is as fast as it gets.
  {
    register const byte *source = dc_source;            
    //register const lighttable_t *colormap = dc_colormap;
    register int heightmask = dc_texheight-1;
    if (dc_texheight & heightmask)
      {
	heightmask++;
	fixed_t fheightmask = heightmask;
          
	if (frac < 0)
	  while ((frac += fheightmask) <  0);
	else
	  while (frac >= fheightmask)
	    frac -= fheightmask;
                  
	do
	  {
	    // Re-map color indices from wall texture column
	    //  using a lighting/special effects LUT.
	    // fheightmask is the Tutti-Frutti fix -- killough
                      
	    *dest = dc_colormap[dc_transmap[(source[frac.floor()] <<8 ) + *dest]];
	    dest += vid.width;
	    if ((frac += fracstep) >= fheightmask)
	      frac -= fheightmask;
	  } 
	while (--count);
      }
    else
      {
	while ((count-=2)>=0)   // texture height is a power of 2 -- killough
          {
	    *dest = dc_colormap[dc_transmap[(source[frac.floor()] <<8) + *dest]];
	    dest += vid.width; 
	    frac += fracstep;
	    *dest = dc_colormap[dc_transmap[(source[frac.floor()] <<8) + *dest]];
	    dest += vid.width; 
	    frac += fracstep;
          }
	if (count & 1)
	  *dest = dc_colormap[dc_transmap[(source[frac.floor()] <<8) + *dest]];
      }
  }
}


//
//  Draw columns upto 128high but remap the green ramp to other colors
//
void R_DrawTranslatedColumn_8()
{
  register int count = dc_yh - dc_yl;

  if (count < 0)
    return;

#ifdef RANGECHECK
  if (unsigned(dc_x) >= vid.width
      || dc_yl < 0
      || dc_yh >= vid.height)
    {
      I_Error("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
    }
#endif
  // FIXME. As above.
  register byte *dest = ylookup[dc_yl] + columnofs[dc_x];

  // Looks familiar.
  register fixed_t fracstep = dc_iscale;
  register fixed_t frac = dc_texturemid + (dc_yl-centery)*fracstep;

  // Here we do an additional index re-mapping.
  do
    {
      // Translation tables are used to map certain colorramps to other ones, used with PLAY sprites.
      // Thus the "green" ramp of the player 0 sprite is mapped to gray, red, black/indigo.
      *dest = dc_colormap[dc_translation[dc_source[frac.floor()]]];

      dest += vid.width;
      frac += fracstep;
    } while (count--);
}


//SoM: Fog wall.
void R_DrawFogColumn_8()
{
  int count = dc_yh - dc_yl;

  // Zero length, column does not exceed a pixel.
  if (count < 0)
    return;

#ifdef RANGECHECK
  if (unsigned(dc_x) >= vid.width
      || dc_yl < 0
      || dc_yh >= vid.height)
    I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

  // Framebuffer destination address.
  // Use ylookup LUT to avoid multiply with ScreenWidth.
  // Use columnofs LUT for subwindows?
  byte *dest = ylookup[dc_yl] + columnofs[dc_x];

  do
    {
      // Simple. Apply the colormap to what's already on the screen.
      *dest = dc_colormap[*dest];
      dest += vid.width;
    } while (count--);
}



// SoM: This is for 3D floors that cast shadows on walls.
// This function just cuts the column up into sections and calls
// R_DrawColumn_8
void R_DrawColumnShadowed_8()
{
  int realyh = dc_yh;
  int realyl = dc_yl;

  int count = dc_yh - dc_yl;

  // Zero length, column does not exceed a pixel.
  if (count < 0)
    return;

#ifdef RANGECHECK
  if (unsigned(dc_x) >= vid.width
      || dc_yl < 0
      || dc_yh >= vid.height)
    I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

  int bheight = 0;

  // SoM: This runs through the lightlist from top to bottom and cuts up
  // the column accordingly.
  for (int i = 0; i < dc_numlights; i++)
    {
      // If the height of the light is above the column, get the colormap
      // anyway because the lighting of the top should be effected.
      int solid = dc_lightlist[i].flags & FF_CUTSOLIDS;

      int height = dc_lightlist[i].height.floor();
      if (solid)
        bheight = dc_lightlist[i].botheight.floor();
      if (height <= dc_yl)
	{
	  dc_colormap = dc_lightlist[i].rcolormap;
	  if (solid && dc_yl < bheight)
	    dc_yl = bheight;
	  continue;
	}
      // Found a break in the column!
      dc_yh = height;

      if (dc_yh > realyh)
        dc_yh = realyh;
      R_DrawColumn_8();
      if (solid)
        dc_yl = bheight;
      else
        dc_yl = dc_yh + 1;

      dc_colormap = dc_lightlist[i].rcolormap;
    }
  dc_yh = realyh;
  if (dc_yl <= realyh)
    R_DrawColumn_8();
}










//==========================================================================
// SPANS
//==========================================================================

#if defined(USEHIRES)
/// For arbitrary-size Textures.
void R_DrawSpan_8()
{ 
  byte *dest = ylookup[ds_y] + columnofs[ds_x1];
  int count = ds_x2 - ds_x1 + 1; 

  // For efficiency, we software-render only powers-of-two sized textures. Bigger ones are truncated.
  // spot = xbits:ybits  (col-major)

  Uint32 xmask = ((1 << ds_xbits) - 1) << 16; // this way we save one shift in the loop...
  Uint32 ymask = (1 << ds_ybits) - 1;
  int xshift = 16 - ds_ybits;

  while (count)
    {
      int spot = ((ds_xfrac.value() & xmask) >> xshift) | (ds_yfrac.floor() & ymask);
      *dest++ = ds_colormap[ds_source[spot]];
      ds_xfrac += ds_xstep;
      ds_yfrac += ds_ystep;
      count--;
    } 
}
#else
// The Boom version
void R_DrawSpan_8()
{ 
  unsigned spot; 
  unsigned xtemp;
  unsigned ytemp;
                
  register unsigned position = ((ds_xfrac<<10)&0xffff0000) | ((ds_yfrac>>6)&0xffff);
  unsigned step = ((ds_xstep<<10)&0xffff0000) | ((ds_ystep>>6)&0xffff);
                
  byte *source = ds_source;
  byte *colormap = ds_colormap; // TODO unnecessary!
  byte *dest = ylookup[ds_y] + columnofs[ds_x1];

  unsigned count = ds_x2 - ds_x1 + 1; 

  while (count >= 4)
    {
      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      dest[0] = colormap[source[spot]];

      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      dest[1] = colormap[source[spot]];

      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      dest[2] = colormap[source[spot]];
        
      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      dest[3] = colormap[source[spot]];

      dest += 4;
      count -= 4;
    }

  while (count)
    { 
      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      *dest = colormap[source[spot]];
      dest++;
      count--;
    } 
}
#endif


#if defined (USEHIRES)
/// For arbitrary-size Textures.
void R_DrawTranslucentSpan_8()
{ 
  byte *dest = ylookup[ds_y] + columnofs[ds_x1];
  int count = ds_x2 - ds_x1 + 1; 

  // For efficiency, we software-render only powers-of-two sized textures. Bigger ones are truncated.
  // spot = xbits:ybits  (col-major)

  Uint32 xmask = ((1 << ds_xbits) - 1) << 16; // this way we save one shift in the loop...
  Uint32 ymask = (1 << ds_ybits) - 1;
  int xshift = 16 - ds_ybits;

  while (count)
    {
      int spot = ((ds_xfrac.value() & xmask) >> xshift) | (ds_yfrac.floor() & ymask);
      *dest = ds_colormap[ds_transmap[(ds_source[spot] << 8) + *dest]];
      dest++;
      ds_xfrac += ds_xstep;
      ds_yfrac += ds_ystep;
      count--;
    } 
}
#else
// The Boom version
void R_DrawTranslucentSpan_8()
{ 
  register unsigned position;
  unsigned step;

  byte *source;
  byte *colormap;
  byte *transmap;
  byte *dest;
    
  unsigned count;
  unsigned spot; 
  unsigned xtemp;
  unsigned ytemp;
                
  position = ((ds_xfrac.value()<<10)&0xffff0000) | ((ds_yfrac.value()>>6)&0xffff);
  step = ((ds_xstep.value()<<10)&0xffff0000) | ((ds_ystep.value()>>6)&0xffff);
                
  source = ds_source;
  colormap = ds_colormap;
  transmap = ds_transmap;
  dest = ylookup[ds_y] + columnofs[ds_x1];
  count = ds_x2 - ds_x1 + 1; 

  while (count >= 4)
    {
      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      dest[0] = colormap[*(transmap + (source[spot] << 8) + (dest[0]))];

      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      dest[1] = colormap[*(transmap + (source[spot] << 8) + (dest[1]))];
        
      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      dest[2] = colormap[*(transmap + (source[spot] << 8) + (dest[2]))];
        
      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      dest[3] = colormap[*(transmap + (source[spot] << 8) + (dest[3]))];

      dest += 4;
      count -= 4;
    }

  while (count--)
    { 
      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      *dest = colormap[*(transmap + (source[spot] << 8) + (*dest))];
      dest++;
      //count--;
    } 
}
#endif


void R_DrawFogSpan_8()
{
  byte *colormap = ds_colormap;
  //byte *transmap = ds_transmap;
  byte *dest = ylookup[ds_y] + columnofs[ds_x1];       
  unsigned count = ds_x2 - ds_x1 + 1; 
        
  while (count >= 4)
    { 
      dest[0] = colormap[dest[0]];
      dest[1] = colormap[dest[1]];
      dest[2] = colormap[dest[2]];
      dest[3] = colormap[dest[3]];
                
      dest += 4;
      count -= 4;
    }

  while (count--)
    {
      *dest = colormap[*dest];
      dest++;
    }
}



