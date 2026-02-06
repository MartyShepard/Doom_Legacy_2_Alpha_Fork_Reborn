// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_main.cpp 533 2009-06-11 08:31:10Z smite-meister $
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
/// \brief Rendering main loop and setup, utility functions (BSP, geometry, trigonometry).

#include "doomdef.h"

#include "command.h"
#include "cvars.h"

#include "g_game.h"
#include "g_player.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "g_map.h"

#include "am_map.h"
#include "hud.h"
#include "p_camera.h"

#include "r_render.h"
#include "r_data.h"
#include "r_draw.h"
#include "r_bsp.h"
#include "r_segs.h"
#include "r_sky.h"
#include "r_plane.h"
#include "r_things.h"
#include "i_video.h"
#include "v_video.h"

#include "w_wad.h"


/*!
  \defgroup g_sw Software renderer

  The Doom software renderer. This is the Legacy subsystem with probably
  the largest amount of original Doom code.
 */


Rend R;

angle_t G_ClipAimingPitch(angle_t pitch);


int                     viewangleoffset = 0; // obsolete, for multiscreen setup...

// increment every time a check is made
int                     validcount = 1;



int                     centerx;
int                     centery;
int                     centerypsp;     //added:06-02-98:cf R_DrawPSprite

fixed_t                 centerxfrac;
fixed_t                 centeryfrac;
fixed_t                 projection;
//added:02-02-98:fixing the aspect ration stuff...
fixed_t                 projectiony;

// just for profiling purposes
int                     framecount;
int                     linecount;
int                     loopcount;

fixed_t                 viewcos;
fixed_t                 viewsin;

//
// precalculated math tables
//
angle_t                 clipangle;

// The viewangletox[viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X.
int                     viewangletox[FINEANGLES/2];

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
angle_t                 xtoviewangle[MAXVIDWIDTH+1];


int         scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
int         scalelightfixed[MAXLIGHTSCALE];
int         zlight[LIGHTLEVELS][MAXLIGHTZ];
int         fixedcolormap;


// bumped light from gun blasts
int                     extralight;


//===========================================
//  client consvars
//===========================================

CV_PossibleValue_t viewsize_cons_t[]={{3,"MIN"},{12,"MAX"},{0,NULL}};
consvar_t cv_viewsize       = {"viewsize","10",CV_SAVE|CV_CALL,viewsize_cons_t,R_SetViewSize};      //3-12
// CV_PossibleValue_t detaillevel_cons_t[]={{0,"High"},{1,"Low"},{0,NULL}};
// consvar_t cv_detaillevel    = {"detaillevel","0",CV_SAVE|CV_CALL,detaillevel_cons_t,R_SetViewSize}; // UNUSED
consvar_t cv_scalestatusbar = {"scalestatusbar","0",CV_SAVE|CV_CALL,CV_YesNo,R_SetViewSize};

CV_PossibleValue_t fov_cons_t[]= {{1,"MIN"}, {179,"MAX"}, {0,NULL} };
consvar_t cv_fov = {"fov", "90", CV_SAVE | CV_CALL | CV_NOINIT, fov_cons_t, R_ExecuteSetViewSize};

void Translucency_OnChange();
void BloodTime_OnChange();
CV_PossibleValue_t bloodtime_cons_t[]={{1,"MIN"},{3600,"MAX"},{0,NULL}};

consvar_t cv_translucency  = {"translucency","1",CV_CALL|CV_SAVE,CV_OnOff, Translucency_OnChange};
// how much tics to last for the last (third) frame of blood (S_BLOODx)
consvar_t cv_splats    = {"splats","1",CV_SAVE,CV_OnOff};
consvar_t cv_bloodtime = {"bloodtime","20",CV_CALL|CV_SAVE,bloodtime_cons_t,BloodTime_OnChange};
consvar_t cv_psprites  = {"playersprites","1",0,CV_OnOff};

CV_PossibleValue_t viewheight_cons_t[]={{16,"MIN"},{56,"MAX"},{0,NULL}};
consvar_t cv_viewheight = {"viewheight", "41",0,viewheight_cons_t,NULL};


//===========================================


angle_t Rend::R_PointToAngle(fixed_t x, fixed_t y)
{
  return R_PointToAngle2(viewx, viewy, x, y);
}


fixed_t Rend::R_PointToDist(fixed_t x, fixed_t y)
{
  return R_PointToDist2(viewx, viewy, x, y);
}


