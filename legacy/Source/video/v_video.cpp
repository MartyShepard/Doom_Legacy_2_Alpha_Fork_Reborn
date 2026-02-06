// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: v_video.cpp 520 2008-01-07 01:37:12Z smite-meister $
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
/// \brief Texture blitting, blitting rectangles between buffers.

#include <ctype.h>

#include "doomdef.h"
#include "console.h"
#include "command.h"

#include "r_data.h"
#include "r_draw.h"

#include "v_video.h"
#include "screen.h"

#include "i_video.h"
#include "z_zone.h"

#include "hardware/oglrenderer.hpp"

using namespace std;

byte *current_colormap; // for applying colormaps to Drawn Textures


//=================================================================
//                    2D Drawing of Textures
//=================================================================


void Material::Draw(float x, float y, int scrn)
{
  if (rendermode == render_opengl)
    {
      if (oglrenderer && oglrenderer->ReadyToDraw())
	// Console tries to use some patches before graphics are
	// initialized. If this is the case, then create the missing
	// texture.
	oglrenderer->Draw2DGraphic_Doom(x, y, this, scrn & V_FLAGMASK);
      return;
    }

  bool masked = tex[0].t->Masked(); // means column_t based, in which case the current algorithm cannot handle y clipping

  int flags = scrn & V_FLAGMASK;
  scrn &= V_SCREENMASK;
  byte *dest_tl = vid.screens[scrn] + vid.scaledofs; // destination top left in LFB

  // location scaling
  if (flags & V_SLOC)
    {
      x *= vid.dupx;
      y *= vid.dupy;
    }

  // size scaling
  fixed_t colfrac = tex[0].xscale;
  fixed_t rowfrac = tex[0].yscale;

  // clipping to LFB
  int x2, y2; // clipped past-the-end coords of the lower right corner in LFB

  if (flags & V_SSIZE)
    {
      x -= leftoffs * vid.dupx;
      y -= topoffs * vid.dupy;
      x2 = min(int(x + worldwidth * vid.dupx), vid.width);
      y2 = masked ? int(y + worldheight * vid.dupy) : min(int(y + worldheight * vid.dupy), vid.height);
      colfrac /= vid.dupx;
      rowfrac /= vid.dupy;
    }
  else
    {
      x -= leftoffs;
      y -= topoffs;
      x2 = min(int(x + worldwidth), vid.width);
      y2 = masked ? int(y + worldheight) : min(int(y + worldheight), vid.height);
    }


  // clipped upper left corner
  int x1 = max(int(x), 0);
  int y1 = masked ? int(y) : max(int(y), 0);

#ifdef RANGECHECK
  if (y < 0 || y2 > vid.height)
    {
      fprintf(stderr, "Patch at (%f,%f) exceeds LFB.\n", x,y);
      // No I_Error abort - what is up with TNT.WAD?
      return;
    }
#endif

  // limits in LFB
  dest_tl += y1*vid.width + x1; // top left
  byte *dest_tr = dest_tl + x2 - x1; // top right, past-the-end
  byte *dest_bl = dest_tl + (y2-y1)*vid.width; // bottom left, past-the-end

  // starting location in texture space
  fixed_t col;
  if (flags & V_FLIPX)
    {
      col = (x2 - x - 1)*colfrac; // a little back from right edge
      colfrac = -colfrac; // reverse stride
    }
  else
    {
      col = (x1 - x)*colfrac;
    }

  fixed_t row = (y1 - y)*rowfrac;


  tex[0].t->Draw(dest_tl, dest_tr, dest_bl, col, row, colfrac, rowfrac, flags);
}


