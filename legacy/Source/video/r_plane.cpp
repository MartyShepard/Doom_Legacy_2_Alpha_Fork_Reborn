// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_plane.cpp 452 2007-05-17 17:11:02Z smite-meister $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
/// \brief Drawing the floors and ceilings,
/// while maintaining a per column clipping list only.
/// Moreover, the sky areas have to be determined.

#include "doomdef.h"
#include "console.h"

#include "g_game.h"
#include "g_map.h"

#include "r_render.h"
#include "r_defs.h"
#include "r_data.h"
#include "r_draw.h"
#include "r_plane.h"
#include "r_splats.h"   //faB(21jan):testing
#include "r_sky.h"
#include "v_video.h"

#include "w_wad.h"
#include "z_zone.h"


planefunction_t         floorfunc;
planefunction_t         ceilingfunc;


// From here came the obnoxious "visplane bug".
//SoM: 3/23/2000: Use Boom visplane hashing.
#define           MAXVISPLANES      128

static visplane_t *visplanes[MAXVISPLANES];
static visplane_t *freetail;
static visplane_t **freehead = &freetail;


visplane_t*             floorplane;
visplane_t*             ceilingplane;

visplane_t*             currentplane;

planemgr_t              ffloor[MAXFFLOORS];
int                     numffloors;

//SoM: 3/23/2000: Boom visplane hashing routine.
inline unsigned visplane_hash(Material *pic, int lightlevel, fixed_t height)
{
  return unsigned(long(pic)*3 + lightlevel + height.value()*7) & (MAXVISPLANES-1);
}


//#define MAXOPENINGS     MAXVIDWIDTH*128

//SoM: 3/23/2000: Use boom opening limit removal
size_t maxopenings;
short *openings,*lastopening;



//
// Clip values are the solid pixel bounding the range.
//  floorclip starts out SCREENHEIGHT
//  ceilingclip starts out -1
//
short                   floorclip[MAXVIDWIDTH];
short                   ceilingclip[MAXVIDWIDTH];
fixed_t                 frontscale[MAXVIDWIDTH];

#ifdef OLDWATER
  short                   waterclip[MAXVIDWIDTH];   //added:18-02-98:WATER!
  bool                 itswater;       //added:24-02-98:WATER!
#endif

//
// spanstart holds the start of a plane span
// initialized to 0 at start
//
int                     spanstart[MAXVIDHEIGHT];
//int                     spanstop[MAXVIDHEIGHT]; //added:08-02-98: Unused!!

//
// texture mapping
//
int      *planezlight;
fixed_t                 planeheight;

//added:10-02-98: yslopetab is what yslope used to be,
//                yslope points somewhere into yslopetab,
//                now (viewheight/2) slopes are calculated above and
//                below the original viewheight for mouselook
//                (this is to calculate yslopes only when really needed)
//                (when mouselookin', yslope is moving into yslopetab)
//                Check R_SetupFrame, R_SetViewSize for more...
fixed_t                 yslopetab[MAXVIDHEIGHT*4];
fixed_t*                yslope;

fixed_t                 distscale[MAXVIDWIDTH]; ///< 1/abs(cos(xtoviewangle[i]))
fixed_t                 basexscale;
fixed_t                 baseyscale;

/// save some calculations for same-height planes
fixed_t                 cachedheight[MAXVIDHEIGHT];
fixed_t                 cacheddistance[MAXVIDHEIGHT];
fixed_t                 cachedxstep[MAXVIDHEIGHT];
fixed_t                 cachedystep[MAXVIDHEIGHT];

fixed_t   xoffs, yoffs;



//profile stuff ---------------------------------------------------------
//#define TIMING
#ifdef TIMING
#include "p5prof.h"
         long long mycount;
         long long mytotal = 0;
         unsigned long  nombre = 100000;
#endif
//profile stuff ---------------------------------------------------------


//
// R_MapPlane
//
// Uses global vars:
//  planeheight
//  ds_source
//  basexscale
//  baseyscale
//  viewx
//  viewy
//  xoffs
//  yoffs
//
// BASIC PRIMITIVE
//
#ifdef OLDWATER
static int bgofs;
static int wtofs=0;
#endif

// current span texture properties
static fixed_t ds_tex_xscale, ds_tex_yscale; 

