// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: png.cpp 536 2009-06-29 06:46:13Z smite-meister $
//
// Copyright (C) 2004-2006 by DooM Legacy Team.
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
/// \brief PNG Textures and screenshots

#ifndef __APPLE_CC__
#include <malloc.h>
#endif
#include <png.h>

#include "doomdef.h"
#include "r_data.h"
#include "w_wad.h"
#include "z_zone.h"

//=========================================
//     callbacks
//=========================================

/// helper struct for reading PNGs from memory
struct PNG_read_t
{
  byte *pos; // current position
  size_t remaining; // bytes of data remaining
} PNG_r;



static void PNG_reader(png_struct *png_p, png_byte *dest, png_size_t len)
{
  // get data i/o struct (instead we cheat using the global struct directly)
  //PNG_read_t *r = (PNG_read_t *)png_get_io_ptr(png_ptr);

  // see if there is enought data left
  if (PNG_r.remaining < len)
    png_error(png_p, "Read Error");

  for (unsigned i=0; i<len; i++)
    dest[i] = PNG_r.pos[i];

  PNG_r.pos += len;
  PNG_r.remaining -= len;
}


static void PNG_error(png_struct *png_p, const char *msg)
{
  I_Error("PNG error: %s", msg);
}


static void PNG_warning(png_struct *png_p, const char *msg)
{
  I_Error("PNG warning: %s", msg);
}




//=========================================
//     PNGTexture implementation
//=========================================


PNGTexture::PNGTexture(const char *n, int l)
  : LumpTexture(n, l, 0, 0)
{
  ReadData(false, false); // read the header (height, width)
  InitializeTexture();
}



byte *PNGTexture::GetData()
{
  if (!pixels)
    ReadData(true, true);

  return pixels;
}


void PNGTexture::GLGetData()
{
  if (!pixels)
    ReadData(true, false);
}



bool PNGTexture::ReadData(bool read_image, bool sw_rend)
{
  unsigned i, j;
  png_struct *png_p = NULL;
  png_info  *info_p = NULL;

  png_p = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, PNG_error, PNG_warning);
  if (!png_p)
    goto fail;

  info_p = png_create_info_struct(png_p);
  if (!info_p)
    goto fail;

  byte *tmpdata; // for raw PNG data

  // no unknown chunk callback or changes in default handling
  // no row read callback

  PNG_r.pos = tmpdata = static_cast<byte *>(fc.CacheLumpNum(lump, PU_TEXTURE));
  PNG_r.remaining = fc.LumpLength(lump);

  // nasty longjmp error recovery
  if (setjmp(png_jmpbuf(png_p)))
    {
      I_Error("PNG texture %s corrupted.\n", name);
      Z_Free(tmpdata); // free raw data
      goto fail;
    }

  png_set_read_fn(png_p, &PNG_r, PNG_reader);

  // read the PNG info chunks
  png_read_info(png_p, info_p);

  png_uint_32 w, h;
  int bit_depth, color_type;
  png_get_IHDR(png_p, info_p, &w, &h, &bit_depth, &color_type, NULL, NULL, NULL);

  //I_OutputMsg("PNG header: %s: %ld, %ld, %d, %d, %ld, chan:%d\n", name, w, h, bit_depth, color_type, png_get_rowbytes(png_p, info_p), info_p->channels);

  width = w;
  height = h;

  if (!read_image)
    {
      png_destroy_read_struct(&png_p, &info_p, NULL);
      Z_Free(tmpdata); // free the raw data
      return false;
    }
  else
    {
      double gamma, screen_gamma;

      // gamma (pc monitor in a dark room (as Doom should be played!))
      screen_gamma = 2.0;
      if (png_get_gAMA(png_p, info_p, &gamma))
        png_set_gamma(png_p, screen_gamma, gamma);
      else
        png_set_gamma(png_p, screen_gamma, 0.45455);

      // get 8-bit data
      if (bit_depth == 16)
        {
          png_set_strip_16(png_p);
#ifdef __LITTLE_ENDIAN__
          //png_set_swap(png_p);
#endif
        }

      // expand to rgb
      if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_p);
      else if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_gray_1_2_4_to_8(png_p);

      // grayscale to rgb
      if (color_type == PNG_COLOR_TYPE_GRAY ||
          color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_p);


      // one byte per pixel
      if (bit_depth < 8)
        png_set_packing(png_p);

      // no fillers
      // png_set_dither for palette conversion?
      // no interlacing

      /*
        // update the info struct
        png_read_update_info(png_p, info_p);

        png_get_IHDR(png_p, info_p, &w, &h, &bit_depth, &color_type, NULL, NULL, NULL);
        printf("PNG header: %ld, %ld, %d, %d, %ld, chan:%d\n", w, h, bit_depth, color_type,
        png_get_rowbytes(png_p, info_p), info_p->channels);
      */

      // read image data
      byte **row_pointers = static_cast<byte **>(alloca(h * sizeof(byte*)));

      if (sw_rend)
	{
	  // software: need indexed col-major data

	  // get rid of alpha channel
	  //if (color_type & PNG_COLOR_MASK_ALPHA)
	  png_set_strip_alpha(png_p);

	  // read and convert to doom palette
	  RGB_t *rgb_buf = static_cast<RGB_t *>(Z_Malloc(w * h * sizeof(RGB_t), PU_TEXTURE, NULL));

	  for (i=0, j=0; i<h; i++, j += w)
	    row_pointers[i] = reinterpret_cast<byte *>(rgb_buf + j);

	  png_read_image(png_p, row_pointers);
	  png_destroy_read_struct(&png_p, &info_p, NULL);
	  // discard any end chunks (comments etc.)
	  Z_Free(tmpdata); // free the raw data

	  // now convert RGB_t to current palette...
	  unsigned len = w * h;
	  Z_Malloc(len, PU_TEXTURE, (void **)(&pixels));

	  // this takes some time, so better do it only once (never flush PNG textures automatically)
	  // colormap and transpose to col-major order
	  unsigned dest = 0;
	  for (i=0; i<len; i++)
	    {
	      pixels[dest] = NearestColor(rgb_buf[i].r, rgb_buf[i].g, rgb_buf[i].b);
	      dest += height;
	      if (dest >= len)
		dest -= len - 1; // next column
	    }

	  Z_Free(rgb_buf); // free the RGB data
	}
      else
	{
	  // OpenGL: need RGBA row-major data

	  // transparency chunk to alpha channel
	  if (png_get_valid(png_p, info_p, PNG_INFO_tRNS))
	    png_set_tRNS_to_alpha(png_p);

	  if (!(color_type & PNG_COLOR_MASK_ALPHA))
	    png_set_filler(png_p, 0xFF, PNG_FILLER_AFTER); // add opaque alpha channel if none present

	  // read it to memory owned by pixels
	  Z_Malloc(w * h * sizeof(RGBA_t), PU_TEXTURE, (void **)(&pixels));

	  for (i=0, j=0; i<h; i++, j += w*sizeof(RGBA_t))
	    row_pointers[i] = &pixels[j];

	  png_read_image(png_p, row_pointers);
	  png_destroy_read_struct(&png_p, &info_p, NULL);
	  // discard any end chunks (comments etc.)
	  Z_Free(tmpdata); // free the raw data
	  gl_format = GL_RGBA;
	}
    }

  return true; // success

 fail:
  png_destroy_read_struct(&png_p, &info_p, NULL);
  I_Error("Could not read PNG texture '%s'.\n", name);
  return false;
}



