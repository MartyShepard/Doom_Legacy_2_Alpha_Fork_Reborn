// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: st_stuff.cpp 526 2008-01-22 23:28:24Z smite-meister $
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
/// \brief Status bar code.
///
/// Numbers, icons, face/direction indicator animation.
/// Does palette flashes as well (red pain/berserk, bright pickup...)

#include "doomdef.h"
#include "command.h"
#include "cvars.h"

#include "am_map.h"

#include "d_event.h"
#include "g_game.h"
#include "g_player.h"
#include "g_pawn.h"

#include "screen.h"
#include "r_data.h"
#include "r_presentation.h"

#include "m_random.h"

#include "hud.h"
#include "st_lib.h"
#include "i_video.h"
#include "v_video.h"
#include "tables.h"

#include "w_wad.h"
#include "z_zone.h"

#include "hardware/oglrenderer.hpp"
#include "hardware/oglhelpers.hpp"

#define FG 0
#define BG 1

// buffer for drawing status bar
int fgbuffer = FG;


// Size of statusbar.
#define ST_HEIGHT_DOOM    32
#define ST_HEIGHT_HERETIC 42
#define ST_HEIGHT_HEXEN   39
#define ST_WIDTH         320


// Palette indices.
// For damage/bonus red-/gold-shifts
#define STARTREDPALS            1
#define NUMREDPALS              8
#define STARTBONUSPALS          9
#define NUMBONUSPALS            4
// Radiation suit, green shift.
#define RADIATIONPAL            13
// new Hexen palettes
#define STARTPOISONPALS  13
#define NUMPOISONPALS    8
#define ICEPAL           21
#define STARTHOLYPALS    22
#define NUMHOLYPALS      3
#define STARTSCOURGEPALS 25
#define NUMSCOURGEPALS   3

#define NUM_PALETTES  (1+NUMREDPALS+NUMBONUSPALS+NUMPOISONPALS+1+NUMHOLYPALS+NUMSCOURGEPALS)

//=========================================
// HUD widget control variables
//=========================================

// palette flashes
static int  st_berzerk;
static bool st_radiation;

static int  st_itemuse;

// inside the HUD class:
// statusbar_on, mainbar_on, invopen

static const bool st_true = true;

static bool st_notdeathmatch;
static bool st_godmode;

static int  st_pawncolor;
static int  st_health;

static int  st_armor;
static int  st_readywp;
static int  st_atype;
static int  st_readywp_ammo;

static int  st_ammo[NUMAMMO];
static int  st_maxammo[NUMAMMO];

// owned keys
static int st_keyboxes[NUMKEYS];

// number of frags so far in deathmatch
static int  st_fragscount;

// Heretic spinning icons
static int  st_flight = -1;
static int  st_book = -1;
static int  st_speed = -1;
static int  st_defense = -1;
static int  st_minotaur = -1;

static inventory_t st_invslots[7+1]; // visible inventory slots (+ one hack slot)
static int st_curpos = 0; // active inv. slot (0-6)

static bool st_weaponsowned[NUMWEAPONS];

static bool st_mana1icon, st_mana2icon;
static int st_mana1, st_mana2;

#define BLINKTHRESHOLD  (4*32)



//=========================================
//  Legacy status bar overlay
//=========================================

static Material *sbohealth;
static Material *sbofrags;
static Material *sboarmor;
static Material *PatchAmmoPic[NUMAMMO + 1];


//=========================================
// Doom status bar graphics
//=========================================

// doom.wad number sets:
// "AMMNUM0", small, thin, gray, no minus
// "WINUM0", large, red, minus is base-2, '%' is base-1

// "STTNUM0", large, red, minus is base-1, '%' is base+10
static Material *PatchBNum[11]; // 0-9, big numbers, STTMINUS
static Material *tallpercent; // big % sign

// "STYSNUM0", small, yellow, no minus
static Material *PatchSNum[11];

// "STGNUM0", small, dark gray, no minus
static Material *PatchArms[6][2]; // weapon ownership patches
static Material *PatchArmsBack; // arms background


static Material *PatchKeys[NUMKEYS]; // 3 key-cards, 3 skulls
static Material *PatchSTATBAR;
static Material *PatchKEYBAR;

// ammo type pics (Material's)
static const char DHAmmoPics[NUMAMMO + 1][10] =
{
  {"SBOAMMO1"}, //{"CLIPA0"},  // 0, bullets
  {"SBOAMMO2"}, //{"SHELA0"},  // shells
  {"SBOAMMO3"}, //{"CELLA0"},  // plasma
  {"SBOAMMO4"}, //{"ROCKA0"},  // rockets
  {"INAMGLD"}, // gold wand
  {"INAMBOW"}, // crossbow
  {"INAMBST"}, // blaster
  {"INAMRAM"}, // skullrod
  {"INAMPNX"}, // phoenix rod
  {"INAMLOB"}, // mace
  {"MANABRT1"}, // mana 1
  {"MANABRT2"}, // mana 2
  {"BLACKSQ"}  // no ammopic
};


//=========================================
// Heretic status bar graphics
//=========================================

static Material *PatchGod[2];
static Material *PatchBARBACK;
static Material *PatchLTFCTOP;
static Material *PatchRTFCTOP;
static Material *PatchARMCLEAR;

static Material *Patch_InvBar[13];
static Material *PatchARTI[NUMARTIFACTS];
static Material *Patch_ChainSlider[5];

// Heretic numbers:
// SMALLIN0, small yellow number
static Material *PatchINum[11];  // IN0, big yellow number
// FONTB16, big green numbers (of a font)
// FONTA16, medium silver numbers (of a font)

// Hexen:
// IN0 : medium yellow numbers with minus (NEGNUM)
// SMALLIN0: little yellow numbers
// INRED0 : medium red numbers, no minus
// FONTA01-59 : medium silver font
// FONTAY01-59 : like FONTA but yellow
// FONTB01-59 : large brown font

