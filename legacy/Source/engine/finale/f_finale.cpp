// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: f_finale.cpp 533 2009-06-11 08:31:10Z smite-meister $
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
/// \brief Finale animations.

#include <string>
#include <ctype.h>

#include <SDL.h>

#include "dstrings.h"
#include "d_event.h"
#include "g_game.h"
#include "g_level.h"
#include "info.h"

#include "screen.h"
#include "r_data.h"
#include "r_sprite.h"
#include "v_video.h"

#include "hud.h"

#include "sounds.h"
#include "s_sound.h"

#include "w_wad.h"
#include "z_zone.h"


void  F_TextInit(int dummy);
void  F_TextTicker();
void  F_TextWrite(int x, int y);

void  F_DoomStart(int dummy);
void  F_DoomDrawer(int dummy);

void  F_StartCast(int dummy);
void  F_CastTicker();
bool  F_CastResponder(event_t *ev);
void  F_CastDrawer(int dummy);

void  F_HereticDrawer(int dummy);

void  F_HexenStart(int stage);
void  F_HexenDrawer(int stage);

struct stage_t
{
  int wait, stage;  // Kludge: for F_TextWrite, these are used as x, y
  void (*init)(int);
  bool (*responder)(event_t *);
  void (*ticker)();
  void (*drawer)(int);
};

stage_t Doom[] =
{
  {10, 10, F_TextInit, NULL, F_TextTicker, NULL}, // text
  {1000, 1, F_DoomStart, NULL, NULL, F_DoomDrawer}, // picture
  {-1}
};

stage_t Doom2[] =
{
  {10, 10, F_TextInit, NULL, F_TextTicker, NULL}, // text
  {1000, 1, F_StartCast, F_CastResponder, F_CastTicker, F_CastDrawer}, // cast
  {-1}
};


stage_t Heretic[] =
{
  {20, 5, F_TextInit, NULL, F_TextTicker, NULL}, // text
  {1000, 1, NULL, NULL, NULL, F_HereticDrawer}, // picture
  {-1}
};

stage_t Hexen[] =
{
  {70, 0, F_HexenStart, NULL, NULL, F_HexenDrawer},
  {20, 1, F_HexenStart, NULL, F_TextTicker, NULL}, // text
  {20, 2, F_HexenStart, NULL, F_TextTicker, NULL}, // text
  {70, 3, F_HexenStart, NULL, NULL, F_HexenDrawer},
  {71, 4, F_HexenStart, NULL, NULL, F_HexenDrawer},
  {20, 5, F_HexenStart, NULL, F_TextTicker, NULL}, // text
  {-1}
};


#define TEXTSPEED       3
#define TEXTWAIT        250


static stage_t *finalestage;
static int      finalecount;
static int      finalewait;
static bool endgame;
static int  gameepisode;
static std::string finaletext;
static Material *finalepic = NULL;
static Material *finaleflat = NULL;

static bool keypressed = false;


void F_StartFinale(const MapCluster *cd, bool enter, bool end)
{
  endgame = end;
  gameepisode = cd->episode;
  finaleflat = materials.Get(cd->finalepic.c_str(), TEX_floor);

  if (enter)
    finaletext = cd->entertext;
  else
    finaletext = cd->exittext;

  S.StartMusic(cd->finalemusic.c_str(), true);

  switch (game.mode)
    {
    case gm_hexen:
      {
        finalestage = Hexen;       
      }
      break;
   
    case gm_heretic:
      {
        finalestage = Heretic;    
      }
      break;

    case gm_doom2:
      {                     
        finalestage = Doom2;   
      }      
      break;

    default:
      {     
        finalestage = Doom;       
      }      
      break;
    }

  finalecount = 0;
  finalewait = finalestage->wait;

  if (finalestage->init)
     (finalestage->init)(finalestage->stage);

}


bool F_Responder(event_t *event)
{
  // special responder?
  if (finalestage->responder)
    return (finalestage->responder)(event);

  if (event->type != ev_keydown)
    return false;

  if (keypressed)
    return false;

  keypressed = true;
  return true;
}


