// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_data.h 536 2009-06-29 06:46:13Z smite-meister $
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
/// \brief Texture cache, Texture classes, fadetables.

#ifndef r_data_h
#define r_data_h 1

/* OpenGL Init --------------------
 * liegt in oglinit.h
   #include <GL/gl.h>
 */
#include "Hardware/ogl_init.h"
/* OpenGL Init End ------------- */
	
#include <vector>
#include <set>

#include "doomtype.h"
#include "m_fixed.h"
#include "z_cache.h"


#define TRANSPARENTPIXEL 247

/// \brief patch_t, the strange Doom graphics format.
///
/// A patch holds one or more columns, which consist of posts separated by holes.
/// A post is a vertical run of pixels.
/// Patches are used for sprites and all masked pictures,
/// and we compose textures from the TEXTURE1/2 lists of patches.
struct patch_t
{
  Uint16 width;         ///< bounding box size
  Uint16 height;
  Sint16 leftoffset;    ///< pixels to the left of origin
  Sint16 topoffset;     ///< pixels above the origin
  Uint32 columnofs[0];  ///< [width] byte offsets to the columns
} __attribute__((packed));


/// posts are vertical runs of nonmasked source pixels in a patch_t
struct post_t
{
  byte topdelta; ///< how many pixels to skip, -1 (0xff) means the column ends
  byte length;   ///< number of data bytes
  byte crap;     ///< post border, should not be drawn
  byte data[0];  ///< data starts here, ends with another crap byte (not included in length)
} __attribute__((packed));


/// column_t is a list of 0 or more post_t, (byte)-1 terminated
typedef post_t column_t;



/// \brief pic_t, a graphics format native to Legacy. No longer supported.
///
/// a pic is an unmasked block of pixels, stored in horizontal way
/*
struct pic_t
{
  enum pic_mode_t
  {
    PALETTE         = 0,  // 1 byte is the index in the doom palette (as usual)
    INTENSITY       = 1,  // 1 byte intensity
    INTENSITY_ALPHA = 2,  // 2 byte : alpha then intensity
    RGB24           = 3,  // 24 bit rgb
    RGBA32          = 4,  // 32 bit rgba
  };

  short  width;
  byte   zero;   // set to 0 allow autodetection of pic_t 
                   // mode instead of patch or raw
  byte   mode;   // see pic_mode_t above
  short  height;
  short  reserved1;  // set to 0
  byte   data[0];
};
*/

//===============================================================================

/// \brief ABC for all 2D bitmaps.
///
/// Hasta la vista, old texture/flat system!
/// This ABC represents one bitmap texture at runtime.
/// They are built out of patch_t's, flats and other image files during loading.
class Texture : public cacheitem_t
{
protected:
  /// Indexed column-major data for sw renderer, built and accessed using GetData(), OR
  /// row-major data for OpenGL, built using GLGetData(), format in 'format'.
  /// NOTE: GLPrepare _must not_ be used on a Texture if any of the "sw renderer" functions are.
  byte *pixels;

public:
  int    lump; ///< lump where the texture data comes from
  short  leftoffs, topoffs; ///< external image offsets in pixels (mostly unused)
  short  width, height;  ///< bitmap dimensions in texels
  byte   w_bits, h_bits; ///< largest power-of-two sizes <= actual bitmap size

  /// To be called after bitmap size is known
  inline void InitializeTexture()
  {
    for (w_bits = 0; 1 << (w_bits+1) <= width;  w_bits++);
    for (h_bits = 0; 1 << (h_bits+1) <= height; h_bits++);
  }

  /// \name OpenGL renderer
  //@{
protected:
  GLenum   gl_format; ///< Storage format of pixels for OpenGL.
  GLuint   gl_id;     ///< OpenGL handle. Accessed using GLPrepare().
  static const GLuint NOTEXTURE = 0;

  /// Helper function for preparing OpenGL textures. Sets 'pixels' pointing to data in format described by 'format'.
  virtual void GLGetData();

public:
  /// Creates an OpenGL texture if necessary, returns the handle. Uses the virtualized GLGetData().
  virtual GLuint GLPrepare();