static Material *PatchFlight[16];
static Material *PatchBook[16];
static Material *PatchSpeed[16];
static Material *PatchDefense[16];
static Material *PatchMinotaur[16];

// Hexen extras
static Material *PatchH2BAR;
static Material *PatchH2TOP;
static Material *PatchMana1[2];
static Material *PatchMana2[2];
static Material *PatchKILLS;

static const char ArtiPatchName[][10] =
{
  {"ARTIBOX"},    // none
  {"ARTIINVU"},   // invulnerability
  {"ARTIINVS"},   // invisibility
  {"ARTIPTN2"},   // health
  {"ARTISPHL"},   // superhealth
  {"ARTIPWBK"},   // tomeofpower
  {"ARTITRCH"},   // torch
  {"ARTIFBMB"},   // firebomb
  {"ARTIEGGC"},   // egg
  {"ARTISOAR"},   // fly
  {"ARTIATLP"},   // teleport

  {"ARTIHRAD"},   // healing radius
  {"ARTISUMN"},   // summon maulotaur
  {"ARTIPORK"},   // porkelator
  {"ARTIBLST"},   // blast radius
  {"ARTIPSBG"},   // poison bag
  {"ARTITELO"},   // teleport other
  {"ARTISPED"},   // speed
  {"ARTIBMAN"},   // boost mana
  {"ARTIBRAC"},   // boost armor

  {"ARTISKLL"},   // arti_puzzskull
  {"ARTIBGEM"},   // arti_puzzgembig
  {"ARTIGEMR"},   // arti_puzzgemred
  {"ARTIGEMG"},   // arti_puzzgemgreen1
  {"ARTIGMG2"},   // arti_puzzgemgreen2
  {"ARTIGEMB"},   // arti_puzzgemblue1
  {"ARTIGMB2"},   // arti_puzzgemblue2
  {"ARTIBOK1"},   // arti_puzzbook1
  {"ARTIBOK2"},   // arti_puzzbook2
  {"ARTISKL2"},   // arti_puzzskull2
  {"ARTIFWEP"},   // arti_puzzfweapon
  {"ARTICWEP"},   // arti_puzzcweapon
  {"ARTIMWEP"},   // arti_puzzmweapon
  {"ARTIGEAR"},   // arti_puzzgear1
  {"ARTIGER2"},   // arti_puzzgear2
  {"ARTIGER3"},   // arti_puzzgear3
  {"ARTIGER4"},   // arti_puzzgear4

  {"WFR1A0"},     // weapon pieces
  {"WFR2A0"},
  {"WFR3A0"},
  {"WCH1A0"},
  {"WCH2A0"},
  {"WCH3A0"},
  {"WMS1A0"},
  {"WMS2A0"},
  {"WMS3A0"}
};



static void ST_SetClassData(int num, int cls)
{
  if (game.mode == gm_hexen)
    {
      int base;
      /*
        PatchWEAPONSLOT = tc.CachePtrNum(fc.GetNumForName("WPSLOT0") + cls);
        PatchWEAPONFULL = tc.CachePtrNum(fc.GetNumForName("WPFULL0") + cls);
        PatchPIECE1     = tc.CachePtrNum(fc.GetNumForName("WPIECEF1") + cls);
        PatchPIECE2     = tc.CachePtrNum(fc.GetNumForName("WPIECEF2") + cls);
        PatchPIECE3     = tc.CachePtrNum(fc.GetNumForName("WPIECEF3") + cls);
      */

      Patch_ChainSlider[1] = materials.GetLumpnum(fc.GetNumForName("CHAIN") + cls);

      base = fc.GetNumForName("LIFEGEM");
      if (!game.multiplayer)
        // single player game uses red life gem
        Patch_ChainSlider[2] = materials.GetLumpnum(base + 4*cls + 1);
      else
        Patch_ChainSlider[2] = materials.GetLumpnum(base + 4*cls + num % 4);
    }
  else
    {
      // heretic
      if (!game.multiplayer)
        // single player game uses red life gem
        Patch_ChainSlider[2] = materials.Get("LIFEGEM2");
      else
        Patch_ChainSlider[2] = materials.GetLumpnum(fc.GetNumForName("LIFEGEM0") + num % 4);
    }
}