void R_MapPlane(int y, int x1, int x2) // t1
{
#ifdef RANGECHECK
  if (x2 < x1
      || x1<0
      || x2>=viewwidth
      || (unsigned)y>viewheight)
    {
      I_Error ("R_MapPlane: %i, %i at %i",x1,x2,y);
    }
#endif

  fixed_t distance;

  if (planeheight != cachedheight[y])
    {
      cachedheight[y] = planeheight;
      distance = cacheddistance[y] = planeheight * yslope[y];
      ds_xstep = cachedxstep[y] = distance * basexscale * ds_tex_xscale;
      ds_ystep = cachedystep[y] = distance * baseyscale * ds_tex_yscale;
    }
  else
    {
      distance = cacheddistance[y];
      ds_xstep = cachedxstep[y];
      ds_ystep = cachedystep[y];
    }
  fixed_t length = distance * distscale[x1];
  angle_t angle = (currentplane->viewangle + xtoviewangle[x1])>>ANGLETOFINESHIFT;
  // SoM: Wouldn't it be faster just to add viewx and viewy to the plane's
  // x/yoffs anyway?? (Besides, it serves my purpose well for portals!)
  ds_xfrac = /*viewx +*/ ((finecosine[angle] * length) + xoffs) * ds_tex_xscale;
  ds_yfrac = /*-viewy*/ (yoffs - (finesine[angle] * length)) * ds_tex_yscale;

#ifdef OLDWATER
  if (itswater)
    {
      //ripples da water texture
      int fuck = (wtofs + (distance << 6).Floor()) & 8191;
      bgofs = (finesine[fuck] / (distance >> 9)).floor();

      angle = (angle + 2048) & 8191;  //90ø
      ds_xfrac += bgofs * finecosine[angle];
      ds_yfrac += bgofs * finesine[angle];

      if (y+bgofs>=viewheight)
	bgofs = viewheight-y-1;
      if (y+bgofs<0)
	bgofs = -y;
    }
#endif

  if (currentplane->extra_colormap && !fixedcolormap)
    ds_colormap = currentplane->extra_colormap->colormap;
  else
    ds_colormap = R.base_colormap;

  if (fixedcolormap)
    ds_colormap += fixedcolormap;
  else
    {
      int index = (distance >> LIGHTZSHIFT).floor();

      if (index >= MAXLIGHTZ)
	index = MAXLIGHTZ-1;

      ds_colormap += planezlight[index];
    }

  ds_y = y;
  ds_x1 = x1;
  ds_x2 = x2;
  // high or low detail

//added:16-01-98:profile hspans drawer.
#ifdef TIMING
  ProfZeroTimer();
#endif

  spanfunc();

#ifdef TIMING
  RDMSR(0x10,&mycount);
  mytotal += mycount;   //64bit add
  if(nombre--==0)
  I_Error("spanfunc() CPU Spy reports: 0x%d %d\n", *((int*)&mytotal+1),
                                        (int)mytotal );
#endif

}


//
// R_ClearPlanes
// At begining of frame.
//
//Fab:26-04-98:
// NOTE : uses con_clipviewtop, so that when console is on,
//        don't draw the part of the view hidden under the console
void Rend::R_ClearPlanes()
{
    int         i, p;
    angle_t     angle;

#ifdef OLDWATER
    int         waterz;
#endif

    // opening / clipping determination
    for (i=0 ; i<viewwidth ; i++)
    {
        floorclip[i] = viewheight;
        ceilingclip[i] = con_clipviewtop;       //Fab:26-04-98: was -1
        frontscale[i] = fixed_t::FMAX;
        for(p = 0; p < MAXFFLOORS; p++)
        {
          ffloor[p].f_clip[i] = viewheight;
          ffloor[p].c_clip[i] = con_clipviewtop;
        }
    }

    numffloors = 0;

#ifdef OLDWATER
    //added:18-02-98:WATER! clear the waterclip
    if (tag < 0)
        waterz = (-tag)<<FRACBITS;
    else
        waterz = fixed_t::FMIN;

    if (viewz>waterz)
    {
        for (i=0; i<viewwidth; i++)
            waterclip[i] = viewheight;
    }
    else
    {
        for (i=0; i<viewwidth; i++)
            waterclip[i] = -1;
    }
#endif

    //SoM: 3/23/2000
    for (i=0;i<MAXVISPLANES;i++)
      for (*freehead = visplanes[i], visplanes[i] = NULL; *freehead; )
        freehead = &(*freehead)->next;

    lastopening = openings;

    // texture calculation
    memset (cachedheight, 0, sizeof(cachedheight));

    // left to right mapping
    angle = (viewangle-ANG90)>>ANGLETOFINESHIFT;

    // scale will be unit scale at SCREENWIDTH/2 distance
    basexscale = finecosine[angle] / centerxfrac;
    baseyscale = -finesine[angle] / centerxfrac;
}


