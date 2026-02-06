// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
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
/// \brief Font system.

#include "console.h"
#include "g_game.h"

#include "r_data.h"
#include "screen.h"
#include "v_video.h"
#include "i_video.h"

#include "w_wad.h"
#include "z_zone.h"

#define NO_TTF // TrueType fonts off by default

/// ucs2 because SDL_ttf currently only understands Unicode codepoints within the BMP.
static int utf8_to_ucs2(const char *utf8, Uint16 *ucs2)
{
  int ch = *(const byte *)utf8;

  // TODO check that in multibyte seqs bytes other than first start with bits 10, otherwise give error

  if (ch < 0x80)
    {
      *ucs2 = ch;
      return 1;
    }

  int n = strlen(utf8);

  if (ch < 0xe0) // >= 0xc0)
    {
      if (n < 2)
	return 0;
      ch  = int(utf8[0] & 0x1f) << 6;
      ch |= int(utf8[1] & 0x3f);
      *ucs2 = ch;
      return 2;
    }
  else if (ch < 0xf0)
    {
      if (n < 3)
	return 0;
      ch  = int(utf8[0] & 0x0f) << 12;
      ch |= int(utf8[1] & 0x3f) << 6;
      ch |= int(utf8[2] & 0x3f);
      *ucs2 = ch;
      return 3;
    }
  else
    {
      if (n < 4)
	return 0;
      ch  = int(utf8[0] & 0x07) << 18;
      ch |= int(utf8[1] & 0x3f) << 12;
      ch |= int(utf8[2] & 0x3f) << 6;
      ch |= int(utf8[3] & 0x3f);
      *ucs2 = ch;
      return 4;
    }
}



#define HU_FONTSTART    '!'     // the first font character
#define HU_FONTEND      '_'     // the last font character
#define HU_FONTSIZE (HU_FONTEND - HU_FONTSTART + 1) // default font size

// Doom:
// STCFN033-95 + 121 : small red font

// Heretic:
// FONTA01-59 : medium silver font
// FONTB01-58 : large green font, some symbols empty

// Hexen:
// FONTA01-59 : medium silver font
// FONTAY01-59 : like FONTA but yellow
// FONTB01-58 : large red font, some symbols empty

font_t *hud_font;
font_t *big_font;


font_t::~font_t()
{}


//======================================================================
//                          RASTER FONTS
//======================================================================

/// \brief Doom-style raster font
class rasterfont_t : public font_t
{
protected:
  char  start, end;     ///< first and last ASCII characters included in the font
  std::vector<class Material*> font; ///< one Material per symbol

public:
  rasterfont_t(int startlump, int endlump, char firstchar = '!');
  virtual ~rasterfont_t();

  virtual float DrawCharacter(float x, float y, int c, int flags);
  virtual float StringWidth(const char *str);
  virtual float StringWidth(const char *str, int n);
};



rasterfont_t::rasterfont_t(int startlump, int endlump, char firstchar)
{
  if (startlump < 0 || endlump < 0)
    I_Error("Incomplete font!\n");

  int truesize = endlump - startlump + 1; // we have this many lumps
  char lastchar = firstchar + truesize - 1;

  // the font range must include '!' and '_'. We will duplicate letters if need be.
  start = min(firstchar, '!');
  end = max(lastchar, '_');
  int size = end - start + 1;

  font.resize(size);

  for (int i = start; i <= end; i++)
    if (i < firstchar || i > lastchar)
      // replace the missing letters with the first char
      font[i - start] = materials.GetLumpnum(startlump);
    else
      font[i - start] = materials.GetLumpnum(i - firstchar + startlump);

  // use the character '0' as a "prototype" for the font
  if (start <= '0' && '0' <= end)
    {
      lineskip = font['0' - start]->worldheight + 1;
      advance  = font['0' - start]->worldwidth;
    }
  else
    {
      lineskip = font[0]->worldheight + 1;
      advance  = font[0]->worldwidth;
    }
}


rasterfont_t::~rasterfont_t()
{
  for (int i = start; i <= end; i++)
    font[i - start]->Release();

  font.clear();
}


float rasterfont_t::DrawCharacter(float x, float y, int c, int flags)
{
  if (flags & V_WHITEMAP)
    {
      current_colormap = whitemap;
      flags |= V_MAP;
    }

  c = toupper(c & 0x7f); // just ASCII here
  if (c < start || c > end)
    return 4; // render a little space for unknown chars in DrawString

  Material *m = font[c - start];
  m->Draw(x, y, flags);

  return m->worldwidth;
}