//
// R_ScaleFromGlobalAngle
// Returns the texture mapping scale
//  for the current line (horizontal span)
//  at the given angle.
// rw_distance must be calculated first.
//
//added:02-02-98:note: THIS IS USED ONLY FOR WALLS!
fixed_t Rend::R_ScaleFromGlobalAngle(angle_t visangle)
{
  // UNUSED
#if 0
  //added:02-02-98:note: I've tried this and it displays weird...
  fixed_t             scale;
  fixed_t             dist;
  fixed_t             z;
  fixed_t             sinv;
  fixed_t             cosv;

  sinv = finesine[(visangle-rw_normalangle)>>ANGLETOFINESHIFT];
  dist = rw_distance / sinv;
  cosv = finecosine[(viewangle-visangle)>>ANGLETOFINESHIFT];
  z = abs(FixedMul(dist, cosv));
  scale = projection / z;
  return scale;

#else
  fixed_t             scale;

  int anglea = ANG90 + (visangle-viewangle);
  int angleb = ANG90 + (visangle-rw_normalangle);

    // both sines are allways positive
  fixed_t sinea = finesine[anglea>>ANGLETOFINESHIFT];
  fixed_t sineb = finesine[angleb>>ANGLETOFINESHIFT];
  //added:02-02-98:now uses projectiony instead of projection for
  //               correct aspect ratio!
  fixed_t num = projectiony * sineb;
  fixed_t den = rw_distance * sinea;

  if (den > num>>16)
    {
      scale = num/den;

      if (scale > 64)
	scale = 64;
      else if (scale.value() < 256)
	scale.setvalue(256);
    }
  else
    scale = 64;

  return scale;
#endif
}




//
// R_InitTextureMapping
//
void R_InitTextureMapping()
{
  int  i, t;

  // Use tangent table to generate viewangletox:
  //  viewangletox will give the next greatest x
  //  after the view angle.
  //
  // Calc focallength
  //  so FIELDOFVIEW angles covers SCREENWIDTH.

  int fov = cv_fov.value;
  fixed_t focallength = centerxfrac / Tan(angle_t(0.5 * fov * ANGLE_1));

  for (i=0 ; i<FINEANGLES/2 ; i++)
    {
      if (finetangent[i] > 2)
	t = -1;
      else if (finetangent[i] < -2)
	t = viewwidth+1;
      else
        {
	  fixed_t temp = 1 - fixed_epsilon;
	  temp -= finetangent[i] * focallength;
	  t = (centerxfrac + temp).floor();

	  if (t < -1)
	    t = -1;
	  else if (t>viewwidth+1)
	    t = viewwidth+1;
        }
      viewangletox[i] = t;
    }

  // Scan viewangletox[] to generate xtoviewangle[]:
  //  xtoviewangle will give the smallest view angle
  //  that maps to x.
  for (int x = 0; x <= viewwidth; x++)
    {
      i = 0;
      while (viewangletox[i]>x)
	i++;
      xtoviewangle[x] = (i<<ANGLETOFINESHIFT)-ANG90;
    }

  // Take out the fencepost cases from viewangletox.
  for (i=0 ; i<FINEANGLES/2 ; i++)
    {
      t = (centerx - (finetangent[i] * focallength)).value(); // FIXME not used!?

      if (viewangletox[i] == -1)
	viewangletox[i] = 0;
      else if (viewangletox[i] == viewwidth+1)
	viewangletox[i]  = viewwidth;
    }

  clipangle = xtoviewangle[0];
}



//
// R_InitLightTables
// Only inits the zlight table,
//  because the scalelight table changes with view size.
//
#define DISTMAP         2

void R_InitLightTables()
{
  if (devparm)
    CONS_Printf(" Creating light tables.\n");

  // Calculate the light levels to use
  //  for each level / distance combination.
  for (int i=0 ; i< LIGHTLEVELS ; i++)
    {
      int startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
      for (int j=0 ; j<MAXLIGHTZ ; j++)
        {
	  //added:02-02-98:use BASEVIDWIDTH, vid.width is not set already,
	  // and it seems it needs to be calculated only once.
	  int scale = (fixed_t(BASEVIDWIDTH/2) / fixed_t(j+1)).floor();
	  int level = startmap - scale/DISTMAP;

	  if (level < 0)
	    level = 0;

	  if (level >= NUMCOLORMAPS)
	    level = NUMCOLORMAPS-1;

	  zlight[i][j] = level*256;
        }
    }
}


//
// R_SetViewSize
// Do not really change anything here,
//  because it might be in the middle of a refresh.
// The change will take effect next refresh.
//
bool         setsizeneeded;

void R_SetViewSize()
{
  setsizeneeded = true;
}