void ST_LoadHexenData()
{
  int i;
  int startLump;

  PatchH2BAR = materials.Get("H2BAR");
  PatchH2TOP = materials.Get("H2TOP");

  PatchSTATBAR = materials.Get("STATBAR");
  PatchKEYBAR = materials.Get("KEYBAR");

  //PatchARTICLEAR = materials.Get("ARTICLS");
  PatchARMCLEAR = materials.Get("ARMCLS");
  //PatchMANACLEAR = materials.Get("MANACLS");
  /*
    PatchMANAVIAL1 = materials.Get("MANAVL1");
    PatchMANAVIAL2 = materials.Get("MANAVL2");
    PatchMANAVIALDIM1 = materials.Get("MANAVL1D");
    PatchMANAVIALDIM2 = materials.Get("MANAVL2D");
  */
  PatchMana1[0] = materials.Get("MANADIM1");
  PatchMana1[1] = materials.Get("MANABRT1");
  PatchMana2[0] = materials.Get("MANADIM2");
  PatchMana2[1] = materials.Get("MANABRT2");

  sbohealth = materials.Get("SBOHEALT"); //"PTN2A0"
  sbofrags  = materials.Get("SBOFRAGS"); //"ARTISKLL"
  sboarmor  = materials.Get("SBOARMOR"); //"ARM1A0"

  Patch_InvBar[0] = materials.Get("INVBAR");
  Patch_InvBar[1] = materials.Get("ARTIBOX");
  Patch_InvBar[2] = materials.Get("SELECTBO");
  Patch_InvBar[3] = materials.Get("INVGEML1");
  Patch_InvBar[4] = materials.Get("INVGEML2");
  Patch_InvBar[5] = materials.Get("INVGEMR1");
  Patch_InvBar[6] = materials.Get("INVGEMR2");
  Patch_InvBar[7] = materials.Get("BLACKSQ");

  // artifact inventory pics
  for (i=0; i <= NUMARTIFACTS; i++)
    PatchARTI[i] = materials.Get(ArtiPatchName[i]);

  // artifact use flash
  startLump = fc.GetNumForName("USEARTIA");
  for (i=0; i<5; i++) Patch_InvBar[i+8] = materials.GetLumpnum(startLump + i);

  // ammo pics
  for (i=0; i < NUMAMMO; i++)
    PatchAmmoPic[i] = materials.Get(DHAmmoPics[i]);

  // keys
  startLump = fc.GetNumForName("KEYSLOT1");
  for (i=0; i<11; i++)
    PatchKeys[i] = materials.GetLumpnum(startLump+i);

  // numbers
  startLump = fc.GetNumForName("IN0");
  for (i = 0; i < 10; i++)
    PatchINum[i] = materials.GetLumpnum(startLump+i);
  PatchINum[10] = materials.Get("NEGNUM");

  // BNum
  startLump = fc.GetNumForName("FONTB16");
  for (i = 0; i < 10; i++)
    PatchBNum[i] = materials.GetLumpnum(startLump+i);
  PatchBNum[10] = materials.GetLumpnum(startLump-3); //("FONTB13")

  //SNum
  startLump = fc.GetNumForName("SMALLIN0");
  for (i = 0; i < 10; i++)
    PatchSNum[i] = materials.GetLumpnum(startLump+i);
  PatchSNum[10] = PatchSNum[0]; // no minus available

  int SpinFlyLump = fc.GetNumForName("SPFLY0");
  int SpinSpeedLump = fc.GetNumForName("SPBOOT0");
  int SpinDefenseLump = fc.GetNumForName("SPSHLD0");
  int SpinMinotaurLump = fc.GetNumForName("SPMINO0");

  for (i=0; i<16; i++)
    {
      PatchFlight[i] = materials.GetLumpnum(SpinFlyLump + i);
      PatchSpeed[i] = materials.GetLumpnum(SpinSpeedLump + i);
      PatchDefense[i] = materials.GetLumpnum(SpinDefenseLump + i);
      PatchMinotaur[i] = materials.GetLumpnum(SpinMinotaurLump + i);
    }

  PatchKILLS = materials.Get("KILLS");

  // health chain slider
  Patch_ChainSlider[0] = NULL;
  Patch_ChainSlider[3] = materials.Get("LFEDGE");
  Patch_ChainSlider[4] = materials.Get("RTEDGE");
  ST_SetClassData(0, 0);
}

void ST_LoadHereticData()
{
  int i;
  int startLump;

  // gargoyle eyes
  PatchGod[0] = materials.Get("GOD1");
  PatchGod[1] = materials.Get("GOD2");

  PatchBARBACK = materials.Get("BARBACK");

  if (cv_deathmatch.value)
    PatchSTATBAR = materials.Get("STATBAR");
  else
    PatchSTATBAR = materials.Get("LIFEBAR");

  PatchLTFCTOP = materials.Get("LTFCTOP");
  PatchRTFCTOP = materials.Get("RTFCTOP");
  PatchARMCLEAR  = materials.Get("ARMCLEAR");

  // inventory bar pics
  Patch_InvBar[0] = materials.Get("INVBAR");
  Patch_InvBar[1] = materials.Get("ARTIBOX");
  Patch_InvBar[2] = materials.Get("SELECTBO");
  Patch_InvBar[3] = materials.Get("INVGEML1");
  Patch_InvBar[4] = materials.Get("INVGEML2");
  Patch_InvBar[5] = materials.Get("INVGEMR1");
  Patch_InvBar[6] = materials.Get("INVGEMR2");
  Patch_InvBar[7] = materials.Get("BLACKSQ"); // useful?

  // artifact use flash
  startLump = fc.GetNumForName("USEARTIA");
  for (i=0; i<5; i++) Patch_InvBar[i+8] = materials.GetLumpnum(startLump + i);

  // artifact inventory pics
  for (i=0; i <= NUMARTIFACTS; i++)
    PatchARTI[i] = materials.Get(ArtiPatchName[i]);

  // ammo pics
  //  for (i=0; i < am_heretic; i++)
  //  PatchAmmoPic[i] = NULL;
  //for (i=am_heretic; i <= NUMAMMO; i++)
  for (i = 0; i < NUMAMMO; i++)
    PatchAmmoPic[i] = materials.Get(DHAmmoPics[i]);

  sbohealth = materials.Get("SBOHEALT"); //"PTN2A0"
  sbofrags  = materials.Get("SBOFRAGS"); //"FACEB1"
  sboarmor  = materials.Get("SBOARMOR"); //"SHLDA0"

  // keys
  PatchKeys[11] = PatchKeys[14] = materials.Get("BKEYICON");
  PatchKeys[12] = PatchKeys[15] = materials.Get("YKEYICON");
  PatchKeys[13] = PatchKeys[16] = materials.Get("GKEYICON");

  // health chain slider
  Patch_ChainSlider[0] = materials.Get("CHAINBAC");
  Patch_ChainSlider[1] = materials.Get("CHAIN");
  Patch_ChainSlider[3] = materials.Get("LTFACE");
  Patch_ChainSlider[4] = materials.Get("RTFACE");
  ST_SetClassData(0, 0);

  // INum
  startLump = fc.GetNumForName("IN0");
  for (i = 0; i < 10; i++)
    PatchINum[i] = materials.GetLumpnum(startLump+i);
  PatchINum[10] = materials.Get("NEGNUM");
  // and "LAME"...

  // BNum
  startLump = fc.GetNumForName("FONTB16");
  for (i = 0; i < 10; i++)
    PatchBNum[i] = materials.GetLumpnum(startLump+i);
  PatchBNum[10] = materials.GetLumpnum(startLump-3); //("FONTB13")

  //SNum
  startLump = fc.GetNumForName("SMALLIN0");
  for (i = 0; i < 10; i++)
    PatchSNum[i] = materials.GetLumpnum(startLump+i);
  PatchSNum[10] = PatchSNum[0]; // no minus available

  int SpinBookLump = fc.GetNumForName("SPINBK0");
  int SpinFlyLump = fc.GetNumForName("SPFLY0");

  for (i=0; i<16; i++)
    {
      PatchFlight[i] = materials.GetLumpnum(SpinFlyLump + i);
      PatchBook[i] = materials.GetLumpnum(SpinBookLump + i);
    }
}




