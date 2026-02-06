// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_data.cpp 570 2009-11-27 23:57:47Z smite-meister $
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
/// \brief Texture generation and caching. Colormap loading.

#include <math.h>
#include <png.h>

#include "doomdef.h"
#include "doomdata.h"
#include "command.h"
#include "cvars.h"
#include "parser.h"

#include "g_game.h"
#include "g_map.h"
#include "g_actor.h"

#include "i_video.h"
#include "r_data.h"
#include "r_main.h"
#include "r_draw.h"
#include "m_swap.h"

#include "w_wad.h"
#include "z_zone.h"

#include "hardware/oglshaders.h"
#include "hardware/hwr_render.h"

/* OpenGL Init --------------------
 * liegt in oglinit.h
		#include <GL/glu.h>
*/
#include "Hardware/ogl_init.h"
/* OpenGL Init End ------------- */


const byte *R_BuildGammaTable();
static int R_TransmapNumForName(const char *name);


// TODO after exiting a mapcluster, flush unnecessary graphics...
//  R_LoadTextures ();
//  R_FlushTextureCache();
//  R_ClearColormaps();


//faB: for debugging/info purpose
int             flatmemory;
int             spritememory;
int             texturememory;


//faB: highcolor stuff
short    color8to16[256];       //remap color index to highcolor rgb value
short*   hicolormaps;           // test a 32k colormap remaps high -> high


//==================================================================
//   Texture cache
//==================================================================


/// Generates a Texture from a single data lump, deduces format.
Texture *texture_cache_t::Load(const char *name)
{
  int lump = fc.FindNumForName(name);
  if (lump < 0)
    return NULL;

  return LoadLump(name, lump);
}

/// Creates a Texture with a given name from a given lump.
Texture *texture_cache_t::LoadLump(const char *name, int lump)
{
  // Because Doom datatypes contain no magic numbers, we have to rely on heuristics to deduce the format...
  Texture *t;

  byte data[8];
  fc.ReadLumpHeader(lump, &data, sizeof(data));
  int size = fc.LumpLength(lump);
  int name_len = strlen(name);

  // Good texture formats have magic numbers. First check if they exist.
  if (!png_sig_cmp(data, 0, sizeof(data)))
    {
      // it's PNG
      t = new PNGTexture(name, lump);
    }
  else if (data[0] == 0xff && data[1] == 0xd8 && data[2] == 0xff && data[3] == 0xe0)
    {
      // it's JPEG/JFIF
      t = new JPEGTexture(name, lump);
    }
  // For TGA, we use the filename extension.
  else if (name_len > 4 && !strcasecmp(&name[name_len - 4], ".tga"))
    {
      // it's TGA
      t = new TGATexture(name, lump);
    }
  // then try some common sizes for raw picture lumps
  else if (size == 320*200)
    {
      // Heretic/Hexen fullscreen picture
      t = new LumpTexture(name, lump, 320, 200);
    }
  else if (size == 4)
    {
      // God-damn-it! Heretic "F_SKY1" tries to be funny!
      t = new LumpTexture(name, lump, 2, 2);
    }
  else if (!strcasecmp(name, "AUTOPAGE"))
    {
      // how many different annoying formats can you invent, anyway?
      if (size % 320)
	I_Error("Size of AUTOPAGE (%d bytes) must be a multiple of 320!\n", size);
      t = new LumpTexture(name, lump, 320, size/320);
    }
  else if (data[2] == 0 && data[6] == 0 && data[7] == 0)
    {
      // likely a pic_t (the magic number is inadequate)
      CONS_Printf("A pic_t image '%s' was found, but this format is no longer supported.\n", name); // root 'em out!
      return NULL;
    }
  else
    {
      // finally assume a patch_t
      t = new PatchTexture(name, lump);
    }

  /*
    else
      {
        CONS_Printf(" Unknown texture format: lump '%8s' in the file '%s'.\n", name, fc.Name(lump >> 16));
        return false;
      }
  */

  return t; // cache_t::Get() does the subsequent inserting into hash_map...
}


texture_cache_t textures;


//==================================================================
//  Utilities
//==================================================================


/// Store lists of lumps for F_START/F_END etc.
struct lumplist_t
{
  int wadfile;
  int firstlump;
  int numlumps;
};


static int R_CheckNumForNameList(const char *name, lumplist_t *ll, int listsize)
{
  for (int i = listsize - 1; i > -1; i--)
    {
      int lump = fc.FindNumForNameFile(name, ll[i].wadfile, ll[i].firstlump);
      if ((lump & 0xffff) >= (ll[i].firstlump + ll[i].numlumps) || lump == -1)
        continue;
      else
        return lump;
    }
  return -1;
}



// given an RGB triplet, returns the index of the nearest color in
// the current "zero"-palette using a quadratic distance measure.
// Thanks to quake2 source!
byte NearestColor(byte r, byte g, byte b)
{
  int bestdistortion = 256 * 256 * 4;
  int bestcolor = 0;

  for (int i = 0; i < 256; i++)
    {
      int dr = r - vid.palette[i].r;
      int dg = g - vid.palette[i].g;
      int db = b - vid.palette[i].b;
      int distortion = dr*dr + dg*dg + db*db;

      if (distortion < bestdistortion)
        {
          if (!distortion)
            return i;

          bestdistortion = distortion;
          bestcolor = i;
        }
    }

  return bestcolor;
}


// create best possible colormap from one palette to another
static byte *R_CreatePaletteConversionColormap(int wadnum)
{
  int i = fc.FindNumForNameFile("PLAYPAL", wadnum);
  if (i == -1)
    {
      // no palette available
      return NULL;
    }

  if (fc.LumpLength(i) < int(256 * sizeof(RGB_t)))
    I_Error("Bad PLAYPAL lump in file %d!\n", wadnum);

  byte *colormap = static_cast<byte*>(Z_Malloc(256, PU_STATIC, NULL));
  RGB_t *pal = static_cast<RGB_t*>(fc.CacheLumpNum(i, PU_STATIC));
  const byte *gamma_table = R_BuildGammaTable();

  for (i=0; i<256; i++)
    colormap[i] = NearestColor(gamma_table[pal[i].r], gamma_table[pal[i].g], gamma_table[pal[i].b]);

  Z_Free(pal);
  return colormap;
}


// applies a given colormap to a patch_t
static void R_ColormapPatch(patch_t *p, byte *colormap)
{
 for (int i=0; i<p->width; i++)
   {
     post_t *post = reinterpret_cast<post_t*>(reinterpret_cast<byte*>(p) + p->columnofs[i]);

     while (post->topdelta != 0xff)
       {
         int count = post->length;
         for (int j=0; j<count; j++)
           post->data[j] = colormap[post->data[j]];

         post = reinterpret_cast<post_t*>(&post->data[post->length + 1]);
       }
   }
}



//==================================================================
//  Texture
//==================================================================

Texture::Texture(const char *n, int l)
  : cacheitem_t(n)
{
  pixels = NULL;
  lump = l;
  leftoffs = topoffs = 0;
  width = height = 0;
  w_bits = h_bits = 0;

  gl_format = 0;
  gl_id = NOTEXTURE;
}


Texture::~Texture()
{
  ClearGLTexture();

  if (pixels)
    Z_Free(pixels);
}