// called from main display loop
void R_ExecuteSetViewSize()
{
  setsizeneeded = false;

  // no reduced view in splitscreen mode
  if (cv_splitscreen.value && cv_viewsize.value < 10)
    cv_viewsize.Set(10);

  if ((rendermode != render_soft) && (cv_viewsize.value < 6))
    cv_viewsize.Set(6);

  hud.ST_Recalc();

  automap.Resize();

  if (rendermode != render_soft)
    return;

  // added 16-6-98:splitscreen
  // NOTE: we only support two viewports in software
  int hhh = cv_splitscreen.value ? vid.height/2 : vid.height;

  //added 01-01-98: full screen view, without statusbar
  if (cv_viewsize.value > 10) // no statusbar
    {
      viewwidth = vid.width;
      viewheight = hhh;
    }
  else
    {
      //added 01-01-98: always a multiple of eight
      viewwidth = (cv_viewsize.value * vid.width/10) & ~7;
      //added:05-02-98: make viewheight multiple of 2 because sometimes a line is not refreshed by R_DrawViewBorder()
      viewheight = (cv_viewsize.value*(hhh - hud.stbarheight)/10) & ~1;
    }

  centery = viewheight/2;
  centerx = viewwidth/2;
  centerxfrac = centerx;
  centeryfrac = centery;

  //added:01-02-98:aspect ratio is now correct, added an 'projectiony'
  //      since the scale is not always the same between horiz. & vert.
  projection  = centerxfrac;
  projectiony = ((vid.height*centerx*BASEVIDWIDTH)/BASEVIDHEIGHT)/vid.width;

  //
  // no more low detail mode, it used to setup the right drawer routines
  // for either detail mode here
  //

  // First viewport coordinates
  viewwindowx = (vid.width-viewwidth) >> 1;

  if (cv_splitscreen.value)
    viewwindowy = 0;
  else if (cv_viewsize.value > 10) // no statusbar
    viewwindowy = 0;
  else
    viewwindowy = (vid.height -hud.stbarheight -viewheight) >> 1;

  R_FillBackScreen(); // redraw the view window border to backbuffer

  R_InitViewBuffer(viewwidth, viewheight);
  R_InitTextureMapping();

  // psprite scales
  centerypsp = viewheight/2;  //added:06-02-98:psprite pos for freelook

  pspritescale  = fixed_t(viewwidth) / BASEVIDWIDTH;
  pspriteiscale = fixed_t(BASEVIDWIDTH) / viewwidth;   // x axis scale
  //added:02-02-98:now aspect ratio correct for psprites
  pspriteyscale = fixed_t((vid.height*viewwidth)/vid.width)/BASEVIDHEIGHT;

  int i;

  // thing clipping
  for (i=0 ; i<viewwidth ; i++)
    screenheightarray[i] = viewheight;

  // planes
  //added:02-02-98:now correct aspect ratio!
  int aspectx = (((vid.height*centerx*BASEVIDWIDTH)/BASEVIDHEIGHT)/vid.width);

  // this is only used for planes rendering in software mode
  int j = viewheight*4;
  for (i=0 ; i<j ; i++)
    {
      //added:10-02-98:(i-centery) became (i-centery*2) and centery*2=viewheight
      fixed_t dy = abs(fixed_t(i - viewheight*2) + 0.5f);
      yslopetab[i] = aspectx / dy;
    }

  for (i=0 ; i<viewwidth ; i++)
    {
      fixed_t cosadj = abs(Cos(xtoviewangle[i]));
      distscale[i] = 1 / cosadj;
    }

  // Calculate the light levels to use
  //  for each level / scale combination.
  for (i=0 ; i< LIGHTLEVELS ; i++)
    {
      int startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
      for (j=0 ; j<MAXLIGHTSCALE ; j++)
        {
	  int level = startmap - j*vid.width/viewwidth/DISTMAP;

	  if (level < 0)
	    level = 0;

	  if (level >= NUMCOLORMAPS)
	    level = NUMCOLORMAPS-1;

	  scalelight[i][j] = level*256;
        }
    }
}


//
// R_Init
//

