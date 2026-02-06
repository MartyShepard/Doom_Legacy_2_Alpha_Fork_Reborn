// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: r_splats.cpp 476 2007-06-14 18:31:04Z smite-meister $
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
/// \brief SW renderer: Decals, floor and wall splats

#include <math.h>

#include "doomdef.h"

#include "g_map.h"
#include "r_render.h"
#include "r_data.h"
#include "r_main.h"
#include "r_splats.h"


//==========================================================================
//                                                               WALL SPLATS
//==========================================================================

static wallsplat_t  wallsplats[MAXLEVELSPLATS];     // WALL splats
static int          freewallsplat;


// --------------------------------------------------------------------------
// Return a pointer to a splat free for use, or NULL if no more splats are
// available
// --------------------------------------------------------------------------
static wallsplat_t *R_AllocWallSplat()
{
  // clear the splat from the line if it was in use
  wallsplat_t *splat = &wallsplats[freewallsplat];
  line_t *li = splat->line;

  if (li)
    {
      // remove splat from line splats list
      if (li->splats == splat)
	li->splats = splat->next;   //remove from head
      else
        {
#ifdef PARANOIA
	  if (!li->splats)
	    I_Error("R_AllocWallSplat : No splat in this line\n");
#endif
	  for (wallsplat_t *p = li->splats; p->next; p = p->next)
	    if (p->next == splat)
                {
		  p->next = splat->next;
		  break;
                }
        }
    }

  memset(splat, 0, sizeof(wallsplat_t));

  // for next allocation
  freewallsplat++;
  if (freewallsplat >= MAXLEVELSPLATS)
    freewallsplat = 0;

  return splat;
}



static fixed_t P_SegLength(line_t *l)
{
  double dx = (l->v2->x - l->v1->x).Float();
  double dy = (l->v2->y - l->v1->y).Float();

  return sqrtf(dx*dx+dy*dy);
}

// Add a new splat to the linedef:
// top : top z coord
// wallfrac : frac along the linedef vector (0 to FRACUNIT)
// splatpatchname : name of patch to draw
void line_t::AddWallSplat(const char *name, int side, fixed_t top, fixed_t wallfrac, int flags)
{
  //CONS_Printf("wall splat: %s\n", name);
  wallsplat_t *splat = R_AllocWallSplat();
  if (!splat)
    return;

  // set the splat
  Material *mat = splat->mat = materials.Get(name);
  side ^= 1;
 
  sector_t *backsec = NULL;
  if (sideptr[side])
    {
      backsec = sideptr[side]->sector;

      // BUGFIX! first backsec must be checked against NULL value!
      if (backsec)
	{
	  if (top < backsec->floorheight)
	    {
	      splat->yoffset = &backsec->floorheight;
	      top -= backsec->floorheight;
	    }
	  else if (top > backsec->ceilingheight)
	    {
	      splat->yoffset = &backsec->ceilingheight;
	      top -= backsec->ceilingheight;
	    }
	}
    }
  //splat->sectorside = side;

  splat->top = top;
  splat->flags = flags;
  
  // offset needed by draw code for texture mapping
  fixed_t linelength = P_SegLength(this);
  float temp = mat->tex[0].worldwidth/2;

  splat->offset = wallfrac*linelength - temp;
  //CONS_Printf("offset splat %d\n",splat->offset);
  fixed_t fracsplat = temp / linelength;
    
  wallfrac -= fracsplat;
  if (wallfrac > linelength)
    return;
  //CONS_Printf("final splat position %f\n",FIXED_TO_FLOAT(wallfrac));
  splat->v1.x = v1->x + (dx * wallfrac);
  splat->v1.y = v1->y + (dy * wallfrac);
  wallfrac += fracsplat + fracsplat;
  if (wallfrac < 0)
    return;
  splat->v2.x = v1->x + (dx * wallfrac);
  splat->v2.y = v1->y + (dy * wallfrac);


  if (frontsector && frontsector == backsec)
    {
/*  BP: dont work texture mapping problem :(
        // in the other side
        vertex_t p = splat->v1;
        splat->v1 = splat->v2;
        splat->v2 = p;
        CONS_Printf("split\n");
*/
      return;
    }

  // insert splat in the linedef splat list
  // BP: why not insert in head is mush more simple ?
  // BP: because for remove it is more simple !
  splat->line = this;
  splat->next = NULL;
  if (splats)
    {
      wallsplat_t *p = splats;
      while (p->next)
	p = p->next;
      p->next = splat;
    }
  else
    splats = splat;
}