void F_Ticker()
{
  // special ticker?
  if (finalestage->ticker)
    (finalestage->ticker)();
  else
    finalecount++; // just advance animation

  // check for next stage
  if (finalecount > finalewait)
    {
      if (endgame)
        {
          // skip to next stage
          finalestage++;
          finalecount = 0;
          finalewait = finalestage->wait;
          if (finalewait == -1)
            game.EndFinale(); // FIXME rather reset the game, give main screen
          else if (finalestage->init)
            (finalestage->init)(finalestage->stage);

          game.screenwipe = 1;
        }
      else
        { // return to game
          game.EndFinale();
          finalecount = MININT;    // wait until map is lunched
        }
    }
}



void F_TextTicker()
{
  finalecount++;

  if (keypressed)
    {
      keypressed = false;
      if (finalecount < finalewait - TEXTWAIT)
        // force text to be written
        finalecount = finalewait - TEXTWAIT;
      else
        // skip waiting time
        finalecount += TEXTWAIT;
    }
}


/* Marty: Fixed Routine am Ende des Codes
void F_Drawer()
{
  if (finalestage->drawer)
    {
      (finalestage->drawer)(finalestage->stage);
      return;
    }

  // else assume text
  F_TextWrite(finalestage->wait, finalestage->stage);
}
*/

void F_TextInit(int dummy)
{
  finalewait = finaletext.length()*TEXTSPEED + TEXTWAIT;
}

/* Marty: Fixed Routine am Ende des Codes
void F_TextWrite(int sx, int sy)
{
  // erase the entire screen to a tiled background (or raw picture)
  if (finalepic)
    finalepic->Draw(0, 0, V_SCALE);
  else
    finaleflat->DrawFill(0,0,vid.width,vid.height);

  // draw some of the text onto the screen
  int count = (finalecount - 10)/TEXTSPEED;
  if (count < 0)
    return;

  if (count > int(finaletext.length()))
    count = finaletext.length();

  // small hack
  char c = finaletext[count];
  finaletext[count] = '\0';
  hud_font->DrawString(sx, sy, finaletext.c_str(), V_SCALE);
  finaletext[count] = c;
}
*/

//
// Final DOOM 2 animation
// Casting by id Software.
//   in order of appearance
//
struct castinfo_t
{
  const char *name;
  mobjtype_t  type;
};

castinfo_t castorder[] =
{
  {NULL, MT_POSSESSED},
  {NULL, MT_SHOTGUY},
  {NULL, MT_CHAINGUY},
  {NULL, MT_TROOP},
  {NULL, MT_SERGEANT},
  {NULL, MT_SKULL},
  {NULL, MT_HEAD},
  {NULL, MT_KNIGHT},
  {NULL, MT_BRUISER},
  {NULL, MT_BABY},
  {NULL, MT_PAIN},
  {NULL, MT_UNDEAD},
  {NULL, MT_FATSO},
  {NULL, MT_VILE},
  {NULL, MT_SPIDER},
  {NULL, MT_CYBORG},
  {NULL, MT_PLAYER},
  {NULL, (mobjtype_t)0}
};

static int          castnum;
static int          casttics;
static state_t*     caststate;
static bool         castdeath;
static int          castframes;
static int          castonmelee;
static bool         castattacking;


void F_StartCast(int dummy)
{
  int i;

  for(i=0;i<17;i++)
    castorder[i].name = text[TXT_CC_ZOMBIE + i];

  game.screenwipe = 1;
  castnum = 0;
  caststate = mobjinfo[castorder[castnum].type].seestate;
  casttics = caststate->tics;
  castdeath = false;
  castframes = 0;
  castonmelee = 0;
  castattacking = false;
  S.StartMusic("D_EVIL", true);
}