  /// Unloads the texture from OpenGL. 
  bool ClearGLTexture();
  //@}

public:
  Texture(const char *name, int lump = -1);
  virtual ~Texture();

  /// \name Software renderer
  //@{
  /// True means that texture is available in column_t format using GetMaskedColumn
  virtual bool Masked() { return false; };

  /// Get masked indexed texture column data. if Masked() is false, returns NULL
  virtual column_t *GetMaskedColumn(fixed_t col) { return NULL; };

  /// Get unmasked indexed texture column data.
  virtual byte *GetColumn(fixed_t col);

  /// Get indexed column-major texture data.
  virtual byte *GetData() { return NULL; }; // Could also be pure virtual, but this way we can use Texture for a link cacheitem_t.

  /// Draw the Texture in the LFB.
  virtual void Draw(byte *dest_tl, byte *dest_tr, byte *dest_bl,
		    fixed_t col, fixed_t row, fixed_t colfrac, fixed_t rowfrac, int flags) {};

  /// Tile an area of the screen with the Texture.
  virtual void DrawFill(int x, int y, int w, int h) {};
  //@}
};


/// \brief Textures which reside in a single lump.
/// This class handles raw pages and Doom flats, derived classes handle png and jpeg,
/// ideally by redefining the constructor and the *GetData() functions only.
class LumpTexture : public Texture
{
protected:
  virtual void GLGetData();

public:
  LumpTexture(const char *name, int lump, int w, int h);

  virtual byte *GetData();
  virtual void Draw(byte *dest_tl, byte *dest_tr, byte *dest_bl,
		    fixed_t col, fixed_t row, fixed_t colfrac, fixed_t rowfrac, int flags);
  virtual void DrawFill(int x, int y, int w, int h);
};



/// \brief Class for PNG Textures
class PNGTexture : public LumpTexture
{
protected:
  bool ReadData(bool read_image, bool col_major);
  virtual void GLGetData();

public:
  PNGTexture(const char *name, int lump);
  virtual byte *GetData();
};



/// \brief Class for JPEG/JFIF Textures
class JPEGTexture : public LumpTexture
{
protected:
  bool ReadData(bool read_image, bool col_major);
  virtual void GLGetData();

public:
  JPEGTexture(const char *name, int lump);
  virtual byte *GetData();
};



/// \brief Class for TGA Textures
class TGATexture : public LumpTexture
{
protected:
  byte *data;      ///< raw TGA file data
  int imageoffset; ///< pixels == &data[imageoffset]
  int bpp;         ///< bits per pixel

  virtual void GLGetData();

public:
  TGATexture(const char *name, int lump);
  virtual ~TGATexture();
  virtual byte *GetData();
};



/// \brief Class for patch_t's.
///
/// This is trickier than a LumpTexture, because the data could be required in two different formats:
/// patch_t (masked) and raw.
class PatchTexture : public Texture
{
protected:
  byte *patch_data; ///< texture in patch_t format, pixels has it in raw column-major format

  patch_t *GeneratePatch();

public:
  PatchTexture(const char *name, int lump);
  virtual ~PatchTexture();

  virtual bool Masked() { return true; };
  virtual column_t *GetMaskedColumn(fixed_t col);
  virtual byte *GetData();

  virtual void Draw(byte *dest_tl, byte *dest_tr, byte *dest_bl,
		    fixed_t col, fixed_t row, fixed_t colfrac, fixed_t rowfrac, int flags);
};



/// \brief Doom Textures (which are built out of patches)
///
/// All the patches are drawn back to front into the cached texture.
/// In SW mode, a single-patch DoomTexture is also stored in patch_t format.
class DoomTexture : public Texture
{
public:
  /// A single patch from a texture definition,
  /// basically a rectangular area within the texture rectangle.
  struct texpatch_t
  {
    /// Block origin (always UL), which has already accounted for the internal origin of the patch.
    int originx, originy;
    /// lump number for the patch
    int patchlump;
  };

  short       patchcount; ///< number of patches in the texture
  texpatch_t *patches;    ///< array for the patch definitions

  Uint32     *columnofs;   ///< offsets from texdata to raw column data
  byte       *patch_data;  ///< texture data in patch_t format

protected:
  short       widthmask;  ///<  (1 << w_bits) - 1