//  Draw a string using the font
//  NOTE: the text is centered for screens larger than the base width
float font_t::DrawString(float x, float y, const char *str, int flags)
{
  float dupx = 1;
  float dupy = 1;

  if (rendermode == render_opengl)
    {
      if (flags & V_SSIZE)
	{
	  dupx = vid.fdupx;
	  dupy = vid.fdupy;
	}

      if (flags & V_SLOC)
	{
	  x *= vid.fdupx;
	  y *= vid.fdupy;
	  flags &= ~V_SLOC; // not passed on to Texture::Draw
	}
    }
  else
    {
      if (flags & V_SSIZE)
	{
	  dupx = vid.dupx;
	  dupy = vid.dupy;
	}

      if (flags & V_SLOC)
	{
	  x *= vid.dupx;
	  y *= vid.dupy;
	  flags &= ~V_SLOC; // not passed on to Texture::Draw
	}
    }

  // cursor coordinates
  float cx = x;
  float cy = y;
  float rowheight = lineskip * dupy;

  while (*str)
    {
      Uint16 c;
      int skip = utf8_to_ucs2(str, &c);
      if (!skip) // bad ucs-8 string
        break;

      str += skip;

      if (c == '\n')
        {
          cx = x;
	  cy += rowheight;
          continue;
        }

      cx += dupx * DrawCharacter(cx, cy, c, flags);
    }

  return cx - x;
}


// returns the width of the string (unscaled)
float rasterfont_t::StringWidth(const char *str)
{
  float w = 0;

  while (*str)
    {
      Uint16 c;
      int skip = utf8_to_ucs2(str, &c);
      if (!skip) // bad ucs-8 string
        return 0;

      str += skip;

      c = toupper(c & 0x7f); // just ASCII
      if (c < start || c > end)
        w += 4;
      else
        w += font[c - start]->worldwidth;
    }

  return w;
}


// returns the width of the next n chars of str
float rasterfont_t::StringWidth(const char *str, int n)
{
  float w = 0;

  for ( ; *str && n > 0; n--)
    {
      Uint16 c;
      int skip = utf8_to_ucs2(str, &c);
      if (!skip) // bad ucs-8 string
        return 0;

      str += skip;

      c = toupper(c & 0x7f); // just ASCII
      if (c < start || c > end)
        w += 4;
      else
        w += font[c - start]->worldwidth;
    }

  return w;
}



//======================================================================
//                         TRUETYPE FONTS
//======================================================================

#ifndef NO_TTF
#include <GL/glu.h>
#include "SDL_ttf.h"


#define SCALE 1.0f

/// \brief TrueType font
class ttfont_t : public font_t
{
protected:
  byte     *data; ///< Raw contents of the .ttf file.
  TTF_Font *font;

  int ascent; ///< max ascent of all glyphs in the font in texels

  struct glyph_t
  {
    int minx, maxx, miny, maxy, advance;
    Material *mat;

    ~glyph_t()
    {
      // The glyph textures nor the materials are in the main cache, so we destroy them directly.
      if (mat)
	{
	  delete mat->tex[0].t;
	  mat->tex.clear(); // so the Material destructor won't cause trouble...
	  delete mat;
	  mat = NULL;
	}
    }
  };

  glyph_t glyphcache[256]; ///< Prerendered glyphs for Latin-1 chars to improve efficiency.

  bool BuildGlyph(glyph_t &g, int c, SDL_Color color);

public:
  ttfont_t(int lump)
  {
    data = static_cast<byte*>(fc.CacheLumpNum(lump, PU_DAVE));
    font = TTF_OpenFontRW(SDL_RWFromConstMem(data, fc.LumpLength(lump)), true, 24);
    //font = TTF_OpenFont("font.ttf", 16); // TEST

    ascent = TTF_FontAscent(font);
    lineskip = TTF_FontLineSkip(font)/SCALE; // monospace the font in y direction
    int ad = 0;
    TTF_GlyphMetrics(font, '0', NULL, NULL, NULL, NULL, &ad);
    advance = ad/SCALE;

    for (int k=0; k<256; k++)
      glyphcache[k].mat = NULL;
  }

  virtual ~ttfont_t()
  {
    TTF_CloseFont(font);
    font = NULL;
    Z_Free(data);
    data = NULL;
    // the glyph_t's destroy themselves
  }

  const char *GetFaceFamilyName() { return TTF_FontFaceFamilyName(font); }
  const char *GetFaceStyleName() { return TTF_FontFaceStyleName(font); }

  virtual float StringWidth(const char *str)
  {
    int w, h;
    return TTF_SizeUTF8(font, str, &w, &h) ? 0 : w/SCALE;
  }

  virtual float StringWidth(const char *str, int n)
  {
    int len = 0;
    Uint16 c;
    // count how many bytes the n letters comprise
    for (int k=0; k<n && str[len]; k++)
      {
	int skip = utf8_to_ucs2(&str[len], &c);
	if (!skip) // bad utf8 string
	  return 0;
	len += skip;
      }

    char temp[len+1];
    strncpy(temp, str, len);
    temp[len] = 0;
    int w, h;
    return TTF_SizeUTF8(font, temp, &w, &h) ? 0 : w/SCALE;
  }

  virtual float DrawCharacter(float x, float y, int c, int flags);

