// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: b_path.cpp 533 2009-06-11 08:31:10Z smite-meister $
//
// Copyright (C) 2002-2004 by DooM Legacy Team.
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
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Pathing system for client-side bots

#include <algorithm>

#include "b_bot.h"
#include "b_path.h"

#include "g_game.h"
#include "g_map.h"
#include "g_blockmap.h"
#include "g_actor.h"
#include "g_pawn.h"

#include "r_defs.h"
#include "p_maputl.h"
#include "m_bbox.h"

#include "z_zone.h"


//=======================================================
//  Search nodes
//=======================================================

void *SearchNode_t::operator new(size_t size)
{
  return Z_Malloc(size, PU_LEVEL, NULL);    
}

void SearchNode_t::operator delete(void *mem)
{
  Z_Free(mem);
}


// create a new search node to the grid coords (nx,ny)
SearchNode_t::SearchNode_t(int nx, int ny, fixed_t x, fixed_t y)
{
  visited = false;

  gx = nx;
  gy = ny;
  mx = x;
  my = y;
  cost = f = heuristic = 0;

  pprevious = NULL;

  //CONS_Printf("Created node at x:%d, y:%d.\n", (x>>FRACBITS), (y>>FRACBITS));
  // newnode->mo = P_SpawnMobj(posX2x(newnode->x), posY2y(newnode->y), GetSubsector(posX2x(newnode->x), posY2y(newnode->y))->sector->floorheight, MT_MISC49);
}


SearchNode_t::~SearchNode_t()
{
#ifdef SHOWBOTPATH
  if (marker)
    delete marker;
#endif
}


/// Bot pathing: Continue the path from a node to its neighbours.
void SearchNode_t::PushSuccessors(priorityQ_t *q, int destgx, int destgy)
{
  for (int angle=0; angle<NUMBOTDIRS; angle++)
    {
      if (dir[angle])
	{
	  SearchNode_t *node = dir[angle];
	  fixed_t n_cost = costDir[angle] + cost;
	  fixed_t n_heuristic = P_AproxDistance(destgx - node->gx, destgy - node->gy) * 10000;
	  //CONS_Printf("got heuristic of %d\n", node->heuristic);
	  fixed_t n_f = n_cost + n_heuristic;

	  if (node->visited && (node->f <= n_f))
	    continue;  //if already been looked at before, and before look was better
 
	  if (q->FindNode(node))
	    {
	      //this node has already been pushed on the todo list
	      if (node->f <= n_f)
		continue;

	      //the path to get here this way is better then the old one's
	      //so use this instead, remove the old
	      //CONS_Printf("found better path\n");
	      q->RemoveNode(node);
	    }

	  node->cost = n_cost;
	  node->heuristic = n_heuristic;
	  node->f = n_f;
	  node->pprevious = this;
	  q->Push(node);
	  //CONS_Printf("pushed node at x:%d, y:%d\n", node->x>>FRACBITS, node->y>>FRACBITS);
	}
    }
}


//=======================================================
//   Priority queue
//=======================================================

// inserts the given node into the queue
void priorityQ_t::Push(SearchNode_t *node)
{
  pq.push_back(node);
  push_heap(pq.begin(), pq.end());
}


// removes and returns the highest-priority node from the queue
SearchNode_t *priorityQ_t::Pop()
{
  if (pq.empty())
    return NULL;

  SearchNode_t *root = pq[0];
  pop_heap(pq.begin(), pq.end());
  pq.pop_back();

  return root;
}


// searches the given node from the queue
SearchNode_t *priorityQ_t::FindNode(SearchNode_t *node)
{
  int n = pq.size();
  for (int i=0; i<n; i++)
    if (pq[i] == node)
      return node;

  return NULL; // not found
}


// searches and removes the given node from the queue
SearchNode_t *priorityQ_t::RemoveNode(SearchNode_t *node)
{
  int n = pq.size();
  for (int i=0; i<n; i++)
    if (pq[i] == node)
      {
	pq[i] = pq[n-1];
	pq.pop_back();
	make_heap(pq.begin(), pq.end());
	return node;
      }

  return NULL; // not found
}




//====================================================
//   BotNodes class
//====================================================

static fixed_t botteledestx, botteledesty;
static bool botteledestfound = false;
static fixed_t pawn_height;
static sector_t *last_sector;