void ST_LoadDoomData()
{
  int  i;
  char namebuf[9];

  // Load the numbers, tall and short
  for (i=0; i<10; i++)
    {
      sprintf(namebuf, "STTNUM%d", i);
      PatchBNum[i] = materials.Get(namebuf);

      sprintf(namebuf, "STYSNUM%d", i);
      PatchSNum[i] = materials.Get(namebuf);
    }

  PatchBNum[10] = materials.Get("STTMINUS");
  PatchSNum[10] = PatchSNum[0]; // no minus available

  // percent signs.
  tallpercent = materials.Get("STTPRCNT");

  // key cards
  for (i=0; i<6; i++)
    {
      sprintf(namebuf, "STKEYS%d", i);
      PatchKeys[i+11] = materials.Get(namebuf);
    }

  // arms background box
  PatchArmsBack = materials.Get("STARMS");

  // arms ownership widgets
  for (i=0;i<6;i++)
    {
      sprintf(namebuf, "STGNUM%d", i+2);

      // gray #
      PatchArms[i][0] = materials.Get(namebuf);

      // yellow #
      PatchArms[i][1] = PatchSNum[i+2];
    }

  // status bar background bits
  PatchSTATBAR = materials.Get("STBAR");

  // ammo pics
  for (i = 0; i < NUMAMMO; i++)
    PatchAmmoPic[i] = materials.Get(DHAmmoPics[i]);

  sbohealth = materials.Get("SBOHEALT"); //"STIMA0"
  sbofrags  = materials.Get("SBOFRAGS"); //"M_SKULL1"
  sboarmor  = materials.Get("SBOARMOR"); //"ARM1A0"
}



void ST_unloadData()
{
  int i;

  // unload the numbers, tall and short
  for (i=0;i<10;i++)
    {
      PatchBNum[i]->Release();
      PatchSNum[i]->Release();
    }
  // unload tall percent
  tallpercent->Release();

  // unload arms background
  PatchArmsBack->Release();

  // unload gray #'s
  for (i=0;i<6;i++)
    PatchArms[i][0]->Release();

  // unload the key cards
  for (i=0;i<NUMKEYS;i++)
    if (PatchKeys[i])
      PatchKeys[i]->Release();

  PatchSTATBAR->Release();
}




// refresh the status bar background
void HUD::ST_RefreshBackground()
{
  int flags = (fgbuffer & V_FLAGMASK) | BG;

  if (game.mode == gm_hexen)
    {
      PatchH2BAR->Draw(st_x, st_y-27, flags); // x=0, y = 134
      if (!automap.active)
	PatchSTATBAR->Draw(st_x+38, st_y+1, flags); // x=38, y=162
      else
	PatchKEYBAR->Draw(st_x+38, st_y+1, flags);
    }
  else if (game.mode == gm_heretic)
    {
      PatchBARBACK->Draw(st_x, st_y, flags);
      PatchSTATBAR->Draw(st_x+34, st_y+2, flags);
      //V_DrawScaledPatch(st_x+34, st_y+2, flags, PatchINVBAR);
      // background:
      //main
      PatchARMCLEAR->Draw(st_x+57, st_y+13, flags);
      //V_DrawScaledPatch(st_x+108, st_y+3, flags, PatchBLACKSQ);
      PatchARMCLEAR->Draw(st_x+224, st_y+13, flags);
    }
  else
    {
      // software mode copies patch to BG buffer,
      // hardware modes directly draw the statusbar to the screen
      PatchSTATBAR->Draw(st_x, st_y, flags);
    }

  // copy the statusbar buffer to the screen
  if (rendermode == render_soft)
    V_CopyRect(0, vid.height-stbarheight, BG, vid.width, stbarheight, 0, vid.height-stbarheight, FG);
}



// Widgets that keep a history cannot be shared between viewports.
static HudSlider *sliders[MAX_GLVIEWPORTS];
static HudFace     *faces[MAX_GLVIEWPORTS];



