// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: hwr_bsp.cpp 388 2006-11-04 14:48:17Z smite-meister $
//
// Copyright (C) 1998-2006 by DooM Legacy Team.
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
/// \brief Contains the Doom map converted to GL friendly format

#include <math.h>

#include "doomdef.h"

#include "hardware/hwr_bsp.h"
#include "hardware/hwr_render.h"
#include "console.h"
#include "command.h"
#include "cvars.h"
#include "m_bbox.h"
#include "r_render.h"
#include "r_main.h"
#include "r_bsp.h"
#include "i_video.h"

#ifndef NO_OPENGL

HWBsp::HWBsp(int size, int bspnum) :
  num_planepolys(size)
{
  CONS_Printf("Generating subsector polygons... %d subsectors\n", size);
  con.Drawer();     // let the user know what we are doing
  I_FinishUpdate(); // page flip or blit buffer

  subsectors.resize(size, 0);
  planepolys.resize(size, 0);

  // TODO: this is already done somewhere in the software renderer, maybe we should keep that value instead of recomputing it
  //NOTE now also found in Map class
  bbox_t rootbbox;
  rootbbox.Clear();
  // find min/max boundaries of map
  for (int i = 0; i < R.numvertexes; i++)
    {
      rootbbox.Add(R.vertexes[i].x, R.vertexes[i].y);
    }

  Poly *rootp = AllocPoly(4);
  PolyVertex *rootpv = rootp->pts;
  rootpv->x = rootbbox[BOXLEFT  ].Float();
  rootpv->y = rootbbox[BOXBOTTOM].Float();
  rootpv++;
  rootpv->x = rootbbox[BOXLEFT  ].Float();
  rootpv->y = rootbbox[BOXTOP   ].Float();
  rootpv++;
  rootpv->x = rootbbox[BOXRIGHT ].Float();
  rootpv->y = rootbbox[BOXTOP   ].Float();
  rootpv++;
  rootpv->x = rootbbox[BOXRIGHT ].Float();
  rootpv->y = rootbbox[BOXBOTTOM].Float();
  rootpv++;
  Traverse(bspnum - 1, rootp, 0, rootbbox); // create sub-sectors
  if (cv_grsolvetjoin.value)
    {
      CONS_Printf("Solving T-joins. This may take a while. Please wait...\n");
      con.Drawer();       //let the user know what we are doing
      I_FinishUpdate();  // page flip or blit buffer
      SolveTProblem();
    }
  AdjustSegs();
}

HWBsp::~HWBsp()
{
  int n;
  n = subsectors.size();
  for (int i = 0; i < n; i++)
    {
      delete subsectors[i];
    }
  subsectors.clear();
  /*
  n = planepolys.size();
  for (int i = 0; i < n; i++)
    {
      delete planepolys[i];
    }
  planepolys.clear();
  */
}

// Add a sub-sector to the "hardware BSP"
// At this point, the poly should be convex and the exact layout of the
// subsector, it is not always the case, so continue to cut off the poly
// into smaller parts with each seg of the subsector.
void HWBsp::AddSubsector(unsigned int num, Poly *poly)
{
  if (poly)
    {
      poly = CutOutSubsecPoly(&R.segs[R.subsectors[num].first_seg], R.subsectors[num].num_segs, poly);

      planepolys[num] = poly; //extra data for this subsector

      subsectors[num] = new Subsector(num, poly);
    }
}