bool WritePNGScreenshot(FILE *fp, byte *lfb, int width, int height, RGB_t *pal)
{
  int i, j;

  png_struct *png_p = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, PNG_error, PNG_warning);
  if (!png_p)
    return false;

  png_info *info_p = png_create_info_struct(png_p);
  if (!info_p)
    {
      png_destroy_write_struct(&png_p, NULL);
      return false;
    }

  // error handler
  if (setjmp(png_jmpbuf(png_p)))
    {
      I_Error("Error writing PNG screenshot.\n");
      png_destroy_write_struct(&png_p, &info_p);
      return false;
    }

  // output fn
  png_init_io(png_p, fp);

  // header info
  png_set_IHDR(png_p, info_p, width, height,
	       8, PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_ADAM7,
	       PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  // define palette
  png_color palette[256];
  for (i=0; i<256; i++)
    {
      palette[i].red   = pal[i].r;
      palette[i].green = pal[i].g;
      palette[i].blue  = pal[i].b;
    }
  png_set_PLTE(png_p, info_p, palette, 256);

  png_set_gAMA(png_p, info_p, 1/2.2); // TODO tweak, use actual gamma value
  //png_set_tIME(png_p, info_p, &modtime);

  // text metadata
  png_text text[3];
  text[0].compression = PNG_TEXT_COMPRESSION_NONE;
  text[0].key = "Title";
  text[0].text = "Screenshot";

  text[1].compression = PNG_TEXT_COMPRESSION_NONE;
  text[1].key = "Author";
  text[1].text = LEGACY_VERSION_BANNER;

  /*
  text[2].compression = PNG_TEXT_COMPRESSION_NONE;
  text[2].key = "Description";
  text[2].text = "";
  */

  png_set_text(png_p, info_p, text, 2);

  byte **row_pointers = static_cast<byte **>(alloca(height * sizeof(byte*)));
  for (i=0, j=0; i<height; i++, j += width)
    row_pointers[i] = &lfb[j];

  png_set_rows(png_p, info_p, row_pointers);
  png_write_png(png_p, info_p, PNG_TRANSFORM_IDENTITY, NULL);


  png_write_end(png_p, info_p);
  png_destroy_write_struct(&png_p, &info_p);

  return true;
}