  /*
  virtual bool CanCompose() const { return true; }
  /// Renders the UFT-8 string using kerning into a Material.
  virtual class Material *ComposeString(const char *str)
  {
    SDL_Surface *text = TTF_RenderUTF8_Solid(font, str, c);
    SDL_Surface *text = TTF_RenderUTF8_Blended(font, str, c);
  }
  */
};



/// SDL_Surface -based Texture
class SDLTexture : public LumpTexture
{
protected:
  SDL_Surface *surf;

  virtual void GLGetData() {} ///< Sets up pixels.

public:
  virtual GLuint GLPrepare()
  {
    if (gl_id == NOTEXTURE)
    {
      glGenTextures(1, &gl_id);
      glBindTexture(GL_TEXTURE_2D, gl_id);
      // default params
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, gl_format, GL_UNSIGNED_BYTE, pixels);

      //  CONS_Printf("Created GL texture %d for %s.\n", gl_id, name);
      // do not free pixels!!
    }

    return gl_id;
  }

  SDLTexture(const char *n, SDL_Surface *s)
    : LumpTexture(n, -1, s->w, s->h)
  {
    surf = s;
    pixels = static_cast<byte*>(surf->pixels);

    // For some strange reason SDL_ttf uses BGRA order.
    gl_format = GL_BGRA;
  }

  virtual ~SDLTexture()
  {
    SDL_FreeSurface(surf);
    surf = NULL;
    pixels = NULL;
  }

  virtual byte *GetData() { return pixels; }
  /*
  {
  if (!pixels)
    {
      Z_Malloc(width*height, PU_TEXTURE, (void **)(&pixels));

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
*/
};



bool ttfont_t::BuildGlyph(glyph_t &g, int c, SDL_Color color)
{
  TTF_GlyphMetrics(font, c, &g.minx, &g.maxx, &g.miny, &g.maxy, &g.advance);

  SDL_Surface *glyph = TTF_RenderGlyph_Blended(font, c, color);
  if (!glyph)
    {
      CONS_Printf("TTF rendering error: %s\n", TTF_GetError());
      return false;
    }

  string name("glyph ");
  name += c;

  Texture *t = new SDLTexture(name.c_str(), glyph);
  t->topoffs = g.maxy - ascent; // clever!
  t->leftoffs = -g.minx;

  g.mat = new Material(name.c_str(), t, SCALE, SCALE); // create a new Material
  return true;
}


float ttfont_t::DrawCharacter(float x, float y, int c, int flags)
{
  static SDL_Color white = {255, 255, 255, 0};
  static SDL_Color red   = {255,   0,   0, 0};
  
  if (flags & V_WHITEMAP)
    {
    }

  float ret;


  // TODO glyphcache into a std::map
  if (c < 32) // control chars
    return 0;
  else if (c < 256) // Latin-1
    {
      glyph_t &g = glyphcache[c];
      if (!g.mat)
	{
	  // not found, render and insert into glyph cache
	  if (!BuildGlyph(g, c, red))
	    return 0;
	}
      g.mat->Draw(x, y, flags);
      ret = g.advance / SCALE;
    }
  else
    {
      glyph_t temp;
      if (!BuildGlyph(temp, c, red))
	return 0;

      temp.mat->Draw(x, y, flags);
      ret = temp.advance / SCALE;
      // TODO temp is deleted, incredibly wasteful!!!
    }

  return ret;
}


#endif




// initialize the font system, load the fonts
void font_t::Init()
{
  int startlump, endlump;

  // "hud font"
  switch (game.mode)
    {
    case gm_heretic:
    case gm_hexen:
      startlump = fc.GetNumForName("FONTA01");
      endlump  =  fc.GetNumForName("FONTA59");
      break;
    default:
      startlump = fc.GetNumForName("STCFN033");
      endlump =   fc.GetNumForName("STCFN095");
    }

  hud_font = new rasterfont_t(startlump, endlump);

  // "big font"
  if (fc.FindNumForName("FONTB_S") < 0)
    big_font = NULL;
  else
    {
      startlump = fc.FindNumForName("FONTB01");
      endlump   = fc.GetNumForName("FONTB58");
      big_font = new rasterfont_t(startlump, endlump);
    }

#ifndef NO_TTF
  if (TTF_Init() < 0)
    {
      CONS_Printf("TTF_Init error: %s\n", TTF_GetError());
      return;
    }

  int lump = fc.FindNumForName("TTF_TEST");
  if (lump < 0)
    {
      I_Error("font_t::Init: Default TrueType font not found!\n");
    }
  ttfont_t *tt = new ttfont_t(lump);

  const char *p = tt->GetFaceFamilyName();
  if (p)
    CONS_Printf("TT font face family name: %s\n", p);

  p = tt->GetFaceStyleName();
  if (p)
    CONS_Printf("TT font face style name: %s\n", p);

  // TTF_Quit();
  hud_font = tt; // TEST
#endif
}