void HWBsp::Traverse(int bspnum, Poly* poly, Uint32 *leafnode, bbox_t &bbox)
{
  if (bspnum & NF_SUBSECTOR) // found a subsector?
    {
      if (bspnum == -1)
        {
          CONS_Printf("Error %s(%d): according to Boris, this should never happen.\n", __FILE__, __LINE__);
          if (poly && poly->numpts > 2)
            {
              if (planepolys.size() <= num_planepolys)
                {
                  planepolys.resize(num_planepolys * 2, 0);
                  subsectors.resize(num_planepolys * 2, 0);
                }
              *leafnode = (unsigned short)num_planepolys | NF_SUBSECTOR;
              planepolys[num_planepolys] = poly;
              num_planepolys++;
            }
          //AddSubsector(0, poly);
        }
      else
        {
          AddSubsector(bspnum&(~NF_SUBSECTOR), poly);
        }
      bbox.Clear();
      poly = planepolys[bspnum&~NF_SUBSECTOR];
      PolyVertex *pt = poly->pts;
      for (int i=0; i<poly->numpts; i++)
        {
          bbox.Add(pt->x, pt->y);
          ++pt;
        }
    }
  else // a group node
    {
      node_t *bsp = &R.nodes[bspnum];
      DivLine fdivline;
      SearchDivLine(bsp, &fdivline);
      Poly *backpoly, *frontpoly;
      SplitPoly(&fdivline, poly, &frontpoly, &backpoly);

      // Recursively divide front space.
      if (frontpoly)
        {
          Traverse(bsp->children[0], frontpoly, &bsp->children[0], bsp->bbox[0]);
          // copy child bbox
          //memcpy(bbox, bsp->bbox[0], sizeof(bbox_t));
	  bbox = bsp->bbox[0];
        }
      else
        {
          I_Error("Traverse: no front poly ?");
        }
      // Recursively divide back space.
      if (backpoly)
        {
          // Correct back bbox to include floor/ceiling convex polygon
          Traverse(bsp->children[1], backpoly, &bsp->children[1], bsp->bbox[1]);
          // enlarge bbox with seconde child
          bbox.Add(bsp->bbox[1][BOXLEFT  ], bsp->bbox[1][BOXTOP   ]);
          bbox.Add(bsp->bbox[1][BOXRIGHT ], bsp->bbox[1][BOXBOTTOM]);
        }
    }
}

void HWBsp::Render(int bspnum)
{
  if (bspnum & NF_SUBSECTOR) // Found a subsector?
    {
      if (bspnum == -1)
        {
          CONS_Printf("Error %s(%d): according to Boris, this should never happen.\n", __FILE__, __LINE__);
          subsectors[0]->Render();
        }
      else
        {
          subsectors[bspnum&(~NF_SUBSECTOR)]->Render();
        }
      return;
    }

  // not a subsector, a nodes
  node_t* node = &R.nodes[bspnum];

  // Decide which side the view point is on.
  int side = R_PointOnSide(R.viewx, R.viewy, node);

  // Recursively divide front space.
  Render(node->children[side]);

  // Possibly divide back space.
  if (R.R_CheckBBox(node->bbox[side^1]))
    {
      Render(node->children[side^1]);
    }
}


// ============================================================================


// Return interception along bsp line, with the polygon segment
PolyVertex* HWBsp::FracDivLine(DivLine* bsp, PolyVertex* v1, PolyVertex* v2)
{
  static PolyVertex pt;
  double frac;
  double num;
  double den;
  double v1x, v1y, v1dx, v1dy;
  double v2x, v2y, v2dx, v2dy;

  // a segment of a polygon
  v1x  = v1->x;
  v1y  = v1->y;
  v1dx = v2->x - v1->x;
  v1dy = v2->y - v1->y;

  // the bsp partition line
  v2x  = bsp->x;
  v2y  = bsp->y;
  v2dx = bsp->dx;
  v2dy = bsp->dy;

  den = v2dy*v1dx - v2dx*v1dy;
  if (den == 0)
    {
      return NULL;       // parallel
    }

  // first check the frac along the polygon segment,
  // (do not accept hit with the extensions)
  num = (v2x - v1x)*v2dy + (v1y - v2y)*v2dx;
  frac = num / den;
  if (frac<0 || frac>1)
    {
      return NULL;
    }

  // now get the frac along the BSP line
  // which is useful to determine what is left, what is right
  num = (v2x - v1x)*v1dy + (v1y - v2y)*v1dx;
  frac = num / den;
  bspfrac = frac;


  // find the interception point along the partition line
  pt.x = v2x + v2dx*frac;
  pt.y = v2y + v2dy*frac;

  return &pt;
}

// if two vertex coords have a x and/or y difference of less or equal than 1 FRACUNIT,
// they are considered the same point. Note: hardcoded value, 1.0f could be anything else.
bool HWBsp::SameVertice(PolyVertex* p1, PolyVertex* p2)
{
#if 0
  float diff;
  diff = p2->x - p1->x;
  if (diff < -1.5f || diff > 1.5f)
    {
      return false;
    }
  diff = p2->y - p1->y;
  if (diff < -1.5f || diff > 1.5f)
    {
      return false;
    }
#else
  if (p1->x != p2->x)
    {
      return false;
    }
  if (p1->y != p2->y)
    {
      return false;
    }
#endif
  // p1 and p2 are considered the same vertex
  return true;
}