//==========================================================================
//                                                              FLOOR SPLATS
//==========================================================================
#ifdef FLOORSPLATS

static floorsplat_t floorsplats[MAXLEVELSPLATS];    // FLOOR splats
static int          freefloorsplat;

struct rastery_t
{
  fixed_t minx, maxx;     // for each raster line starting at line 0
  fixed_t tx1,ty1;
  fixed_t tx2,ty2;        // start/end points in texture at this line
};

// for floorsplats, accessed by asm code
static rastery_t rastertab[MAXVIDHEIGHT];
rastery_t *prastertab;


//r_plane.c
extern fixed_t     cachedheight[MAXVIDHEIGHT];
extern fixed_t     cacheddistance[MAXVIDHEIGHT];
extern fixed_t     cachedxstep[MAXVIDHEIGHT];
extern fixed_t     cachedystep[MAXVIDHEIGHT];
extern fixed_t     basexscale;
extern fixed_t     baseyscale;

static void prepare_rastertab()
{
  for (int i=0; i<vid.height; i++)
    {
      rastertab[i].minx = fixed_t::FMAX;
      rastertab[i].maxx = fixed_t::FMIN;
    }
}


// --------------------------------------------------------------------------
// Return a pointer to a splat free for use, or NULL if no more splats are
// available
// --------------------------------------------------------------------------
static floorsplat_t* R_AllocFloorSplat()
{
    floorsplat_t* splat;
    floorsplat_t* p_splat;
    subsector_t*  sub;

    // find splat to use
    freefloorsplat++;
    if (freefloorsplat >= MAXLEVELSPLATS)
        freefloorsplat = 0;

    // clear the splat from the line if it was in use
    splat = &floorsplats[freefloorsplat];
    if ( (sub=splat->subsector) )
    {
        // remove splat from subsector splats list
        if (sub->splats == splat)
            sub->splats = splat->next;   //remove from head
        else
        {
            p_splat = sub->splats;
            while (p_splat->next)
            {
                if (p_splat->next == splat)
                    p_splat->next = splat->next;
            }
        }
    }

    memset (splat, 0, sizeof(floorsplat_t));
    return splat;
}


// --------------------------------------------------------------------------
// Add a floor splat to the subsector
// --------------------------------------------------------------------------
void R_AddFloorSplat (subsector_t* subsec, char* picname, fixed_t x, fixed_t y, fixed_t z, int flags)
{
    floorsplat_t*    splat;
    floorsplat_t*    p_splat;

    splat = R_AllocFloorSplat ();
    if (!splat)
        return;

    CONS_Printf ("added a floor splat\n");

    // set the splat
    splat->pic = fc.GetNumForName (picname);

    splat->flags = flags;
    
    //

    //for test fix 64x64
    // 3--2
    // |  |
    // 0--1
    //
    splat->z = z;
    splat->verts[0].x = splat->verts[3].x = x - (32<<FRACBITS);
    splat->verts[2].x = splat->verts[1].x = x + (31<<FRACBITS);
    splat->verts[3].y = splat->verts[2].y = y + (31<<FRACBITS);
    splat->verts[0].y = splat->verts[1].y = y - (32<<FRACBITS);

    // insert splat in the subsector splat list
    splat->subsector = subsec;
    splat->next = NULL;
    if (subsec->splats)
    {
        p_splat = subsec->splats;
        while (p_splat->next)
            p_splat = p_splat->next;
        p_splat->next = splat;
    }
    else
        subsec->splats = splat;
}