void PatchTexture::Draw(byte *dest_tl, byte *dest_tr, byte *dest_bl,
			fixed_t col, fixed_t row, fixed_t colfrac, fixed_t rowfrac, int flags)
{
  patch_t *p = GeneratePatch();

  for ( ; dest_tl < dest_tr; col += colfrac, dest_tl++)
    {
#warning FIXME LFB top/bottom limits
      post_t *post = reinterpret_cast<post_t *>(patch_data + p->columnofs[col.floor()]);

      // step through the posts in a column
      while (post->topdelta != 0xff)
	{
	  // step through the posts in a column
	  byte *dest = dest_tl + (post->topdelta/rowfrac).floor()*vid.width;
	  byte *source = post->data;
	  int count = (post->length/rowfrac).floor();

	  fixed_t row = 0;
	  while (count--)
	    {
	      byte pixel = source[row.floor()];

	      // the compiler is supposed to optimize the ifs out of the loop
	      if (flags & V_MAP)
		pixel = current_colormap[pixel];

	      if (flags & V_TL)
		pixel = transtables[0][(pixel << 8) + *dest];

	      *dest = pixel;
	      dest += vid.width;
	      row += rowfrac;
	    }
	  post = reinterpret_cast<post_t *>(&post->data[post->length + 1]); // next post
	}
    }
}



void LumpTexture::Draw(byte *dest_tl, byte *dest_tr, byte *dest_bl,
		       fixed_t col, fixed_t startrow, fixed_t colfrac, fixed_t rowfrac, int flags)
{
  byte *base = GetData(); // in col-major order!

  for ( ; dest_tl < dest_tr; col += colfrac, dest_tl++, dest_bl++)
    {
      byte *source = base + height * col.floor();

      fixed_t row = startrow;
      for (byte *dest = dest_tl; dest < dest_bl; row += rowfrac, dest += vid.width)
	{
	  byte pixel = source[row.floor()];

	  // the compiler is supposed to optimize the ifs out of the loop
	  if (flags & V_MAP)
	    pixel = current_colormap[pixel];

	  if (flags & V_TL)
	    pixel = transtables[0][(pixel << 8) + *dest];

	  *dest = pixel;
	}
    }
}


void Material::DrawFill(int x, int y, int w, int h)
{
  if (rendermode == render_opengl)
    {
      if (oglrenderer && oglrenderer->ReadyToDraw()) 
	oglrenderer->Draw2DGraphicFill_Doom(x, y, w, h, this);
      return;
    }
  
  tex[0].t->DrawFill(x, y, w, h);
}

// Fills a box of pixels using a repeated texture as a pattern,
// scaled to screen size.
void LumpTexture::DrawFill(int x, int y, int w, int h)
{
  byte *flat = GetData(); // in col-major order
  byte *base_dest = vid.screens[0] + y*vid.dupy*vid.width + x*vid.dupx + vid.scaledofs;

  w *= vid.dupx;
  h *= vid.dupy;

  fixed_t dx = fixed_t(1) / vid.dupx;
  fixed_t dy = fixed_t(1) / vid.dupy;

  fixed_t xfrac = 0;
  for (int u=0; u<w; u++, xfrac += dx, base_dest++)
    {
      fixed_t yfrac = 0;
      byte *src = flat + (xfrac.floor() % width) * height;
      byte *dest = base_dest;
      for (int v=0; v<h; v++, yfrac += dy)
        {
          *dest = src[yfrac.floor() % height];
	  dest += vid.width;
        }
    }
}



//=================================================================
//                         Blitting
//=================================================================

/// Copies a rectangular area from one screen buffer to another
void V_CopyRect(float srcx, float srcy, int srcscrn,
                float width, float height,
                float destx, float desty, int destscrn)
{
  // WARNING don't mix
  if ((srcscrn & V_SLOC) || (destscrn & V_SLOC))
    {
      srcx*=vid.dupx;
      srcy*=vid.dupy;
      width*=vid.dupx;
      height*=vid.dupy;
      destx*=vid.dupx;
      desty*=vid.dupy;
    }
  srcscrn &= 0xffff;
  destscrn &= 0xffff;

#ifdef RANGECHECK
  if (srcx<0
      ||srcx+width >vid.width
      || srcy<0
      || srcy+height>vid.height
      ||destx<0||destx+width >vid.width
      || desty<0
      || desty+height>vid.height
      || (unsigned)srcscrn>4
      || (unsigned)destscrn>4)
    I_Error ("Bad V_CopyRect %d %d %d %d %d %d %d %d", srcx, srcy,
             srcscrn, width, height, destx, desty, destscrn);
#endif

  int sx = int(srcx); int sy = int(srcy);
  int dx = int(destx); int dy = int(desty);
  int w = int(width); int h = int(height);

  byte *src = vid.screens[srcscrn] + vid.width*sy + sx + vid.scaledofs;
  byte *dest = vid.screens[destscrn] + vid.width*dy + dx + vid.scaledofs;

  for (; h>0 ; h--)
    {
      memcpy (dest, src, w);
      src += vid.width;
      dest += vid.width;
    }
}