// split a _CONVEX_ polygon in two convex polygons
// outputs:
//   frontpoly : polygon on right side of bsp line
//   backpoly  : polygon on left side
void HWBsp::SplitPoly(DivLine* bsp,      // splitting parametric line
                      Poly* poly,        // the convex poly we split
                      Poly** frontpoly,  // return one poly here
                      Poly** backpoly)   // return the other here
{
  // here, s -> start, e -> end
  PolyVertex *pv, lastpv;
  PolyVertex vs, ve;
  float fracs = 0.0, frace = 0.0;  // used to tell which poly is on the front side of the bsp partition line
  int ps = -1, pe = -1;
  int psonline = 0, peonline = 0;

  for (int i=0; i<poly->numpts; i++)
    {
      int j = i + 1;
      if (j==poly->numpts)
        {
          j = 0;
        }

      // start & end points
      pv = FracDivLine(bsp, &poly->pts[i], &poly->pts[j]);

      if (pv)
        {
          if (ps<0)
            {
              // first point
              ps = i;
              vs = *pv;
              fracs = bspfrac;
            }
          else
            {
              //the partition line traverse a junction between two segments
              // or the two points are so close, they can be considered as one
              // thus, don't accept, since split 2 must be another vertex
              if (SameVertice(pv, &lastpv))
                {
                  if (pe<0)
                    {
                      ps = i;
                      psonline = 1;
                    }
                  else
                    {
                      pe = i;
                      peonline = 1;
                    }
                }
              else
                {
                  if (pe<0)
                    {
                      pe = i;
                      ve = *pv;
                      frace = bspfrac;
                    }
                  else
                    {
                      // a frac, not same vertice as last one
                      // we already got pt2 so pt 2 is not on the line,
                      // so we probably got back to the start point
                      // which is on the line
                      if (SameVertice(pv, &vs))
                        {
                          psonline = 1;
                        }
                      break;
                    }
                }
            }
          // remember last point intercept to detect identical points
          lastpv = *pv;
        }
    }
  // no split : the partition line is either parallel and
  // aligned with one of the poly segments, or the line is totally
  // out of the polygon and doesn't traverse it (happens if the bsp
  // is fooled by some trick where the sidedefs don't point to
  // the right sectors)
  if (ps<0)
    {
      //I_Error ("SplitPoly: did not split polygon (%d %d)\n"
      //         "debugpos %d",ps,pe,debugpos);

      // this eventually happens with 'broken' BSP's that accept
      // linedefs where each side point the same sector, that is:
      // the deep water effect with the original Doom

      //TODO: make sure front poly is to front of partition line?

      *frontpoly = poly;
      *backpoly  = NULL;
      return;
    }

  if (ps>=0 && pe<0)
    {
      //I_Error ("SplitPoly: only one point for split line (%d %d)",ps,pe);
      *frontpoly = poly;
      *backpoly  = NULL;
      return;
    }
  if (pe<=ps)
    {
      I_Error ("SplitPoly: invalid splitting line (%d %d)",ps,pe);
    }

  // number of points on each side, _not_ counting those
  // that may lie just one the line
  int nptback  = pe - ps - peonline;
  int nptfront = poly->numpts - peonline - psonline - nptback;

  if (nptback>0)
    {
      *backpoly = AllocPoly (2 + nptback);
    }
  else
    {
      *backpoly = NULL;
    }
  if (nptfront)
    {
      *frontpoly = AllocPoly (2 + nptfront);
    }
  else
    {
      *frontpoly = NULL;
    }

  // generate FRONT poly
  if (*frontpoly)
    {
      pv = (*frontpoly)->pts;
      *pv++ = vs;
      *pv++ = ve;
      int i = pe;
      do
        {
          if (++i == poly->numpts)
            {
              i=0;
            }
          *pv++ = poly->pts[i];
        }
      while (i!=ps && --nptfront);
    }

  // generate BACK poly
  if (*backpoly)
    {
      pv = (*backpoly)->pts;
      *pv++ = ve;
      *pv++ = vs;
      int i = ps;
      do
        {
          if (++i == poly->numpts)
            {
              i=0;
            }
          *pv++ = poly->pts[i];
        }
      while (i!=pe && --nptback);
    }

  // make sure frontpoly is the one on the 'right' side
  // of the partition line
  if (fracs>frace)
    {
      Poly* swappoly;
      swappoly = *backpoly;
      *backpoly= *frontpoly;
      *frontpoly = swappoly;
    }

  FreePoly (poly);
}