static void TestAnims()
{
  // This function can be used to test the integrity of the mobj states (sort of)

  int i, j, k, l, spr;
  mobjinfo_t *info;
  state_t *s, *n;

  state_t *mobjinfo_t::*seqptr[9] =
  {
    &mobjinfo_t::spawnstate,
    &mobjinfo_t::seestate,
    &mobjinfo_t::painstate,
    &mobjinfo_t::meleestate,
    &mobjinfo_t::missilestate,
    &mobjinfo_t::deathstate,
    &mobjinfo_t::xdeathstate,
    &mobjinfo_t::crashstate,
    &mobjinfo_t::raisestate
  };
  const char *snames[9] =
  {
    "spawn  ",
    "see    ",
    "pain   ",
    "melee  ",
    "missile",
    "death  ",
    "xdeath ",
    "crash  ",
    "raise  "
  };

  state_t *seq[9];

  for (i=0; i<NUMMOBJTYPES; i++)
    {
      info = &mobjinfo[i];

      s = info->spawnstate;
      spr = s->sprite;
      printf("\n%d: %s\n", i, spritenames[spr]);

      for (j = 0; j<9; j++)
	seq[j] = info->*seqptr[j];

      for (j = 0; j<9; j++)
	{
	  s = n = seq[j];
	  printf(" %s: ", snames[j]);
	  if (!n)
	    {
	      printf("(none)\n");
	      continue;
	    }

	  for (k = 0; k < 40; k++)
	    {
	      if (n == &states[S_NULL])
		{
		  printf("S_NULL, %d\n", k);
		  break;
		}

	      if (n->sprite != spr)
		{
		  printf("! name: %s: ", spritenames[n->sprite]);
		  spr = n->sprite;
		}

	      if (n->tics < 0)
		{
		  printf("hold, %d\n", k+1);
		  break;
		}

	      n = n->nextstate;
	      if (n == s)
		{
		  printf("loop, %d\n", k+1);
		  break;
		}
	      else for (l=0; l<9; l++)
		if (n == seq[l] && n != &states[S_NULL])
		  break;

	      if (l < 9)
		{
		  printf("6-loop to %s, %d+\n", snames[l], k+1);
		  break;
		}
	    }
	  if (k == 40)
	    printf("l >= 40 !!!\n");
	}
    }

  I_Error("\n ... done.\n");
}



#define HU_CROSSHAIRS 3
Material* crosshair[HU_CROSSHAIRS]; // crosshair graphics


/// Initializes the essential parts of the renderer
/// (even a dedicated server needs these)
void R_ServerInit()
{
  // server needs to know the texture names and dimensions
  CONS_Printf("Creating textures...\n");
  materials.Clear();
  materials.SetDefaultItem("DEF_TEX");
  materials.ReadTextures();
  //materials.Inventory();

  // set the default items for sprite and model caches
  CONS_Printf("Initializing sprites and models...\n");
  R_InitSprites();
}


/// Initializes the client renderer.
/// The server part has already been initialized in R_ServerInit.
void R_Init()
{
  CONS_Printf("Initializing the renderer...\n");
  //TestAnims();

  // Read texture animations, insert them into the cache, replacing the originals.
  if (R_Read_ANIMDEFS(fc.FindNumForName("ANIMDEFS")) < 0)
    R_Read_ANIMATED(fc.FindNumForName("ANIMATED"));

  // prepare the window border textures
  R_InitViewBorder();

  // setsizeneeded is set true, the viewport params will be recalculated before next rendering.
  R_SetViewSize();

  // load lightlevel colormaps and Boom extra colormaps
  R_InitColormaps();

  // initialize sw renderer lightlevel tables (colormaps...)
  R_InitLightTables();

  // load playercolor translation colormaps
  R_InitTranslationTables();

  // load or create translucency tables
  R_InitTranslucencyTables();

  R_InitDrawNodes();

  framecount = 0;

  // load crosshairs
  int startlump = fc.GetNumForName("CROSHAI1");
  for (int i=0; i<HU_CROSSHAIRS; i++)
    crosshair[i] = materials.GetLumpnum(startlump + i);
}


//
// R_SetupFrame
//
bool drawPsprites; // FIXME HACK


