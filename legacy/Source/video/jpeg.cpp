// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2007 by DooM Legacy Team.
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
/// \brief JPEG/JFIF Textures.

#include <stdio.h>
// some broken versions of the libjpeg headers are missing the extern "C"
extern "C"
{
#include <jpeglib.h>
#include <jerror.h>
}
#include "doomdef.h"
#include "r_data.h"
#include "screen.h"
#include "w_wad.h"
#include "z_zone.h"


JPEGTexture::JPEGTexture(const char *n, int l)
  : LumpTexture(n, l, 0, 0)
{
  ReadData(false, false); // read the header (height, width)
  InitializeTexture();
}


byte *JPEGTexture::GetData()
{
  if (!pixels)
    ReadData(true, true);

  return pixels;
}


void JPEGTexture::GLGetData()
{
  if (!pixels)
    ReadData(true, false);
}


/// fatal error handler
static void Legacy_JPEG_error_exit(j_common_ptr cinfo)
{
  cinfo->err->output_message(cinfo);
  throw -1; // called on a fatal error, must not simply return;
}


/// print error messages
static void Legacy_JPEG_output_message(j_common_ptr cinfo)
{
  char buffer[JMSG_LENGTH_MAX];

  cinfo->err->format_message(cinfo, buffer);
  CONS_Printf("JPEG error: %s\n", buffer);
}



/// Reads JPEGs from lumps
class JPEG_LumpSourceManager : public jpeg_source_mgr
{
protected:
  typedef JPEG_LumpSourceManager mytype;

  byte *data;

  /// Called by jpeg_read_header before any data is actually read.
  static void Legacy_JPEG_init_source(j_decompress_ptr cinfo) {}

  /// Called whenever buffer is emptied.
  static boolean Legacy_JPEG_fill_input_buffer(j_decompress_ptr cinfo)
  {
    ERREXIT(cinfo, JWRN_JPEG_EOF); // calls error_exit, which throws an exception.

    /*
    // TODO if we want to handle broken JPEGs more cleanly...
    WARNMS(cinfo, JWRN_JPEG_EOF);

    // Insert a fake EOI marker
    mytype *src = reinterpret_cast<mytype*>(cinfo->src);

    src->buffer[0] = (JOCTET) 0xFF;
    src->buffer[1] = (JOCTET) JPEG_EOI;
    int nbytes = 2;

    src->next_input_byte = src->buffer;
    src->bytes_in_buffer = nbytes;
    */
    return TRUE;
  }

  /// Skip over a potentially large amount of uninteresting data (such as an APPn marker).
  static void Legacy_JPEG_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
  {
    mytype *src = reinterpret_cast<mytype*>(cinfo->src);

    // NOTE: If we did buffered reading and num_bytes was big enough, it would make
    // sense to seek over some data altogether without reading it.
    if (num_bytes > 0)
      {
	while (num_bytes > static_cast<long>(src->bytes_in_buffer))
	  {
	    num_bytes -= src->bytes_in_buffer;
	    src->fill_input_buffer(cinfo); // currently this causes an error
	  }
	src->next_input_byte += num_bytes;
	src->bytes_in_buffer -= num_bytes;
      }
  }

  /// Called by jpeg_finish_decompress after all data has been read.  Often a no-op.
  static void Legacy_JPEG_term_source(j_decompress_ptr cinfo) {}

public:
  /// constructor
  JPEG_LumpSourceManager(j_decompress_ptr cinfo, int lump)
  {
    cinfo->src = this;

    // set the callbacks
    init_source       = Legacy_JPEG_init_source;
    fill_input_buffer = Legacy_JPEG_fill_input_buffer;
    skip_input_data   = Legacy_JPEG_skip_input_data;
    resync_to_restart = jpeg_resync_to_restart; // original
    term_source       = Legacy_JPEG_term_source;

    // no fancy buffered reading, we do it all at once
    next_input_byte = data = static_cast<byte *>(fc.CacheLumpNum(lump, PU_TEXTURE));
    bytes_in_buffer = fc.LumpLength(lump);

    /*
    if (bytes_in_buffer == 0)
      ERREXIT(cinfo, JERR_INPUT_EMPTY);
      // inside try block?
    */
  }

  /// destructor
  ~JPEG_LumpSourceManager()
  {
    if (data)
      Z_Free(data);
  }
};



