// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_things.cpp 533 2009-06-11 08:31:10Z smite-meister $
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
/// \brief Software renderer: Sprite rendering, masked texture rendering.

#include "doomdef.h"
#include "command.h"
#include "cvars.h"
#include "console.h"

#include "g_game.h"
#include "g_actor.h"
#include "g_pawn.h"

#include "r_render.h"
#include "r_main.h"
#include "r_bsp.h"
#include "r_plane.h"
#include "r_sprite.h"
#include "r_draw.h"
#include "r_data.h"
#include "v_video.h"

#include "p_pspr.h"

#include "w_wad.h"
#include "wad.h"

#include "z_zone.h"
#include "z_cache.h"

#include "i_video.h"            //rendermode


#define MINZ                  (4)
#define BASEYCENTER           (BASEVIDHEIGHT/2)
#define min(x,y) ( ((x)<(y)) ? (x) : (y) )

// put this in transmap of visprite to draw a shade
// ahaha, the pointer cannot be changed but its target can!
lighttable_t* VIS_SMOKESHADE = (lighttable_t *)-1;


struct maskdraw_t
{
  int  x1, x2;
  int  column;
  int  topclip;
  int  bottomclip;
};


// SoM: A drawnode is something that points to a 3D floor, 3D side or masked
// middle texture. This is used for sorting with sprites.
struct drawnode_t
{
  visplane_t*   plane;
  drawseg_t*    seg;
  drawseg_t*    thickseg;
  ffloor_t*     ffloor;
  vissprite_t*  sprite;

  drawnode_t *next, *prev;
};


//
// Sprite rotation 0 is facing the viewer,
//  rotation 1 is one angle turn CLOCKWISE around the axis.
// This is not the same as the angle,
//  which increases counter clockwise (protractor).
// There was a lot of stuff grabbed wrong, so I changed it...
//
fixed_t         pspritescale;
fixed_t         pspriteyscale;  //added:02-02-98:aspect ratio for psprites
fixed_t         pspriteiscale;

int *spritelights;

// constant arrays
//  used for psprite clipping and initializing clipping
short           negonearray[MAXVIDWIDTH];
short           screenheightarray[MAXVIDWIDTH];



//=============================================================
//                     VisSprites
//=============================================================


/// A vissprite_t represents a sprite object that will be drawn during a refresh.
/// I.e. a mapthing that is (at least) partly visible.
struct vissprite_t
{
  // Doubly linked list.
  vissprite_t* prev;
  vissprite_t* next;

  /// left and right screen coordinate limits for the sprite
  int x1, x2;

  /// bottom and top screen coordinate limits for the sprite.
  int yb, yt;

  /// thing x,y position in world coords (for line side calculation)
  fixed_t px, py;

  /// thing bottom and top in world coords (for sorting with 3D floors)
  fixed_t pz, pzt;

  /// sprite bottom and top edges in world coords (for silhouette clipping)
  fixed_t gz, gzt;

  /// world coords * scale = screen coords
  fixed_t xscale, yscale;

  /// screen coords * xiscale = texture coords
  fixed_t xiscale;

  /// horizontal position of x1 (in texture coords)
  fixed_t startfrac;

  /// sprite top edge z relative to viewer in world coords
  fixed_t   sprite_top;
  Material  *mat;

  /// clipping away some of the lower part of the sprite
  fixed_t floorclip;

  /// colormap index used for lightlevel changes
  int lightmap;

  /// colormap used for color translation
  byte           *translationmap;

  /// which translucency table to use?
  byte           *transmap;

  /// SoM: 3/6/2000: height sector for underwater/fake ceiling support
  int                 heightsec;

  ///SoM: 4/3/2000: Global colormaps!
  fadetable_t    *extra_colormap;


  enum spritecut_e
  {
    SC_NONE = 0,
    SC_TOP = 1,
    SC_BOTTOM = 2
  };

  /// 0 for none, bit 1 for top, bit 2 for bottom
  int cut;  

public:
  vissprite_t *SplitSprite(Actor *thing, int cut_y, lightlist_t *ll);
  void DrawVisSprite();
};


#define MAXVISSPRITES   256
static vissprite_t  vissprites[MAXVISSPRITES];
static vissprite_t *vissprite_p;
static vissprite_t  overflowsprite;

static vissprite_t* R_NewVisSprite()
{
  if (vissprite_p == &vissprites[MAXVISSPRITES])
    return &overflowsprite;

  vissprite_p++;
  return vissprite_p-1;
}


// Called at frame start.
void R_ClearSprites()
{
  vissprite_p = vissprites;
}



// sprite rendering hacks
static fixed_t proj_tz, proj_tx;