void F_CastTicker()
{
  int         st;
  int         sfx;

  if (--casttics > 0)
    return;                 // not time to change state yet

  if (caststate->tics == -1 || caststate->nextstate == &states[S_NULL])
    {
      // switch from deathstate to next monster
      castnum++;
      castdeath = false;
      if (castorder[castnum].name == NULL)
        castnum = 0;
      if (mobjinfo[castorder[castnum].type].seesound)
        S_StartLocalAmbSound(mobjinfo[castorder[castnum].type].seesound);
      caststate = mobjinfo[castorder[castnum].type].seestate;
      castframes = 0;
    }
  else
    {
      // just advance to next state in animation
      if (caststate == &states[S_PLAY_ATK1])
        goto stopattack;    // Oh, gross hack!
      caststate = caststate->nextstate;
      st = caststate - states;

      castframes++;

      // sound hacks....
      switch (st)
        {
        case S_PLAY_ATK1:     sfx = sfx_dshtgn; break;
        case S_POSS_ATK2:     sfx = sfx_pistol; break;
        case S_SPOS_ATK2:     sfx = sfx_shotgn; break;
        case S_VILE_ATK2:     sfx = sfx_vilatk; break;
        case S_SKEL_FIST2:    sfx = sfx_skeswg; break;
        case S_SKEL_FIST4:    sfx = sfx_skepch; break;
        case S_SKEL_MISS2:    sfx = sfx_skeatk; break;
        case S_FATT_ATK8:
        case S_FATT_ATK5:
        case S_FATT_ATK2:     sfx = sfx_firsht; break;
        case S_CPOS_ATK2:
        case S_CPOS_ATK3:
        case S_CPOS_ATK4:     sfx = sfx_shotgn; break;
        case S_TROO_ATK3:     sfx = sfx_claw; break;
        case S_SARG_ATK2:     sfx = sfx_sgtatk; break;
        case S_BOSS_ATK2:
        case S_BOS2_ATK2:
        case S_HEAD_ATK2:     sfx = sfx_firsht; break;
        case S_SKULL_ATK2:    sfx = sfx_sklatk; break;
        case S_SPID_ATK2:
        case S_SPID_ATK3:     sfx = sfx_shotgn; break;
        case S_BSPI_ATK2:     sfx = sfx_plasma; break;
        case S_CYBER_ATK2:
        case S_CYBER_ATK4:
        case S_CYBER_ATK6:    sfx = sfx_rlaunc; break;
        case S_PAIN_ATK3:     sfx = sfx_sklatk; break;
        default: sfx = 0; break;
        }

      if (sfx)
        S_StartLocalAmbSound(sfx);
    }

  if (castframes == 12)
    {
      // go into attack frame
      castattacking = true;
      if (castonmelee)
        caststate = mobjinfo[castorder[castnum].type].meleestate;
      else
        caststate = mobjinfo[castorder[castnum].type].missilestate;
      castonmelee ^= 1;
      if (caststate == &states[S_NULL])
        {
          if (castonmelee)
            caststate = mobjinfo[castorder[castnum].type].meleestate;
          else
            caststate = mobjinfo[castorder[castnum].type].missilestate;
        }
    }

  if (castattacking)
    {
      if (castframes == 24
          ||  caststate == mobjinfo[castorder[castnum].type].seestate )
        {
        stopattack:
          castattacking = false;
          castframes = 0;
          caststate = mobjinfo[castorder[castnum].type].seestate;
        }
    }

  casttics = caststate->tics;
  if (casttics == -1)
    casttics = 15;
}



bool F_CastResponder(event_t* ev)
{
  if (ev->type != ev_keydown)
    return false;

  if (castdeath)
    return true;                    // already in dying frames

  // go into death frame
  castdeath = true;
  caststate = mobjinfo[castorder[castnum].type].deathstate;
  casttics = caststate->tics;
  castframes = 0;
  castattacking = false;
  if (mobjinfo[castorder[castnum].type].deathsound)
    S_StartLocalAmbSound(mobjinfo[castorder[castnum].type].deathsound);

  return true;
}


void F_CastPrint(const char* text)
{
  hud_font->DrawString((BASEVIDWIDTH - hud_font->StringWidth(text)) / 2, 180, text, V_SCALE);
}


void D_PageDrawer(const char *lumpname);

void F_CastDrawer(int dummy)
{
  // erase the entire screen to a background
  //V_DrawPatch (0,0,0, materials.Get ("BOSSBACK", PU_CACHE));
  D_PageDrawer("BOSSBACK");

  F_CastPrint(castorder[castnum].name);

  // draw the current frame in the middle of the screen
  sprite_t *sprdef = sprites.Get(spritenames[caststate->sprite]);
  spriteframe_t *sprframe = &sprdef->spriteframes[ caststate->frame & TFF_FRAMEMASK];
  Material *t = sprframe->tex[0];
  bool flip = sprframe->flip[0];

  t->Draw(BASEVIDWIDTH>>1,170,(flip ? V_FLIPX : 0) | V_SCALE);
}