// use each seg of the poly as a partition line, keep only the part of the
// convex poly to the front of the seg (that is, the part inside the sector),
// the part behind the seg, is  the void space and is cut out
Poly* HWBsp::CutOutSubsecPoly(seg_t* lseg, int count, Poly* poly)
{
  PolyVertex *pv;
  int nump=0, ps, pe;
  PolyVertex vs, ve, p1, p2;
  float fracs=0.0;
  DivLine cutseg;     //x, y, dx, dy as start of node_t struct
  Poly* temppoly;

  // for each seg of the subsector
  for(;count--;lseg++)
    {
      //x, y, dx, dy (like a divline)
      line_t *line = lseg->linedef;
      p1.x = (lseg->side?line->v2->x:line->v1->x).Float();
      p1.y = (lseg->side?line->v2->y:line->v1->y).Float();
      p2.x = (lseg->side?line->v1->x:line->v2->x).Float();
      p2.y = (lseg->side?line->v1->y:line->v2->y).Float();

      cutseg.x = p1.x;
      cutseg.y = p1.y;
      cutseg.dx = p2.x - p1.x;
      cutseg.dy = p2.y - p1.y;

      // see if it cuts the convex poly
      ps = -1;
      pe = -1;
      for (int i=0; i<poly->numpts; i++)
        {
          int j = i + 1;
          if (j==poly->numpts)
            {
              j=0;
            }

          pv = FracDivLine(&cutseg, &poly->pts[i], &poly->pts[j]);

          if (pv)
            {
              if (ps<0)
                {
                  ps = i;
                  vs = *pv;
                  fracs = bspfrac;
                }
              else
                {
                  //frac 1 on previous segment,
                  //     0 on the next,
                  //the split line goes through one of the convex poly
                  // vertices, happens quite often since the convex
                  // poly is already adjacent to the subsector segs
                  // on most borders
                  if (SameVertice(pv, &vs))
                    {
                      continue;
                    }

                  if (fracs<=bspfrac)
                    {
                      nump = 2 + poly->numpts - (i-ps);
                      pe = ps;
                      ps = i;
                      ve = *pv;
                    }
                  else
                    {
                      nump = 2 + (i-ps);
                      pe = i;
                      ve = vs;
                      vs = *pv;
                    }
                  //found 2nd point
                  break;
                }
            }
        }

      // there was a split
      if (ps>=0)
        {
          //need 2 points
          if (pe>=0)
            {
              // generate FRONT poly
              temppoly = AllocPoly (nump);
              pv = temppoly->pts;
              *pv++ = vs;
              *pv++ = ve;
              do
                {
                  if (++ps == poly->numpts)
                    {
                      ps=0;
                    }
                  *pv++ = poly->pts[ps];
                }
              while (ps!=pe);
              FreePoly(poly);
              poly = temppoly;
            }
          //hmmm... maybe we should NOT accept this, but this happens
          // only when the cut is not needed it seems (when the cut
          // line is aligned to one of the borders of the poly, and
          // only some times..)
          else
            {
              CONS_Printf("CutOutPoly: only one point for split line (%d %d)\n", ps, pe);
              //skipcut++;
              //I_Error ("CutOutPoly: only one point for split line (%d %d) %d",ps,pe,debugpos);
            }
        }
    }
  return poly;
}

// the bsp divline have not enough precision search for the segs source of this divline
void HWBsp::SearchDivLine(node_t* bsp, DivLine *divline)
{
#if 0
  // Hurdler: the code here has been removed (see the CVS and also CutOutSubsecPoly)
  // MAR - If you don't use the same partition line that the BSP uses, the front/back polys won't match the subsectors in the BSP!
#endif
  divline->x=bsp->x.Float();
  divline->y=bsp->y.Float();
  divline->dx=bsp->dx.Float();
  divline->dy=bsp->dy.Float();
}