/// this does the actual work of drawing the sprite in the SW renderer
void spritepres_t::Project(Actor *p)
{
  int frame = state->frame & TFF_FRAMEMASK;

  if (frame >= spr->numframes)
    {
      frame = spr->numframes - 1; // Thing may require more frames than the defaultsprite has...
      //I_Error("spritepres_t::Project: invalid sprite frame %d (%d)\n", frame, spr->numframes);
    }
    

  spriteframe_t *sprframe = &spr->spriteframes[frame];

  Material  *mat;
  bool      flip;

  // decide which patch to use for sprite relative to player
  if (sprframe->rotate)
    {
      // choose a different rotation based on player view
      angle_t ang = R.R_PointToAngle(p->pos.x, p->pos.y); // uses viewx,viewy
      unsigned rot = (ang - p->yaw + unsigned(ANG45/2) * 9) >> 29;

      mat = sprframe->tex[rot];
      flip = sprframe->flip[rot];
    }
  else
    {
      // use single rotation for all views
      mat = sprframe->tex[0];
      flip = sprframe->flip[0];
    }

  Material::TextureRef &tr = mat->tex[0];

  // software renderer part
  // aspect ratio stuff :
  fixed_t  xscale = projection  / proj_tz;
  fixed_t  yscale = projectiony / proj_tz; //added:02-02-98:aaargll..if I were a math-guy!!!

  // calculate edges of the shape
  proj_tx -= mat->leftoffs; // left edge of sprite, world
  int x1 = (centerxfrac + (proj_tx * xscale)).floor(); // in screen coords

  // off the right side?
  if (x1 > viewwidth)
    return;

  proj_tx += mat->worldwidth;
  int x2 = (centerxfrac + (proj_tx * xscale)).floor() - 1;

  // off the left side
  if (x2 < 0)
    return;

  //SoM: 3/17/2000: Disregard sprites that are out of view..
  fixed_t gzt = p->pos.z + mat->topoffs; // top edge of sprite, world units
  int light = 0;

  sector_t *sec = p->subsector->sector;

  if (sec->numlights)
    {
      int lightnum;
      light = R_GetPlaneLight(sec, gzt, false);
      if (sec->lightlist[light].caster && sec->lightlist[light].caster->flags & FF_FOG)
        lightnum = (*sec->lightlist[light].lightlevel  >> LIGHTSEGSHIFT);
      else
        lightnum = (*sec->lightlist[light].lightlevel  >> LIGHTSEGSHIFT)+extralight;

      if (lightnum < 0)
	spritelights = scalelight[0];
      else if (lightnum >= LIGHTLEVELS)
	spritelights = scalelight[LIGHTLEVELS-1];
      else
	spritelights = scalelight[lightnum];
    }

  int heightsec = sec->heightsec;

  if (heightsec != -1)   // only clip things which are in special sectors
    {
      int phs = R.viewplayer->subsector->sector->heightsec;
      if (phs != -1 && R.viewz < R.sectors[phs].floorheight ?
          p->pos.z >= R.sectors[heightsec].floorheight :
          gzt < R.sectors[heightsec].floorheight)
        return;
      if (phs != -1 && R.viewz > R.sectors[phs].ceilingheight ?
          gzt < R.sectors[heightsec].ceilingheight &&
          R.viewz >= R.sectors[heightsec].ceilingheight :
          p->pos.z >= R.sectors[heightsec].ceilingheight)
        return;
    }

  // store information in a vissprite
  vissprite_t *vis = R_NewVisSprite();
  vis->heightsec = heightsec; //SoM: 3/17/2000

  //vis->mobjflags = p->flags;

  vis->xscale = xscale;
  vis->yscale = yscale;

  vis->px = p->pos.x;
  vis->py = p->pos.y;
  vis->pz = p->Feet();
  vis->pzt = p->Top();

  vis->gz = gzt - mat->worldheight;
  vis->gzt = gzt;
  vis->sprite_top = vis->gzt - R.viewz - p->floorclip;

  // foot clipping
  vis->floorclip = p->floorclip;

  vis->x1 = x1 < 0 ? 0 : x1;
  vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;
  vis->yt = (centeryfrac - ((vis->gzt - R.viewz) * yscale)).floor();
  vis->yb =  (centeryfrac - ((vis->gz - R.viewz) * yscale)).floor();

  vis->cut = 0;
  if (sec->numlights)
    vis->extra_colormap = sec->lightlist[light].extra_colormap;
  else
    vis->extra_colormap = sec->extra_colormap;

  if (flip)
    {
      vis->startfrac = tr.t->width - fixed_epsilon; // TODO what about x2?
      vis->xiscale = -tr.xscale / xscale;
    }
  else
    {
      vis->startfrac = 0;
      vis->xiscale = tr.xscale / xscale;
    }

  if (vis->x1 > x1)
    vis->startfrac += (vis->x1 - x1) * vis->xiscale;

  // texture to use
  vis->mat = mat;

  // determine the lightlevel & special effects

  vis->transmap = NULL;

  // specific translucency
  if (flags & TFF_SMOKESHADE)
    vis->transmap = VIS_SMOKESHADE; // not really a transmap ... see R_DrawVisSprite
  else
    {
      if (flags & TFF_TRANSMASK)
        vis->transmap = transtables[((flags & TFF_TRANSMASK) >> TFF_TRANSSHIFT) - 1];
      else if (p->flags & (MF_SHADOW | MF_ALTSHADOW)) // TODO altshadow should transpose the translucency table...
        // actually only the player should use this (temporary invisibility)
        // because now the translucency is set through TFF_TRANSMASK
        vis->transmap = transtables[tr_transhi - 1];

      if (fixedcolormap)
        {
          // fixed map : all the screen has the same colormap
          //  eg: negative effect of invulnerability
          vis->lightmap = fixedcolormap;
        }
      else if ((flags & (TFF_FULLBRIGHT | TFF_TRANSMASK) || p->flags & MF_SHADOW) &&
	       (!vis->extra_colormap || !vis->extra_colormap->fog))
        {
          // full bright : goggles
          vis->lightmap = 0;
        }
      else
        {
          // diminished light
          int index = (xscale << LIGHTSCALESHIFT).floor();

          if (index >= MAXLIGHTSCALE)
            index = MAXLIGHTSCALE-1;

          vis->lightmap = spritelights[index];
        }
    }

  // color translation
  if (color)
    vis->translationmap = translationtables[color];
  else
    vis->translationmap = NULL;

  // split the vissprite into several if necessary
  for (int i = 1; i < sec->numlights; i++)
    {
      if (sec->lightlist[i].height >= vis->gzt || !(sec->lightlist[i].caster->flags & FF_CUTSPRITES))
	continue;
      if (sec->lightlist[i].height <= vis->gz)
	return;

      int cut_y = (centeryfrac - ((sec->lightlist[i].height - R.viewz) * vis->yscale)).floor();
      if (cut_y < 0)
	continue;
      if (cut_y > vid.height)
	return;

      // Found a split! Make a new sprite, copy the old sprite to it, and
      // adjust the heights.
      vis = vis->SplitSprite(p, cut_y, &sec->lightlist[i]);
    }

}



