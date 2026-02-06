// $Id: r_render.cpp 286 2005-09-29 15:47:55Z smite-meister $
// temporary? Renderer class implementation
// Ville Bergholm

#include "doomdef.h"
#include "r_render.h"
#include "r_draw.h"
#include "r_data.h"
#include "g_map.h"


void Rend::SetMap(Map *mp)
{
  m = mp;

  numvertexes = m->numvertexes;
  vertexes = m->vertexes;

  segs = m->segs;

  numsectors = m->numsectors;
  sectors = m->sectors;

  numsubsectors = m->numsubsectors;
  subsectors = m->subsectors;

  numnodes = m->numnodes;
  nodes = m->nodes;

  numlines = m->numlines;
  lines = m->lines;

  sides = m->sides;

  base_colormap = m->fadetable->colormap;
}