void HUD::UpdateWidgets(PlayerInfo *st_player, int vp)
{
  //ST_SetClassData(pawndata[p->ptype].pclass, p->number); TODO FIXME


  PlayerPawn *st_pawn = st_player->pawn;

  if (!st_pawn || st_pawn->eflags & MFE_REMOVE) // TEST
    {
      // it will be deleted soon
      return;
    }

  if (st_pawn->powers[pw_strength])
    st_berzerk = 12 - (st_pawn->powers[pw_strength]>>6);  // slowly fade the berzerk out??? FIXME it's on/off!
  else
    st_berzerk = 0;

  st_radiation = (st_pawn->powers[pw_ironfeet] > BLINKTHRESHOLD || st_pawn->powers[pw_ironfeet] & 8);

  const int largeammo = 1994; // means "n/a"
  int i;

  statusbar_on = (cv_viewsize.value < 11) || automap.active;

  // status bar overlay at viewsize 11
  overlay_on = (cv_viewsize.value == 11);
  if (overlay_on)
    fgbuffer |= V_TL;
  else
    fgbuffer &= ~V_TL;

  // when pawn is detached from player, no more update
  if (st_pawn->player)
    st_fragscount = st_pawn->player->score;

  st_notdeathmatch = !cv_deathmatch.value;

  st_godmode = (st_pawn->cheats & CF_GODMODE);

  st_pawncolor = st_pawn->pres->color;
  st_health = st_pawn->health;

  if (game.mode == gm_heretic || game.mode == gm_hexen)
    {
      if (st_player->invTics)
	{
	  if (!invopen)
	    st_refresh = true;
	  invopen = true;
	}
      else
	{
	  if (invopen)
	    st_refresh = true;
	  invopen = false;
	}

      mainbar_on = statusbar_on && !invopen && !automap.active;

      // inventory
      st_itemuse = st_player->itemuse;

      int n = st_pawn->inventory.size();

      // kind of a HACK
      if (st_player->invSlot >= n)
	st_player->invSlot = 0;

      if (st_player->invSlot < st_player->invPos)
	st_player->invPos = st_player->invSlot;

      st_curpos = st_player->invPos;
      int left = st_player->invSlot - st_curpos; // how many slots are there left of the first visible slot?
      for (i=0; i<7; i++)
        if (i+left < n && st_pawn->inventory[left+i].type != arti_none)
          st_invslots[i] = st_pawn->inventory[left+i];
        else
          st_invslots[i] = inventory_t(arti_none, 0);

      st_invslots[7].type = (left > 0) ? 1 : 0; // hack
      st_invslots[7].count = (n - left > 7) ? 1 : 0;

      int frame = (game.tic/3) & 15;
      // flight icon
      if (st_pawn->powers[pw_flight] > BLINKTHRESHOLD || (st_pawn->powers[pw_flight] & 16))
        st_flight = frame;
          // TODO stop the spinning when not in air?
          // if (st_pawn->flags2 & MF2_FLY)
      else
        st_flight = -1;

      // book icon
      if ((st_pawn->powers[pw_weaponlevel2] > BLINKTHRESHOLD || (st_pawn->powers[pw_weaponlevel2] & 16)) && !st_pawn->morphTics)
        st_book = frame;
      else
        st_book = -1;

      // speed icon
      if (st_pawn->powers[pw_speed] > BLINKTHRESHOLD || (st_pawn->powers[pw_speed] & 16))
        st_speed = frame;
      else
        st_speed = -1;

      // defense icon
      if (st_pawn->powers[pw_invulnerability] > BLINKTHRESHOLD || (st_pawn->powers[pw_invulnerability] & 16))
        st_defense = frame;
      else
        st_defense = -1;

      // minotaur icon
      if (st_pawn->powers[pw_minotaur] > BLINKTHRESHOLD || (st_pawn->powers[pw_minotaur] & 16))
        st_minotaur = frame;
      else
        st_minotaur = -1;

      st_mana1 = st_pawn->ammo[am_mana1];
      st_mana2 = st_pawn->ammo[am_mana2];
      st_mana1icon = (st_mana1 > 0);
      st_mana2icon = (st_mana2 > 0);
    }
  else
    {
      // doom
      for (i=0; i<NUMAMMO; i++)
        {
          st_ammo[i] = st_pawn->ammo[i];
          st_maxammo[i] = st_pawn->maxammo[i];
        }

      for (i=0; i<NUMWEAPONS; i++)
	st_weaponsowned[i] = st_pawn->weaponowned[i];

      faces[vp]->updateFaceWidget(st_player); // these are personal
    }

  st_armor = int(100 * st_pawn->toughness);
  for (i=0; i<NUMARMOR; i++)
    st_armor += st_pawn->armorpoints[i];

  if (game.mode == gm_hexen)
    st_armor /= 5; // "AC"

  st_readywp = st_pawn->readyweapon;

  st_atype = st_pawn->weaponinfo[st_pawn->readyweapon].ammo;
  if (st_atype == am_noammo)
    st_readywp_ammo = largeammo;
  else if (st_atype == am_manaboth)
    st_readywp_ammo = min(st_pawn->ammo[am_mana1], st_pawn->ammo[am_mana2]);
  else
    st_readywp_ammo = st_pawn->ammo[st_atype];

  // update keycard widgets
  for (i=0; i<NUMKEYS; i++)
    st_keyboxes[i] = (st_pawn->keycards & (1 << i)) ? i : -1;
}