/// Read and decompress texture header and data
bool JPEGTexture::ReadData(bool read_image, bool sw_rend)
{
  int i, j;

  // decompression data struct
  jpeg_decompress_struct cinfo;

  // override some error handlers
  jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error(&jerr);
  cinfo.err->error_exit = Legacy_JPEG_error_exit;
  cinfo.err->output_message = Legacy_JPEG_output_message;

  // initialize
  jpeg_create_decompress(&cinfo);

  // create the data source
  JPEG_LumpSourceManager xxx(&cinfo, lump);

  try
    {
      jpeg_read_header(&cinfo, TRUE); // accept only full JPEG files

      width  = cinfo.image_width;
      height = cinfo.image_height;
      if (read_image)
	{
	  // set parameters for decompression
	  cinfo.out_color_space = JCS_RGB;

	  if (sw_rend)
	    {
	      // software: need indexed col-major data

	      // NOTE: libjpeg can quantize RGB images into a palette. It achieves better tone reproduction
	      // than our humble NearestColor(), but sometimes causes ugly pixel artifacts...
	      // Probably the Doom palette is to blame.
//#define USE_LIBJPEG_QUANTIZATION
#ifdef USE_LIBJPEG_QUANTIZATION
	      cinfo.quantize_colors = TRUE;
	      cinfo.actual_number_of_colors = 256; // number of colors in the color map.

	      // ugly HACK to convert our palette to a form libjpeg understands
	      byte temp_pal[3][256]; // transpose the palette
	      for (int k=0; k<256; k++)
		{
		  temp_pal[0][k] = vid.palette[k].r;
		  temp_pal[1][k] = vid.palette[k].g;
		  temp_pal[2][k] = vid.palette[k].b;
		}

	      byte *hack_palette[3];
	      for (i=0; i<3; i++)
		hack_palette[i] = temp_pal[i];

	      cinfo.colormap = hack_palette; // palette to use
#endif
	      jpeg_start_decompress(&cinfo);

	      // scanline length in bytes as output by libjpeg
	      int row_stride = cinfo.output_width * cinfo.output_components;

	      /*
	      if (row_stride != width)
		I_Error("bug in JPEG handling\n");
	      */

	      // allocate pixels
	      Z_Malloc(height * width, PU_TEXTURE, (void **)(&pixels));

#ifdef USE_LIBJPEG_QUANTIZATION
	      byte *buffer = static_cast<byte *>(alloca(row_stride));
	      byte **row_pointer = &buffer;
#else
	      RGB_t *buffer = static_cast<RGB_t *>(alloca(row_stride));
	      byte **row_pointer = reinterpret_cast<byte**>(&buffer);
#endif

	      int row = 0;
	      while (cinfo.output_scanline < cinfo.output_height)
		{
		  int num_scanlines = jpeg_read_scanlines(&cinfo, row_pointer, 1);
		  // write scanline transposed
		  byte *dest = pixels + row;
		  for (i=0; i < width; i++)
		    {
#ifdef USE_LIBJPEG_QUANTIZATION
		      *dest = buffer[i];
#else
		      *dest = NearestColor(buffer[i].r, buffer[i].g, buffer[i].b);
#endif
		      dest += height;
		    }

		  row += num_scanlines;
		}
	    }
	  else
	    {
	      // OpenGL: need RGB row-major data
	      jpeg_start_decompress(&cinfo);

	      unsigned row_stride = cinfo.output_width * cinfo.output_components;
	      if (row_stride != width*sizeof(RGB_t))
		I_Error("bug in JPEG handling!!\n");

	      // allocate pixels
	      Z_Malloc(height * row_stride, PU_TEXTURE, (void **)(&pixels));

	      // read image data
	      byte *row_pointers[height];
	      for (i=0, j=0; i < height; i++, j += row_stride)
		row_pointers[i] = &pixels[j];

	      int row = 0;
	      while (cinfo.output_scanline < cinfo.output_height)
		{
		  int num_scanlines = jpeg_read_scanlines(&cinfo, &row_pointers[row], height);
		  row += num_scanlines;
		}

	      gl_format = GL_RGB;
	    }

	  jpeg_finish_decompress(&cinfo);
	}

      jpeg_destroy_decompress(&cinfo);
    }
  catch (int)
    {
      CONS_Printf(" in texture %s\n", name);
      jpeg_destroy_decompress(&cinfo);
      I_Error("Error reading JPEG texture.\n");
      return false;
    }

  return true;
}