// --------------------------------------------------------------------------
// Before each frame being rendered, clear the visible floorsplats list
// --------------------------------------------------------------------------
static floorsplat_t*   visfloorsplats;

void R_ClearVisibleFloorSplats()
{
    visfloorsplats = NULL;
}


// --------------------------------------------------------------------------
// Add a floorsplat to the visible floorsplats list, for the current frame
// --------------------------------------------------------------------------
void R_AddVisibleFloorSplats (subsector_t* subsec)
{
    floorsplat_t* pSplat;
#ifdef PARANOIA
    if (subsec->splats==NULL)
        I_Error ("R_AddVisibleFloorSplats: call with no splats");
#endif

    pSplat = subsec->splats;
    // the splat is not visible from below
    // FIXME: depending on some flag in pSplat->flags, some splats may be visible from 2 sides (above/below)
    if (pSplat->z < viewz) {
        pSplat->nextvis = visfloorsplats;
        visfloorsplats = pSplat;
    }

    while (pSplat->next)
    {
        pSplat = pSplat->next;
        if (pSplat->z < viewz) {
            pSplat->nextvis = visfloorsplats;
            visfloorsplats = pSplat;
        }
    }
}


// tv1,tv2 = x/y qui varie dans la texture, tc = x/y qui est constant.
void    ASMCALL rasterize_segment_tex (int x1, int y1, int x2, int y2, int tv1, int tv2, int tc, int dir);

// current test with floor tile
#define TEXWIDTH 64
#define TEXHEIGHT 64
//#define FLOORSPLATSOLIDCOLOR