//SoM: 3/23/2000: New function, by Lee Killough
static visplane_t *new_visplane(unsigned hash)
{
  visplane_t *check = freetail;
  if (!check)
    check = (visplane_t *)calloc(1, sizeof *check);
  else
    if (!(freetail = freetail->next))
      freehead = &freetail;
  check->next = visplanes[hash];
  visplanes[hash] = check;
  return check;
}



//
// R_FindPlane : cherche un visplane ayant les valeurs identiques:
//               meme hauteur, meme flattexture, meme lightlevel.
//               Sinon en alloue un autre.
//               Merde. Documentation en francais.
// TODO  R_FindPlane could use some cleanup/redesign...
visplane_t* Rend::R_FindPlane(fixed_t height, Material *pic, int lightlevel, fixed_t xoff, fixed_t yoff,
			      fadetable_t* planecolormap, ffloor_t* ffloor, bool sky)
{
  visplane_t* check;
  unsigned    hash; //SoM: 3/23/2000

  xoff += viewx; // SoM
  yoff = -viewy + yoff;

  if (sky)
    {
      height = 0;                     // all skys map together
      lightlevel = 0;
    }

  //SoM: 3/23/2000: New visplane algorithm uses hash table -- killough
  hash = visplane_hash(pic,lightlevel,height);

  for (check = visplanes[hash]; check; check = check->next)
    if (height == check->height &&
	pic == check->pic &&
	lightlevel == check->lightlevel &&
	xoff == check->xoffs &&
	yoff == check->yoffs &&
	planecolormap == check->extra_colormap &&
	!ffloor && !check->ffloor &&
	check->viewz == viewz &&
	check->viewangle == viewangle)
      return check;

  check = new_visplane(hash);

  check->sky = sky;
  check->height = height;
  check->pic = pic;
  check->lightlevel = lightlevel;
  check->minx = vid.width;
  check->maxx = -1;
  check->xoffs = xoff;
  check->yoffs = yoff;
  check->extra_colormap = planecolormap;
  check->ffloor = ffloor;
  check->viewz = viewz;
  check->viewangle = viewangle;

  memset (check->top, 0xff, sizeof(check->top));

  return check;
}




//
// R_CheckPlane : return same visplane or alloc a new one if needed
//
visplane_t*  R_CheckPlane( visplane_t*   pl,
                           int           start,
                           int           stop )
{
    int         intrl;
    int         intrh;
    int         unionl;
    int         unionh;
    int         x;

    if (start < pl->minx)
    {
        intrl = pl->minx;
        unionl = start;
    }
    else
    {
        unionl = pl->minx;
        intrl = start;
    }

    if (stop > pl->maxx)
    {
        intrh = pl->maxx;
        unionh = stop;
    }
    else
    {
        unionh = pl->maxx;
        intrh = stop;
    }

    //added 30-12-97 : 0xff ne vaut plus -1 avec un short...
    for (x=intrl ; x<= intrh ; x++)
        if (pl->top[x] != 0xffff)
            break;

    //SoM: 3/23/2000: Boom code
    if (x > intrh)
      pl->minx = unionl, pl->maxx = unionh;
    else
      {
        unsigned hash = visplane_hash(pl->pic, pl->lightlevel, pl->height);
        visplane_t *new_pl = new_visplane(hash);
  
	new_pl->sky = pl->sky;
        new_pl->height = pl->height;
        new_pl->pic = pl->pic;
        new_pl->lightlevel = pl->lightlevel;
        new_pl->xoffs = pl->xoffs;           // killough 2/28/98
        new_pl->yoffs = pl->yoffs;
        new_pl->extra_colormap = pl->extra_colormap;
        new_pl->ffloor = pl->ffloor;
        new_pl->viewz = pl->viewz;
        new_pl->viewangle = pl->viewangle;
        pl = new_pl;
        pl->minx = start;
        pl->maxx = stop;
        memset(pl->top, 0xff, sizeof pl->top);
      }
    return pl;
}