//
// Used for sprites and masked mid textures.
// Masked means: partly transparent, i.e. stored
//  in posts/runs of opaque pixels.
//
short*          mfloorclip;
short*          mceilingclip;

fixed_t         spryscale; ///< distance scaling, does not include texture scaling
/// sprite screen coords
fixed_t         sprtopscreen;
fixed_t         sprbotscreen;
fixed_t         windowtop;
fixed_t         windowbottom;

void R_DrawMaskedColumn(column_t* column)
{
  fixed_t basetexturemid = dc_texturemid;

  for ( ; column->topdelta != 0xff ; )
    {
      // calculate unclipped screen coordinates for post
      fixed_t topscreen = sprtopscreen + column->topdelta / dc_iscale;
      fixed_t bottomscreen = sprbotscreen == fixed_t::FMAX ?
	topscreen + column->length/dc_iscale :
	//sprbotscreen + column->length/dc_iscale; // huh???
        sprbotscreen;

      dc_yl = 1 + (topscreen - fixed_epsilon).floor();
      dc_yh = (bottomscreen - fixed_epsilon).floor();

      if (windowtop != fixed_t::FMAX && windowbottom != fixed_t::FMAX)
        {
          if (windowtop > topscreen)
            dc_yl = 1 + (windowtop - fixed_epsilon).floor();
          if (windowbottom < bottomscreen)
            dc_yh = (windowbottom - fixed_epsilon).floor();
        }

      if (dc_yh >= mfloorclip[dc_x])
	dc_yh = mfloorclip[dc_x]-1;
      if (dc_yl <= mceilingclip[dc_x])
	dc_yl = mceilingclip[dc_x]+1;

      if (dc_yl <= dc_yh && dc_yl < vid.height && dc_yh > 0)
        {
	  dc_source = column->data;
	  dc_texturemid = basetexturemid - column->topdelta;

	  // Drawn by either R_DrawColumn
	  //  or (SHADOW) R_DrawFuzzColumn.
	  // FIXME a quick fix
	  if (!ylookup[dc_yl] && colfunc==R_DrawColumn_8)
	    {
	      static int first = 1;
	      if (first)
		{
		  CONS_Printf("WARNING: avoiding a crash in %s %d\n", __FILE__, __LINE__);
		  first = 0;
		}
	    }
	  else
	    colfunc();
        }

      column = (column_t *)&column->data[column->length + 1];
    }

  dc_texturemid = basetexturemid;
}



//  mfloorclip and mceilingclip should also be set.
void vissprite_t::DrawVisSprite()
{
  if (transmap == VIS_SMOKESHADE)
    // shadecolfunc uses 'colormaps'
    colfunc = shadecolfunc;
  else if (transmap)
    {
      colfunc = fuzzcolfunc;
      dc_transmap = transmap;    //Fab:29-04-98: translucency table
    }

  if (translationmap)
    {
      // translate green skin to another color
      colfunc = transcolfunc;
      /*
	// Support for translated and translucent sprites. SSNTails 11-11-2002
      if (transmap)
        colfunc = transtransfunc; // TODO
      */

      dc_translation = translationmap;
    }


  if (extra_colormap && !fixedcolormap)
    dc_colormap = extra_colormap->colormap;
  else
    dc_colormap = R.base_colormap;

  dc_colormap += lightmap;

  spryscale = yscale;
  sprtopscreen = centeryfrac - (sprite_top * yscale);
  windowtop = windowbottom = sprbotscreen = fixed_t::FMAX;

  // initialize drawers
  float temp = mat->tex[0].yscale;
  dc_iscale = temp / yscale;
  dc_texturemid = sprite_top * temp;
  dc_texheight = 0; // clever way of drawing nonrepeating textures

  if (floorclip != 0)
    sprbotscreen = sprtopscreen + (mat->worldheight*yscale) -(floorclip * spryscale);

  fixed_t frac = startfrac;
  for (dc_x = x1; dc_x <= x2; dc_x++, frac += xiscale)
    {
#ifdef RANGECHECK
      int texturecolumn = frac.floor();
      if (texturecolumn < 0 || texturecolumn >= t->width)
        I_Error ("R_DrawSpriteRange: bad texturecolumn");
#endif
      if (mat->Masked())
	R_DrawMaskedColumn(mat->GetMaskedColumn(frac));
      // TODO unmasked drawer...
    }

  colfunc = basecolfunc;
}