// --------------------------------------------------------------------------
// Rasterize the four edges of a floor splat polygon,
// fill the polygon with linear interpolation, call span drawer for each
// scan line
// --------------------------------------------------------------------------
static void R_RenderFloorSplat (floorsplat_t* pSplat, vertex_t* verts, byte* pTex)
{
    // resterizing
    int     miny = vid.height + 1;
        int     maxy = 0;
        int     x, y;
    int     x1, y1, x2, y2;
    byte*   pDest;
        int     tx, ty, tdx, tdy;

    // rendering
    lighttable_t**  planezlight;
    fixed_t         planeheight;
    angle_t     angle;
    fixed_t     distance;
    fixed_t     length;
    unsigned    index;
    int         light;

    fixed_t     offsetx,offsety;

    offsetx = pSplat->verts[0].x & 0x3fffff;
    offsety = pSplat->verts[0].y & 0x3fffff;

        // do for each segment, starting with the first one
    /*CONS_Printf ("floor splat (%d,%d) (%d,%d) (%d,%d) (%d,%d)\n",
                  verts[3].x,verts[3].y,verts[2].x,verts[2].y,
                  verts[1].x,verts[1].y,verts[0].x,verts[0].y);*/

    // do segment a -> top of texture
            x1 = verts[3].x;
            y1 = verts[3].y;
            x2 = verts[2].x;
            y2 = verts[2].y;
    if (y1<0) y1=0;
    if (y1>=vid.height) y1 = vid.height-1;
    if (y2<0) y2=0;
    if (y2>=vid.height) y2 = vid.height-1;
        rasterize_segment_tex (x1, y1, x2, y2, 0, TEXWIDTH-1, 0, 0);
            if( y1 < miny )
                    miny = y1;
            if( y1 > maxy )
                    maxy = y1;

    // do segment b -> right side of texture
                x1 = x2;
                y1 = y2;
                x2 = verts[1].x;
                y2 = verts[1].y;
    if (y1<0) y1=0;
    if (y1>=vid.height) y1 = vid.height-1;
    if (y2<0) y2=0;
    if (y2>=vid.height) y2 = vid.height-1;
                rasterize_segment_tex (x1, y1, x2, y2, 0, TEXHEIGHT-1, TEXWIDTH-1, 1);
                if( y1 < miny )
                    miny = y1;
                if( y1 > maxy )
                    maxy = y1;
        
    // do segment c -> bottom of texture
                x1 = x2;
                y1 = y2;
                x2 = verts[0].x;
                y2 = verts[0].y;
    if (y1<0) y1=0;
    if (y1>=vid.height) y1 = vid.height-1;
    if (y2<0) y2=0;
    if (y2>=vid.height) y2 = vid.height-1;
                rasterize_segment_tex (x1, y1, x2, y2, TEXWIDTH-1, 0, TEXHEIGHT-1, 0);
                if( y1 < miny )
                    miny = y1;
                if( y1 > maxy )
                    maxy = y1;
        
    // do segment d -> left side of texture
                x1 = x2;
                y1 = y2;
                x2 = verts[3].x;
                y2 = verts[3].y;
    if (y1<0) y1=0;
    if (y1>=vid.height) y1 = vid.height-1;
    if (y2<0) y2=0;
    if (y2>=vid.height) y2 = vid.height-1;
                rasterize_segment_tex (x1, y1, x2, y2, TEXHEIGHT-1, 0, 0, 1);
                if( y1 < miny )
                    miny = y1;
                if( y1 > maxy )
                    maxy = y1;

        // remplissage du polygone a 4 cotes AVEC UNE TEXTURE
        //fill_texture_linear( trashbmp, tex->imgdata, miny, maxy );
        //return;

#ifndef FLOORSPLATSOLIDCOLOR

    // prepare values for all the splat
    ds_source = (byte *)fc.CacheLumpNum(pSplat->pic,PU_CACHE);
    planeheight = abs(pSplat->z - viewz);
    light = (pSplat->subsector->sector->lightlevel >> LIGHTSEGSHIFT)+extralight;
    if (light >= LIGHTLEVELS)
        light = LIGHTLEVELS-1;
    if (light < 0)
        light = 0;
    planezlight = zlight[light];

    for (y=miny; y<=maxy; y++)
    {
            x1 = rastertab[y].minx >> FRACBITS;
            x2 = rastertab[y].maxx >> FRACBITS;

        if (x1<0) x1 = 0;
        if (x2>=vid.width) x2 = vid.width-1;

        if (planeheight != cachedheight[y])
        {
            cachedheight[y] = planeheight;
            distance = cacheddistance[y] = (planeheight * yslope[y]);
            ds_xstep = cachedxstep[y] = (distance * basexscale);
            ds_ystep = cachedystep[y] = (distance * baseyscale);
        }
        else
        {
            distance = cacheddistance[y];
            ds_xstep = cachedxstep[y];
            ds_ystep = cachedystep[y];
        }
        length = (distance * distscale[x1]);
        angle = (viewangle + xtoviewangle[x1])>>ANGLETOFINESHIFT;
        ds_xfrac = viewx + (finecosine[angle] * length);
        ds_yfrac = -viewy -(finesine[angle] * length);
        ds_xfrac -= offsetx;
        ds_yfrac += offsety;

        if (fixedcolormap)
            ds_colormap = fixedcolormap;
        else
        {
	  index = distance >> LIGHTZSHIFT >> FRACBITS;
            if (index >= MAXLIGHTZ )
                index = MAXLIGHTZ-1;
            ds_colormap = planezlight[index];
        }

        ds_y = y;
        ds_x1 = x1;
        ds_x2 = x2;
        spanfunc ();

        // reset for next calls to edge rasterizer
            rastertab[y].minx = fixed_t::FMAX;
            rastertab[y].maxx = fixed_t::FMIN;
        }

#else

    for (y=miny; y<=maxy; y++)
    {
        x1 = rastertab[y].minx >> FRACBITS;
        x2 = rastertab[y].maxx >> FRACBITS;
        /*if ((unsigned)x1 >= vid.width)
            continue;
        if ((unsigned)x2 >= vid.width)
            continue;*/
        if (x1<0) x1 = 0;
        //if (x1>=vid.width) x1 = vid.width-1;
        //if (x2<0) x1 = 0;
        if (x2>=vid.width)
            x2 = vid.width-1;

        pDest = ylookup[y] + columnofs[x1];

        x = (x2-x1) + 1;

        //point de d‚part dans la texture
        tx = rastertab[y].tx1;
        ty = rastertab[y].ty1;

        // HORRIBLE BUG!!!
        if(x>0)
        {
             tdx = (rastertab[y].tx2 - tx) / x;
             tdy = (rastertab[y].ty2 - ty) / x;

             while (x-- > 0)
             {
                 *(pDest++) = (y&255);
                 tx += tdx;
                 ty += tdy;
             }
        }

        // r‚initialise les minimus maximus pour le prochain appel
        rastertab[y].minx = fixed_t::FMAX;
        rastertab[y].maxx = fixed_t::FMIN;
    }
#endif
}