// This basic version of the virtual method is used for native indexed col-major formats.
void Texture::GLGetData()
{
  if (!pixels)
    {
      GetData(); // reads data into pixels as indexed, col-major
      byte *index_in = pixels;

      RGBA_t *result = static_cast<RGBA_t*>(Z_Malloc(sizeof(RGBA_t)*width*height, PU_TEXTURE, NULL));
      RGB_t *palette = static_cast<RGB_t*>(fc.CacheLumpNum(materials.GetPaletteLump(lump >> 16), PU_DAVE));

      for (int i=0; i<width; i++)
	for (int j=0; j<height; j++)
	  {
	    byte curbyte = *index_in++;
	    RGB_t  *rgb_in = &palette[curbyte];
	    RGBA_t *rgba_out = &result[j*width + i]; // transposed
	    rgba_out->red   = rgb_in->r;
	    rgba_out->green = rgb_in->g;
	    rgba_out->blue  = rgb_in->b;
	    if (curbyte == TRANSPARENTPIXEL)
	      rgba_out->alpha = 0;
	    else
	      rgba_out->alpha = 255;
	  }

      Z_Free(palette);
      // replace pixels by the RGBA row-major version
      Z_Free(pixels);
      pixels = reinterpret_cast<byte*>(result);
      gl_format = GL_RGBA;
    }
}


/// Creates a GL texture.
/// Uses the virtualized GLGetData().
GLuint Texture::GLPrepare()
{
  if (gl_id == NOTEXTURE)
    {
      GLGetData();

      glGenTextures(1, &gl_id);
      glBindTexture(GL_TEXTURE_2D, gl_id);
      // default params
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, gl_format, GL_UNSIGNED_BYTE, pixels);

      //  CONS_Printf("Created GL texture %d for %s.\n", gl_id, name);
      Z_Free(pixels); // no longer needed
      pixels = NULL;
    }

  return gl_id;
}


/// Returns true if texture existed and was deleted. False otherwise.
bool Texture::ClearGLTexture()
{
  if (gl_id != NOTEXTURE)
    {
      glDeleteTextures(1, &gl_id);
      gl_id = NOTEXTURE;
      return true;
    }
  return false;
}



// Basic form for GetColumn, uses virtualized GetData.
byte *Texture::GetColumn(fixed_t fcol)
{
  int col = fcol.floor() % width;

  if (col < 0)
    col += width; // wraparound

  return GetData() + col * height;
}


//==================================================================
//  LumpTexture
//==================================================================


// Flats etc.
LumpTexture::LumpTexture(const char *n, int l, int w, int h)
  : Texture(n, l)
{
  width = w;
  height = h;
  InitializeTexture();
}



/// This method handles the Doom flats and raw pages for OpenGL.
void LumpTexture::GLGetData()
{
  if (!pixels)
    {
      // load indexed data
      int len = fc.LumpLength(lump); // len must be width*height (or more...)
      byte *temp = static_cast<byte*>(Z_Malloc(len, PU_STATIC, NULL));
      fc.ReadLump(lump, temp); // to avoid unnecessary memcpy

      // allocate space for RGBA data, owner is pixels
      Z_Malloc(sizeof(RGBA_t)*width*height, PU_TEXTURE, (void **)(&pixels));

      // convert to RGBA
      byte   *index_in = temp;
      RGB_t  *palette  = static_cast<RGB_t*>(fc.CacheLumpNum(materials.GetPaletteLump(lump >> 16), PU_DAVE));
      RGBA_t *rgba_out = reinterpret_cast<RGBA_t*>(pixels);

      for (int i=0; i<width; i++)
	for (int j=0; j<height; j++)
	  {
	    byte curbyte = *index_in++;
	    RGB_t *rgb_in = &palette[curbyte]; // untransposed
	    rgba_out->red   = rgb_in->r;
	    rgba_out->green = rgb_in->g;
	    rgba_out->blue  = rgb_in->b;
	    if (curbyte == TRANSPARENTPIXEL)
	      rgba_out->alpha = 0;
	    else
	      rgba_out->alpha = 255;

	    rgba_out++;
	  }

      Z_Free(palette);
      Z_Free(temp); // free indexed data
      gl_format = GL_RGBA;
    }
}


/// This method handles the Doom flats and raw pages.
byte *LumpTexture::GetData()
{
  if (!pixels)
    {
      int len = fc.LumpLength(lump);
      Z_Malloc(len, PU_TEXTURE, (void **)(&pixels));

      byte *temp = static_cast<byte*>(fc.CacheLumpNum(lump, PU_STATIC));

      // transposed to col-major order
      int dest = 0;
      for (int i=0; i<len; i++)
	{
	  pixels[dest] = temp[i];
	  dest += height;
	  if (dest >= len)
	    dest -= len - 1; // next column
	}
      Z_Free(temp);

      // do a palette conversion if needed
      byte *colormap = materials.GetPaletteConv(lump >> 16);
      if (colormap)
	for (int i=0; i<len; i++)
	  pixels[i] = colormap[pixels[i]];

      // convert to high color
      // short pix16 = ((color8to16[*data++] & 0x7bde) + ((i<<9|j<<4) & 0x7bde))>>1;
    }

  return pixels;
}



//==================================================================
//  PatchTexture
//==================================================================

// Clip and draw a column from a patch into a cached post.
static void R_DrawColumnInCache(column_t *col, byte *cache, int originy, int cacheheight)
{
  while (col->topdelta != 0xff)
    {
      byte *source = col->data; // go to the data
      int count = col->length;
      int position = originy + col->topdelta;

      if (position < 0)
        {
          count += position;
          source -= position;
          position = 0;
        }

      if (position + count > cacheheight)
        count = cacheheight - position;

      if (count > 0)
        memcpy(cache + position, source, count);

      col = reinterpret_cast<column_t*>(&col->data[col->length + 1]);
    }
}


PatchTexture::PatchTexture(const char *n, int l)
  : Texture(n, l)
{
  patch_t p;
  fc.ReadLumpHeader(lump, &p, sizeof(patch_t));
  width = SHORT(p.width);
  height = SHORT(p.height);
  leftoffs = SHORT(p.leftoffset);
  topoffs = SHORT(p.topoffset);

  InitializeTexture();
  // nothing more is needed until the texture is Generated.

  patch_data = NULL;

  if (fc.LumpLength(lump) <= width*4 + 8)
    I_Error("PatchTexture: lump %d (%s) is invalid\n", l, n);
}


PatchTexture::~PatchTexture()
{
  if (patch_data)
    Z_Free(patch_data);
}


/// sets patch_data
patch_t *PatchTexture::GeneratePatch()
{
  if (!patch_data)
    {
      int len = fc.LumpLength(lump);
      patch_t *p = static_cast<patch_t*>(Z_Malloc(len, PU_TEXTURE, (void **)&patch_data));

      // to avoid unnecessary memcpy
      fc.ReadLump(lump, patch_data);

      // [segabor] necessary endianness conversion for p
      // [smite] should not be necessary, because the other fields are never used 
	  
      for (int i=0; i < width; i++)
        p->columnofs[i] = LONG(p->columnofs[i]);

      // do a palette conversion if needed
      byte *colormap = materials.GetPaletteConv(lump >> 16);
      if (colormap)
	{
	  p->width = SHORT(p->width);
	  R_ColormapPatch(p, colormap);
	}
    }

  return reinterpret_cast<patch_t*>(patch_data);
}


/// sets pixels (which implies that patch_data is also set)
byte *PatchTexture::GetData()
{
  if (!pixels)
    {
      // we need to draw the patch into a rectangular bitmap in column-major order
      int len = width*height;
      Z_Malloc(len, PU_TEXTURE, (void **)&pixels);
      memset(pixels, TRANSPARENTPIXEL, len);

      patch_t *p = GeneratePatch();

      for (int col = 0; col < width; col++)
	{
	  column_t *patchcol = reinterpret_cast<column_t*>(patch_data + p->columnofs[col]);
	  R_DrawColumnInCache(patchcol, pixels + col * height, 0, height);
	}
    }

  return pixels;
}


column_t *PatchTexture::GetMaskedColumn(fixed_t fcol)
{
  int col = fcol.floor() % width;

  if (col < 0)
    col += width; // wraparound

  patch_t *p = GeneratePatch();
  return reinterpret_cast<column_t*>(patch_data + p->columnofs[col]);
}


//==================================================================
//  DoomTexture
//==================================================================