void F_BunnyScroll()
{
  char        name[10];
  int         stage;
  static int  laststage;

  Material *p1 = materials.Get("PFUB2");
  Material *p2 = materials.Get("PFUB1");

  //V_MarkRect (0, 0, vid.width, vid.height);

  int scrolled = 320 - (finalecount-230)/2;
  if (scrolled > 320)
    scrolled = 320;
  if (scrolled < 0)
    scrolled = 0;

  /*
      for ( x=0 ; x<320 ; x++)
        {
          if (x+scrolled < 320)
            F_DrawPatchCol (x, p1, x+scrolled);
          else
            F_DrawPatchCol (x, p2, x+scrolled - 320);
        }
  */

  // FIXME WRONG, draws outside the frame buffer...
  p1->Draw(-scrolled, 0, V_SCALE);
  p2->Draw(320-scrolled, 0, V_SCALE);


  if (finalecount < 1130)
    return;
  if (finalecount < 1180)
    {
      materials.Get("END0")->Draw((320-13*8)/2, (200-8*8)/2, V_SCALE);
      laststage = 0;
      return;
    }

  stage = (finalecount-1180) / 5;
  if (stage > 6)
    stage = 6;
  if (stage > laststage)
    {
      S_StartLocalAmbSound(sfx_pistol);
      laststage = stage;
    }

  sprintf (name,"END%i",stage);
  materials.Get(name)->Draw((320-13*8)/2, (200-8*8)/2, V_SCALE);
}



void F_DoomStart(int dummy)
{
  if (gameepisode == 3)
    S.StartMusic("D_BUNNY");
}



void F_DoomDrawer(int dummy)
{
  switch (gameepisode)
    {
    case 1:
      //materials.Get("HELP2")->Draw(0,0,V_SCALE); // ordering info for shareware
      materials.Get("CREDIT")->Draw(0,0, V_SCALE); // id credits
      break;
    case 2:
      materials.Get("VICTORY2")->Draw(0,0, V_SCALE); // deimos over hell
      break;
    case 3:
      F_BunnyScroll();
      break;
    case 4:
      materials.Get("ENDPIC")->Draw(0,0,V_SCALE);
      break;
    }
}



void F_DemonScroll()
{
  int scrolled = (finalecount-70)/3;
  if (scrolled > 200)
    scrolled = 200;
  if (scrolled < 0)
    scrolled = 0;

  materials.Get("FINAL1")->Draw(0, scrolled*vid.dupy, V_SCALE);
  if (scrolled>0)
    materials.Get("FINAL2")->Draw(0, (scrolled-200)*vid.dupy, V_SCALE);
}



void F_DrawUnderwater()
{
  static bool underwawa = false;

  if (!underwawa)
    {
      // FIXME only works the first time in each game... reset palette somewhere
      underwawa = true;
      vid.SetPaletteLump("E2PAL");
    }
  materials.Get("E2END")->Draw(0, 0, V_SCALE);
}



void F_HereticDrawer(int dummy)
{
  switch (gameepisode)
    {
    case 1:
      if (fc.FindNumForName("E2M1") == -1)
        materials.Get("ORDER")->Draw(0, 0, V_SCALE);
      else
        materials.Get("CREDIT")->Draw(0, 0, V_SCALE);
      break;
    case 2:
      F_DrawUnderwater();
      break;
    case 3:
      F_DemonScroll();
      break;
    case 4: // Just show credits screen for extended episodes
    case 5:
      materials.Get("CREDIT")->Draw(0, 0, V_SCALE);
      break;
    }
}