//
// R_ExpandPlane
//
// SoM: This function basically expands the visplane or I_Errors
// The reason for this is that when creating 3D floor planes, there is no
// need to create new ones with R_CheckPlane, because 3D floor planes
// are created by subsector and there is no way a subsector can graphically
// overlap.
void R_ExpandPlane(visplane_t*  pl, int start, int stop)
{
    int         intrl;
    int         intrh;
    int         unionl;
    int         unionh;
	int			x;

    if (start < pl->minx)
    {
        intrl = pl->minx;
        unionl = start;
    }
    else
    {
        unionl = pl->minx;
        intrl = start;
    }

    if (stop > pl->maxx)
    {
        intrh = pl->maxx;
        unionh = stop;
    }
    else
    {
        unionh = pl->maxx;
        intrh = stop;
    }

    for (x = start ; x <= stop ; x++)
        if (pl->top[x] != 0xffff)
            break;

    //SoM: 3/23/2000: Boom code
    if (x > stop)
      pl->minx = unionl, pl->maxx = unionh;
//    else
//      I_Error("R_ExpandPlane: planes in same subsector overlap?!\nminx: %i, maxx: %i, start: %i, stop: %i\n", pl->minx, pl->maxx, start, stop);

    pl->minx = unionl, pl->maxx = unionh;
}

//
// R_MakeSpans
//
void R_MakeSpans
( int           x,
  int           t1,
  int           b1,
  int           t2,
  int           b2 )
{
    while (t1 < t2 && t1<=b1)
    {
        R_MapPlane (t1,spanstart[t1],x-1);
        t1++;
    }
    while (b1 > b2 && b1>=t1)
    {
        R_MapPlane (b1,spanstart[b1],x-1);
        b1--;
    }

    while (t2 < t1 && t2<=b2)
    {
        spanstart[t2] = x;
        t2++;
    }
    while (b2 > b1 && b2>=t2)
    {
        spanstart[b2] = x;
        b2--;
    }
}


#ifdef OLDWATER
static int waterofs;

// la texture flat anim‚e de l'eau contient en fait des index
// de colormaps , plutot qu'une transparence, il s'agit d'ombrer et
// d'eclaircir pour donner l'effet de bosses de l'eau.
#ifdef couille
void R_DrawWaterSpan (void)
{
    fixed_t             xfrac;
    fixed_t             yfrac;
    byte*               dest;
    int                 count;
    int                 spot;

    //byte*               brighten = transtables+(84<<8);
    byte*               brighten = colormaps;

//#ifdef RANGECHECK
    if (ds_x2 < ds_x1
        || ds_x1<0
        || ds_x2>=vid.width
        || (unsigned)ds_y>=vid.height)
    {
        I_Error( "R_DrawWaterSpan: %i to %i at %i",
                 ds_x1,ds_x2,ds_y);
    }
//      dscount++;
//#endif


    xfrac = ds_xfrac;
    yfrac = (ds_yfrac + waterofs) & 0x3fffff;

    dest = ylookup[ds_y] + columnofs[ds_x1];

    // We do not check for zero spans here?
    count = ds_x2 - ds_x1;

// *dest++ = 192;
// --count;
    do //while(count--)
    {
        // Current texture index in u,v.
        spot = ((yfrac>>(16-6))&(63*64)) + ((xfrac>>16)&63);

        // Lookup pixel from flat texture tile,
        //  re-index using light/colormap.
        *dest++ = *( brighten + (ds_source[spot]<<8) + (*dest) );
        // Next step in u,v.
        xfrac += ds_xstep;
        yfrac += ds_ystep;

    } while(count--); //
// if (count==-1)
//     *dest = 200;
}
#endif //couille