DoomTexture::DoomTexture(const char *n, int l, const maptexture_t *mtex)
  : Texture(n, l)
{
  patchcount = SHORT(mtex->patchcount);
  patches = static_cast<texpatch_t*>(Z_Malloc(sizeof(texpatch_t)*patchcount, PU_TEXTURE, 0));

  width  = SHORT(mtex->width);
  height = SHORT(mtex->height);

  InitializeTexture();
  widthmask = (1 << w_bits) - 1;

  patch_data = NULL;
}


DoomTexture::~DoomTexture()
{
  Z_Free(patches);

  if (patch_data)
    Z_Free(patch_data);
}



// TODO better DoomTexture handling?
// When a texture is first needed,
//  it counts the number of composite columns
//  required in the texture and allocates space
//  for a column directory and any new columns.
// The directory will simply point inside other patches
//  if there is only one patch in a given column,
//  but any columns with multiple patches
//  will have new column_ts generated.


//   Allocate space for full size texture, either single patch or 'composite'
//   Build the full textures from patches.
//   The texture caching system is a little more hungry of memory, but has
//   been simplified for the sake of highcolor, dynamic lighting, & speed.

patch_t *DoomTexture::GeneratePatch()
{
  // patchcount must be 1! no splicing!
  // single-patch textures can have holes in it and may be used on
  // 2-sided lines so they need to be kept in patch_t format
  if (!patch_data)
    {
      CONS_Printf("Generating patch for '%s'\n", name);
      texpatch_t *tp = patches;
      int blocksize = fc.LumpLength(tp->patchlump);
      //CONS_Printf ("R_GenTex SINGLE %.8s size: %d\n",name,blocksize);

      patch_t *p = static_cast<patch_t*>(Z_Malloc(blocksize, PU_TEXTURE, (void **)&patch_data));
      texturememory += blocksize;
      fc.ReadLump(tp->patchlump, patch_data);


      p->width = SHORT(p->width); // endianness...

      // FIXME should use patch width here? texture may be wider!
      if (width > p->width)
	{
	  CONS_Printf("masked tex '%s' too wide\n", name);
	  width = p->width;
	  InitializeTexture();
	  widthmask = (1 << w_bits) - 1;
	}

      // use the patch's column lookup (columnofs is reserved for the raw bitmap version!)
      // do not skip post_t info by default
      for (int i=0; i<width; i++)
        p->columnofs[i] = LONG(p->columnofs[i]);

      // do a palette conversion if needed
      byte *colormap = materials.GetPaletteConv(tp->patchlump >> 16);
      if (colormap)
	R_ColormapPatch(p, colormap);
    }

  return reinterpret_cast<patch_t*>(patch_data);
}


byte *DoomTexture::GetData()
{
  if (!pixels)
    {
      //CONS_Printf("Generating data for '%s'\n", name);
      int i;
      // multi-patch (or 'composite') textures are stored as a simple bitmap

      int size = width * height;
      int blocksize = size + width*sizeof(Uint32); // first raw col-major data, then columnofs table
      //CONS_Printf ("R_GenTex MULTI  %.8s size: %d\n",name,blocksize);

      Z_Malloc(blocksize, PU_TEXTURE, (void **)&pixels);
      texturememory += blocksize;

      // generate column offset lookup table
      columnofs = reinterpret_cast<Uint32*>(pixels + size);
      for (i=0; i<width; i++)
        columnofs[i] = i * height;

      // prepare texture bitmap
      memset(pixels, TRANSPARENTPIXEL, size);

      texpatch_t *tp;
      // Composite the patches together.
      for (i=0, tp = patches; i<patchcount; i++, tp++)
        {
	  patch_t *p = static_cast<patch_t*>(fc.CacheLumpNum(tp->patchlump, PU_STATIC));
          int x1 = tp->originx;
          int x2 = x1 + SHORT(p->width);

          int x = x1;
          if (x < 0)
            x = 0;

          if (x2 > width)
            x2 = width;

          for ( ; x < x2; x++)
            {
              column_t *patchcol = reinterpret_cast<column_t*>(reinterpret_cast<byte*>(p) + LONG(p->columnofs[x-x1]));
              R_DrawColumnInCache(patchcol, pixels + columnofs[x], tp->originy, height);
            }

	  Z_Free(p);
        }

      // TODO do a palette conversion if needed
    }

  return pixels;
}



// returns a pointer to column-major raw data
byte *DoomTexture::GetColumn(fixed_t fcol)
{
  int col = fcol.floor();

  return GetData() + columnofs[col & widthmask];
}


column_t *DoomTexture::GetMaskedColumn(fixed_t fcol)
{
  if (patchcount == 1)
    {
      int col = fcol.floor();
      patch_t *p = GeneratePatch();
      return reinterpret_cast<column_t*>(patch_data + p->columnofs[col & widthmask]);
    }
  else
    return NULL;
}



//==================================================================
//  Materials
//==================================================================

Material::Material(const char *name)
  : cacheitem_t(name)
{
  id_number = -1;
  shader = NULL;
  // The rest are taken care of by InitializeMaterial() later when Textures have been attached.
}


// Single-texture Materials. tex.size() guaranteed to be 1 after this constructor.
Material::Material(const char *name, Texture *t, float xs, float ys)
  : cacheitem_t(name)
{
  id_number = -1;
  shader = NULL;

  tex.resize(1);
  tex[0].t = t;
  tex[0].xscale = xs;
  tex[0].yscale = ys;

  // t is used by this material, so
  t->AddRef(); // NOTE: we don't currently use links for Textures, this functionality is subsumed in material_cache_t::Get

  InitializeMaterial();  
}


Material::~Material()
{
  int n = tex.size(); // number of texture units
  for (int i=0; i<n; i++)
    tex[i].t->Release();
}


Material::TextureRef::TextureRef()
{
  // default scaling and OpenGL params
  t = NULL;
  xscale = yscale = 1;

  mag_filter = 0; // If these are zero, take values from consvars.
  min_filter = 0;
  max_anisotropy = 0.0;
}


void Material::TextureRef::GLSetTextureParams()
{
  glBindTexture(GL_TEXTURE_2D, t->GLPrepare()); // bind the texture

  GLint magf = mag_filter ? mag_filter : (cv_grfiltermode.value ? GL_LINEAR : GL_NEAREST);
  // HACK, relies on the order of the GL numeric constants which should be always the same
  GLint minf = min_filter ? min_filter : GL_NEAREST_MIPMAP_NEAREST + cv_grfiltermode.value;

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magf);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minf);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_anisotropy ? max_anisotropy : cv_granisotropy.value);
}


int Material::GLUse()
{ 
	#ifdef GL_USE_GLEXT		
		static PFNGLACTIVETEXTUREPROC _GLActiveTexture= NULL;			
		_GLActiveTexture = (PFNGLACTIVETEXTUREPROC) wglGetProcAddress("glActiveTexture");
	#else
		#define glActiveTexture _GLActiveTexture
	#endif
		
  if (shader)
    {
      shader->Use();
      shader->SetUniforms();
    }
  else
    ShaderProg::DisableShaders();


  int n = tex.size(); // number of texture units
	
  for (int i=0; i<n; i++)
    {
      _GLActiveTexture/*glActiveTexture*/(GL_TEXTURE0 + i); // activate correct texture unit
      tex[i].GLSetTextureParams(); // and set its parameters
    }

  return n;
}



//==================================================================
//  Material cache
//==================================================================

material_cache_t materials;


material_cache_t::material_cache_t()
{
  default_item = NULL;
}

material_cache_t::~material_cache_t()
{
  if (default_item)
    delete default_item;
}


void material_cache_t::SetDefaultItem(const char *name)
{
  if (default_item)
    CONS_Printf("Material_Cache: Replacing default_item!\n");
  // TODO delete the old default item?

  textures.SetDefaultItem(name);
  Texture *t = textures.default_item;
  if (!t)
    I_Error("Material_Cache: New default_item '%s' not found!\n", name);

  default_item = new Material(name, t);
}



