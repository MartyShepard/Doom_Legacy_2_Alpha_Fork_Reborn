// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: st_lib.h 463 2007-05-21 23:32:04Z smite-meister $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2004 by DooM Legacy Team.
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
//
//-----------------------------------------------------------------------------

/// \file
/// \brief HUD widgets

#ifndef st_lib_h
#define st_lib_h 1

#include "d_items.h"

/// \brief ABC for HUD widgets
///
/// HUD widget classes currently include
/// number, percent, multicon, binicon, slider and inventory widgets.
class HudWidget
{
protected:
  int x, y; ///< screen coordinates

  virtual void Draw() = 0; ///< pure virtual drawing routine

public:
  HudWidget(int x, int y);
  virtual ~HudWidget() {};

  /// force: update even if no change
  virtual void Update(bool force)  { Draw(); };
};



/// \brief Number widget
///
/// Screen coordinates mark the upper right-hand corner of the number (right-justified).
/// Note! 1994 is a magic number representing a non-number value!
class HudNumber : public HudWidget
{
private:
  int digits;  ///< max # of digits in the number
  class Material **nums; ///< graphics for numbers 0-9, nums[10] is the minus sign

protected:
  const int *n; ///< pointer to current value
  int     oldn; ///< last drawn value

  virtual void Draw();

public:
  HudNumber(int x, int y, int digits, const int *number, Material **tex);

  virtual void Update(bool force);  
  void Set(const int *nnum) { n = nnum; }
};



/// \brief Percentile widget
class HudPercent : public HudNumber
{
private:
  Material *pcent; ///< percent sign graphic

public:
  HudPercent(int x, int y, const int *number, Material **tex, Material *percent);
  virtual void Update(bool force);
};



/// \brief Multiple Icons widget
class HudMultIcon : public HudWidget
{
private:
  int     oldinum; ///< previous icon number
  const int *inum; ///< pointer to current icon number, -1 is a magic value meaning no icon is drawn
  Material **icons; ///< array of icons

  virtual void Draw();

public:
  HudMultIcon(int x, int y, const int *inum, Material **tex);
  virtual void Update(bool force);
};



/// \brief Binary Icon widget
class HudBinIcon : public HudWidget
{
private:
  // center-justified location of icon
  bool     oldstatus;  ///< last icon value
  const bool *status;  ///< pointer to current icon status
  Material  *icons[2];  ///< The icons. If p[0] == NULL, draw background instead

  virtual void Draw();

public:
  HudBinIcon(int x, int y, const bool *status, Material *p0, Material *p1);
  virtual void Update(bool force);
};



#define ST_NUMPAINFACES         5
#define ST_NUMSTRAIGHTFACES     3
#define ST_NUMTURNFACES         2
#define ST_NUMSPECIALFACES      3
#define ST_FACESTRIDE (ST_NUMSTRAIGHTFACES+ST_NUMTURNFACES+ST_NUMSPECIALFACES)
#define ST_NUMEXTRAFACES        2
#define ST_NUMFACES (ST_FACESTRIDE*ST_NUMPAINFACES+ST_NUMEXTRAFACES)

/// \brief Marine face widget
class HudFace : public HudWidget
{
private:
  int     priority; ///< priority of current expression
  int        count; ///< count until next change

  int    oldhealth; ///< to get appopriately pained face
  int    maxhealth;
  int rampagecount; ///< tics to go until rampage
  bool oldweaponsowned[NUMWEAPONS]; 
  int        color; ///< pawn color, determines face background color
  
  int    faceindex; ///< icon to draw
  int oldfaceindex; ///< previous icon number

  Material   **faces; ///< array of icons
  Material *faceback; ///< face background

  int calcPainOffset(int health);
  virtual void Draw();

public:
  HudFace(int x, int y);
  void updateFaceWidget(const class PlayerInfo *p);
  virtual void Update(bool force);
};




/// \brief Horizontal "slow" slider widget (Heretic health bar)
class HudSlider : public HudWidget
{
protected:
  int minval, maxval; ///< min and max values
  int   oldval, cval; ///< old and current values
  const int     *val; ///< pointer to current value
  Material      **tex; ///< Slider graphics. See the Draw() implementation.

  virtual void Draw();

public:
  HudSlider(int x, int y, const int *v, int mi, int ma, Material **t);
  virtual void Update(bool force);
};


/// \brief Horizontal "slow" slider widget (Hexen health bar)
class HexenHudSlider : public HudSlider
{
protected:
  virtual void Draw();

public:
  HexenHudSlider(int x, int y, const int *v, int mi, int ma, Material **t)
    : HudSlider(x, y, v, mi, ma, t) {};
};



/// \brief Inventory widget for Heretic
///
/// A set of 7 multicons plus a little extra.
class HudInventory : public HudWidget
{
protected:
  const bool    *open; ///< show expanded inventory or just one slot?
  const int  *itemuse; ///< counter for the item use animation
  const struct inventory_t *slots;
  const int *selected; ///< selected slot number 0-6
  Material **nums;  ///< small numbers 0-9
  Material **items; ///< item graphics
  Material **tex;   ///< inventory graphics

  virtual void Draw();
  void DrawNumber(int x, int y, int val);

public:
  HudInventory(int x, int y, const bool *op, const int *iu, const inventory_t *vals,
	       const int *sel, Material **nn, Material **it, Material **t);
  virtual void Update(bool force);
};


/// \brief Inventory widget for Hexen
///
/// Exactly like a Heretic inventory, but with different internal drawing offsets.
class HexenHudInventory : public HudInventory
{
protected:
  virtual void Draw();

public:
  HexenHudInventory(int x, int y, const bool *op, const int *iu, const inventory_t *vals,
		    const int *sel, Material **nn, Material **it, Material **t)
    : HudInventory(x, y, op, iu, vals, sel, nn, it, t) {};
};

#endif