/// Examines the trace intercept, sees it if can be activated/opened/circumvented.
/// Sets the teledest / door variables above accordingly.
static bool PTR_BotPath(intercept_t *in)
{
  static sector_t *oksector = NULL; // latest reached sector

  if (in->isaline)
    {
      line_t *line = in->line;

      if (!(line->flags & ML_TWOSIDED) || (line->flags & ML_BLOCKING))
	return false;

      switch (line->special)
	{
	case 11: // doors
	case 12:
	case 13:
	case 62: // plat
	  // Determine if looking at backsector/frontsector.
	  oksector = (line->backsector == last_sector) ? line->frontsector : line->backsector;
	  break;

	case 70: // teleports
	case 71:
	  /*
	  {
	    int i;
	    int tag = line->args[0];
	    if (tag == 255)
	      {
		// special case to handle converted Doom/Heretic linedefs
		tag = line->tag;
	      }

	    // type of teleport
	    int type = line->args[1];
	    if (type == 0) // Hexen TID teleport
	      {
		// TODO
	      }
	    else if (type == 1) // Doom tagged sector teleport
	      {
		for (i = -1; (i = FindSectorFromTag(tag, i)) >= 0; )
		  for (Actor *m = sectors[i].thinglist; m != NULL; m = m->snext)
		    {
		      if (!m->IsOf(DActor::_type))
			continue;
		      DActor *dm = (DActor *)m;
		      // not a teleportman
		      if (dm->type == MT_TELEPORTMAN)
			{
			  botteledestfound = true;
			  botteledestx = m->x;
			  botteledesty = m->y;
			  break;
			  //CONS_Printf("found a teleporter line going to x:%d, y:%d\n", botteledestx, botteledesty);
			}
		    }
	      }
	    else
	      {
		// Silent line to line Boom teleporter
		line_t *l2;
		for (i = -1; (l2 = FindLineFromTag(line->tag, &i)) >= 0; )
		  if (l2 != line)
		    {
		      botteledestfound = true;
		      botteledestx = (l2->v1->x + l2->v2->x)/2;
		      botteledesty = (l2->v1->y + l2->v2->y)/2;
		      break;
		      //CONS_Printf("found a teleporter line going to x:%d, y:%d\n", botteledestx, botteledesty);
		    }
	      }
	  }
	  */
	  break;

	default:
	  {
	    botteledestfound = false;
	    //Determine if looking at backsector/frontsector.
	    sector_t *s = (line->backsector == last_sector) ? line->frontsector : line->backsector;
	    fixed_t ceilingheight = s->ceilingheight;
	    fixed_t floorheight = s->floorheight;
	    if (s != oksector)
	      {
		if (ceilingheight - floorheight < pawn_height && !s->tag)
		  return false; // can't fit
		if (floorheight > last_sector->floorheight + 45 ||
		    (floorheight > last_sector->floorheight + 37 && last_sector->floortype == FLOOR_WATER))
		  return false; // can't jump or reach there
	      }
	  }
	}

      return true;
    }
  else if ((in->thing->flags & MF_SOLID) && !(in->thing->flags & MF_SHOOTABLE))
    return false;

  return true;
}


bool PIT_BBoxFit(line_t *ld);


/// Checks if the given destination is reachable from the given starting location (by mo!).
bool BotNodes::DirectlyReachable(Actor *mo, fixed_t x, fixed_t y, fixed_t destx, fixed_t desty)
{
  int nx = x2PosX(destx);
  int ny = y2PosY(desty);

  vec_t<fixed_t> start(x,y,0);
  vec_t<fixed_t> dest(destx, desty, 0);

  botteledestfound = false;

  if ((nx >= 0) && (nx < xSize) && (ny >= 0) && (ny < ySize))
    { 
      botteledestx = destx;
      botteledesty = desty;
      last_sector = mp->GetSubsector(x, y)->sector;

      if (mo)
	{
	  pawn_height = mo->height;
	  if (mp->blockmap->IterateLinesRadius(destx, desty, mo->radius, PIT_BBoxFit) // does it fit there?
	      /*
	      && mp->PathTraverse(x, y, destx - 1, desty + 1, PT_ADDLINES|PT_ADDTHINGS, PTR_BotPath)
	      && mp->PathTraverse(x, y, destx + 1, desty + 1, PT_ADDLINES|PT_ADDTHINGS, PTR_BotPath)
	      && mp->PathTraverse(x, y, destx - 1, desty - 1, PT_ADDLINES|PT_ADDTHINGS, PTR_BotPath)
	      && mp->PathTraverse(x, y, destx + 1, desty - 1, PT_ADDLINES|PT_ADDTHINGS, PTR_BotPath)
	      && mp->PathTraverse(x - 1, y + 1, destx, desty, PT_ADDLINES|PT_ADDTHINGS, PTR_BotPath)
	      && mp->PathTraverse(x + 1, y + 1, destx, desty, PT_ADDLINES|PT_ADDTHINGS, PTR_BotPath)
	      && mp->PathTraverse(x - 1, y - 1, destx, desty, PT_ADDLINES|PT_ADDTHINGS, PTR_BotPath)
	      && mp->PathTraverse(x + 1, y - 1, destx, desty, PT_ADDLINES|PT_ADDTHINGS, PTR_BotPath)
	      */
	      && mp->blockmap->PathTraverse(start, dest, PT_ADDLINES|PT_ADDTHINGS, PTR_BotPath))
	    return true; // FIXME why do many traces with nearly identical endpoints??
	  else
	    {
	      botteledestfound = false;
	      return false;
	    }
	}
      else
	{
	  pawn_height = 56;
	  return mp->blockmap->PathTraverse(start, dest, PT_ADDLINES, PTR_BotPath);
	}
    }

  return false;
}