// splits the vissprite into two  (different light conditions on different parts of the sprite!)
vissprite_t *vissprite_t::SplitSprite(Actor *thing, int cut_y, lightlist_t *ll)
{
  vissprite_t *newsprite = R_NewVisSprite();
  memcpy(newsprite, this, sizeof(vissprite_t));

  cut |= vissprite_t::SC_BOTTOM;
  gz = ll->height;

  newsprite->gzt = gz;

  yb = cut_y;
  newsprite->yt = yb - 1;

  if (ll->height < pzt && ll->height > pz)
    pz = newsprite->pzt = ll->height;
  else
    {
      // TODO this cannot be correct?!?
      newsprite->pz = newsprite->gz;
      newsprite->pzt = newsprite->gzt;
    }

  newsprite->cut |= vissprite_t::SC_TOP;
  if (!(ll->caster->flags & FF_NOSHADE))
    {
      int lightnum;

      if (ll->caster->flags & FF_FOG)
	lightnum = (*ll->lightlevel >> LIGHTSEGSHIFT);
      else
	lightnum = (*ll->lightlevel >> LIGHTSEGSHIFT) + extralight;

      if (lightnum < 0)
	spritelights = scalelight[0];
      else if (lightnum >= LIGHTLEVELS)
	spritelights = scalelight[LIGHTLEVELS-1];
      else
	spritelights = scalelight[lightnum];

      newsprite->extra_colormap = ll->extra_colormap;

      // TODO ??? FIXME
      if (thing->pres->flags & TFF_SMOKESHADE)
	;
      else
	{
	  /*
	    if (thing->frame & TFF_TRANSMASK)
	    ;
	    else if (thing->flags & MF_SHADOW)
	    ;
	  */

	  if (fixedcolormap)
	    ;
	  else if ((thing->pres->flags & (TFF_FULLBRIGHT | TFF_TRANSMASK) || thing->flags & MF_SHADOW)
		   && (!newsprite->extra_colormap || !newsprite->extra_colormap->fog))
	    ;
	  else
	    {
	      int index = (xscale << LIGHTSCALESHIFT).floor();

	      if (index >= MAXLIGHTSCALE)
		index = MAXLIGHTSCALE-1;
	      newsprite->lightmap = spritelights[index];
	    }
	}
    }
  return newsprite;
}




//
// R_AddSprites
// During BSP traversal, this adds sprites by sector.
//
void Rend::R_AddSprites(sector_t* sec, int lightlevel)
{
  // BSP is traversed by subsector.
  // A sector might have been split into several
  //  subsectors during BSP building.
  // Thus we check whether its already added.
  if (sec->validcount == validcount)
    return;

  // Well, now it will be done.
  sec->validcount = validcount;

  if (!sec->numlights)
    {
      if (sec->heightsec == -1)
        lightlevel = sec->lightlevel;

      int lightnum = (lightlevel >> LIGHTSEGSHIFT)+extralight;

      if (lightnum < 0)
        spritelights = scalelight[0];
      else if (lightnum >= LIGHTLEVELS)
        spritelights = scalelight[LIGHTLEVELS-1];
      else
        spritelights = scalelight[lightnum];
    }

  // Handle all things in sector.
  for (Actor *thing = sec->thinglist; thing; thing = thing->snext)
    if (!(thing->flags2 & MF2_DONTDRAW) && thing->pres)
      {
        // transform the origin point
        fixed_t  tr_x = thing->pos.x - viewx;
        fixed_t  tr_y = thing->pos.y - viewy;

        proj_tz = (tr_x * viewcos) + (tr_y * viewsin);

        // thing is behind view plane?
        if (proj_tz < MINZ)
          continue;

        proj_tx = (tr_x * viewsin) - (tr_y * viewcos);

        // too far off the side?
        if (abs(proj_tx) > (proj_tz << 2))
          continue;

        thing->pres->Project(thing);
      }
}