// sets the new palette based upon current values of damagecount
// and bonuscount
void HUD::PaletteFlash(PlayerInfo *p)
{
  int palette = 0;
  int dcount = p->damagecount;

  if (st_berzerk > dcount)
    dcount = st_berzerk;

  if (p->palette >= 0)
    {
      palette = p->palette;
    }
  else /* if (p->pawn->poisoncount) TODO
    {
      palette = (p->pawn->poisoncount + 7) >> 3;
      if (palette >= NUMPOISONPALS)
        palette = NUMPOISONPALS-1;

      palette += STARTPOISONPALS;
    }
  else */ if (dcount)
    {
      palette = (dcount + 7) >> 3;

      if (palette >= NUMREDPALS)
        palette = NUMREDPALS-1;

      palette += STARTREDPALS;
    }
  else if (p->bonuscount)
    {
      palette = (p->bonuscount + 7) >> 3;

      if (palette >= NUMBONUSPALS)
        palette = NUMBONUSPALS-1;

      palette += STARTBONUSPALS;
    }
  else if (st_radiation)
    palette = RADIATIONPAL; // not relevant in heretic
  /*
  else if (sbpawn->flags2 & MF2_ICEDAMAGE)
    { // TODO Frozen player
      palette = ICEPAL;
    }
  */

  palette = min(palette, NUM_PALETTES-1);

  if (palette != st_palette)
    {
      st_palette = palette;

      if (rendermode == render_opengl)
        {
	  float opengl_color[NUM_PALETTES][4] = {
	    { 0.0f,  0.0f,  0.0f, 0.0f}, // normal

	    {0.59f, 0.21f, 0.21f, 1.0f}, // damage/berserk (8 reds)
	    {0.59f, 0.21f, 0.21f, 1.0f},
	    {0.65f, 0.18f, 0.18f, 1.0f},
	    {0.71f, 0.15f, 0.15f, 1.0f},
	    {0.78f, 0.12f, 0.12f, 1.0f},
	    {0.84f, 0.09f, 0.09f, 1.0f},
	    {0.90f, 0.06f, 0.06f, 1.0f}, 
	    {0.97f, 0.03f, 0.03f, 1.0f},

	    {0.37f, 0.37f, 1.00f, 1.0f}, // bonus (4 gold/green?)
	    {0.56f, 0.62f, 0.44f, 1.0f},
	    {0.59f, 0.69f, 0.40f, 1.0f},
	    {0.62f, 0.75f, 0.37f, 1.0f},

	    // TODO rest of colors
	    {0.37f, 1.00f, 0.37f, 1.0f}, // Doom: radiation suit (1 green), Hexen: poisoning (8 greens)
	    {0.37f, 0.37f, 1.00f, 1.0f},
	    {0.37f, 0.37f, 1.00f, 1.0f},
	    {0.37f, 0.37f, 1.00f, 1.0f},
	    {0.37f, 0.37f, 1.00f, 1.0f},
	    {0.37f, 0.37f, 1.00f, 1.0f},
	    {0.37f, 0.37f, 1.00f, 1.0f},
	    {0.37f, 0.37f, 1.00f, 1.0f},

	    {0.37f, 0.37f, 1.00f, 1.0f}, // ice (1 bluish)

	    {0.37f, 0.37f, 1.00f, 1.0f}, // wraithverge (3 )
	    {0.37f, 0.37f, 1.00f, 1.0f},
	    {0.37f, 0.37f, 1.00f, 1.0f},

	    {0.37f, 0.37f, 1.00f, 1.0f}, // bloodscourge (3 reddish)
	    {0.37f, 0.37f, 1.00f, 1.0f},
	    {0.37f, 0.37f, 1.00f, 1.0f},
	  };

	  oglrenderer->SetGlobalColor(opengl_color[palette]);
        }
      else
        {
	  vid.SetPalette(palette);
        }
    }
}



void HUD::ST_Recalc()
{
  switch (game.mode)
    {
    case gm_hexen:
      stbarheight = ST_HEIGHT_HEXEN;
      break;
    case gm_heretic:
      stbarheight = ST_HEIGHT_HERETIC;
      break;
    default:
      stbarheight = ST_HEIGHT_DOOM;
      break;
    }

  if (cv_scalestatusbar.value || cv_viewsize.value > 10)
    {
      fgbuffer = FG | V_SCALE; // scale patch by default

      if (rendermode == render_opengl)
        {
          st_x = 0;
          st_y = BASEVIDHEIGHT - stbarheight + 1;

	  stbarheight = int(stbarheight * vid.fdupy); // real height
        }
      else
        {
          st_x = (vid.width - ST_WIDTH * vid.dupx) / (2 * vid.dupx);
          st_y = (vid.height - stbarheight * vid.dupy) / vid.dupy;

	  stbarheight *= vid.dupy; // real height
        }
    }
  else
    {
      fgbuffer = FG;
      st_x = (vid.width - ST_WIDTH) >> 1;
      st_y = vid.height - stbarheight;
    }

  // TODO not good. When should the widgets be created?
  // and renew the widgets
  ST_CreateWidgets();

  st_refresh = true;
}



void HUD::CreateHexenWidgets()
{
  HudWidget *h;

  // health sliders
  for (int k=0; k<MAX_GLVIEWPORTS; k++)
    {
      sliders[k] = new HexenHudSlider(st_x, st_y+32, &st_health, 0, 100, Patch_ChainSlider);
      faces[k] = NULL;
    }

  // spinning icons
  h = new HudMultIcon(st_x+20, 19, &st_flight, PatchFlight);
  statusbar.push_back(h);

  h = new HudMultIcon(st_x+60, 19, &st_speed, PatchSpeed);
  statusbar.push_back(h);

  h = new HudMultIcon(st_x+260, 19, &st_defense, PatchDefense);
  statusbar.push_back(h);

  h = new HudMultIcon(st_x+300, 19, &st_minotaur, PatchMinotaur);
  statusbar.push_back(h);

  // gargoyle wings
  h = new HudBinIcon(st_x, st_y-27, &st_true, NULL, PatchH2TOP);
  always.push_back(h);

  // mainbar (closed inventory shown)
  // frags / health
  if (cv_deathmatch.value)
    //V_DrawPatch(38, 162, PatchKILLS);
    h = new HudNumber(st_x+50, st_y+16, 3, &st_fragscount, PatchINum);
  else
    // TODO: use red numbers if health is low
    h = new HudNumber(st_x+65, st_y+16, 3, &st_health, PatchINum);
  mainbar.push_back(h);

  // mana
  h = new HudNumber(st_x+92, st_y+21, 3, &st_mana1, PatchSNum);
  mainbar.push_back(h);
  h = new HudBinIcon(st_x+77, st_y+4, &st_mana1icon, PatchMana1[0], PatchMana1[1]);
  mainbar.push_back(h);

  h = new HudNumber(st_x+124, st_y+21, 3, &st_mana2, PatchSNum);
  mainbar.push_back(h);
  h = new HudBinIcon(st_x+110, st_y+4, &st_mana2icon, PatchMana2[0], PatchMana2[1]);
  mainbar.push_back(h);
  // TODO mana vials (new widget type?)

  // armor
  h = new HudNumber(st_x+275, st_y+16, 3, &st_armor, PatchINum);
  mainbar.push_back(h);

  // inventory system
  h = new HexenHudInventory(st_x+38, st_y-1, &invopen, &st_itemuse, st_invslots, &st_curpos,
			    PatchSNum, PatchARTI, Patch_InvBar);
  statusbar.push_back(h);

  // TODO Weapon Pieces
  int i;

  // Keybar (in map screen, keys and armor pieces)
  for (i=0; i<5; i++)
    {
      h = new HudMultIcon(st_x+46+i*20, st_y+3, &st_keyboxes[i], PatchKeys);
      keybar.push_back(h);
    }

  for (i=1; i<NUMARMOR; i++)
    {
      /*
      // TODO fading icons: 0:nothing, <= ArmInc[cl][i]>>2:fuzz, <= ArmInc[cl][i]>>1:altfuzz, otherwise normal
      h = new HudFadeIcon(st_x+119+i*31, st_y+3, &st_armorboxes[i], "ARMSLOT1"+i-1);
      keybar.push_back(h);
      */
    }
}