void material_cache_t::Clear()
{
  new_tex.Clear();
  doom_tex.Clear();
  flat_tex.Clear();
  sprite_tex.Clear();
  lod_tex.Clear();

  all_materials.clear();
}


void material_cache_t::ClearGLTextures()
{
  int count = 0;
  for (material_iterator_t ti = all_materials.begin(); ti != all_materials.end(); ti++)
    {
      Material *m = *ti;
      if (m)
	{
	  int n = m->tex.size();
	  for (int k = 0; k < n; k++)
	    if (m->tex[k].t->ClearGLTexture())
	      count++;
	}
    }

  if (count)
    CONS_Printf("Cleared %d OpenGL textures.\n", count);
}


/// Creates a new blank Material in cache or returns an existing one for updating. Does not change reference counts. For NTEXTURE.
Material *material_cache_t::Edit(const char *name, material_class_t mode, bool create)
{
  Material *t;

  switch (mode)
    {
    case TEX_wall: // walls
      if (!(t = new_tex.Find(name)))
	if (!(t = doom_tex.Find(name)))
	  t = flat_tex.Find(name);
      break;

    case TEX_floor: // floors, ceilings
      if (!(t = new_tex.Find(name)))
	if (!(t = flat_tex.Find(name)))
	  t = doom_tex.Find(name);
      break;

    case TEX_sprite: // sprite frames
      t = sprite_tex.Find(name);
      break;

    case TEX_lod:
      I_Error("material_cache_t::Edit() cannot be used with TEX_lod!\n");
      return NULL;
    }

  if (!t && create)
    {
      Material *m = new Material(name);
      if (mode == TEX_sprite)
	sprite_tex.Insert(m);
      else
	new_tex.Insert(m);

      Register(m);
      return m;
    }
  
  return t;
}


/// Build a single-Texture Material during startup, insert it into a given source.
/// Texture is assumed not to be in cache before call (unless there is a namespace overlap).
Material *material_cache_t::BuildMaterial(Texture *t, cachesource_t<Material> &source, bool h_start)
{
  if (!t)
    return NULL;

  // insert Texture into cache, change name if it is already taken (HACK for namespace overlaps, just a few)
  string name = t->GetName(); // char* won't do since we may change the Texture's name
  if (textures.Exists(name.c_str()))
    {
      CONS_Printf("Overlap in Texture names '%s'!\n", name.c_str());
      t->SetName((name + "_xxx").c_str());
    }

  if (!textures.Insert(t))
    CONS_Printf("Error: Overlapping Texture names '%s'!\n", t->GetName());


  // see if we already have a Material with this name
  Material *m = source.Find(name.c_str());

  if (h_start) // H_START blocks work a bit differently
    {
      if (!m)
	return NULL; // no original with same name, ignore

      // take over the original, scale so that worldsize stays the same
      Material::TextureRef &r = m->tex[0];

      // replace Texture
      r.t->Release();
      t->AddRef();
      r.t = t;
      r.xscale = t->width / r.worldwidth;
      r.yscale = t->height / r.worldheight;
    }
  else
    {
      // normal material
      if (!m)
	{
	  m = new Material(name.c_str(), t); // create a new Material
	  source.Insert(m);
	  Register(m);
	}
      else
	{
	  // replace Texture in an existing Material (assume it has just one)
	  Material::TextureRef &r = m->tex[0];

	  r.t->Release();
	  t->AddRef();
	  r.t = t;
	  r.xscale = 1;
	  r.yscale = 1;
	  m->InitializeMaterial();
	}
    }

  return m;
}




void material_cache_t::InitPaletteConversion()
{
  // create the palette conversion colormaps
  unsigned i;
  for (i=0; i<palette_conversion.size(); i++)
    if (palette_conversion[i])
      Z_Free(palette_conversion[i]);

  unsigned n = fc.Size(); // number of resource files
  palette_conversion.resize(n);
  palette_lump.resize(n);

  // For OpenGL, just find the correct palettes to use for each file
  int def_palette = fc.FindNumForName("PLAYPAL");
  for (i=0; i<n; i++)
    {
      palette_conversion[i] = R_CreatePaletteConversionColormap(i);
      int lump = fc.FindNumForNameFile("PLAYPAL", i);
      palette_lump[i] = (lump < 0) ? def_palette : lump;
    }
}


/// Returns an existing Material, or tries creating it if nonexistant.
Material *material_cache_t::Get8char(const char *name, material_class_t mode)
{
  char name8[9]; // NOTE texture names in Doom map format are max 8 chars long and not always NUL-terminated!

  strncpy(name8, name, 8);
  name8[8] = '\0';
  strupr(name8); // TODO unfortunate, could be avoided if we used a non-case-sensitive string comparison functor...

  return Get(name8, mode);
};


/// Returns pointer to an existing Material, or tries creating it if nonexistant.
Material *material_cache_t::Get(const char *name, material_class_t mode)
{
  // "No texture" marker.
  if (name[0] == '-')
    return NULL;

  Material *t;

  switch (mode)
    {
    case TEX_wall: // walls
      if (!(t = new_tex.Find(name)))
	if (!(t = doom_tex.Find(name)))
	  t = flat_tex.Find(name);
      break;

    case TEX_floor: // floors, ceilings
      if (!(t = new_tex.Find(name)))
	if (!(t = flat_tex.Find(name)))
	  t = doom_tex.Find(name);
      break;

    case TEX_sprite: // sprite frames
      t = sprite_tex.Find(name);
      break;

    case TEX_lod: // menu, HUD, console, background images
      if (!(t = new_tex.Find(name)))
	if (!(t = lod_tex.Find(name)))
	  {
	    // Not found there either, try loading on demand
	    Texture *tex = textures.Load(name);
	    t = BuildMaterial(tex, lod_tex); // wasteful...
	  }
      break;
    }

  if (!t)
    {
      // Item not found at all.
      // Some nonexistant items are asked again and again.
      // We use a special cacheitem_t to link their names to the default item.
      t = new Material(name);
      t->MakeLink();
      // This is a dummy Material which has no textures attached, so the tex vector remains empty.
      // Direct pointers to dummy Materials must not be passed outside material_cache_t.

      // NOTE insertion to cachesource only, not to id-map
      if (mode == TEX_sprite)
	sprite_tex.Insert(t);
      else
	new_tex.Insert(t);
    }

  if (t->AddRef())
    {
      // a "link" to default_item
      default_item->AddRef();
      // CONS_Printf("Def. material used for '%s'\n", name);
      return default_item;
    }
  else
    return t;
}



/// Shorthand.
Material *material_cache_t::GetLumpnum(int n)
{
  return Get(fc.FindNameForNum(n), TEX_lod);
}



// semi-hack for linedeftype 242 and the like, where the
// "texture name" field can hold a colormap name instead.
Material *material_cache_t::GetMaterialOrColormap(const char *name, fadetable_t*& cmap)
{
  // "NoTexture" marker. No texture, no colormap.
  if (name[0] == '-')
    {
      cmap = NULL;
      return NULL;
    }

  fadetable_t *temp = R_FindColormap(name); // check C_*... lists

  if (temp)
    {
      // it is a colormap lumpname, not a texture
      cmap = temp;
      return NULL;
    }

  cmap = NULL;

  // it could be a texture, let's check
  return Get8char(name, TEX_wall);
}


// semi-hack for linedeftype 260, where the
// "texture name" field can hold a transmap name instead.
Material *material_cache_t::GetMaterialOrTransmap(const char *name, int& map_num)
{
  // "NoTexture" marker. No texture, no colormap.
  if (name[0] == '-')
    {
      map_num = -1;
      return NULL;
    }

  int temp = R_TransmapNumForName(name);

  if (temp >= 0)
    {
      // it is a transmap lumpname, not a texture
      map_num = temp;
      return NULL;
    }

  map_num = -1;

  // it could be a texture, let's check
  return Get8char(name, TEX_wall);
}