static const fixed_t PSpriteSY[NUMWEAPONS] =
{
    fixed_t(0),             // staff
    fixed_t(5),             // goldwand
    fixed_t(15),            // crossbow
    fixed_t(15),            // blaster
    fixed_t(15),            // skullrod
    fixed_t(15),            // phoenix rod
    fixed_t(15),            // mace
    fixed_t(15),            // gauntlets
    fixed_t(15)             // beak
};
/*
const fixed_t PSpriteSY[NUMWEAPONS] =
{
     0,             // staff
     5,    // goldwand
    15,    // crossbow
    15,    // blaster
    15,    // skullrod
    15,    // phoenix rod
    15,    // mace
    15,    // gauntlets
    15     // beak
};

cc -c -g -O0 -Wl,--subsystem,console -std=gnu++11 -Wall -D__WIN32__ -DSDL -I/mingw32/include/SDL -D_GNU_SOURCE=1    -I../include r_things.cpp -o ../objs/r_things.o
r_things.cpp: In member function 'virtual void spritepres_t::Project(Actor*)':
r_things.cpp:394:50: warning: array subscript has type 'char' [-Wchar-subscripts]
     vis->translationmap = translationtables[color];
                                                  ^
r_things.cpp: At global scope:
r_things.cpp:702:1: error: converting to 'fixed_t' from initializer list would use explicit constructor 'fixed_t::fixed_t()'

Jetzt knall er hier wieder ...
*/
/*
// TODO Hexen weaponsprite y adjustments...
int PSpriteSY[NUMCLASSES][NUMWEAPONS] =
{
  { 0, -12, -10, 10 }, // Fighter
  { -8, 10, 10, 0 }, // Cleric
  { 9, 20, 20, 20 }, // Mage
  { 10, 10, 10, 10 } // Pig
};
*/
//
// R_DrawPSprite
//
void Rend::R_DrawPSprite(pspdef_t *psp)
{
  // decide which patch to use

  sprite_t *sprdef = sprites.Get(spritenames[psp->state->sprite]);
  spriteframe_t *sprframe = &sprdef->spriteframes[psp->state->frame & TFF_FRAMEMASK];

#ifdef PARANOIA
  if (!sprframe)
    I_Error("sprframes NULL for state %d\n", psp->state - weaponstates);
#endif

  Material *mat = sprframe->tex[0];
  Material::TextureRef &tr = mat->tex[0];

  // calculate edges of the shape

  //added:08-01-98:replaced mul by shift
  fixed_t tx = psp->sx - (BASEVIDWIDTH/2);

  //added:02-02-98:spriteoffset should be abs coords for psprites, based on
  //               320x200
  tx -= mat->leftoffs;
  int x1 = (centerxfrac + (tx * pspritescale)).floor();

  // off the right side
  if (x1 > viewwidth)
    return;

  tx += tr.worldwidth;
  int x2 = (centerxfrac + (tx * pspritescale)).floor() - 1;

  // off the left side
  if (x2 < 0)
    return;

  // store information in a vissprite
  vissprite_t avis;
  vissprite_t *vis = &avis;
  if (cv_splitscreen.value)
    vis->sprite_top = 120;
  else
    vis->sprite_top = BASEYCENTER;

  vis->sprite_top += - psp->sy + mat->topoffs + 0.5f;
  vis->floorclip = 0;

  /*
  if (game.mode >= gm_heretic)
    if (viewheight == vid.height || (!cv_scalestatusbar.value && vid.dupy>1))
      vis->sprite_top -= PSpriteSY[viewplayer->readyweapon];
  */

  //vis->sprite_top += 0.5f;

  vis->x1 = x1 < 0 ? 0 : x1;
  vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;
  vis->yscale = pspriteyscale;

  if (sprframe->flip[0])
    {
      vis->xiscale = -pspriteiscale * tr.xscale;
      vis->startfrac = tr.t->width - fixed_epsilon; // TODO what about x2?
    }
  else
    {
      vis->xiscale = pspriteiscale * tr.xscale;
      vis->startfrac = 0;
    }

  if (vis->x1 > x1)
    vis->startfrac += (vis->x1 - x1) * vis->xiscale;

  vis->mat = mat;
  vis->transmap = NULL;
  vis->translationmap = NULL;

  if (viewplayer->flags & MF_SHADOW)      // invisibility effect
    {
      vis->lightmap = 0;   // use translucency

      // in Doom2, it used to switch between invis/opaque the last seconds
      // now it switch between invis/less invis the last seconds
      if (viewplayer->powers[pw_invisibility] > 4*TICRATE
          || viewplayer->powers[pw_invisibility] & 8)
        vis->transmap = transtables[tr_transhi - 1];
      else
        vis->transmap = transtables[tr_transmed - 1];
    }
  else if (fixedcolormap)
    {
      // fixed color
      vis->lightmap = fixedcolormap;
    }
  else if (psp->state->frame & TFF_FULLBRIGHT)
    {
      // full bright
      vis->lightmap = 0;
    }
  else
    {
      // local light
      vis->lightmap = spritelights[MAXLIGHTSCALE-1];
    }

  if(viewplayer->subsector->sector->numlights)
    {
      int lightnum;
      int light = R_GetPlaneLight(viewplayer->subsector->sector, viewplayer->Feet() + 41, false);
      vis->extra_colormap = viewplayer->subsector->sector->lightlist[light].extra_colormap;
      lightnum = (*viewplayer->subsector->sector->lightlist[light].lightlevel  >> LIGHTSEGSHIFT)+extralight;

      if (lightnum < 0)
        spritelights = scalelight[0];
      else if (lightnum >= LIGHTLEVELS)
        spritelights = scalelight[LIGHTLEVELS-1];
      else
        spritelights = scalelight[lightnum];

      vis->lightmap = spritelights[MAXLIGHTSCALE-1];
    }
  else
    vis->extra_colormap = viewplayer->subsector->sector->extra_colormap;

  vis->DrawVisSprite();
}



/// Renders the first person (weapon) sprites
void Rend::R_DrawPlayerSprites()
{
  int         lightnum;
  int         light = 0;

  if (rendermode != render_soft)
    return;

  // get light level
  if (viewplayer->subsector->sector->numlights)
    {
      light = R_GetPlaneLight(viewplayer->subsector->sector, viewplayer->Top(), false);
      lightnum = (*viewplayer->subsector->sector->lightlist[0].lightlevel >> LIGHTSEGSHIFT) + extralight;
    }
  else
    lightnum = (viewplayer->subsector->sector->lightlevel >> LIGHTSEGSHIFT) + extralight;

  if (lightnum < 0)
    spritelights = scalelight[0];
  else if (lightnum >= LIGHTLEVELS)
    spritelights = scalelight[LIGHTLEVELS-1];
  else
    spritelights = scalelight[lightnum];

    // clip to screen bounds
  mfloorclip = screenheightarray;
  mceilingclip = negonearray;

    //added:06-02-98: quickie fix for psprite pos because of freelook
  int kikhak = centery;
  centery = centerypsp;             //for R_DrawColumn
  centeryfrac = centery;  //for R_DrawVisSprite

  // add all active psprites
  pspdef_t *psp = viewplayer->psprites;
  for (int i=0; i<NUMPSPRITES; i++, psp++)
    {
      if (psp->state)
	R_DrawPSprite(psp);
    }

  //added:06-02-98: oooo dirty boy
  centery = kikhak;
  centeryfrac = centery;
}



//
// R_SortVisSprites
//
static vissprite_t vsprsortedhead;

