// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: r_sprite.h 533 2009-06-11 08:31:10Z smite-meister $
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
/// \brief Sprites, sprite presentations and sprite skins.

#ifndef r_sprite_h
#define r_sprite_h 1

#include "doomtype.h"
#include "r_presentation.h"
#include "z_cache.h"


//========================================================
//                       Sprites
//========================================================

/// \brief One frame of a sprite_t
///
/// A sprite is a set of Materials that represent
/// a three dimensional object seen from different angles.
/// Doom sprites in wads are patches with a special naming convention
/// so they can be recognized by R_InitSprites.
/// The base name is NNNNFx or NNNNFxFx, with x indicating the rotation, x = 0--7.
/// Horizontal flipping is used to save space, thus NNNNF2F5 defines a mirrored patch.
/// Some sprites only have one picture used for all directions: NNNNF0
struct spriteframe_t
{
  // If false use 0 for any position.
  // NOTE: as eight entries are available,
  //  we might as well insert the same name eight times.
  char   rotate;

  class Material *tex[8]; ///< Material to use for view angles 0-7.
  bool           flip[8]; ///< Flip bit to use for view angles 0-7.
};



/// \brief An animated collection of 2D frames
class sprite_t : public cacheitem_t
{
  friend class spritecache_t;
  friend class spritepres_t;
  //protected: // FIXME R_DrawPSprite() wants to use spriteframes directly.
public:
  sprite_t(const char *name);
  virtual ~sprite_t();

  Sint32  iname;  ///< sprite name (4 chars) as an int
  int            numframes;
  spriteframe_t *spriteframes;
};



/// \brief Cache for sprite_t's.
class spritecache_t : public cache_t<sprite_t>
{
protected:
  virtual sprite_t *Load(const char *name);
};

extern spritecache_t sprites;




/// \brief Sprite presentation
///
/// Sprites can be animated in two ways, either using SetAnim or SetFrame.
/// Both use the states table. A sprite presentation is defined by an ActorInfo.
class spritepres_t : public presentation_t
{
protected:
  sprite_t *spr;
  const class ActorInfo *info; ///< this is used to know which sequence corresponds to which state
  const state_t *state; ///< animation frames are tied to the states table

public:
  spritepres_t() {}; ///< simple constructor for unserialization
  spritepres_t(const ActorInfo *inf, int col = 0); ///< normal constructor
  spritepres_t(class TNL::BitStream *s);
  virtual ~spritepres_t();

  virtual void SetFrame(const state_t *st); // Only used by DActors with sprites
  virtual void SetAnim(animseq_e seq);

  virtual bool Update(int nowtic);
  virtual void Project(Actor *p);
  virtual bool Draw(const Actor *p);
  virtual spriteframe_t *GetFrame();
  virtual int  Marshal(LArchive &a);

  /// Netcode
  virtual void   Pack(class TNL::BitStream *s);
  virtual void Unpack(class TNL::BitStream *s) {}
  virtual void   PackAnim(class TNL::BitStream *s);
  virtual void UnpackAnim(class TNL::BitStream *s);
};





//========================================================
//                    Sprite skins
//========================================================


struct skin_t : public cacheitem_t
{
  // 10 customisable sounds for Skins
  enum skinsound_t
  {
    SKSPLPAIN,
    SKSSLOP,
    SKSOOF,
    SKSPLDETH,
    SKSPDIEHI,
    SKSNOWAY,
    SKSPUNCH,
    SKSRADIO,
    SKSJUMP,
    SKSOUCH,
    NUMSKINSOUNDS
  };

#define SKIN_NUMFACES 42

  sprite_t *sprite;
  char      faceprefix[4];      ///< 3 chars+'\0', default is "STF"
  Material *faces[SKIN_NUMFACES]; ///< marine face graphics
  Material *faceback;           ///< face background

  // specific sounds per skin
  //short       soundsid[NUMSKINSOUNDS]; // sound # in S_sfx table

public:
  skin_t(const char *name, const char *spritename, const char *fprefix);
  ~skin_t();
};



skin_t *GetSkin();
void    SetPlayerSkin(int playernum,char *skinname);
void    R_AddSkins(int wadnum);

#endif