bool Read_NTEXTURE(int lump);

/// Initializes the material cache, fills the cachesource_t containers with Material objects.
/*!
  Follows the JDS texture standard.
  The reason we use texture_cache_t::LoadLump instead of texture_cache_t::Get here is to
  avoid namespace overlaps by making sure a new Texture is created each time from a particular lump.
  material_cache_t::BuildMaterial takes care of inserting the newly created Texture into the texture cache,
  changing the name if it is already taken.
*/
int material_cache_t::ReadTextures()
{
  int i, lump;
  int num_textures = 0;

  char name8[9];
  name8[8] = 0; // NUL-terminated


  // TEXTUREx/PNAMES:
  {
    // Load the patch names from the PNAMES lump
    struct pnames_t
    {
      Uint32 count;
      char names[][8]; // list of 8-byte patch names
    } __attribute__((packed));

    lump = fc.GetNumForName("PNAMES");
    pnames_t *pnames = static_cast<pnames_t*>(fc.CacheLumpNum(lump, PU_STATIC));
    int numpatches = LONG(pnames->count);

    if (devparm)
      CONS_Printf(" PNAMES: lump %d:%d, %d patches\n", lump >> 16, lump & 0xffff, numpatches);

    if (fc.LumpLength(lump) != 4 + 8*numpatches)
      I_Error("Corrupted PNAMES lump.\n");

    vector<int> patchlookup(numpatches); // mapping from patchnumber to lumpnumber

    for (i=0 ; i<numpatches ; i++)
      {
	strncpy(name8, pnames->names[i], 8);
	patchlookup[i] = fc.FindNumForName(name8);
	if (patchlookup[i] < 0)
	  CONS_Printf(" Patch '%s' (%d) not found!\n", name8, i);
      }

    // Load the map texture definitions from textures.lmp.
    // The data is contained in one or two lumps,
    //  TEXTURE1 for shareware, plus TEXTURE2 for commercial.
    int  *maptex, *maptex1, *maptex2;

    lump = fc.GetNumForName("TEXTURE1");
    maptex = maptex1 = static_cast<int*>(fc.CacheLumpNum(lump, PU_STATIC));
    int numtextures1 = LONG(*maptex);
    int maxoff = fc.LumpLength(lump);
    if (devparm)
      CONS_Printf(" TEXTURE1: lump %d:%d, %d textures\n", lump >> 16, lump & 0xffff, numtextures1);
    int *directory = maptex+1;

    int numtextures2, maxoff2;
    if (fc.FindNumForName("TEXTURE2") != -1)
      {
	lump = fc.GetNumForName("TEXTURE2");
	maptex2 = static_cast<int*>(fc.CacheLumpNum(lump, PU_STATIC));
	numtextures2 = LONG(*maptex2);
	maxoff2 = fc.LumpLength(lump);
	if (devparm)
	  CONS_Printf(" TEXTURE2: lump %d:%d, %d textures\n", lump >> 16, lump & 0xffff, numtextures2);
      }
    else
      {
	maptex2 = NULL;
	numtextures2 = 0;
	maxoff2 = 0;
      }

    int numtextures = numtextures1 + numtextures2;

    for (i=0 ; i<numtextures ; i++, directory++)
      {
	//only during game startup
	//if (!(i&63))
	//    CONS_Printf (".");

	if (i == numtextures1)
	  {
	    // Start looking in second texture file.
	    maptex = maptex2;
	    maxoff = maxoff2;
	    directory = maptex+1;
	  }

	// offset to the current texture in TEXTURESn lump
	int offset = LONG(*directory);

	if (offset > maxoff)
	  I_Error("ReadTextures: bad texture directory");

	// maptexture describes texture name, size, and
	// used patches in z order from bottom to top
	maptexture_t *mtex = reinterpret_cast<maptexture_t*>(reinterpret_cast<byte*>(maptex) + offset);
	strncpy(name8, mtex->name, 8);
	DoomTexture *tex = new DoomTexture(name8, lump, mtex); // lump is not so crucial here, it merely needs to belong to the correct datafile

	mappatch_t *mp = mtex->patches;
	DoomTexture::texpatch_t *p = tex->patches;

	for (int j=0 ; j < tex->patchcount ; j++, mp++, p++)
	  {
	    p->originx = SHORT(mp->originx);
	    p->originy = SHORT(mp->originy);
	    p->patchlump = patchlookup[SHORT(mp->patch)];
	    if (p->patchlump == -1)
	      I_Error("ReadTextures: Missing patch %d in texture %.8s (%d)\n",
		      SHORT(mp->patch), mtex->name, i);
	  }

	Material *m = BuildMaterial(tex, doom_tex);

	// ZDoom extension, scaling. FIXME this could be done more gracefully (params to BuildMaterial?)
	m->tex[0].xscale = mtex->xscale ? mtex->xscale / 8.0 : 1;
	m->tex[0].yscale = mtex->yscale ? mtex->yscale / 8.0 : 1;
	m->InitializeMaterial(); // again...
      }

    Z_Free(maptex1);
    if (maptex2)
      Z_Free(maptex2);


    // then rest of PNAMES
    for (i=0 ; i<numpatches ; i++)
      if (patchlookup[i] >= 0)
	{
	  strncpy(name8, pnames->names[i], 8);
	  if (doom_tex.Find(name8))
	    continue; // already defined in TEXTUREx

	  PatchTexture *tex = new PatchTexture(name8, patchlookup[i]);
	  BuildMaterial(tex, doom_tex);
	  numtextures++;

          //CONS_Printf(" Bare PNAMES texture '%s' found!\n", name8);
	}

    Z_Free(pnames);

    num_textures += numtextures;
  }


  int nwads = fc.Size();

  // TX_START
  // later files override earlier ones
  for (i = nwads-1; i >= 0; i--)
    {
      int lump = fc.FindNumForNameFile("TX_START", i, 0);
      if (lump == -1)
	continue;
      else
        lump++;

      int end = fc.FindNumForNameFile("TX_END", i, 0);
      if (end == -1)
	{
	  CONS_Printf(" TX_END missing in file '%s'.\n", fc.Name(i));
	  continue; // no TX_END, nothing accepted
	}

      for ( ; lump < end; lump++)
	if (BuildMaterial(textures.LoadLump(fc.FindNameForNum(lump), lump), new_tex))
	  num_textures++;
    }

  // F_START, FF_START
  for (i = nwads-1; i >= 0; i--)
    {
      // Only accept flats between F(F)_START and F(F)_END

      int lump = fc.FindNumForNameFile("F_START", i, 0);
      if (lump == -1)
        lump = fc.FindNumForNameFile("FF_START", i, 0); //deutex compatib.

      if (lump == -1)
	continue; // no flats in this wad
      else
        lump++;  // just after F_START

      int end = fc.FindNumForNameFile("F_END", i, 0);
      if (end == -1)
        end = fc.FindNumForNameFile("FF_END", i, 0);     //deutex compatib.

      if (end == -1)
	{
	  CONS_Printf(" F_END missing in file '%s'.\n", fc.Name(i));
	  continue; // no F_END, no flats accepted
	}

      for ( ; lump < end; lump++)
	{
	  LumpTexture *t;

	  const char *name = fc.FindNameForNum(lump); // now always NUL-terminated
	  if (flat_tex.Find(name))
	    continue; // already defined

	  int size = fc.LumpLength(lump);
	    
	  if (size ==  64*64 || // normal flats
	      size ==  65*64)   // Damn you, Heretic animated flats!
	    // Flat is 64*64 bytes of raw paletted picture data in one lump
	    t = new LumpTexture(name, lump, 64, 64);
	  else if (size == 128*64) // Some Hexen flats (X_001-X_011) are larger! Why?
	    t = new LumpTexture(name, lump, 128, 64);
	  else if (size == 128*128)
	    t = new LumpTexture(name, lump, 128, 128);
	  else if (size == 256*128)
	    t = new LumpTexture(name, lump, 256, 128);
	  else if (size == 256*256)
	    t = new LumpTexture(name, lump, 256, 256);
	  else
	    {
	      if (size != 0) // markers are OK
		CONS_Printf(" Unknown flat type: lump '%8s' in the file '%s'.\n", name, fc.Name(i));
	      continue;
	    }

	  BuildMaterial(t, flat_tex);
	  num_textures++;
	}
    }


  // S_START, SS_START
  for (i = nwads-1; i >= 0; i--)
    {
      // Only accept patch_t's between S(S)_START and S(S)_END

      int lump = fc.FindNumForNameFile("S_START", i, 0);
      if (lump == -1)
        lump = fc.FindNumForNameFile("SS_START", i, 0); //deutex compatib.

      if (lump == -1)
	continue; // no spriteframes in this wad
      else
        lump++;  // just after S_START

      int end = fc.FindNumForNameFile("S_END", i, 0);
      if (end == -1)
        end = fc.FindNumForNameFile("SS_END", i, 0);     //deutex compatib.

      if (end == -1)
	{
	  CONS_Printf(" S_END missing in file '%s'.\n", fc.Name(i));
	  continue; // no S_END, no spriteframes accepted
	}

      for ( ; lump < end; lump++)
	if (BuildMaterial(textures.LoadLump(fc.FindNameForNum(lump), lump), sprite_tex))
	  num_textures++;
    }


  // H_START: Variable-resolution textures that are scaled so they match
  // the size of a corresponding texture in TEXTUREx or F_START.
  for (i = nwads-1; i >= 0; i--)
    {
      int lump = fc.FindNumForNameFile("H_START", i, 0);
      if (lump == -1)
        continue;
      else
        lump++;

      int end = fc.FindNumForNameFile("H_END", i, 0);
      if (end == -1)
        {
          CONS_Printf(" H_END missing in file '%s'.\n", fc.Name(i));
          continue; // no H_END, nothing accepted
        }

      for ( ; lump < end; lump++)
	{
	  Texture *tex = textures.LoadLump(fc.FindNameForNum(lump), lump);
	  if (!BuildMaterial(tex, doom_tex, true) &&
	      !BuildMaterial(tex, flat_tex, true))
	    {
	      CONS_Printf(" H_START texture '%8s' in file '%s' has no original, ignored.\n",
			  fc.FindNameForNum(lump), fc.Name(i));
	    }
	}
    }

  Read_NTEXTURE(fc.FindNumForName("NTEXTURE"));
  Read_NTEXTURE(fc.FindNumForName("NSPRITES"));

  return num_textures;
}