void R_SortVisSprites()
{
  vissprite_t *ds;
  vissprite_t *best = NULL;      //shut up compiler

  int count = vissprite_p - vissprites;
  if (!count)
    return;

  vissprite_t unsorted;
  unsorted.next = unsorted.prev = &unsorted;

  for (ds=vissprites ; ds<vissprite_p ; ds++)
    {
      ds->next = ds+1;
      ds->prev = ds-1;
    }

  vissprites[0].prev = &unsorted;
  unsorted.next = &vissprites[0];
  (vissprite_p - 1)->next = &unsorted;
  unsorted.prev = vissprite_p-1;

  // pull the vissprites out by scale
  vsprsortedhead.next = vsprsortedhead.prev = &vsprsortedhead;
  for (int i=0 ; i<count ; i++)
    {
      fixed_t bestscale = fixed_t::FMAX;
      for (ds=unsorted.next ; ds!= &unsorted ; ds=ds->next)
        {
	  if (ds->yscale < bestscale)
            {
	      bestscale = ds->yscale;
	      best = ds;
            }
        }
      best->next->prev = best->prev;
      best->prev->next = best->next;
      best->next = &vsprsortedhead;
      best->prev = vsprsortedhead.prev;
      vsprsortedhead.prev->next = best;
      vsprsortedhead.prev = best;
    }
}



//=============================================================================
//    Drawnodes
//=============================================================================

static drawnode_t *R_CreateDrawNode(drawnode_t *link);
static drawnode_t  nodebankhead;
static drawnode_t  nodehead;

/// Creates and sorts a list of drawnodes for the scene being rendered.
void Rend::R_CreateDrawNodes()
{
  drawnode_t*   entry;
  int           i, p, x1, x2;
  


  fixed_t       gzm;
  fixed_t       scale;

  // Add the 3D floors, thicksides, and masked textures...
  for (drawseg_t *ds = ds_p; ds-- > drawsegs;)
    {
      if (ds->numthicksides)
	{
	  for (i = 0; i < ds->numthicksides; i++)
	    {
	      entry = R_CreateDrawNode(&nodehead);
	      entry->thickseg = ds;
	      entry->ffloor = ds->thicksides[i];
	    }
	}
      if (ds->maskedtexturecol)
	{
	  entry = R_CreateDrawNode(&nodehead);
	  entry->seg = ds;
	}
      if (ds->numffloorplanes)
	{
	  for(i = 0; i < ds->numffloorplanes; i++)
	    {
	      int best = -1;
	      fixed_t bestdelta = 0;
	      for(p = 0; p < ds->numffloorplanes; p++)
		{
		  if (!ds->ffloorplanes[p])
		    continue;
		  visplane_t *plane = ds->ffloorplanes[p];
		  R_PlaneBounds(plane);
		  if (plane->low < con_clipviewtop || plane->high > vid.height || plane->high > plane->low)
		    {
		      ds->ffloorplanes[p] = NULL;
		      continue;
		    }

		  fixed_t delta = abs(plane->height - viewz);
		  if (delta > bestdelta)
		    {
		      best = p;
		      bestdelta = delta;
		    }
		}
	      if (best != -1)
		{
		  entry = R_CreateDrawNode(&nodehead);
		  entry->plane = ds->ffloorplanes[best];
		  entry->seg = ds;
		  ds->ffloorplanes[best] = NULL;
		}
	      else
		break;
	    }
	}
    }

  if (vissprite_p == vissprites)
    return;

  R_SortVisSprites();
  for (vissprite_t *rover = vsprsortedhead.prev; rover != &vsprsortedhead; rover = rover->prev)
    {
      if (rover->yt > vid.height || rover->yb < 0)
        continue;

      int sintersect = (rover->x1 + rover->x2) / 2;
      gzm = (rover->gz + rover->gzt) / 2;

      drawnode_t *r2;
      for (r2 = nodehead.next; r2 != &nodehead; r2 = r2->next)
	{
	  if (r2->plane)
	    {
	      if (r2->plane->minx > rover->x2 || r2->plane->maxx < rover->x1)
		continue;
	      if (rover->yt > r2->plane->low || rover->yb < r2->plane->high)
		continue;

	      if ((r2->plane->height < viewz && rover->pz < r2->plane->height) ||
		  (r2->plane->height > viewz && rover->pzt > r2->plane->height))
		{
		  // SoM: NOTE: Because a visplane's shape and scale is not directly
		  // bound to any single lindef, a simple poll of it's frontscale is
		  // not adiquate. We must check the entire frontscale array for any
		  // part that is in front of the sprite.

		  x1 = rover->x1;
		  x2 = rover->x2;
		  if (x1 < r2->plane->minx) x1 = r2->plane->minx;
		  if (x2 > r2->plane->maxx) x2 = r2->plane->maxx;

		  for(i = x1; i <= x2; i++)
		    {
		      if (r2->seg->frontscale[i] > rover->yscale)
			break;
		    }
		  if (i > x2)
		    continue;

		  entry = R_CreateDrawNode(NULL);
		  (entry->prev = r2->prev)->next = entry;
		  (entry->next = r2)->prev = entry;
		  entry->sprite = rover;
		  break;
		}
	    }
	  else if (r2->thickseg)
	    {
	      if (rover->x1 > r2->thickseg->x2 || rover->x2 < r2->thickseg->x1)
		continue;

	      scale = r2->thickseg->scale1 > r2->thickseg->scale2 ? r2->thickseg->scale1 : r2->thickseg->scale2;
	      if (scale <= rover->yscale)
		continue;
	      scale = r2->thickseg->scale1 + (r2->thickseg->scalestep * (sintersect - r2->thickseg->x1));
	      if (scale <= rover->yscale)
		continue;

	      if ((*r2->ffloor->topheight > viewz && *r2->ffloor->bottomheight < viewz) ||
		  (*r2->ffloor->topheight < viewz && rover->gzt < *r2->ffloor->topheight) ||
		  (*r2->ffloor->bottomheight > viewz && rover->gz > *r2->ffloor->bottomheight))
		{
		  entry = R_CreateDrawNode(NULL);
		  (entry->prev = r2->prev)->next = entry;
		  (entry->next = r2)->prev = entry;
		  entry->sprite = rover;
		  break;
		}
	    }
	  else if (r2->seg)
	    {
	      if (rover->x1 > r2->seg->x2 || rover->x2 < r2->seg->x1)
		continue;

	      scale = r2->seg->scale1 > r2->seg->scale2 ? r2->seg->scale1 : r2->seg->scale2;
	      if (scale <= rover->yscale)
		continue;
	      scale = r2->seg->scale1 + (r2->seg->scalestep * (sintersect - r2->seg->x1));

	      if (rover->yscale < scale)
		{
		  entry = R_CreateDrawNode(NULL);
		  (entry->prev = r2->prev)->next = entry;
		  (entry->next = r2)->prev = entry;
		  entry->sprite = rover;
		  break;
		}
	    }
	  else if (r2->sprite)
	    {
	      if (r2->sprite->x1 > rover->x2 || r2->sprite->x2 < rover->x1)
		continue;
	      if (r2->sprite->yt > rover->yb || r2->sprite->yb < rover->yt)
		continue;

	      if (r2->sprite->yscale > rover->yscale)
		{
		  entry = R_CreateDrawNode(NULL);
		  (entry->prev = r2->prev)->next = entry;
		  (entry->next = r2)->prev = entry;
		  entry->sprite = rover;
		  break;
		}
	    }
	}
      if (r2 == &nodehead)
	{
	  entry = R_CreateDrawNode(&nodehead);
	  entry->sprite = rover;
	}
    }
}