/// spirals outwards from the (x,y) cell searching for a node
SearchNode_t *BotNodes::FindClosestNode(fixed_t x, fixed_t y)
{
  int depth = 0;

  botdirtype_t dir = BDI_SOUTH;
  SearchNode_t *closest = NULL;

  int xx, yy;
  int i = xx = x2PosX(x);
  int j = yy = y2PosY(y);

  while (!closest && (depth < 50))
    {
      if ((i >= 0) && (i < xSize) && (j >= 0) && (j < ySize))
	{
	  if (botNodeArray[i][j])
	    closest = botNodeArray[i][j];
	}

      switch (dir)
	{
	case (BDI_EAST):
	  if (++i > xx + depth)
	    {
	      i--; //change it back
	      dir = BDI_NORTH; //go in the new direction
	    }
	  break;
	case (BDI_NORTH):
	  if (++j > yy + depth)
	    {
	      j--; //change it back
	      dir = BDI_WEST;
	    }
	  break;
	case (BDI_WEST):
	  if (--i < xx - depth)
	    {
	      i++; //change it back
	      dir = BDI_SOUTH;
	    }
	  break;
	case (BDI_SOUTH):
	  if (--j < yy - depth)
	    {
	      j++; //change it back
	      dir = BDI_EAST;
	      depth++;
	    }
	  break;
	default: //shouldn't ever happen
	  break;
	}
    }

  return closest;
}


/// Returns the closest reachable node to the coords (x,y).
/// First checks if the node in this cell is reachable.
/// If not, checks the reachability of the surrounding 8 nodes.
/// If still no luck, just returns the closest node to (x,y)
SearchNode_t *BotNodes::GetClosestReachableNode(fixed_t x, fixed_t y)
{
  SearchNode_t *temp;

  int nx = x2PosX(x);
  int ny = y2PosY(y);

  if ((nx >= 0) && (nx < xSize) && (ny >= 0) && (ny < ySize))
    {
      temp = botNodeArray[nx][ny];
      if (temp && DirectlyReachable(NULL, x, y, temp->mx, temp->my))
	return temp;
    }

  for (int i = nx-1; i <= nx+1; i++)
    for (int j = ny-1; j <= ny+1; j++)
      if ((i >= 0) && (i < xSize) && (j >= 0) && (j < ySize))
	{
	  temp = botNodeArray[i][j];
	  if (temp && DirectlyReachable(NULL, x, y, temp->mx, temp->my))
	    return temp;
	}

  return FindClosestNode(x, y);
}


/// Like above, but checks the path FROM the node TO (x,y). Also, does not call FindClosestNode.
SearchNode_t *BotNodes::GetNodeAt(const vec_t<fixed_t>& r)
{
  SearchNode_t *temp = NULL;

  int nx = x2PosX(r.x);
  int ny = y2PosY(r.y);

  if ((nx >= 0) && (nx < xSize) && (ny >= 0) && (ny < ySize))
    {
      temp = botNodeArray[nx][ny];
      if (temp && DirectlyReachable(NULL, temp->mx, temp->my, r.x, r.y))
	return temp;
    }

  for (int i = nx-1; i <= nx+1; i++)
    for (int j = ny-1; j <= ny+1; j++)
      if ((i >= 0) && (i < xSize) && (j >= 0) && (j < ySize) && ((i != nx) && (j != ny)))
	{
	  temp = botNodeArray[i][j];
	  if (temp && DirectlyReachable(NULL, temp->mx, temp->my, r.x, r.y))
	    return temp;
	}

  return NULL;
}