  patch_t *GeneratePatch();

public:
  DoomTexture(const char *name, int lump, const struct maptexture_t *mtex);
  virtual ~DoomTexture();

  virtual bool Masked() { return (patchcount == 1); };
  virtual column_t *GetMaskedColumn(fixed_t col);
  virtual byte *GetColumn(fixed_t col);
  virtual byte *GetData();
};


//===============================================================================

/// \brief Cache for Textures.
class texture_cache_t : public cache_t<Texture>
{
  friend class material_cache_t;
protected:
  /// Generates a Texture from a single data lump, deduces format.
  virtual Texture *Load(const char *name);

public:
  /// Creates a Texture with a given name from a given lump.
  Texture *LoadLump(const char *name, int lump);
};


extern texture_cache_t textures;


//===============================================================================

/// "Material" definition, uses one or more Textures and maybe shaders.
class Material : public cacheitem_t
{
  friend class material_cache_t;
public:
  int   id_number; ///< index of this Material in the all_materials vector
  float worldwidth, worldheight; ///< dimensions in world units (from tex0)
  float leftoffs, topoffs;       ///< external image offsets in world units (from tex0) (used by sprites and HUD graphics)

  struct TextureRef
  {
  public:
    Texture *t; ///< Texture we are referring to

    float worldwidth, worldheight; ///< dimensions in world units
    float xscale, yscale;          ///< texel-size / world-size    
    GLint mag_filter, min_filter;  ///< OpenGL magnification and minification filters to use
    float max_anisotropy;          ///< Maximum amount of filtering anisotropy.

    // TODO blending type

  public:
    /// default constructor
    TextureRef();

    /// To be called after bitmap size and scale are known
    inline void InitializeTexRef()
    {
      worldwidth  = t->width / xscale;
      worldheight = t->height / yscale;
    }

    /// Binds the Texture and sets its params
    void GLSetTextureParams();
  };

  std::vector<TextureRef> tex; ///< allow several textures per material
  class ShaderProg *shader;

  void InitializeMaterial()
  {
    int n = tex.size();
    for (int k=0; k < n; k++)
      tex[k].InitializeTexRef();

    TextureRef &tr = tex[0];

    worldwidth = tr.worldwidth;
    worldheight = tr.worldheight;
    leftoffs = tr.t->leftoffs / tr.xscale;
    topoffs = tr.t->topoffs / tr.yscale;
  };

public:
  Material(const char *name); ///< Create a Material with no Textures. Requires a call to InitializeMaterial when Textures have been attached.
  Material(const char *name, Texture *t, float xscale = 1, float yscale = 1); ///< Create a single-Texture Material.

  virtual ~Material();

  /// \name Software renderer functions, these are wrappers for tex[0].
  //@{
  /// True means that texture is available in column_t format using GetMaskedColumn
  inline bool Masked() { return tex[0].t->Masked(); };

  /// Get masked indexed texture column data. if Masked() is false, returns NULL
  inline column_t *GetMaskedColumn(fixed_t col) { return tex[0].t->GetMaskedColumn(col * tex[0].xscale); }

  /// Get unmasked indexed texture column data.
  inline byte *GetColumn(fixed_t col) { return tex[0].t->GetColumn(col * tex[0].xscale); }

  /// Get indexed column-major texture data.
  inline byte *GetData() { return tex[0].t->GetData(); }
  //@}

public:
  /// draw the Material flat on screen.
  void Draw(float x, float y, int scrn); // scrn may contain flags

  /// tile an area of the screen with the Texture
  void DrawFill(int x, int y, int w, int h);

  /// Make OpenGL to use the Material. After this you can call glBegin(). Returns the number of texture units required.
  int GLUse();
};


//===============================================================================

/// Material lookup orders
enum material_class_t
{
  TEX_wall = 0,
  TEX_floor,
  TEX_sprite,
  TEX_lod
};


/// \brief Cache for Materials
class material_cache_t
{
protected:
  cachesource_t<Material> new_tex,   ///< advanced textures, TX_START
    doom_tex,   ///< TEXTUREx/PNAMES
    flat_tex,   ///< F_START
    sprite_tex, ///< advanced spritetextures, S_START
    lod_tex;    ///< load-on-demand textures, mostly for misc. graphics