void HUD::CreateHereticWidgets()
{
  // statusbar_on, mainbar_on, invopen

  // health sliders
  for (int k=0; k<MAX_GLVIEWPORTS; k++)
    {
      sliders[k] = new HudSlider(st_x, st_y+32, &st_health, 0, 100, Patch_ChainSlider);
      faces[k] = NULL;
    }

  int i;
  HudWidget *h;

  h = new HudMultIcon(st_x+20, 20, &st_flight, PatchFlight);
  statusbar.push_back(h);

  h = new HudMultIcon(st_x+300, 20, &st_book, PatchBook);
  statusbar.push_back(h);

  // godmode indicators
  h = new HudBinIcon(st_x+16, st_y+9, &st_godmode, NULL, PatchGod[0]);
  statusbar.push_back(h);
  h = new HudBinIcon(st_x+287, st_y+9, &st_godmode, NULL, PatchGod[1]);
  statusbar.push_back(h);

  // gargoyle horns
  h = new HudBinIcon(st_x, st_y-10, &st_true, NULL, PatchLTFCTOP);
  always.push_back(h);
  h = new HudBinIcon(st_x+290, st_y-10, &st_true, NULL, PatchRTFCTOP);
  always.push_back(h);

  // inventory system
  h = new HudInventory(st_x+34, st_y+1, &invopen, &st_itemuse, st_invslots, &st_curpos,
                       PatchSNum, PatchARTI, Patch_InvBar);
  statusbar.push_back(h);

  // mainbar (closed inventory shown)
  // frags / health
  if (cv_deathmatch.value)
    h = new HudNumber(st_x+61+27, st_y+12, 3, &st_fragscount, PatchINum);
  else
    h = new HudNumber(st_x+61+27, st_y+12, 3, &st_health, PatchINum);
  mainbar.push_back(h);

  // Keys
  const int ST_KEYY[3] = {22, 6, 14};
  for (i=0; i<6; i++)
    {
      h = new HudMultIcon(st_x+153, st_y+ST_KEYY[i%3], &st_keyboxes[i+11], PatchKeys);
      mainbar.push_back(h);
    }

  // readyweapon ammo
  h = new HudNumber(st_x + 109 + 27, st_y + 4, 3, &st_readywp_ammo, PatchINum);
  mainbar.push_back(h);

  // ammo type icon
  h = new HudMultIcon(st_x + 111, st_y + 14, &st_atype, PatchAmmoPic);
  mainbar.push_back(h);

  // armor
  h = new HudNumber(st_x+228+27, st_y+12, 3, &st_armor, PatchINum);
  mainbar.push_back(h);
}

/*
  DOOM widgets:
  readyweapon ammo
  frags
  ammo * 4
  maxammo * 4
  health
  armor
  armsbg
  weapons owned * 6
  keycards * 3 (6!)
  face
 */

void HUD::CreateDoomWidgets()
{
  int i;
  HudWidget *h;

  // health sliders, faces
  for (int k=0; k<MAX_GLVIEWPORTS; k++)
    {
      sliders[k] = NULL;
      faces[k] = new HudFace(st_x+143, st_y);
    }

  // ready weapon ammo
  h = new HudNumber(st_x+44, st_y+3, 3, &st_readywp_ammo, PatchBNum);
  statusbar.push_back(h);

  // ammo count and maxammo (all four kinds)
  const int ST_AMMOY[4] = {5, 11, 23, 17};
  for (i=0; i<4; i++)
    {
      h = new HudNumber(st_x+288, st_y + ST_AMMOY[i], 3, &st_ammo[i], PatchSNum);
      statusbar.push_back(h);

      h = new HudNumber(st_x+314, st_y + ST_AMMOY[i], 3, &st_maxammo[i], PatchSNum);
      statusbar.push_back(h);
    }

  // health percentage
  h = new HudPercent(st_x+90, st_y+3, &st_health, PatchBNum, tallpercent);
  statusbar.push_back(h);

  // armor percentage - should be colored later
  h = new HudPercent(st_x+221, st_y+3, &st_armor, PatchBNum, tallpercent);
  statusbar.push_back(h);

  // Weapons

  if (cv_deathmatch.value)
    {
      // frags
      h = new HudNumber(st_x+138, st_y+3, 2, &st_fragscount, PatchBNum);
      statusbar.push_back(h);
    }
  else
    {
      // weapons owned
      for (i=0; i<6; i++)
	{
	  // these are shown
	  const int wsel[] = {wp_pistol, wp_shotgun, wp_chaingun, wp_missile, wp_plasma, wp_bfg};
	  h = new HudBinIcon(st_x+111+(i%3)*12, st_y+4+(i/3)*10, &st_weaponsowned[wsel[i]],
			     PatchArms[i][0], PatchArms[i][1]);
	  statusbar.push_back(h);
	}
    }

  // arms background
  h = new HudBinIcon(st_x+104, st_y, &st_notdeathmatch, NULL, PatchArmsBack);
  statusbar.push_back(h);

  // Key icon positions.
  const int ST_KEYY[3] = {3, 13, 23};
  for (i=0; i<6; i++)
    {
      h = new HudMultIcon(st_x+239+(i/3)*10, st_y+ST_KEYY[i%3], &st_keyboxes[i+11], PatchKeys);
      statusbar.push_back(h);
    }
}