void F_HexenStart(int stage)
{
  switch (stage)
    {
    case 0:
      finalepic = materials.Get("FINALE1");
      S.StartMusic("HALL", false);
      break;

    case 1:
      //finaletext = "win1msg",
      F_TextInit(0);
      finalestage->stage = 5; // for the kludge
      break;

    case 2:
      //finaletext = "win2msg",
      F_TextInit(0);
      finalestage->stage = 5;
      finalepic = materials.Get("FINALE2");
      S.StartMusic("ORB", false);
      break;

    case 3:
      // TODO fade out
      break;

    case 4:
      // TODO fade in
      finalepic = materials.Get("FINALE3");
      S.StartMusic("CHESS", true);
      break;

    case 5:
      //finaletext = "win3msg"
      F_TextInit(0);
      finalestage->stage = 135;
      break;

    }
}

void F_HexenDrawer(int stage)
{
  finalepic->Draw(0, 0, V_SCALE);

  int base = fc.GetNumForName("CHESSC");
  // Chess pic, draw the correct character graphic
  if (stage == 4 || stage == 5)
    {
      int i = 0; // playerclass...
      if (game.multiplayer)
        materials.Get("CHESSALL")->Draw(20,0,V_SCALE);
      else
        materials.GetLumpnum(base + i)->Draw(60,0,V_SCALE);
    }
}

/* Fixed Routines */
void F_Drawer()
{  
    if (!finalestage)
    {
        CONS_Printf("CRASH-GEFahr: finalestage ist NULL!\n");
        return;
    }
    //CONS_Printf("F_Drawer: finalestage gültig, drawer = %p, stage = %d\n",
    //            (void*)finalestage->drawer, finalestage->stage);

    if (finalestage->drawer)
    {
        //CONS_Printf("Rufe drawer-Funktion auf...\n");
        (finalestage->drawer)(finalestage->stage);
        return;
    }
    //CONS_Printf("Fallback zu Text\n");
    F_TextWrite(finalestage->wait, finalestage->stage);
}

void F_TextWrite(int sx, int sy)
{
  /*
  CONS_Printf("[%s][%d]::F_TextWrite()\n"
              "                        sx  = %d\n"
              "                        sy  = %d\n"
              "                 vid.width  = %d\n"
              "                 vid.height = %d\n",__FILE__,__LINE__, sx, sy, vid.width,vid.height);  
  */
  
  /*
  SDL_Surface* s = SDL_GetVideoSurface();
  if (s)
  {
      int bpp = s->format->BitsPerPixel;
      int expected = vid.width * (bpp / 8);
      CONS_Printf("Pitch-Check in F_TextWrite: width=%d, bpp=%d, expected_pitch=%d, real_pitch=%d\n",
                  vid.width, bpp, expected, s->pitch);

      if (s->pitch != expected)
          CONS_Printf("!!! PITCH MISMATCH !!! Das ist höchstwahrscheinlich der Crash-Grund\n");
  }  
  */
  // erase the entire screen to a tiled background (or raw picture)   
  if (finalepic)
    finalepic->Draw(0, 0, V_SCALE);
  else if (finaleflat)
  {
     //finaleflat->DrawFill(0,0,vid.width,vid.height);  
      /* Marty: Fixed Final Middle Stage Screen.
      * finaleflat->DrawFill(0,0,vid.width,vid.height) ohne Skalierung → bei
      * höheren Auflösungen (z. B. 800×600 statt 320×200) schreibt es mehr Pixel,
      * als der Buffer hergibt → Overflow → Crash
      * Mit /vid.dupx und /vid.dupy wird die Fill-Größe auf die Original-Skalierung
      * zurückgerechnet → passt wieder in den Buffer
      */
      memcpy(vid.screens[0], vid.screens[1], vid.width * vid.height * (vid.BytesPerPixel));
      finaleflat->DrawFill(0, 0, vid.width/vid.dupx, vid.height/vid.dupy);
      
  }
  else
  {
      vid.FillScreen(0);  // Schwarz als Fallback
      CONS_Printf("WARN: Kein Finale-Hintergrund – schwarzer Fallback\n");
  }

  // draw some of the text onto the screen
  int count = (finalecount - 10)/TEXTSPEED;
  if (count < 0)
    return;
 
  if (count > int(finaletext.length()))
    count = finaletext.length();

  // small hack
  char c = finaletext[count];
  
  finaletext[count] = '\0';
  
  hud_font->DrawString(sx, sy, finaletext.c_str(), V_SCALE);
   
  finaletext[count] = c;
}