#define MAXDIST   (1.5f)
// BP: can't move vertex : DON'T change polygone geometry ! (convex)
//#define MOVEVERTEX
bool HWBsp::PointInSeg(PolyVertex* a, PolyVertex* v1, PolyVertex* v2)
{
  register float ax,ay,bx,by,cx,cy,d,norm;
  register PolyVertex* p;

  // check bbox of the seg first
  if (v1->x>v2->x)
    {
      p=v1;
      v1=v2;
      v2=p;
    }
  if(a->x<v1->x-MAXDIST || a->x>v2->x+MAXDIST)
    {
      return false;
    }

  if (v1->y>v2->y)
    {
      p=v1;
      v1=v2;
      v2=p;
    }
  if(a->y<v1->y-MAXDIST || a->y>v2->y+MAXDIST)
    {
      return false;
    }

  // v1 = origine
  ax= v2->x-v1->x;
  ay= v2->y-v1->y;
  norm = sqrt(ax*ax+ay*ay);
  ax/=norm;
  ay/=norm;
  bx=a->x-v1->x;
  by=a->y-v1->y;
  //d = a.b
  d =ax*bx+ay*by;
  // bound of the seg
  if(d<0 || d>norm)
    {
      return false;
    }
  //c=d.1a-b
  cx=ax*d-bx;
  cy=ay*d-by;
#ifdef MOVEVERTEX
  if(cx*cx+cy*cy<=MAXDIST*MAXDIST)
    {
      // adjust a little the point position
      a->x=ax*d+v1->x;
      a->y=ay*d+v1->y;
      // anyway the correction is not enough
      return true;
    }
  return false;
#else
  return cx*cx+cy*cy<=MAXDIST*MAXDIST;
#endif
}

void HWBsp::SearchSegInBSP(int bspnum,PolyVertex *p,Poly *poly)
{
  if (bspnum & NF_SUBSECTOR)
    {
      if (bspnum!=-1)
        {
          bspnum &= ~NF_SUBSECTOR;
          Poly *q = planepolys[bspnum];
          if (poly==q || !q)
            {
              return;
            }
          for(int j=0; j<q->numpts; j++)
            {
              int k = j + 1;
              if (k==q->numpts)
                {
                  k = 0;
                }
              if (!SameVertice(p,&q->pts[j]) &&
                  !SameVertice(p,&q->pts[k]) &&
                  PointInSeg(p,&q->pts[j],&q->pts[k]))
                {
                  Poly *newpoly = AllocPoly(q->numpts+1);
                  int n;
                  for(n=0; n<=j; n++)
                    {
                      newpoly->pts[n] = q->pts[n];
                    }
                  newpoly->pts[k]=*p;
                  for(n=k+1; n<newpoly->numpts; n++)
                    {
                      newpoly->pts[n] = q->pts[n-1];
                    }
                  planepolys[bspnum] = newpoly;
                  FreePoly(q);
                  return;
                }
            }
        }
      return;
    }

  if ((R.nodes[bspnum].bbox[0][BOXBOTTOM].Float()-MAXDIST<=p->y) &&
      (R.nodes[bspnum].bbox[0][BOXTOP   ].Float()+MAXDIST>=p->y) &&
      (R.nodes[bspnum].bbox[0][BOXLEFT  ].Float()-MAXDIST<=p->x) &&
      (R.nodes[bspnum].bbox[0][BOXRIGHT ].Float()+MAXDIST>=p->x))
    {
      SearchSegInBSP(R.nodes[bspnum].children[0],p,poly);
    }

  if ((R.nodes[bspnum].bbox[1][BOXBOTTOM].Float()-MAXDIST<=p->y) &&
      (R.nodes[bspnum].bbox[1][BOXTOP   ].Float()+MAXDIST>=p->y) &&
      (R.nodes[bspnum].bbox[1][BOXLEFT  ].Float()-MAXDIST<=p->x) &&
      (R.nodes[bspnum].bbox[1][BOXRIGHT ].Float()+MAXDIST>=p->x))
    {
      SearchSegInBSP(R.nodes[bspnum].children[1],p,poly);
    }
}

// search for T-intersection problem
// BP : It can be mush more faster doing this at the same time of the splitpoly
// but we must use a different structure : polygon pointing on segs
// segs pointing on polygon and on vertex (too mush complicated, well not
// realy but i am soo lasy), the method described is also better for segs precision
extern consvar_t cv_grsolvetjoin;

void HWBsp::SolveTProblem()
{
  for(unsigned int l=0; l<num_planepolys; l++)
    {
      Poly *p = planepolys[l];
      if (p)
        {
          for(int i=0; i<p->numpts; i++)
            {
              SearchSegInBSP(R.numnodes-1, &p->pts[i], p);
            }
        }
    }
}

#define NEARDIST (0.75f)
#define MYMAX    (10000000000000.0f)

/*  Adjust true segs (from the segs lump) to be exactly the same as
 *  plane polygon segs
 *  This also convert fixed_t point of segs in float (in most case
 *  it share the same vertice
 */