/// Main pathfinding routine
bool BotNodes::FindPath(list<SearchNode_t *> &path, SearchNode_t *start, SearchNode_t *dest)
{
  bool found = false;
  SearchNode_t *best = NULL; // if can't reach destination, try heading towards this closest point
  int numNodesSearched = 0;

  if (start)// || P_AproxDistance(pawn->x - start->x, pawn->y - start->y) > (BOTNODEGRIDSIZE<<1)) //no nodes can get here
    {
      path.clear();

      priorityQ_t pq;
      deque<SearchNode_t *> visitedList;

      //CONS_Printf("closest node found is x:%d, y:%d\n", start->x>>FRACBITS, start->y>>FRACBITS);
      start->pprevious = NULL;
      start->cost = 0;
      start->f = start->heuristic = P_AproxDistance(start->gx - dest->gx, start->gy - dest->gy) * 10000;

      pq.Push(start);

      while (!pq.Empty() && !found) // while there are nodes left to check 
	{
	  SearchNode_t *temp = pq.Pop();

	  //CONS_Printf("doing node a node at x:%d, y:%d\n", temp->x>>FRACBITS, temp->y>>FRACBITS);
	  if (temp == dest)
	    {
	      //I have found the sector where I want to get to
	      best = temp;
	      found = true;
	    }
	  else
	    {
	      if (!best || (temp->heuristic < best->heuristic))
		best = temp;

	      temp->PushSuccessors(&pq, dest->gx, dest->gy);
 
	      if (!temp->visited)
		{
		  visitedList.push_front(temp); //so later can set this back to not visited
		  temp->visited = true;
		}
	    }
	  numNodesSearched++;
	}

      // reset all visited nodes to not visited
      while (!visitedList.empty())
	{
	  visitedList.front()->visited = false;
	  visitedList.pop_front();
	}

      if (best && (best != start))
	{
	  while (best->pprevious) // backtrack the route, store the nodes in the path list
	    {
#ifdef SHOWBOTPATH
	      fixed_t x = posX2x(best->gx);
	      fixed_t y = posY2y(best->gy);
	      best->marker = mp->SpawnDActor(x, y, ONFLOORZ, MT_MISC49);
#endif
	      path.push_front(best);
	      best = best->pprevious;
	    }

	  found = true;
	}
    }
  //else
  // CONS_Printf("Bot is stuck here x:%d y:%d\n", pawn->x>>FRACBITS, pawn->y>>FRACBITS);

  return found;
}