//==================================================================
//    Colormaps
//==================================================================



/// extra boom colormaps
/*
int             num_extra_colormaps;
extracolormap_t extra_colormaps[MAXCOLORMAPS];
*/


/// the fadetable "cache"
static vector<fadetable_t*> fadetables;

/// lumplist for C_START - C_END pairs in wads
static int         numcolormaplumps;
static lumplist_t *colormaplumps;



fadetable_t::~fadetable_t()
{
  if (colormap)
    Z_Free(colormap);
}


/// For custom, OpenGL-friendly colormaps
fadetable_t::fadetable_t()
{
  lump = -1;
  colormap = NULL;
}

/// For lump-based colormaps
fadetable_t::fadetable_t(int lumpnum)
{
  lump = lumpnum;

  int length = fc.LumpLength(lump);
  if (length != 34*256)
    {
      CONS_Printf("Unknown colormap size: %d bytes\n", length);
    }

  // (aligned on 8 bit for asm code) now 64k aligned for smokie...
  colormap = static_cast<lighttable_t*>(Z_MallocAlign(length, PU_STATIC, 0, 16)); // 8);
  fc.ReadLump(lump, colormap);

  // SoM: Added, we set all params of the colormap to normal because there
  // is no real way to tell how GL should handle a colormap lump anyway..
  // TODO try to analyze the colormap and somehow simulate it in GL...
  maskcolor = 0xffff;
  fadecolor = 0x0;
  maskamt = 0x0;
  fadestart = 0;
  fadeend = 33;
  fog = 0;
}



/// Returns the name of the fadetable
const char *R_ColormapNameForNum(const fadetable_t *p)
{
  if (!p)
    return "NONE";

  int temp = p->lump;
  if (temp == -1)
    return "INLEVEL";

  return fc.FindNameForNum(temp);
}



/// Called in R_Init, prepares the lightlevel colormaps and Boom extra colormaps.
void R_InitColormaps()
{
  if (devparm)
    CONS_Printf(" Loading colormaps.\n");

  // build the colormap lumplists (which record the locations of C_START and C_END in each wad)
  numcolormaplumps = 0;
  colormaplumps = NULL;

  int nwads = fc.Size();
  for (int file = 0; file < nwads; file++)
    {
      int start = fc.FindNumForNameFile("C_START", file, 0);
      if (start == -1)
        continue; // required

      int end = fc.FindNumForNameFile("C_END", file, 0);
      if (end == -1)
	{
	  CONS_Printf("R_InitColormaps: C_START without C_END in file '%s'!\n", fc.Name(file));
	  continue;
	}

      colormaplumps = static_cast<lumplist_t*>(realloc(colormaplumps, sizeof(lumplist_t) * (numcolormaplumps + 1)));
      colormaplumps[numcolormaplumps].wadfile = start >> 16;
      colormaplumps[numcolormaplumps].firstlump = (start & 0xFFFF) + 1; // after C_START
      colormaplumps[numcolormaplumps].numlumps = end - (start + 1);
      numcolormaplumps++;
    }

  //SoM: 3/30/2000: Init Boom colormaps.
  R_ClearColormaps();

  // Load in the basic lightlevel colormaps
  fadetables.push_back(new fadetable_t(fc.GetNumForName("COLORMAP")));
}


/// Clears out all extra colormaps.
void R_ClearColormaps()
{
  int n = fadetables.size();
  for (int i=0; i < n; i++)
    delete fadetables[i];

  fadetables.clear();
}


/// Auxiliary Get func for fadetables
fadetable_t *R_GetFadetable(unsigned n)
{
  if (n < fadetables.size())
    return fadetables[n];
  else
    return NULL;
}


/// Tries to retrieve a colormap by name. Only checks stuff between C_START/C_END.
fadetable_t *R_FindColormap(const char *name)
{
  int lump = R_CheckNumForNameList(name, colormaplumps, numcolormaplumps);
  if (lump == -1)
    return NULL; // no such colormap
  
  int n = fadetables.size();
  for (int i=0; i < n; i++)
    if (fadetables[i]->lump == lump)
      return fadetables[i];

  // not already cached, we must read it from WAD
  if (n == MAXCOLORMAPS)
    I_Error("R_ColormapNumForName: Too many colormaps!\n");

  fadetable_t *temp = new fadetable_t(lump);
  fadetables.push_back(temp);
  return temp;
}


/// Sets the default lighlevel colormaps used by the software renderer in this map.
/// All lumps are searched.
bool Map::R_SetFadetable(const char *name)
{
  int lump = fc.FindNumForName(name);
  if (lump < 0)
    {
      CONS_Printf("R_SetFadetable: Fadetable '%s' not found.\n", name);
      fadetable = fadetables[0]; // use COLORMAP
      return false;
    }

  int n = fadetables.size();
  for (int i=0; i < n; i++)
    if (fadetables[i]->lump == lump)
      {
	fadetable = fadetables[i];
	return true;
      }

  // not already cached, we must read it from WAD
  if (n == MAXCOLORMAPS)
    I_Error("R_SetFadetable: Too many colormaps!\n");

  fadetable = new fadetable_t(lump);
  fadetables.push_back(fadetable);
  return true;
}