void HWBsp::AdjustSegs()
{
  int v1found = 0, v2found = 0;
  float nearv1, nearv2;

  for(int i=0; i<R.numsubsectors; i++)
    {
      Poly *p = planepolys[i];
      if(!p)
        {
          continue;
        }
      seg_t* lseg = &R.segs[R.subsectors[i].first_seg];
      for(int count=R.subsectors[i].num_segs; count--; lseg++)
        {
          float distv1,distv2,tmp;
          nearv1=nearv2=MYMAX;
          for(int j=0; j<p->numpts; j++)
            {
              distv1 = p->pts[j].x - (lseg->v1->x).Float();
              tmp    = p->pts[j].y - (lseg->v1->y).Float();
              distv1 = distv1*distv1+tmp*tmp;
              if (distv1 <= nearv1)
                {
                  v1found = j;
                  nearv1 = distv1;
                }
              // the same with v2
              distv2 = p->pts[j].x - (lseg->v2->x).Float();
              tmp    = p->pts[j].y - (lseg->v2->y).Float();
              distv2 = distv2*distv2+tmp*tmp;
              if (distv2 <= nearv2)
                {
                  v2found = j;
                  nearv2 = distv2;
                }
            }
          if (nearv1<=NEARDIST*NEARDIST)
            {
              // share vertice with segs
              lseg->v1 = (vertex_t *)&(p->pts[v1found]); //Hurdler: geez, what a hack!
            }
          else
            {
              // BP: here we can do better, using PointInSeg and compute
              // the right point position also split a polygone side to
              // solve a T-intersection, but too mush work

              // convert fixed vertex to float vertex
              PolyVertex *p=AllocVertex();
              p->x=lseg->v1->x.Float();
              p->y=lseg->v1->y.Float();
              lseg->v1 = (vertex_t *)p; //Hurdler: geez, what a hack!
            }
          if (nearv2<=NEARDIST*NEARDIST)
            {
              lseg->v2 = (vertex_t *)&(p->pts[v2found]); //Hurdler: geez, what a hack!
            }
          else
            {
              PolyVertex *p=AllocVertex();
              p->x=lseg->v2->x.Float();
              p->y=lseg->v2->y.Float();
              lseg->v2 = (vertex_t *)p; //Hurdler: geez, what a hack!
            }

#if 0  //FIXME: Hurdler put it back if it's necessary with the new renderer
          // recompute length
          float x = ((PolyVertex *)lseg->v2)->x-((PolyVertex *)lseg->v1)->x+0.5*fixedtofloat;
          float y = ((PolyVertex *)lseg->v2)->y-((PolyVertex *)lseg->v1)->y+0.5*fixedtofloat;
          lseg->length = sqrt(x*x+y*y)*FRACUNIT;
          // BP: debug see this kind of segs
          //if (nearv2>NEARDIST*NEARDIST || nearv1>NEARDIST*NEARDIST)
          //    lseg->length=1;
#endif
        }
    }
}


// ============================================================================