static drawnode_t *R_CreateDrawNode(drawnode_t *link)
{
  drawnode_t *node = nodebankhead.next;
  if (node == &nodebankhead)
    {
      node = (drawnode_t *)malloc(sizeof(drawnode_t));
    }
  else
    (nodebankhead.next = node->next)->prev = &nodebankhead;

  if (link)
    {
      node->next = link;
      node->prev = link->prev;
      link->prev->next = node;
      link->prev = node;
    }

  node->plane = NULL;
  node->seg = NULL;
  node->thickseg = NULL;
  node->ffloor = NULL;
  node->sprite = NULL;
  return node;
}



static void R_DoneWithNode(drawnode_t *node)
{
  (node->next->prev = node->prev)->next = node->next;
  (node->next = nodebankhead.next)->prev = node;
  (node->prev = &nodebankhead)->next = node;
}



static void R_ClearDrawNodes()
{
  for (drawnode_t *rover = nodehead.next; rover != &nodehead; )
    {
      drawnode_t *next = rover->next;
      R_DoneWithNode(rover);
      rover = next;
    }

  nodehead.next = nodehead.prev = &nodehead;
}



void R_InitDrawNodes()
{
  nodebankhead.next = nodebankhead.prev = &nodebankhead;
  nodehead.next = nodehead.prev = &nodehead;
}