// Rounds off floating numbers and checks for 0 - 255 bounds
static int RoundUp(double number)
{
  if (number >= 255.0)
    return 255;
  if (number <= 0)
    return 0;

  if (int(number) <= (number - 0.5))
    return int(number) + 1;

  return int(number);
}


// R_CreateColormap
// This is a more GL friendly way of doing colormaps: Specify colormap
// data in a special linedef's texture areas and use that to generate
// custom colormaps at runtime. NOTE: For GL mode, we only need to color
// data and not the colormap data.

fadetable_t *R_CreateColormap(char *p1, char *p2, char *p3)
{
  int    i, p;
  double cmaskr, cmaskg, cmaskb;
  double maskamt = 0, othermask = 0;
  unsigned int  cr, cg, cb, ca;
  unsigned int  maskcolor;
  int rgba;

  //TODO: Hurdler: check this entire function
  // for now, full support of toptexture only

#define HEX2INT(x) (x >= '0' && x <= '9' ? x - '0' : \
		    x >= 'a' && x <= 'f' ? x - 'a' + 10 : \
		    x >= 'A' && x <= 'F' ? x - 'A' + 10 : 0)
#define ALPHA2INT(x) (x >= 'a' && x <= 'z' ? x - 'a' : \
		      x >= 'A' && x <= 'Z' ? x - 'A' : 0)
  if (p1[0] == '#')
    {
      cmaskr = cr = ((HEX2INT(p1[1]) << 4) + HEX2INT(p1[2]));
      cmaskg = cg = ((HEX2INT(p1[3]) << 4) + HEX2INT(p1[4]));
      cmaskb = cb = ((HEX2INT(p1[5]) << 4) + HEX2INT(p1[6]));
      maskamt = ca = ALPHA2INT(p1[7]);

      // 24-bit color
      rgba = cr + (cg << 8) + (cb << 16) + (ca << 24);

      // Create a rough approximation of the color (a 16 bit color)
      maskcolor = (cb >> 3) + ((cg >> 2) << 5) + ((cr >> 3) << 11);

      maskamt /= 25.0;

      othermask = 1 - maskamt;
      maskamt /= 0xff;
      cmaskr *= maskamt;
      cmaskg *= maskamt;
      cmaskb *= maskamt;
    }
  else
    {
      cmaskr = 0xff;
      cmaskg = 0xff;
      cmaskb = 0xff;
      maskamt = 0;
      rgba = 0;
      maskcolor = ((0xff) >> 3) + (((0xff) >> 2) << 5) + (((0xff) >> 3) << 11);
    }
#undef ALPHA2INT


  unsigned int  fadestart = 0, fadeend = 33, fadedist = 33;
  int fog = 0;

#define NUMFROMCHAR(c)  (c >= '0' && c <= '9' ? c - '0' : 0)
  if (p2[0] == '#')
    {
      // SoM: Get parameters like, fadestart, fadeend, and the fogflag...
      fadestart = NUMFROMCHAR(p2[3]) + (NUMFROMCHAR(p2[2]) * 10);
      fadeend = NUMFROMCHAR(p2[5]) + (NUMFROMCHAR(p2[4]) * 10);
      if (fadestart > 32 || fadestart < 0)
        fadestart = 0;
      if (fadeend > 33 || fadeend < 1)
        fadeend = 33;
      fadedist = fadeend - fadestart;
      fog = NUMFROMCHAR(p2[1]) ? 1 : 0;
    }
#undef NUMFROMCHAR


  double cdestr, cdestg, cdestb;
  unsigned int fadecolor;
  if (p3[0] == '#')
    {
      cdestr = cr = ((HEX2INT(p3[1]) << 4) + HEX2INT(p3[2]));
      cdestg = cg = ((HEX2INT(p3[3]) << 4) + HEX2INT(p3[4]));
      cdestb = cb = ((HEX2INT(p3[5]) << 4) + HEX2INT(p3[6]));
      fadecolor = (((cb) >> 3) + (((cg) >> 2) << 5) + (((cr) >> 3) << 11));
    }
  else
    {
      cdestr = 0;
      cdestg = 0;
      cdestb = 0;
      fadecolor = 0;
    }
#undef HEX2INT

  fadetable_t *f;

  // check if we already have created one like it
  int n = fadetables.size();
  for (i = 0; i < n; i++)
    {
      f = fadetables[i];
      if (f->lump == -1 &&
	  maskcolor == f->maskcolor &&
	  fadecolor == f->fadecolor &&
	  maskamt == f->maskamt &&
	  fadestart == f->fadestart &&
	  fadeend == f->fadeend &&
	  fog == f->fog &&
	  rgba == f->rgba)
	return f;
    }

  // nope, we have to create it now

  if (n == MAXCOLORMAPS)
    I_Error("R_CreateColormap: Too many colormaps!\n");

  f = new fadetable_t();
  fadetables.push_back(f);

  f->maskcolor = maskcolor;
  f->fadecolor = fadecolor;
  f->maskamt = maskamt;
  f->fadestart = fadestart;
  f->fadeend = fadeend;
  f->fog = fog;
  f->rgba = rgba;


  // OpenGL renderer does not need the colormap part
  if (rendermode == render_soft)
  {
    double deltas[256][3], cmap[256][3];

    for (i = 0; i < 256; i++)
    {
      double r, g, b;
      r = vid.palette[i].r;
      g = vid.palette[i].g;
      b = vid.palette[i].b;
      double cbrightness = sqrt((r*r) + (g*g) + (b*b));

      cmap[i][0] = (cbrightness * cmaskr) + (r * othermask);
      if(cmap[i][0] > 255.0)
        cmap[i][0] = 255.0;
      deltas[i][0] = (cmap[i][0] - cdestr) / (double)fadedist;

      cmap[i][1] = (cbrightness * cmaskg) + (g * othermask);
      if(cmap[i][1] > 255.0)
        cmap[i][1] = 255.0;
      deltas[i][1] = (cmap[i][1] - cdestg) / (double)fadedist;

      cmap[i][2] = (cbrightness * cmaskb) + (b * othermask);
      if(cmap[i][2] > 255.0)
        cmap[i][2] = 255.0;
      deltas[i][2] = (cmap[i][2] - cdestb) / (double)fadedist;
    }

    char *colormap_p = static_cast<char*>(Z_MallocAlign((256 * 34) + 1, PU_LEVEL, 0, 8));
    f->colormap = reinterpret_cast<lighttable_t*>(colormap_p);

    for(p = 0; p < 34; p++)
    {
      for(i = 0; i < 256; i++)
      {
        *colormap_p = NearestColor(RoundUp(cmap[i][0]), RoundUp(cmap[i][1]), RoundUp(cmap[i][2]));
        colormap_p++;

        if((unsigned int)p < fadestart)
          continue;

#define ABS2(x) ((x) < 0 ? -(x) : (x))

        if(ABS2(cmap[i][0] - cdestr) > ABS2(deltas[i][0]))
          cmap[i][0] -= deltas[i][0];
        else
          cmap[i][0] = cdestr;

        if(ABS2(cmap[i][1] - cdestg) > ABS2(deltas[i][1]))
          cmap[i][1] -= deltas[i][1];
        else
          cmap[i][1] = cdestg;

        if(ABS2(cmap[i][2] - cdestb) > ABS2(deltas[i][1]))
          cmap[i][2] -= deltas[i][2];
        else
          cmap[i][2] = cdestb;

#undef ABS2
      }
    }
  }

  return f;
}


//  build a table for quick conversion from 8bpp to 15bpp
static int makecol15(int r, int g, int b)
{
  return (((r >> 3) << 10) | ((g >> 3) << 5) | ((b >> 3)));
}

