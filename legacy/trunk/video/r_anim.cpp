// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_anim.cpp 504 2007-12-16 17:58:26Z smite-meister $
//
// Copyright (C) 2003-2007 by DooM Legacy Team.
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
/// \brief Texture animation, parser for ANIMDEFS and ANIMATED lumps.

#include <string>

#include "doomdef.h"
#include "doomdata.h"

#include "g_game.h"

#include "r_data.h"
#include "r_defs.h" // for floortypes

#include "parser.h"
#include "m_swap.h"
#include "m_random.h"
#include "w_wad.h"
#include "z_zone.h"


// Floor/ceiling animation sequences,
//  defined by first and last frame,
//  i.e. the flat (64x64 tile) name to
//  be used.
// The full animation sequence is given
//  using all the flats between the start
//  and end entry, in the order found in
//  the WAD file.

static ANIMATED_t DoomAnims[] =
{
  // DOOM II flat animations.
  {false,     "NUKAGE3",      "NUKAGE1",      8},
  {false,     "FWATER4",      "FWATER1",      8},
  {false,     "SWATER4",      "SWATER1",      8},
  {false,     "LAVA4",        "LAVA1",        8},
  {false,     "BLOOD3",       "BLOOD1",       8},

  {false,     "RROCK08",      "RROCK05",      8},
  {false,     "SLIME04",      "SLIME01",      8},
  {false,     "SLIME08",      "SLIME05",      8},
  {false,     "SLIME12",      "SLIME09",      8},

    // animated textures
  {true,      "BLODGR4",      "BLODGR1",      8},
  {true,      "SLADRIP3",     "SLADRIP1",     8},

  {true,      "BLODRIP4",     "BLODRIP1",     8},
  {true,      "FIREWALL",     "FIREWALA",     8},
  {true,      "GSTFONT3",     "GSTFONT1",     8},
  {true,      "FIRELAVA",     "FIRELAV3",     8},
  {true,      "FIREMAG3",     "FIREMAG1",     8},
  {true,      "FIREBLU2",     "FIREBLU1",     8},
  {true,      "ROCKRED3",     "ROCKRED1",     8},

  {true,      "BFALL4",       "BFALL1",       8},
  {true,      "SFALL4",       "SFALL1",       8},
  {true,      "WFALL4",       "WFALL1",       8},
  {true,      "DBRAIN4",      "DBRAIN1",      8},
  {-1}
};

static ANIMATED_t HereticAnims[] =
{
  // heretic 
  {false,     "FLTWAWA3",     "FLTWAWA1",     8}, // Water
  {false,     "FLTSLUD3",     "FLTSLUD1",     8}, // Sludge
  {false,     "FLTTELE4",     "FLTTELE1",     6}, // Teleport
  {false,     "FLTFLWW3",     "FLTFLWW1",     9}, // River - West
  {false,     "FLTLAVA4",     "FLTLAVA1",     8}, // Lava
  {false,     "FLATHUH4",     "FLATHUH1",     8}, // Super Lava
  {true,      "LAVAFL3",      "LAVAFL1",      6}, // Texture: Lavaflow
  {true,      "WATRWAL3",     "WATRWAL1",     4}, // Texture: Waterfall
  {-1}
};




#define MAX_ANIM_DEFS  20
#define MAX_FRAME_DEFS 20


/// \brief Class for keeping track of animated Materials
class AnimatedMaterial
{
public:
  struct framedef_t
  {
    Texture *tx;
    int tics;
  };

  Material      *mat; ///< Material to animate
  framedef_t *frames; ///< the animation frames
  int      numframes; ///< number of frames
  int   currentframe; ///< current frame index
  float   nextchange; ///< when will the current frame end?
  bool        master; ///< master copy (the one which owns (and deletes) the frames)


  static vector<AnimatedMaterial *> animated_materials;

public:
  AnimatedMaterial(Material *m, int nframes);
  AnimatedMaterial(Material *m, const AnimatedMaterial *a, int frame); // for constructing the slave copies
  ~AnimatedMaterial();

  void Update(tic_t t)
  {
    if (nextchange + 250 < t)
      {
	// skip too long intervals (also sort of handles the wrapping of tic_t, in case your game runs more than 3.8 years continuously...:)
	nextchange = t+10;
	return;
      }

    while (nextchange <= t)
      {
	// next frame
	if (++currentframe == numframes)
	  currentframe = 0;

	tic_t tics = frames[currentframe].tics;
	if (tics > 255)
	  tics = (tics >> 16) + P_Random() % ((tics & 0xFF00) >> 8); // Random tics     
   
	nextchange += tics;
      }

    mat->tex[0].t = frames[currentframe].tx; // change the tex0
  }
};


vector<AnimatedMaterial *> AnimatedMaterial::animated_materials;


AnimatedMaterial::AnimatedMaterial(Material *m, int n)
  : mat(m)
{
  frames = static_cast<framedef_t*>(Z_Malloc(n*sizeof(framedef_t), PU_TEXTURE, NULL));
  numframes = n;
  currentframe = 0;
  nextchange = game.tic;
  master = true;

  animated_materials.push_back(this);
}


AnimatedMaterial::AnimatedMaterial(Material *m, const AnimatedMaterial *a, int f)
  : mat(m)
{
  // copy most fields from the master
  numframes = a->numframes;
  currentframe = f; // starts from a different frame
  nextchange = game.tic;

  frames = a->frames;
  master = false; // does not own the frames

  animated_materials.push_back(this);
}


AnimatedMaterial::~AnimatedMaterial()
{
  if (master && frames)
    Z_Free(frames);
}