void R_DrawWaterSpan_8 (void)
{
    fixed_t             xfrac;
    fixed_t             yfrac;
    byte*               dest;
    byte*               dsrc;
    int                 count;
    int                 spot;

    //byte*               brighten = transtables+(84<<8);
    byte*               brighten = colormaps-(8*256);

//#ifdef RANGECHECK
    if (ds_x2 < ds_x1
        || ds_x1<0
        || ds_x2>=vid.width
        || ds_y>=vid.height)
    {
        I_Error( "R_DrawWaterSpan: %i to %i at %i",
                 ds_x1,ds_x2,ds_y);
    }
//      dscount++;
//#endif

    xfrac = ds_xfrac;
    yfrac = (ds_yfrac + waterofs) & 0x3fffff;

    // methode a : le fond est d‚form‚
    dest = ylookup[ds_y] + columnofs[ds_x1];
    dsrc = screens[2] + ((ds_y+bgofs)*vid.width) + columnofs[ds_x1];

    // m‚thode b : la surface est d‚form‚e !
    //dest = ylookup[ds_y+bgofs] + columnofs[ds_x1];
    //dsrc = screens[2] + (ds_y*vid.width) + columnofs[ds_x1];

    // We do not check for zero spans here?
    count = ds_x2 - ds_x1;

// *dest++ = 192;
// --count;

    do //while(count--)
    {
        // Current texture index in u,v.
        spot = ((yfrac>>(16-6))&(63*64)) + ((xfrac>>16)&63);

        // Lookup pixel from flat texture tile,
        //  re-index using light/colormap.
        *dest++ = *( brighten + (ds_source[spot]<<8) + (*dsrc++) );
        // Next step in u,v.
        xfrac += ds_xstep;
        yfrac += ds_ystep;

    } while(count--); //
// if (count==-1)
//     *dest = 200;
}




static int wateranim;
#endif //Oldwater


void Rend::R_DrawPlanes()
{
  visplane_t*         pl;
  int  x, i;
  Material *skytex = m->skytexture;

  //
  // DRAW NON-WATER VISPLANES FIRST
  //
#ifdef OLDWATER
  bool watertodraw = false;
  itswater = false;
#endif

  spanfunc = basespanfunc;

  for (i=0;i<MAXVISPLANES;i++, pl++)
    for (pl=visplanes[i]; pl; pl=pl->next)
      {
#ifdef OLDWATER
        if (pl->picnum==1998)   //dont draw water visplanes now.
	  {
            watertodraw = true;
            continue;
	  }
#endif

        // sky flat
        if (pl->sky)
	  {
	    extern fixed_t pspriteyscale;
            //added:12-02-98: use correct aspect ratio scale
            dc_iscale = (1 / pspriteyscale) * skytex->tex[0].yscale;

// Kik test non-moving sky .. weird
// cy = centery;
// centery = (viewheight/2);

            // Sky is allways drawn full bright,
            //  i.e. colormaps[0] is used.
            // Because of this hack, sky is not affected
            //  by INVUL inverse mapping.
#if 0
            // BP: this fix sky not inversed in invuln but it is a original doom2 feature (bug?)
            if(fixedcolormap)
	      dc_colormap = fixedcolormap + base_colormap;
            else
#endif
	      dc_colormap = base_colormap;
            dc_texturemid = skytexturemid;
            dc_texheight = skytex->tex[0].t->height;
            for (x=pl->minx ; x <= pl->maxx ; x++)
	      {
                dc_yl = pl->top[x];
                dc_yh = pl->bottom[x];

                if (dc_yl <= dc_yh)
		  {
		    fixed_t skycol;
		    skycol.setvalue((viewangle + xtoviewangle[x]) >> (ANGLETOSKYSHIFT - fixed_t::FBITS));
                    dc_x = x;
                    dc_source = skytex->GetColumn(skycol);
                    skycolfunc();
		  }
	      }
// centery = cy;
            continue;
	  }

        if(pl->ffloor)
          continue;

        R_DrawSinglePlane(pl, true);
      }

  //
  // DRAW WATER VISPLANES AFTER
  //

#ifdef OLDWATER
    R_DrawSprites ();   //draw sprites before water. just a damn hack


    //added:24-02-98: SALE GROS HACK POURRI
    if (!watertodraw)
      goto skipwaterdraw;

    VID_BlitLinearScreen ( screens[0], screens[2],
                           vid.width, vid.height,
                           vid.width, vid.width );

    spanfunc = R_DrawWaterSpan_8;
    itswater = true;
    // always the same flat!!!
    ds_source = W_CacheLumpNum(firstwaterflat + ((wateranim>>3)&7), PU_STATIC);

    for (i=0;i<MAXVISPLANES;i++, pl++)
    for (pl=visplanes[i]; pl; pl=pl->next)
    {
        if (pl->picnum!=1998)
            continue;

        R_DrawSinglePlane(pl, false);
    }
    Z_ChangeTag (ds_source, PU_CACHE);
    itswater = false;
    spanfunc = basespanfunc;

skipwaterdraw:

    waterofs += (1<<14);
    wateranim++;
    wtofs += 75;
    //if (!wateranim)
    //    waterofs -= (32<<16);
#endif //OLDWATER
}