void Rend::R_SetupFrame(PlayerInfo *player)
{
  extralight = player->pawn->extralight;

  viewplayer = player->pawn; // for colormap effects due to IR visor etc.
  viewactor  = player->pov; // the point of view for this player (usually same as pawn, but may be a camera too)

  viewx = viewactor->pos.x;
  viewy = viewactor->pos.y;
  viewz = viewactor->GetViewZ(); // bobbing

  drawPsprites = (viewactor == viewplayer);

  int fixedcolormap_setup = player->pawn->fixedcolormap;
  //fixedcolormap_setup = script_camera->fixedcolormap;

  viewangle = viewactor->yaw + viewangleoffset;
  aimingangle = viewactor->pitch;

  viewsin = finesine[viewangle>>ANGLETOFINESHIFT];
  viewcos = finecosine[viewangle>>ANGLETOFINESHIFT];

  if (fixedcolormap_setup)
    {
      fixedcolormap = fixedcolormap_setup*256*sizeof(lighttable_t);

      walllights = scalelightfixed;

      for (int i=0 ; i<MAXLIGHTSCALE ; i++)
        scalelightfixed[i] = fixedcolormap;
    }
  else
    fixedcolormap = 0;

  //added:06-02-98:recalc necessary stuff for mouseaiming
  //               slopes are already calculated for the full
  //               possible view (which is 4*viewheight).

  int dy = 0;

  if ( rendermode == render_soft )
    {
      // clip it in the case we are looking a hardware 90° full aiming
      // (lmps, nework and use F12...)
      aimingangle = G_ClipAimingPitch(aimingangle);

      // WARNING : a should be unsigned but to add with 2048, it isn't !
#define AIMINGTODY(a) (finetangent[(2048+(int(a) >> ANGLETOFINESHIFT)) & FINEMASK] * 160).floor()

      if(!cv_splitscreen.value)
        dy = AIMINGTODY(aimingangle)* viewheight/BASEVIDHEIGHT ;
      else
        dy = AIMINGTODY(aimingangle)* viewheight*2/BASEVIDHEIGHT ;

      yslope = &yslopetab[(3*viewheight/2) - dy];
    }
  centery = (viewheight/2) + dy;
  centeryfrac = centery;

  framecount++;
  validcount++;
}


void R_SetViewport(int viewport)
{
  if (viewport == 0) // support just two viewports for now
    {
      viewwindowy = 0;
      ylookup = ylookup1;
      vid.scaledofs = (cv_splitscreen.value) ? -vid.width * vid.height / 2 : 0;
      // for 2d drawing in upper splitscreen viewport we pretend the actual screen is shifted up by 0.5*vid.height
    }
  else
    {
      //faB: Boris hack :P !!
      viewwindowy = vid.height/2;
      ylookup = ylookup2;
      vid.scaledofs = 0;
    }
}


// ================
// R_RenderView
// ================

//                     FAB NOTE FOR WIN32 PORT !! I'm not finished already,
// but I suspect network may have problems with the video buffer being locked
// for all duration of rendering, and being released only once at the end..
// I mean, there is a win16lock() or something that lasts all the rendering,
// so maybe we should release screen lock before each netupdate below..?

void Rend::R_RenderPlayerView(PlayerInfo *player)
{
  SetMap(player->mp);
  R_SetupFrame(player);

  // Clear buffers.
  R_ClearClipSegs();
  R_ClearDrawSegs();
  R_ClearPlanes();
  //R_ClearPortals();
  R_ClearSprites();

#ifdef FLOORSPLATS
  R_ClearVisibleFloorSplats();
#endif

  // check for new console commands.
  //NetUpdate ();

  // The head node is the last node output.

  //profile stuff ---------------------------------------------------------
#ifdef TIMING
  mytotal=0;
  ProfZeroTimer();
#endif

  R_RenderBSPNode(numnodes-1);

#ifdef TIMING
  RDMSR(0x10,&mycount);
  mytotal += mycount;   //64bit add
  CONS_Printf("RenderBSPNode: 0x%d %d\n", *((int*)&mytotal+1), (int)mytotal );
#endif
  //profile stuff ---------------------------------------------------------

  // Check for new console commands.
  //NetUpdate ();

  //R_DrawPortals();
  R_DrawPlanes();

  // Check for new console commands.
  //NetUpdate ();

#ifdef FLOORSPLATS
  //faB(21jan): testing
  R_DrawVisibleFloorSplats();
#endif

  // draw mid texture and sprite
  // SoM: And now 3D floors/sides!
  R_DrawMasked();

  // draw the psprites on top of everything
  //  but does not draw on side views
  if (!viewangleoffset && cv_psprites.value && drawPsprites)
    R_DrawPlayerSprites();

  // Check for new console commands.
  //NetUpdate ();
  player->pawn->flags &= ~MF_NOSECTOR; // don't show self (uninit) clientprediction code

  // draw the crosshair, not with chasecam
  int temp = vid.scaledofs; // ugly HACK
  vid.scaledofs = 0;
  if (true) // && !LocalPlayers[i].chasecam)
    {
      int c = 1; //LocalPlayers[i].crosshair & 3;
      crosshair[c-1]->Draw(vid.width >> 1, viewwindowy + (viewheight>>1), V_TL | V_SSIZE);
    }
  vid.scaledofs = temp;
}