void R_Init8to16()
{
  //printf("\n [%s][%d]R_Init8to16\n",__FILE__,__LINE__);
  int i;
  byte *palette = static_cast<byte*>(fc.CacheLumpName("PLAYPAL", PU_DAVE));


  //printf(" [%s][%d]R_Init8to16\n",__FILE__,__LINE__);
  for (i=0;i<256;i++)
    {
      // doom PLAYPAL are 8 bit values
      color8to16[i] = makecol15(palette[0],palette[1],palette[2]);
      palette += 3;
    }
 // Z_Free(palette); Marty: Das Crashed!

  //printf(" [%s][%d]R_Init8to16\n",__FILE__,__LINE__);
  // test a big colormap
  hicolormaps = static_cast<short int*>(Z_Malloc(32768 /**34*/, PU_STATIC, 0));
  for (i=0;i<16384;i++)
    hicolormaps[i] = i<<1;
  
  //printf(" [%s][%d]R_Init8to16\n",__FILE__,__LINE__);  
}



//==================================================================
//    Translucency tables
//==================================================================


static int num_transtables;
int  transtable_lumps[MAXTRANSTABLES]; // i'm so lazy
byte *transtables[MAXTRANSTABLES];


/// Tries to retrieve a transmap by name. Returns transtable number, or -1 if unsuccesful.
static int R_TransmapNumForName(const char *name)
{
  int lump = fc.FindNumForName(name);
  if (lump >= 0 && fc.LumpLength(lump) == tr_size)
    {
      for (int i=0; i<num_transtables; i++)
	if (transtable_lumps[i] == lump)
	  return i;

      if (num_transtables == MAXTRANSTABLES)
	I_Error("R_TransmapNumForName: Too many transmaps!\n");
      
      int n = num_transtables++;
      transtables[n] = static_cast<byte*>(Z_MallocAlign(tr_size, PU_STATIC, 0, 16));
      fc.ReadLump(lump, transtables[n]);
      return n;
    }
  return -1;
}


void R_InitTranslucencyTables()
{
  if (devparm)
    CONS_Printf(" Loading translucency tables.\n");

  // NOTE: the translation tables MUST BE aligned on 64k for the asm optimised code
  //       (in other words, transtables pointer low word is 0)
  int i;

  for (i=0; i<MAXTRANSTABLES; i++)
    {
      transtable_lumps[i] = -1;
      transtables[i] = NULL;
    }
    
  transtables[0] = static_cast<byte*>(Z_MallocAlign(tr_size, PU_STATIC, 0, 16));

  // load in translucency tables

  // first transtable
  // check for the Boom default transtable lump
  int lump = fc.FindNumForName("TRANMAP");
  if (lump >= 0)
    fc.ReadLump(lump, transtables[0]);
  else if (game.mode >= gm_heretic)
    fc.ReadLump(fc.GetNumForName("TINTTAB"), transtables[0]);
  else
    fc.ReadLump(fc.GetNumForName("TRANSMED"), transtables[0]); // in legacy.wad

  if (game.mode >= gm_heretic)
    {
      // all the basic transmaps are the same
      for (i=1; i<5; i++)
	transtables[i] = transtables[0];
    }
  else
    {
      // we can use the transmaps in legacy.wad
      for (i=1; i<5; i++)
	transtables[i] = static_cast<byte*>(Z_MallocAlign(tr_size, PU_STATIC, 0, 16));

      fc.ReadLump(fc.GetNumForName("TRANSMOR"), transtables[1]);
      fc.ReadLump(fc.GetNumForName("TRANSHI"),  transtables[2]);
      fc.ReadLump(fc.GetNumForName("TRANSFIR"), transtables[3]);
      fc.ReadLump(fc.GetNumForName("TRANSFX1"), transtables[4]);
    }

  num_transtables = 5;


  // Compose a default linear filter map based on PLAYPAL.
  /*
  // Thanks to TeamTNT for prBoom sources!
  if (false)
    {
      // filter weights
      float w1 = 0.66;
      float w2 = 1 - w1;

      int i, j;
      byte *tp = transtables[0];

      for (i=0; i<256; i++)
	{
	  float r2 = vid.palette[i].red   * w2;
	  float g2 = vid.palette[i].green * w2;
	  float b2 = vid.palette[i].blue  * w2;

	  for (j=0; j<256; j++, tp++)
	    {
	      byte r = vid.palette[j].red   * w1 + r2;
	      byte g = vid.palette[j].green * w1 + g2;
	      byte b = vid.palette[j].blue  * w1 + b2;

	      *tp = NearestColor(r, g, b);
	    }
	}
    }
  */
}



//
// TODO Preloads all relevant graphics for the Map.
//
void Map::PrecacheMap()
{
  // Precache textures.
    //
    // no need to precache all software textures in 3D mode
    // (note they are still used with the reference software view)
    //texturepresent = (char *)alloca(numtextures);
  /*
  char *texturepresent = (char *)Z_Malloc(numtextures, PU_STATIC, 0);
  memset(texturepresent,0, numtextures);

    for (i=0 ; i<numsides ; i++)
    {
        //Hurdler: huh, a potential bug here????
        if (sides[i].toptexture < numtextures)
            texturepresent[sides[i].toptexture] = 1;
        if (sides[i].midtexture < numtextures)
            texturepresent[sides[i].midtexture] = 1;
        if (sides[i].bottomtexture < numtextures)
            texturepresent[sides[i].bottomtexture] = 1;
    }

    // Sky texture is always present.
    // Note that F_SKY1 is the name used to
    //  indicate a sky floor/ceiling as a flat,
    //  while the sky texture is stored like
    //  a wall texture, with an episode dependend
    //  name.
    texturepresent[skytexture] = 1;

    //if (devparm)
    //    CONS_Printf("Generating textures..\n");

    texturememory = 0;
    for (i=0 ; i<numtextures ; i++)
    {
        if (!texturepresent[i])
            continue;

        //texture = textures[i];
        if( material_cache[i]==NULL )
            R_GenerateTexture (i);
        //numgenerated++;

        // note: pre-caching individual patches that compose textures became
        //       obsolete since we now cache entire composite textures

        //for (j=0 ; j<texture->patchcount ; j++)
        //{
        //    lump = texture->patches[j].patch;
        //    texturememory += fc.LumpLength(lump);
        //    fc.CacheLumpNum(lump , PU_CACHE);
        //}
    }
    Z_Free(texturepresent);
  */
    //CONS_Printf ("total mem for %d textures: %d k\n",numgenerated,texturememory>>10);

    //
    // Precache sprites.
    //
    /*
    spritepresent = (char *)alloca(numsprites);
    spritepresent = (char *)Z_Malloc(numsprites, PU_STATIC, 0);
    memset (spritepresent,0, numsprites);

    Thinker *th;

    for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    {
      if (th->Type() == Thinker::tt_dactor)
        spritepresent[((DActor *)th)->sprite] = 1;
    }

    spriteframe_t*      sf;
    spritememory = 0;
    for (i=0 ; i<numsprites ; i++)
    {
        if (!spritepresent[i])
            continue;

        for (j=0 ; j<sprites[i].numframes ; j++)
        {
            sf = &sprites[i].spriteframes[j];
            for (k=0 ; k<8 ; k++)
            {
                //Fab: see R_InitSprites for more about lumppat,lumpid
                lump = sf->lumppat[k];
                if(devparm)
                   spritememory += fc.LumpLength(lump);
                fc.CachePatchNum(lump , PU_CACHE);
            }
        }
    }
    Z_Free(spritepresent);

    if (devparm)
    {
        CONS_Printf("Precache level done:\n"
                    "flatmemory:    %ld k\n"
                    "texturememory: %ld k\n"
                    "spritememory:  %ld k\n", flatmemory>>10, texturememory>>10, spritememory>>10 );
    }
    */
}