void HUD::ST_CreateWidgets()
{
  CreateOverlayWidgets();

  for (int i = statusbar.size()-1; i>=0; i--)
    delete statusbar[i];
  statusbar.clear();

  for (int i = mainbar.size()-1; i>=0; i--)
    delete mainbar[i];
  mainbar.clear();

  for (int i = keybar.size()-1; i>=0; i--)
    delete keybar[i];
  keybar.clear();

  for (int i = always.size()-1; i>=0; i--)
    delete always[i];
  always.clear();

  for (int k=0; k<MAX_GLVIEWPORTS; k++)
    {
      if (sliders[k]) delete sliders[k];
      sliders[k] = NULL;
      if (faces[k]) delete faces[k];
      faces[k] = NULL;
    }

  switch (game.mode)
    {
    case gm_hexen:
      CreateHexenWidgets();
      break;
    case gm_heretic:
      CreateHereticWidgets();
      break;
    default:
      CreateDoomWidgets();
      break;
    }

  st_refresh = true;
}


void HUD::ST_Drawer(int vp)
{
  int i;

  // is either statusbar or overlay on?
  if (statusbar_on)
    {
      if (rendermode == render_opengl)
	st_refresh = true; // draw always

      // after ST_Start(), screen refresh needed, or vid mode change
      if (st_refresh)
        {
          // draw status bar background to off-screen buff
          ST_RefreshBackground();
        }

      for (i = always.size()-1; i>=0; i--)
	always[i]->Update(true);

      if (game.mode < gm_heretic)
	faces[vp]->Update(st_refresh);
      else
	sliders[vp]->Update(st_refresh);

      for (i = statusbar.size()-1; i>=0; i--)
	statusbar[i]->Update(st_refresh);

      if (mainbar_on)
	for (i = mainbar.size()-1; i>=0; i--)
	  mainbar[i]->Update(st_refresh);
      else if (automap.active)
	for (i = keybar.size()-1; i>=0; i--)
	  keybar[i]->Update(st_refresh);
    }
  else if (overlay_on)
    {
      if (!drawscore || cv_splitscreen.value)
	for (i = overlay.size()-1; i>=0; i--)
	  overlay[i]->Update(true);
    }
}



// stops the status bar
void HUD::ST_Stop()
{
  if (!st_active)
    return;

  vid.SetPalette(0);
  st_active = false;
}


// activates status bar
void HUD::ST_Start()
{
  if (st_active)
    ST_Stop();

  st_palette = -1;
  st_active = true;
}



//=========================================================================
//                         STATUS BAR OVERLAY
//=========================================================================


// recreates the overlay widget set based on the consvar
void HUD::CreateOverlayWidgets()
{
  const char *cmds = cv_stbaroverlay.str;
  HudWidget *h;

  for (int i = overlay.size()-1; i>=0; i--)
    delete overlay[i];
  overlay.clear();

  for (char c = *cmds++; c; c = *cmds++)
    {
      if (c >= 'A' && c <= 'Z')
        c = c + 'a' - 'A';

      switch (c)
        {
        case 'i': // inventory
          h = new HudInventory(st_x+34, st_y+9, &invopen, &st_itemuse, st_invslots, &st_curpos,
                               PatchSNum, PatchARTI, Patch_InvBar);
          overlay.push_back(h);
          break;

        case 'h': // draw health
          h = new HudNumber(st_x+70, st_y+14, 3, &st_health, PatchBNum);
          overlay.push_back(h);
          h = new HudBinIcon(st_x+72, st_y+14, &st_true, NULL, sbohealth);
          overlay.push_back(h);
          break;

        case 'a': // draw ammo
          h = new HudNumber(st_x+170, st_y+14, 3, &st_readywp_ammo, PatchBNum);
          overlay.push_back(h);
          h = new HudMultIcon(st_x+172, st_y+14, &st_atype, PatchAmmoPic);
          overlay.push_back(h);
          break;

	case 'm': // draw armor
	  h = new HudNumber(st_x+270, st_y+14, 3, &st_armor, PatchBNum);
	  overlay.push_back(h);
	  h = new HudBinIcon(st_x+272, st_y+14, &st_true, NULL, sboarmor);
	  overlay.push_back(h);
	  break;

        case 'k': // draw keys
          for (int i=0; i<6; i++)
            {
              h = new HudMultIcon(st_x+308-(i/3)*10, st_y+22-(i%3)*10, &st_keyboxes[i+11], PatchKeys);
              overlay.push_back(h);
            }
          for (int i=0; i<11; i++)
            {
              h = new HudMultIcon(st_x+4+(i/6)*296, 25+(i%6)*30, &st_keyboxes[i], PatchKeys);
              overlay.push_back(h);
            }
          break;

        case 'f': // draw frags
          h = new HudNumber(st_x+300, 2, 3, &st_fragscount, PatchBNum);
          overlay.push_back(h);
          h = new HudBinIcon(st_x+302, 2, &st_true, NULL, sbofrags);
          overlay.push_back(h);
          break;


        default:
          break;
           /*
             //TODO
        case 'e': // number of monster killed
          if ((!cv_deathmatch.value) && (!cv_splitscreen.value))
            {
              char buf[16];
              sprintf(buf, "%d/%d", 0, 0); // FIXME! should be sbpawn.kills, map.kills
              V_DrawString(SCX(318-V_StringWidth(buf)), SCY(1), V_NOSCALESTART, buf);

            }
          break;

        case 's': // number of secrets found
          if ((!cv_deathmatch.value) && (!cv_splitscreen.value))
            {
              char buf[16];
              sprintf(buf, "%d/%d", 0, 0); // FIXME! should be sbpawn., map.secrets
              V_DrawString(SCX(318-V_StringWidth(buf)), SCY(11), V_NOSCALESTART, buf);
            }
          break;
          case 'r': // current frame rate
          {
          char buf[8];
          int framerate = 35;
          sprintf(buf, "%d FPS", framerate);
          V_DrawString(SCX(2), SCY(4), V_NOSCALESTART, buf);
          }
          break;
          */
        }
    }
}