  Material   *default_item; ///< the default data item

  /// All known Materials stored here in the order they were created, each also resides in one cachesource_t.
  /// Used for texture animation.
  std::vector<Material*> all_materials;
  typedef std::vector<Material*>::iterator material_iterator_t;

  /// Inserts the Material into the all_materials vector and gives it a corresponding id.
  inline void Register(Material *m)
  {
    m->id_number = all_materials.size();
    all_materials.push_back(m);
  }


  /// Creates a Material from the Texture, inserts it to the given source, inserts the Texture to texturecache.
  Material *BuildMaterial(Texture *t, cachesource_t<Material> &source, bool h_start = false);

  /// sw renderer: colormaps for palette conversions (one for each resource file)
  std::vector<byte *> palette_conversion;
  /// OpenGL renderer: lump containing PLAYPAL meant for this resource file
  std::vector<int> palette_lump;

public:
  /// Returns the Material with id_number id, or NULL.
  inline Material *GetID(unsigned id)
  {
    return (id < all_materials.size()) ? all_materials[id] : NULL;
  }

  /// creates the palette conversion colormaps
  void InitPaletteConversion();

  /// returns the palette conversion colormap for the given file
  inline byte *GetPaletteConv(int i) { return palette_conversion[i]; }

  /// returns the palette lump for the given file
  inline int GetPaletteLump(int i) { return palette_lump[i]; }

public:
  material_cache_t();
  ~material_cache_t();

  /// sets the default Material
  void SetDefaultItem(const char *name);

  /// empties the cache, deletes all Materials
  void Clear();

  /// Deletes all OpenGL textures within Materials
  void ClearGLTextures();

  /// Returns an existing Material for updating. Does not change reference counts. For animated materials and NTEXTURE.
  /// If none exists, either creates a new blank Material in cache or returns NULL.
  Material *Edit(const char *name, material_class_t mode, bool create = false);

  /// Returns the named Material (with TEX_lod, tries creating it if nonexistant), or the default material if it does not exist.
  Material *Get(const char *name, material_class_t mode = TEX_lod);

  /// Finds the name of the lump n, then acts like Get(name, TEX_lod);
  Material *GetLumpnum(int n);

  /// Like Get, but truncates name to 8 chars (use with Doom map data structures!).
  Material *Get8char(const char *name, material_class_t mode = TEX_wall);

  /// First checks if the lump is a valid colormap (or transmap). If not, acts like Get(name, TEX_wall);
  Material *GetMaterialOrColormap(const char *name, class fadetable_t*& cmap);
  Material *GetMaterialOrTransmap(const char *name, int& map_num);

  /// Reads the TEXTUREn/PNAMES lumps and F_START lists, generates the corresponding Materials.
  int ReadTextures();
};


extern material_cache_t materials;


/// \brief Set of 34 lightlevel colormaps from COLORMAP lump, also called a fadetable.
///
/// Used also for "colored lighting" colormaps from Boom etc.
class fadetable_t
{
  typedef byte lighttable_t;
public:
  int             lump; ///< the lump number of the colormap
  lighttable_t   *colormap;

  // GL emulation for the fadetable
  unsigned short  maskcolor;
  unsigned short  fadecolor;
  double          maskamt;
  unsigned short  fadestart, fadeend;
  int             fog;

  //Hurdler: rgba is used in hw mode for coloured sector lighting
  int             rgba; // similar to maskcolor in sw mode

public:
  fadetable_t();
  fadetable_t(int lumpnum);
  ~fadetable_t();
};



// Quantizes an RGB color into current palette
byte NearestColor(byte r, byte g, byte b);

// initializes the part of the renderer that even a dedicated server needs
void R_ServerInit();

// initialize texture animations
int R_Read_ANIMATED(int lump);
int R_Read_ANIMDEFS(int lump);


// colormap management
void R_InitColormaps();
void R_ClearColormaps();
fadetable_t *R_GetFadetable(unsigned n);
fadetable_t *R_FindColormap(const char *name);
fadetable_t *R_CreateColormap(char *p1, char *p2, char *p3);
const char *R_ColormapNameForNum(const fadetable_t *p);

#endif