// Copy a rectangular area from one bitmap to another (8bpp)
void VID_BlitLinearScreen(byte *src, byte *dest, int width, int height,
                          int srcrowbytes, int destrowbytes)
{
  if (srcrowbytes == destrowbytes)
    memcpy(dest, src, srcrowbytes * height);
  else
    {
      while (height--)
        {
          memcpy(dest, src, width);

          dest += destrowbytes;
          src += srcrowbytes;
        }
    }
}





//======================================================================
//                     Misc. drawing stuff
//======================================================================


//  Fills a box of pixels with a single color, NOTE: scaled to screen size
void V_DrawFill(int x, int y, int w, int h, int c)
{
  if (rendermode != render_soft)
    {
      OGLRenderer::DrawFill(x, y, w, h, c);
      return;
    }

  byte *dest = vid.screens[0] + y*vid.dupy*vid.width + x*vid.dupx + vid.scaledofs;

  w *= vid.dupx;
  h *= vid.dupy;

  for (int v=0 ; v<h ; v++, dest += vid.width)
    for (int u=0 ; u<w ; u++)
      dest[u] = c;
}




//
//  Fade all the screen buffer, so that the menu is more readable,
//  especially now that we use the small hufont in the menus...
//
void V_DrawFadeScreen()
{
  if (rendermode != render_soft)
    {
      OGLRenderer::FadeScreenMenuBack(0x01010160, 0);  //faB: hack, 0 means full height :o
      return;
    }

  int w = vid.width>>2;

  byte  p1, p2, p3, p4;
  byte *fadetable = R_GetFadetable(0)->colormap + 16*256;

  for (int y=0 ; y<vid.height ; y++)
    {
      int *buf = (int *)(vid.screens[0] + vid.width*y);
      for (int x=0 ; x<w ; x++)
        {
          int quad = buf[x];
          p1 = fadetable[quad&255];
          p2 = fadetable[(quad>>8)&255];
          p3 = fadetable[(quad>>16)&255];
          p4 = fadetable[quad>>24];
          buf[x] = (p4<<24) | (p3<<16) | (p2<<8) | p1;
        }
    }

#ifdef _16bitcrapneverfinished
    else
    {
        w = vid.width;
        for (y=0 ; y<vid.height ; y++)
        {
            wput = (short*) (vid.screens[0] + vid.width*y);
            for (x=0 ; x<w ; x++)
            {
                *wput = (*wput>>1) & 0x3def;
                wput++;
            }
        }
    }
#endif
}


/// Simple translucence with one color, coords are true LFB coords
void V_DrawFadeConsBack(float x1, float y1, float x2, float y2)
{
  if (rendermode!=render_soft)
    {
      //FIXME OGLRenderer::FadeScreenMenuBack(0x00500000, y2);
      return;
    }

  if (vid.BytesPerPixel == 1)
    {
      for (int y=int(y1); y<int(y2); y++)
        {
	  byte *buf = vid.screens[0] + vid.width*y;
          for (int x=int(x1) ; x<int(x2) ; x++)
            buf[x] = greenmap[buf[x]];
        }
    }
  else
    {
      int w = int(x2-x1);
      for (int y=int(y1) ; y<int(y2) ; y++)
        {
	  short *wput = (short*)(vid.screens[0] + vid.width*y) + int(x1);
          for (int x=0 ; x<w ; x++)
	    {
	      *wput = ((*wput&0x7bde) + (15<<5)) >>1;
	      wput++;
	    }
        }
    }
}