void Rend::R_DrawSinglePlane(visplane_t* pl, bool handlesource)
{
  int                 light = 0;
  int                 x;
  int                 stop;
  int                 angle;

  if (pl->minx > pl->maxx)
    return;

  spanfunc = basespanfunc;
  if(pl->ffloor)
  {
    if(pl->ffloor->flags & FF_TRANSLUCENT)
    {
      spanfunc = R_DrawTranslucentSpan_8;
      ds_transmap = transtables[1];
      if(pl->extra_colormap && pl->extra_colormap->fog)
        light = (pl->lightlevel >> LIGHTSEGSHIFT);
      else
        light = LIGHTLEVELS-1;
    }
    else if(pl->ffloor->flags & FF_FOG)
    {
      spanfunc = R_DrawFogSpan_8;
      light = (pl->lightlevel >> LIGHTSEGSHIFT);
    }
    else if(pl->extra_colormap && pl->extra_colormap->fog)
      light = (pl->lightlevel >> LIGHTSEGSHIFT);
    else
      light = (pl->lightlevel >> LIGHTSEGSHIFT)+extralight;
  }
  else
  {
    if(pl->extra_colormap && pl->extra_colormap->fog)
      light = (pl->lightlevel >> LIGHTSEGSHIFT);
    else
      light = (pl->lightlevel >> LIGHTSEGSHIFT)+extralight;
  }

  if(viewangle != pl->viewangle)
  {
    memset (cachedheight, 0, sizeof(cachedheight));

    angle = (pl->viewangle-ANG90)>>ANGLETOFINESHIFT;

    basexscale = finecosine[angle] / centerxfrac;
    baseyscale = -finesine[angle] / centerxfrac;
    viewangle = pl->viewangle;
  }

  currentplane = pl;

  if (handlesource)
    {
      Material *m = pl->pic;
      if (!m)
	return; // HACK, should be done more intelligently (earlier!)

      ds_tex_xscale = m->tex[0].xscale;
      ds_tex_yscale = m->tex[0].yscale;

      Texture *t = m->tex[0].t;
      ds_source = t->GetData();
      ds_xbits  = t->w_bits;
      ds_ybits  = t->h_bits;
    }

  xoffs = pl->xoffs;
  yoffs = pl->yoffs;
  planeheight = abs(pl->height - pl->viewz);

  if (light >= LIGHTLEVELS)
      light = LIGHTLEVELS-1;

  if (light < 0)
      light = 0;

  planezlight = zlight[light];

  //set the MAXIMUM value for unsigned
  pl->top[pl->maxx+1] = 0xffff;
  pl->top[pl->minx-1] = 0xffff;

  stop = pl->maxx + 1;

  for (x=pl->minx ; x<= stop ; x++)
  {
    R_MakeSpans(x,pl->top[x-1],
                pl->bottom[x-1],
                pl->top[x],
                pl->bottom[x]);
  }

  /*
  if(handlesource)
    Z_ChangeTag (ds_source, PU_CACHE);
  */
}


void R_PlaneBounds(visplane_t* plane)
{
  int  i;
  int  hi, low;

  hi = plane->top[plane->minx];
  low = plane->bottom[plane->minx];

  for(i = plane->minx + 1; i <= plane->maxx; i++)
  {
    if(plane->top[i] < hi)
      hi = plane->top[i];
    if(plane->bottom[i] > low)
      low = plane->bottom[i];
  }
  plane->high = hi;
  plane->low = low;
}