/// Builds a connected network of nodes starting from "node"
void BotNodes::BuildNodes(SearchNode_t *node)
{
  deque<SearchNode_t*> deck;
  deck.push_back(node);

  while (!deck.empty())
    {
      node = deck.front();
      deck.pop_front();
      node->dir[BDI_TELEPORT] = NULL;

      for (int angle = BDI_EAST; angle <= BDI_SOUTHEAST; angle++)
	{
	  int cost = 0;
	  int nx, ny;
	  
	  switch(angle)
	    {
	    case (BDI_EAST):
	      nx = node->gx + 1;
	      ny = node->gy;
	      break;
	    case (BDI_NORTHEAST):
	      nx = node->gx + 1;
	      ny = node->gy + 1;
	      cost = 5000; //because diagonal
	      break;
	    case (BDI_NORTH):
	      nx = node->gx;
	      ny = node->gy + 1;
	      break;
	    case (BDI_NORTHWEST):
	      nx = node->gx - 1;
	      ny = node->gy + 1;
	      cost = 5000; //because diagonal
	      break;
	    case (BDI_WEST):
	      nx = node->gx - 1;
	      ny = node->gy;
	      break;
	    case (BDI_SOUTHWEST):
	      nx = node->gx - 1;
	      ny = node->gy - 1;
	      cost = 5000; //because diagonal
	      break;
	    case (BDI_SOUTH):
	      nx = node->gx;
	      ny = node->gy - 1;
	      break;
	    case (BDI_SOUTHEAST):
	    default: //shouldn't ever happen
	      nx = node->gx + 1;
	      ny = node->gy - 1;
	      cost = 5000; //because diagonal
	      break;
	    }

	  // FIXME use some temp Actor here for fitting instead of NULL
	  if (DirectlyReachable(NULL, node->mx, node->my, posX2x(nx), posY2y(ny)))
	    {
	      SearchNode_t *temp;
	      if (!botNodeArray[nx][ny])
		{
		  temp = botNodeArray[nx][ny] = new SearchNode_t(nx, ny, posX2x(nx), posY2y(ny));
		  numbotnodes++;
		  deck.push_back(temp);
		}
	      else
		temp = botNodeArray[nx][ny];

	      node->dir[angle] = temp;

	      sector_t *sector = mp->GetSubsector(temp->mx, temp->my)->sector;

	      if (sector->floortype == FLOOR_LAVA)
		cost += 50000;
	      else
		cost += 10000;
	      /*
	      else if (sector->floortype == FLOOR_LAVA)
		cost += 40000; // FIXME
	      */

	      node->costDir[angle] = cost;

	      // teleports
	      if (botteledestfound)
		{
		  nx = x2PosX(botteledestx);
		  ny = y2PosY(botteledesty);
      //CONS_Printf("trying to make a tele node at x:%d, y:%d\n", botteledestx>>FRACBITS, botteledesty>>FRACBITS);
 
		  if (!botNodeArray[nx][ny])
		    {
		      temp->dir[BDI_TELEPORT] = botNodeArray[nx][ny] = new SearchNode_t(nx, ny, posX2x(nx), posY2y(ny));
      //CONS_Printf("created teleporter node at x:%d, y:%d\n", botteledestx>>FRACBITS, botteledesty>>FRACBITS);
		      numbotnodes++;
		      // now build nodes for the teleport destination
		      BuildNodes(temp->dir[BDI_TELEPORT]);
		    }
		  else
		    temp->dir[BDI_TELEPORT] = botNodeArray[nx][ny];

		  temp->costDir[BDI_TELEPORT] = 20000;//B_GetNodeCost(node->dir[TELEPORT]);
		}
	    }
	  else
	    node->dir[angle] = NULL;
	}
    }
}



/// Builds the bot nodes for the Map, 
BotNodes::BotNodes(Map *m)
{
  mp = m;

  // origin for the node grid
  xOrigin = m->root_bbox[BOXLEFT];
  yOrigin = m->root_bbox[BOXBOTTOM];

  xSize = x2PosX(m->root_bbox[BOXRIGHT]) + 1;
  ySize = y2PosY(m->root_bbox[BOXTOP]) + 1;

  numbotnodes = 0;
  botNodeArray = (SearchNode_t***)Z_Malloc(xSize * sizeof(SearchNode_t**), PU_LEVEL, 0);
  for (int i=0; i<xSize; i++)
    {
      botNodeArray[i] = (SearchNode_t**)Z_Malloc(ySize * sizeof(SearchNode_t*), PU_LEVEL, 0);
      for (int j=0; j<ySize; j++)
	botNodeArray[i][j] = NULL;
    }

  CONS_Printf("Building nodes for acbot.....\n");

  SearchNode_t *temp;
  int px, py;
  multimap<int, mapthing_t *>::iterator t;

  for (t = m->playerstarts.begin(); t != m->playerstarts.end(); t++)
    {
      mapthing_t *mt = t->second;
      px = x2PosX(mt->x);
      py = y2PosY(mt->y);
      if (((px >= 0) && (px < xSize) && (py >= 0) && (py < ySize)) && (!botNodeArray[px][py]))
	{
	  temp = botNodeArray[px][py] = new SearchNode_t(px, py, posX2x(px), posY2y(py));
	  numbotnodes++;
	  BuildNodes(temp);
	}
    }

  int n = m->dmstarts.size();
  for (int i = 0; i < n; i++)
    {
      px = x2PosX(m->dmstarts[i]->x);
      py = y2PosY(m->dmstarts[i]->y);
      if (((px >= 0) && (px < xSize) && (py >= 0) && (py < ySize)) && (!botNodeArray[px][py]))
	{
	  temp =  botNodeArray[px][py] = new SearchNode_t(px, py, posX2x(px), posY2y(py));
	  numbotnodes++;
	  BuildNodes(temp);
	}
    }

  CONS_Printf("Completed building %d nodes.\n", numbotnodes);
}