// NOTE : uses con_clipviewtop, so that when console is on,
//        don't draw the part of sprites hidden under the console
void Rend::R_DrawSprite(vissprite_t *spr)
{
  short               clipbot[MAXVIDWIDTH];
  short               cliptop[MAXVIDWIDTH];
  int                 x;

  for (x = spr->x1 ; x<=spr->x2 ; x++)
    clipbot[x] = cliptop[x] = -2;

  // Scan drawsegs from end to start for obscuring segs.
  // The first drawseg that has a greater scale
  //  is the clip seg.
  //SoM: 4/8/2000:
  // Pointer check was originally nonportable
  // and buggy, by going past LEFT end of array:

  //    for (ds=ds_p-1 ; ds >= drawsegs ; ds--)    old buggy code
  for (drawseg_t *ds = ds_p; ds-- > drawsegs; )
    {

      // determine if the drawseg obscures the sprite
      if (ds->x1 > spr->x2
	  || ds->x2 < spr->x1
	  || (!ds->silhouette && !ds->maskedtexturecol))
        {
	  // does not cover sprite
	  continue;
        }

      int r1 = ds->x1 < spr->x1 ? spr->x1 : ds->x1;
      int r2 = ds->x2 > spr->x2 ? spr->x2 : ds->x2;

      fixed_t scale, lowscale;

      if (ds->scale1 > ds->scale2)
        {
	  lowscale = ds->scale2;
	  scale = ds->scale1;
        }
      else
        {
	  lowscale = ds->scale1;
	  scale = ds->scale2;
        }

      if (scale < spr->yscale
	  || (lowscale < spr->yscale
	      && !divline_t(ds->curline).PointOnSide(spr->px, spr->py)))
        {
	  // masked mid texture?
	  /*if (ds->maskedtexturecol)
	    R_RenderMaskedSegRange (ds, r1, r2);*/
	  // seg is behind sprite
	  continue;
        }

      // clip this piece of the sprite
      int silhouette = ds->silhouette;

      if (spr->gz >= ds->bsilheight)
	silhouette &= ~SIL_BOTTOM;

      if (spr->gzt <= ds->tsilheight)
	silhouette &= ~SIL_TOP;

      if (silhouette == SIL_BOTTOM)
        {
	  // bottom sil
	  for (x=r1 ; x<=r2 ; x++)
	    if (clipbot[x] == -2)
	      clipbot[x] = ds->sprbottomclip[x];
        }
      else if (silhouette == SIL_TOP)
        {
	  // top sil
	  for (x=r1 ; x<=r2 ; x++)
	    if (cliptop[x] == -2)
	      cliptop[x] = ds->sprtopclip[x];
        }
      else if (silhouette == SIL_BOTH)
        {
	  // both
	  for (x=r1 ; x<=r2 ; x++)
            {
	      if (clipbot[x] == -2)
		clipbot[x] = ds->sprbottomclip[x];
	      if (cliptop[x] == -2)
		cliptop[x] = ds->sprtopclip[x];
            }
        }
    }
  //SoM: 3/17/2000: Clip sprites in water.
  if (spr->heightsec != -1)  // only things in specially marked sectors
    {
      fixed_t mh, temp;
      int h;
      int phs = viewplayer->subsector->sector->heightsec;
      if ((mh = sectors[spr->heightsec].floorheight) > spr->gz &&
	  (temp = centeryfrac - ((mh -= viewz) * spr->yscale)) >= 0 &&
	  (h = temp.floor()) < viewheight)
        {
	  if (mh <= 0 || (phs != -1 && viewz > sectors[phs].floorheight))
            {                          // clip bottom
              for (x=spr->x1 ; x<=spr->x2 ; x++)
                if (clipbot[x] == -2 || h < clipbot[x])
                  clipbot[x] = h;
            }
	  else                        // clip top
            {
              for (x=spr->x1 ; x<=spr->x2 ; x++)
                if (cliptop[x] == -2 || h > cliptop[x])
                  cliptop[x] = h;
            }
        }

      if ((mh = sectors[spr->heightsec].ceilingheight) < spr->gzt &&
	  (temp = centeryfrac - ((mh-viewz) * spr->yscale)) >= 0 &&
	  (h = temp.floor()) < viewheight)
        {
	  if (phs != -1 && viewz >= sectors[phs].ceilingheight)
            {                         // clip bottom
              for (x=spr->x1 ; x<=spr->x2 ; x++)
                if (clipbot[x] == -2 || h < clipbot[x])
                  clipbot[x] = h;
            }
	  else                       // clip top
            {
              for (x=spr->x1 ; x<=spr->x2 ; x++)
                if (cliptop[x] == -2 || h > cliptop[x])
                  cliptop[x] = h;
            }
        }
    }
  if(spr->cut & vissprite_t::SC_TOP && spr->cut & vissprite_t::SC_BOTTOM)
    {
      int h;
      for(x = spr->x1; x <= spr->x2; x++)
	{
	  h = spr->yt;
	  if(cliptop[x] == -2 || h > cliptop[x])
	    cliptop[x] = h;

	  h = spr->yb;
	  if(clipbot[x] == -2 || h < clipbot[x])
	    clipbot[x] = h;
	}
    }
  else if(spr->cut & vissprite_t::SC_TOP)
    {
      int h;
      for(x = spr->x1; x <= spr->x2; x++)
	{
	  h = spr->yt;
	  if(cliptop[x] == -2 || h > cliptop[x])
	    cliptop[x] = h;
	}
    }
  else if(spr->cut & vissprite_t::SC_BOTTOM)
    {
      int h;
      for(x = spr->x1; x <= spr->x2; x++)
	{
	  h = spr->yb;
	  if(clipbot[x] == -2 || h < clipbot[x])
	    clipbot[x] = h;
	}
    }

  // all clipping has been performed, so draw the sprite

    // check for unclipped columns
  for (x = spr->x1 ; x<=spr->x2 ; x++)
    {
      if (clipbot[x] == -2)
	clipbot[x] = viewheight;

      if (cliptop[x] == -2)
	//Fab:26-04-98: was -1, now clips against console bottom
	cliptop[x] = con_clipviewtop;
    }

  mfloorclip = clipbot;
  mceilingclip = cliptop;
  spr->DrawVisSprite();
}


//
// was R_DrawMasked
//
void Rend::R_DrawMasked()
{
    drawnode_t*           r2;
    drawnode_t*           next;

    R_CreateDrawNodes();

    for(r2 = nodehead.next; r2 != &nodehead; r2 = r2->next)
    {
      if(r2->plane)
      {
        next = r2->prev;
        R_DrawSinglePlane(r2->plane, true);
        R_DoneWithNode(r2);
        r2 = next;
      }
      else if(r2->seg && r2->seg->maskedtexturecol != NULL)
      {
        next = r2->prev;
        R_RenderMaskedSegRange(r2->seg, r2->seg->x1, r2->seg->x2);
        r2->seg->maskedtexturecol = NULL;
        R_DoneWithNode(r2);
        r2 = next;
      }
      else if(r2->thickseg)
      {
        next = r2->prev;
        R.R_RenderThickSideRange(r2->thickseg, r2->thickseg->x1, r2->thickseg->x2, r2->ffloor);
        R_DoneWithNode(r2);
        r2 = next;
      }
      else if(r2->sprite)
      {
        next = r2->prev;
        R.R_DrawSprite(r2->sprite);
        R_DoneWithNode(r2);
        r2 = next;
      }
    }
    R_ClearDrawNodes();
}