// TO Finish
Subsector::Subsector(int num, Poly *poly)
{
  int count;
  seg_t *line, *prev_line, *next_line;

  if (num < R.numsubsectors)
    {
      CONS_Printf("found a sub sector: %d\n", num);
      sub = &R.subsectors[num]; // subsector
      count = sub->num_segs; // how many segs
      line = &R.segs[sub->first_seg]; // first line seg
      prev_line = line + count - 1;   // last line seg
      next_line = line + 1;
    }
  else
    {
      CONS_Printf("I think this is strange, there are no segs but only planes: %d\n", num);
      sub = &R.subsectors[0];
      count = 0;
      line = NULL;
      prev_line = NULL;
      next_line = NULL;
    }

  sector_t tempsec;
  int floorlightlevel, ceilinglightlevel;
  frontsector = R.R_FakeFlat(sub->sector, &tempsec, &floorlightlevel, &ceilinglightlevel, false);

  fixed_t locFloorHeight, locCeilingHeight;
  if (frontsector->pseudoSector)
    {
      locFloorHeight   = frontsector->virtualFloorheight;
      locCeilingHeight = frontsector->virtualCeilingheight;
    }
  else if (frontsector->virtualFloor)
    {
      locFloorHeight   = frontsector->virtualFloorheight;
      locCeilingHeight = frontsector->virtualCeiling ? frontsector->virtualCeilingheight : frontsector->ceilingheight;
    }
  else if (frontsector->virtualCeiling)
    {
      locFloorHeight   = frontsector->floorheight;
      locCeilingHeight = frontsector->virtualCeilingheight;
    }
  else
    {
      locFloorHeight   = frontsector->floorheight;
      locCeilingHeight = frontsector->ceilingheight;
    }

  if (frontsector->ffloors)
    {
      if (frontsector->moved)
        {
          frontsector->numlights = sub->sector->numlights = 0;
          R.R_Prep3DFloors(frontsector);
          sub->sector->lightlist = frontsector->lightlist;
          sub->sector->numlights = frontsector->numlights;
          sub->sector->moved = frontsector->moved = false;
        }
      // Hurdler: humm, this is done a bit differently in the software renderer, why ?
      floorlightlevel = *frontsector->lightlist[R_GetPlaneLight(frontsector, locFloorHeight, false)].lightlevel;
      ceilinglightlevel = *frontsector->lightlist[R_GetPlaneLight(frontsector, locCeilingHeight, false)].lightlevel;
    }

  // We have now all the info we need to create the different arrays (colors, texcoords and vertices)

  State *state = new State();
  state->SetColor((rand() % 256) / 255.0f, (rand() % 256) / 255.0f, (rand() % 256) / 255.0f, 1.0f);
  if (poly->numpts > 2)
    {
      // -= FLOOR =-
      Geometry *geometry = new Geometry();

      GLuint *primitive_length = new GLuint(poly->numpts);
      GLuint *primitive_type = new GLuint(GL_TRIANGLE_FAN);
      GLfloat *vertex_array = new GLfloat[3 * poly->numpts];
      GLfloat *texcoord_array = new GLfloat[2 * poly->numpts];
      GLushort *indices = new GLushort[poly->numpts];

      float *pts = &poly->pts[0].x;
      for (int i = 0; i < poly->numpts; i++)
        {
          vertex_array[i * 3 + 0] = *pts++;
          vertex_array[i * 3 + 1] = locFloorHeight.Float();
          vertex_array[i * 3 + 2] = *pts++;
          texcoord_array[i * 2 + 0] = 0.0f;
          texcoord_array[i * 2 + 1] = 0.0f;
          indices[i] = i;
        }
      geometry->SetPrimitiveLength(primitive_length);
      geometry->SetPrimitiveType(primitive_type);
      geometry->SetNumPrimitives(1);
      geometry->SetAttributes(Geometry::VERTEX_ARRAY, vertex_array);
      geometry->SetAttributes(Geometry::TEXCOORD_ARRAY0, texcoord_array);
      geometry->SetIndices(indices);
      geometries.push_back(geometry);
      states.push_back(state);

      // -= CEILING =-
      geometry = new Geometry();

      primitive_length = new GLuint(poly->numpts);
      primitive_type = new GLuint(GL_TRIANGLE_FAN);
      vertex_array = new GLfloat[3 * poly->numpts];
      texcoord_array = new GLfloat[2 * poly->numpts];
      indices = new GLushort[poly->numpts];

      pts = &poly->pts[0].x;
      for (int i = 0; i < poly->numpts; i++)
        {
          vertex_array[i * 3 + 0] = *pts++;
          vertex_array[i * 3 + 1] = locCeilingHeight.Float();
          vertex_array[i * 3 + 2] = *pts++;
          texcoord_array[i * 2 + 0] = 0.0f;
          texcoord_array[i * 2 + 1] = 0.0f;
          indices[i] = i;
        }
      geometry->SetPrimitiveLength(primitive_length);
      geometry->SetPrimitiveType(primitive_type);
      geometry->SetNumPrimitives(1);
      geometry->SetAttributes(Geometry::VERTEX_ARRAY, vertex_array);
      geometry->SetAttributes(Geometry::TEXCOORD_ARRAY0, texcoord_array);
      geometry->SetIndices(indices);
      geometries.push_back(geometry);
      states.push_back(state);
    }
  else
    {
      CONS_Printf("Error: this polyong has only %d points\n", poly->numpts);
    }

  // -= WALLS =-
  if (line)  // FIXME: this probably doesn't work with 3D-floors yet
    {
      while (count--)
        {
          if (line->backsector)
            {
              if ((locFloorHeight < line->backsector->floorheight) && (locCeilingHeight > line->backsector->floorheight))
                {
                  AddWall(line, prev_line, next_line, locFloorHeight, line->backsector->floorheight); // bottom
                }
              if ((locCeilingHeight > line->backsector->ceilingheight) && (locFloorHeight < line->backsector->ceilingheight))
                {
                  AddWall(line, prev_line, next_line, line->backsector->ceilingheight, locCeilingHeight);  // top
                }
            }
          else
            {
              AddWall(line, prev_line, next_line, locFloorHeight, locCeilingHeight); // plain wall
            }
          line++;
          if (count > 1)
            next_line++;
          else
            next_line = &R.segs[sub->first_seg];
          prev_line++;
        }
    }
}