// --------------------------------------------------------------------------
// R_DrawFloorSplats
// draw the flat floor/ceiling splats
// --------------------------------------------------------------------------
void R_DrawVisibleFloorSplats()
{
    floorsplat_t* pSplat;
    int           iCount = 0;
    
    fixed_t       tr_x;
    fixed_t       tr_y;
    fixed_t       rot_x;
    fixed_t       rot_y;
    fixed_t       rot_z;
    fixed_t       xscale;
    fixed_t       yscale;
    vertex_t*     v3d;
    vertex_t      v2d[4];
    int           i;

    pSplat = visfloorsplats;
    while (pSplat)
    {
        iCount++;

        // Draw a floor splat
        // 3--2
        // |  |
        // 0--1
        
        rot_z = pSplat->z - viewz;
        for (i=0; i<4; i++)
        {
            v3d = &pSplat->verts[i];
            
            // transform the origin point
            tr_x = v3d->x - viewx;
            tr_y = v3d->y - viewy;

            // rotation around vertical y axis
            rot_x = (tr_x * viewsin) -(tr_y * viewcos);
            rot_y = (tr_x * viewcos) +(tr_y * viewsin);

            if (rot_y < 4*FRACUNIT)
                goto skipit;

            // note: y from view above of map, is distance far away
            xscale = FixedDiv(projection, rot_y);
            yscale = -FixedDiv(projectiony, rot_y);

            // projection
            v2d[i].x = (centerxfrac + (rot_x * xscale)).floor();
            v2d[i].y = (centeryfrac + (rot_z * yscale)).floor();
        }
        /*
        pSplat->verts[3].x = 100 + iCount;
        pSplat->verts[3].y = 10 + iCount;
        pSplat->verts[2].x = 160 + iCount;
        pSplat->verts[2].y = 80 + iCount;
        pSplat->verts[1].x = 100 + iCount;
        pSplat->verts[1].y = 150 + iCount;
        pSplat->verts[0].x = 8 + iCount;
        pSplat->verts[0].y = 90 + iCount;
        */
        R_RenderFloorSplat (pSplat, v2d, NULL);
skipit:
        pSplat = pSplat->nextvis;
    }
    
    CONS_Printf ("%d floor splats in view\n", iCount);
}

#endif // FLOORSPLATS




//--------------------------------------------------------------------------
// setup splat cache
//--------------------------------------------------------------------------

void R_ClearLevelSplats()
{
  freewallsplat = 0;
  memset(wallsplats, 0, sizeof(wallsplats));

#ifdef FLOORSPLATS
  freefloorsplat = 0;
  memset(floorsplats, 0, sizeof(floorsplats));

  //setup to draw floorsplats
  prastertab = rastertab;
  prepare_rastertab();
#endif
}