void R_Update(tic_t t)
{
  int n = AnimatedMaterial::animated_materials.size();
  for (int i=0; i<n; i++)
    AnimatedMaterial::animated_materials[i]->Update(t);
}




/// Reads and interprets the Boom ANIMATED lump
int R_Read_ANIMATED(int lump)
{
  int i, count = 0;

  ANIMATED_t *anims;
  if (lump >= 0)
    {
      CONS_Printf("Reading ANIMATED...\n");
      anims = static_cast<ANIMATED_t*>(fc.CacheLumpNum(lump, PU_STATIC));
    }
  else if (game.mode == gm_heretic)
    anims = HereticAnims;
  else
    anims = DoomAnims;

  for (ANIMATED_t *a = anims; a->istexture != -1; a++)
    {
      Material *first, *last;
      if (a->istexture)
	{
	  first = materials.Edit(a->startname, TEX_wall);
	  last  = materials.Edit(a->endname, TEX_wall);
	}
      else
	{
	  first = materials.Edit(a->startname, TEX_floor);
	  last  = materials.Edit(a->endname, TEX_floor);
	}

      if (!first)
	{
	  if (lump >= 0)
	    CONS_Printf(" Unknown material '%s'\n", a->startname);
	  continue;
	}

      if (!last)
	{
	  if (lump >= 0)
	    CONS_Printf(" Unknown material '%s'\n", a->endname);
	  continue;
	}

      int n = last->id_number - first->id_number + 1; // number of frames
      if (n < 2 || n > MAX_FRAME_DEFS)
	{
	  CONS_Printf(" Bad cycle from %s to %s\n", a->startname, a->endname);
	  continue;
	}

      int base = first->id_number;
      int tics = (lump >= 0) ? LONG(a->speed) : a->speed; // duration of one frame in tics

      // create the master copy, picking the frame Textures from the already generated Materials
      AnimatedMaterial *m = new AnimatedMaterial(first, n);
      for (i=0; i<n; i++)
	{
	  m->frames[i].tx = materials.GetID(base + i)->tex[0].t;
	  m->frames[i].tics = tics;
	}

      // create one slave instance for each frame of animation
      for (i=1; i<n; i++)
	new AnimatedMaterial(materials.GetID(base + i), m, i);

      count++;
    }

  if (lump >= 0)
    {
      Z_Free(anims);
      CONS_Printf(" ...done. %d animations found.\n", count);
    }

  return count;
}




/// Parses the Hexen ANIMDEFS lump, creates the required animated textures
int R_Read_ANIMDEFS(int lump)
{
  int i, count = 0;
  Parser p;

  if (!p.Open(lump))
    return -1;

  CONS_Printf("Reading ANIMDEFS...\n");

  vector<AnimatedMaterial::framedef_t> frames;

  p.RemoveComments(';');
  enum p_state_e {PS_NONE, PS_FLAT, PS_TEX};
  p_state_e state = PS_NONE;

  Material *first;
  int base = 0;

  char *name = NULL;
  while (p.NewLine())
    {
      // texture/flat <name>
      // pic <n> tics <t>
      // pic <n> rand <min> <max>
      char *word;

      if (state != PS_NONE)
	{
	  // in record
	  char temp = p.Peek();
	  if (temp != 'p' && temp != 'P')
	    {
	      // record just ended

	      int n = frames.size();
	      if (n < 2)
		I_Error("AnimDef has framecount < 2.");

	      // create the master copy, picking the frame Textures from the already generated Materials
	      AnimatedMaterial *m = new AnimatedMaterial(first, n);
	      for (i=0; i<n; i++)
		m->frames[i] = frames[i];

	      // create one slave instance for each frame of animation
	      for (i=1; i<n; i++)
		new AnimatedMaterial(materials.GetID(base + i), m, i);

	      count++;
	      frames.clear();

	      state = PS_NONE;
	    }
	  else
	    {
	      // read in the frames
	      word = p.GetToken(" ");
	      if (word && !strcasecmp(word, "pic"))
		{
		  if (frames.size() >= MAX_FRAME_DEFS)
		    I_Error("Too many FrameDefs.");

		  AnimatedMaterial::framedef_t fd;

		  int n = base + p.GetInt() - 1;
		  fd.tx = materials.GetID(n)->tex[0].t;

		  word = p.GetToken(" ");
		  if (!strcasecmp(word, "tics"))
		    fd.tics = p.GetInt();
		  else if (!strcasecmp(word, "rand"))
		    {
		      n = p.GetInt();          // min
		      i = p.GetInt() - n + 1;  // range
		      fd.tics = (n << 16) + (i << 8);
		    }
		  else
		    {
		      if (word)
			CONS_Printf(" Unknown modifier: '%s'\n", word);
		      else
			CONS_Printf(" Missing modifier!\n");
		      fd.tics = 35;
		    }

		  frames.push_back(fd);
		}
	      else
		state = PS_NONE; // end of animdef

	      continue; // next line
	    }
	}

      // no active records
      word = p.GetToken(" ");
      name = p.GetToken(" ");
      strupr(name);

      if (!strcasecmp(word, "flat"))
	{
	  first = materials.Edit(name, TEX_floor);
	  if (first)
	    {
	      state = PS_FLAT;
	      base = first->id_number;
	    }
	}
      else if (!strcasecmp(word, "texture"))
	{
	  first = materials.Edit(name, TEX_wall);
	  if (first)
	    {
	      state = PS_TEX;
	      base = first->id_number;
	    }
	}
      else
	{
	  CONS_Printf(" Unknown command: '%s'", word);
	  continue;
	}
    }


  if (count >= MAX_ANIM_DEFS)
    I_Error("Too many AnimDefs.");

  CONS_Printf(" ...done. %d animations found.\n", count);
  return count;
}
