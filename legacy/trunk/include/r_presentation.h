// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
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
/// \brief Game entity graphic presentations.

#ifndef r_presentation_h
#define r_presentation_h 1

#include "doomtype.h"


//========================================================
//                     Presentations
//========================================================

/// \brief ABC for presentation objects
///
/// Idea: Game entities have a pointer to a graphic presentation.
/// The animation data is stored in the presentation object.
/// The actual implementation of the "graphic presentation" can be a sprite, MD3 or anything.
class presentation_t
{
protected:
  //vector<animation_t> anim; // all known animation sequences
public:
  enum animseq_e
  {
    // Legacy "animation sequences"
    Idle = 0, ///< spawnstate
    Run,      ///< seestate
    Back,     ///< walk backwards (used only with players)
    LAST_LOOPING = Back,
    Pain,     ///< painstate
    Melee,    ///< meleestate (attack 1)
    Shoot,    ///< missilestate (attack 2)
    
    Death1,   ///< deathstate
    Death2,   ///< xdeathstate (explode, die)
    Death3,   ///< crashstate, one more way to die (heretic/hexen imps)

    Raise     ///< raisestate, being raised from death by an arch-vile
  };

  char  color;       ///< Skin colormap.
  animseq_e animseq; ///< Current animation sequence.
  int   flags;       ///< Effects. Translucency, fullbright etc.
  int   lastupdate;  ///< Time of last update in tics.

  virtual ~presentation_t() {};

  virtual void SetFrame(const struct state_t *st) = 0; // Only used by DActors with sprites
  virtual void SetAnim(animseq_e seq) = 0;  // starts a requested animation sequence
  int GetAnim() { return animseq; };

  virtual bool Update(int nowtic)      = 0; // Updates the animation, called before drawing
  virtual void Project(class Actor *p) = 0; ///< Drawing in SW renderer. Generates a vissprite_t.
  virtual bool Draw(const Actor *p) = 0; ///< Drawing in OpenGL.
  virtual struct spriteframe_t *GetFrame() { return NULL; }; // Menu uses this.
  virtual int Marshal(class LArchive &a) = 0;

  void *operator new(size_t size);
  void  operator delete(void *mem);

  static int Serialize(presentation_t *p, LArchive &a);
  static presentation_t *Unserialize(LArchive &a);

  /// Netcode
  virtual void   Pack(class TNL::BitStream *s) = 0;
  virtual void Unpack(class TNL::BitStream *s) = 0;
  virtual void   PackAnim(class TNL::BitStream *s) = 0;
  virtual void UnpackAnim(class TNL::BitStream *s) = 0;
};


#endif
