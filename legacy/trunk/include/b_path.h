// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: b_path.h 529 2008-09-12 07:15:16Z smite-meister $
//
// Copyright (C) 2002-2008 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Pathfinding system for client-side bots.

#ifndef b_node_h
#define b_node_h 1

#include <vector>
#include <list>
#include "vect.h"
#include "m_fixed.h"

//#define SHOWBOTPATH //show the path the bot is taking in game?


enum botdirtype_t
{
 BDI_EAST,
 BDI_NORTHEAST,
 BDI_NORTH,
 BDI_NORTHWEST,
 BDI_WEST,
 BDI_SOUTHWEST,
 BDI_SOUTH,
 BDI_SOUTHEAST,
 BDI_TELEPORT,
 NUMBOTDIRS
};


/// \brief Pathnode for bots
class SearchNode_t
{
  friend class BotNodes;
protected:
  // used only during pathfinding
  bool visited;
  fixed_t cost;      ///< cumulative cost based on path length and difficulty
  fixed_t heuristic; ///< "distance to destination" 
  fixed_t f;         ///< cost + heuristic
  SearchNode_t *pprevious; ///< previous node in the path

  SearchNode_t *dir[NUMBOTDIRS]; ///< neighbor nodes
  fixed_t   costDir[NUMBOTDIRS]; ///< the cost of going from this node to a neighboring one

public:
  /// grid coordinates of the node
  int gx, gy;

  /// map coordinates of the node
  fixed_t mx, my; 

#ifdef SHOWBOTPATH
  class Actor *marker; ///< visible path marker for debugging
#endif

public:
  SearchNode_t(int gx, int gy, fixed_t mx, fixed_t my);
  ~SearchNode_t();

  void PushSuccessors(class priorityQ_t *q, int destgx, int destgy);

  void *operator new(size_t size);
  void  operator delete(void *mem);
};



/// \brief BotNode structure for Maps
class BotNodes
{
  class Map *mp;

  // node grid
  fixed_t xOrigin, yOrigin;
  int xSize, ySize;
  int numbotnodes;

  SearchNode_t ***botNodeArray;

public:
  enum
  {
    GRIDBITS = 5,
    GRIDSIZE = (1 << GRIDBITS) // 32 units
  };

  BotNodes(Map *m);

  void BuildNodes(SearchNode_t *node);

  bool DirectlyReachable(class Actor *a, fixed_t x, fixed_t y, fixed_t destx, fixed_t desty);

  SearchNode_t *FindClosestNode(fixed_t x, fixed_t y);
  SearchNode_t *GetClosestReachableNode(fixed_t x, fixed_t y);
  SearchNode_t *GetNodeAt(const vec_t<fixed_t>& r);

  bool FindPath(std::list<SearchNode_t *> &path, SearchNode_t *start, SearchNode_t *dest);

  // nodes lie at the middle of their grid cell
  // map coordinates into grid coordinates
  inline int x2PosX(fixed_t x) { return (x - xOrigin).floor() >> GRIDBITS; };
  inline int y2PosY(fixed_t y) { return (y - yOrigin).floor() >> GRIDBITS; };

  // grid coordinates into map coordinates
  inline fixed_t posX2x(int nx) { return fixed_t((nx << GRIDBITS) + GRIDSIZE/2) + xOrigin; };
  inline fixed_t posY2y(int ny) { return fixed_t((ny << GRIDBITS) + GRIDSIZE/2) + yOrigin; };
};



/// \brief Heap-based priority queue for SearchNodes. 
class priorityQ_t
{
private:
  std::vector<SearchNode_t*> pq; // a priority queue

public:
  void Push(SearchNode_t *node);
  SearchNode_t *Pop();
  SearchNode_t *FindNode(SearchNode_t *node);
  SearchNode_t *RemoveNode(SearchNode_t *node);

  inline bool Empty() { return pq.empty(); };
};


#endif