void Subsector::AddWall(seg_t *line, seg_t *prev_line, seg_t *next_line, fixed_t floor, fixed_t ceiling)
{
  Geometry *geometry = new Geometry();
  State *state = new State();
  state->SetColor((rand() % 256) / 255.0f, (rand() % 256) / 255.0f, (rand() % 256) / 255.0f, 1.0f);

  // TODO: prev_line and next_line are here to solve T-Intersection problem
  //       but it's not that easy. In fact, the wall at the right (resp. left) side can
  //       be prev_line (resp. next_line) itself, or the left (resp. right) side of the
  //       backsector of prev_line (resp. next_line). It all depends if the wall at the
  //       left (resp. right) side is in the same subsector or not). And then it might
  //       depend if there are top or bottom wall on the left (resp. right side). This
  //       is thus a bit tricky. The wall on the left (resp. right side) is from another
  //       subsector if prev_line->backsector (resp. next_line->backsector) is not null.

  GLuint *primitive_length = new GLuint(4);
  GLuint *primitive_type = new GLuint(GL_TRIANGLE_FAN);
  GLfloat *vertex_array = new GLfloat[3 * 4];
  GLfloat *texcoord_array = new GLfloat[2 * 4];
  GLushort *indices = new GLushort[4];

  // 3--2
  // | /|
  // |/ |
  // 0--1

  int i = 0;
  vertex_array[i * 3 + 0] = line->v1->x.Float();
  vertex_array[i * 3 + 1] = floor.Float();
  vertex_array[i * 3 + 2] = line->v1->y.Float();
  texcoord_array[i * 2 + 0] = 0.0f;
  texcoord_array[i * 2 + 1] = 0.0f;
  indices[i] = i;

  i++;
  vertex_array[i * 3 + 0] = line->v2->x.Float();
  vertex_array[i * 3 + 1] = floor.Float();
  vertex_array[i * 3 + 2] = line->v2->y.Float();
  texcoord_array[i * 2 + 0] = 0.0f;
  texcoord_array[i * 2 + 1] = 0.0f;
  indices[i] = i;

  i++;
  vertex_array[i * 3 + 0] = line->v2->x.Float();
  vertex_array[i * 3 + 1] = ceiling.Float();
  vertex_array[i * 3 + 2] = line->v2->y.Float();
  texcoord_array[i * 2 + 0] = 0.0f;
  texcoord_array[i * 2 + 1] = 0.0f;
  indices[i] = i;

  i++;
  vertex_array[i * 3 + 0] = line->v1->x.Float();
  vertex_array[i * 3 + 1] = ceiling.Float();
  vertex_array[i * 3 + 2] = line->v1->y.Float();
  texcoord_array[i * 2 + 0] = 0.0f;
  texcoord_array[i * 2 + 1] = 0.0f;
  indices[i] = i;

  geometry->SetPrimitiveLength(primitive_length);
  geometry->SetPrimitiveType(primitive_type);
  geometry->SetNumPrimitives(1);
  geometry->SetAttributes(Geometry::VERTEX_ARRAY, vertex_array);
  geometry->SetAttributes(Geometry::TEXCOORD_ARRAY0, texcoord_array);
  geometry->SetIndices(indices);
  geometries.push_back(geometry);
  states.push_back(state);
}

Subsector::~Subsector()
{
  geometries.clear();
  states.clear();
}

// TO Finish
void Subsector::Render()
{
/*
  sector_t tempsec;
  int floorlightlevel, ceilinglightlevel;
  frontsector = R.R_FakeFlat(sub->sector, &tempsec, &floorlightlevel, &ceilinglightlevel, false);
  if (frontsector->moved)
    {
      frontsector->moved = false;
      CONS_Printf("This sector has moved, we have to do some computation\n");
    }
*/
  unsigned int n = geometries.size();
  for (unsigned i = 0; i < n; i++)
    {
      // TODO: push that on a stack, then render by avoiding render state and render back to front
      //       for translucent polygons and front to back for opaque one.
      // TODO: move geometry if necessary (polyobj, ...) by looking at the sub member
      states[i]->Apply();
      geometries[i]->Draw();
    }
}

#endif // NO_OPENGL
