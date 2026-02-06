// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_sprite.cpp 533 2009-06-11 08:31:10Z smite-meister $
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
/// \brief Sprites and sprite presentations.

#include "doomdef.h"

#include "g_actor.h"
#include "g_decorate.h"

#include "r_data.h"
#include "r_sprite.h"
#include "r_things.h"
#include "r_render.h"
#include "r_draw.h"
#include "i_video.h"
#include "hardware/oglrenderer.hpp"

#include "info.h"
#include "tables.h"
#include "w_wad.h"
#include "z_zone.h"
#include "st_lib.h"

#include "hardware/md3.h"

//==========================================================================
//                         Global functions
//==========================================================================

static void R_InitSkins();

/// Initialize sprite system. Called at program start.
void R_InitSprites()
{
  for (int i=0 ; i<MAXVIDWIDTH ; i++)
    negonearray[i] = -1;

  sprites.SetDefaultItem("SMOK");
#ifdef TEST_MD3
  models.SetDefaultItem("models/players/imp/");
#endif
  //MD3_InitNormLookup();

  // now check for sprite skins
  R_InitSkins();
}



//==========================================================================
//                              SKINS CODE
//==========================================================================


// don't work because it must be inistilised before the config load
//#define SKINVALUES
#ifdef SKINVALUES
CV_PossibleValue_t skin_cons_t[MAXSKINS+1];
#endif

skin_t::skin_t(const char *n, const char *spritename, const char *fprefix)
  : cacheitem_t(n)
{
  // make sure base face name is no more than 3 chars
  strncpy(faceprefix, fprefix, 3);
  faceprefix[3] = '\0';

  /*
  // FIXME skin sounds must be stored with the skins, not in S_Sfx
    for (int i=0;i<sfx_freeslot0;i++)
        if (S_sfx[i].skinsound!=-1)
        {
            skin->soundsid[S_sfx[i].skinsound] = i;
        }
  */
  sprite = sprites.Get(spritename);

  char lumpname[9];
  strcpy(lumpname, faceprefix);  // copy base name
  char *p = lumpname;
  while (*p > ' ') p++;

  // face states
  int facenum = 0;
  for (int i=0; i<ST_NUMPAINFACES; i++)
    {
      for (int j=0; j<ST_NUMSTRAIGHTFACES; j++)
        {
          sprintf(p, "ST%d%d", i, j);
          faces[facenum++] = materials.Get(lumpname);
        }
      sprintf(p, "TR%d0", i);        // turn right
      faces[facenum++] = materials.Get(lumpname);
      sprintf(p, "TL%d0", i);        // turn left
      faces[facenum++] = materials.Get(lumpname);
      sprintf(p, "OUCH%d", i);       // ouch!
      faces[facenum++] = materials.Get(lumpname);
      sprintf(p, "EVL%d", i);        // evil grin ;)
      faces[facenum++] = materials.Get(lumpname);
      sprintf(p, "KILL%d", i);       // pissed off
      faces[facenum++] = materials.Get(lumpname);
    }
  strcpy (p, "GOD0");
  faces[facenum++] = materials.Get(lumpname);
  strcpy (p, "DEAD0");
  faces[facenum++] = materials.Get(lumpname);

  // face backgrounds for different player colors
  //added:08-02-98: uses only STFB0, which is remapped to the right
  //                colors using the player translation tables, so if
  //                you add new player colors, it is automatically
  //                used for the statusbar.
  strcpy(p, "B0");
  int i = fc.FindNumForName(lumpname);
  if (i != -1)
    faceback = materials.GetLumpnum(i);
  else
    faceback = materials.Get("STFB0");
}


skin_t::~skin_t()
{
  sprite->Release();

  for (int i=0; i<SKIN_NUMFACES; i++)
    faces[i]->Release();

  faceback->Release();
}

static skin_t *the_skin; // TEMP

//
// Initialize the basic skins
//
void R_InitSkins()
{
#ifdef SKINVALUES
  for(i=0;i<=MAXSKINS;i++)
    {
      skin_cons_t[i].value=0;
      skin_cons_t[i].strvalue=NULL;
    }

  skin_cons_t[0].strvalue=skins[0].name;
#endif

  // make the standard Doom2 marine as the default skin
  the_skin = new skin_t("default", "PLAY", "STF");

  int nwads = fc.Size();
  for (int i=0; i<nwads; i++)
    R_AddSkins(i);
}


skin_t *GetSkin()
{
  return the_skin;
}

// network code calls this when a 'skin change' is received
void SetPlayerSkin(int playernum, char *skinname)
{
  /*
  int   i;

    // FIXME!
    for (i=0;i<numskins;i++)
    {
        // search in the skin list
        if (strcasecmp(skins[i].name,skinname)==0)
        {

            // change the face graphics
          // FIXME this should be done in HUD, not here.
            if (playernum == hud.sbpawn->player->number &&
            // for save time test it there is a real change
                strcmp (skins[players[playernum].skin].faceprefix, skins[i].faceprefix) )
            {
                ST_unloadFaceGraphics ();
                ST_loadFaceGraphics (skins[i].faceprefix);
            }

            players[playernum].skin = i;
            if (players[playernum].mo)
                players[playernum].mo->skin = &skins[i];

            return;
        }
    }

    CONS_Printf("Skin %s not found\n",skinname);
    players[playernum].skin = 0;  // not found put the old marine skin

    // a copy of the skin value
    // so that dead body detached from respawning player keeps the skin
    if (players[playernum].mo)
        players[playernum].mo->skin = &skins[0];
  */
}

//
// Add skins from a pwad, each skin preceded by 'S_SKIN' marker
//
// Does the same is in w_wad, but check only for
// the first 6 characters (this is so we can have S_SKIN1, S_SKIN2..
// for wad editors that don't like multiple resources of the same name)
//
// Find skin sprites, sounds & optional status bar face, & add them
//
void R_AddSkins(int wadnum)
{
  int lump = 0;
  int numskins = 0;
  /*

  char*       sprname=NULL;
  //
  // search for all skin markers in pwad
  //

  while ((lump = fc.FindNumForNameFile("S_SKIN", wadnum, lump)) != -1)
    {
      if (++numskins > MAXSKINS)
        {
	  //CONS_Printf ("ignored skin (%d skins maximum)\n",MAXSKINS);
	  lump++;
	  continue;
        }

      // cache skin lump, add NUL to the end
      char *buf  = static_cast<char*>(fc.CacheLumpNum(lump, PU_DAVE, true));

      // set defaults
      Sk_SetDefaultValue(&skins[numskins]);
      sprintf (skins[numskins].name,"skin %d",numskins);

      // parse
      char *token = strtok(buf, "\r\n= ");
      while (token)
        {
          if (token[0]=='/' && token[1]=='/') // skip comments
            {
              token = strtok(NULL, "\r\n"); // skip end of line
              goto next_tokn;               // find the real next token
            }

	  char *value = strtok(NULL, "\r\n= ");
          //            CONS_Printf("token = %s, value = %s",token,value);
          //            CONS_Error("ga");

          if (!value)
            I_Error("R_AddSkins: syntax error in S_SKIN lump# %d in WAD %d\n", lump & 0xFFFF, wadnum);

          if (!strcasecmp(token, "name"))
            {
	      // the skin name must uniquely identify a single skin
	      // I'm lazy so if name is already used I leave the 'skin x'
	      // default skin name set above
	      if (!R_SkinAvailable (value))
                {
		  strncpy (skins[numskins].name, value, SKINNAMESIZE);
		  strlwr (skins[numskins].name);
                }
            }
	  else if (!strcasecmp(token, "face"))
            {
	      strncpy (skins[numskins].faceprefix, value, 3);
	      skins[numskins].faceprefix[3] = 0;
	      strupr (skins[numskins].faceprefix);
            }
	  else if (!strcasecmp(token, "sprite"))
            {
	      sprname = value;
	      strupr(sprname);
            }
	  else
            {
	      int found=false;
	      // copy name of sounds that are remapped for this skin
	      for (i=0;i<sfx_freeslot0;i++)
                {
		  if (!S_sfx[i].name)
		    continue;
		  if (S_sfx[i].skinsound!=-1 &&
		      !strcasecmp(S_sfx[i].name, token+2) )
                    {
		      skins[numskins].soundsid[S_sfx[i].skinsound]=
			S_AddSoundFx(value+2,S_sfx[i].singularity);
		      found=true;
                    }
                }
	      if(!found)
		I_Error("R_AddSkins: Unknown keyword '%s' in S_SKIN lump# %d (WAD %i)\n",token,lumpnum&0xFFFF, wadnum);
            }
	next_token:
	  token = strtok (NULL,"\r\n= ");
        }

      // if no sprite defined use spirte juste after this one
      if( !sprname )
        {
            lumpnum &= 0xFFFF;      // get rid of wad number
            lumpnum++;
            lumpinfo = fc.GetLumpinfo(wadnum);

            // get the base name of this skin's sprite (4 chars)
            sprname = lumpinfo[lumpnum].name;
            int intname = *(int *)sprname;

            // skip to end of this skin's frames
            lastlump = lumpnum;
            while (*(int *)lumpinfo[lastlump].name == intname)
                lastlump++;
            // allocate (or replace) sprite frames, and set spritedef
            R_AddSingleSpriteDef (sprname, &skins[numskins].spritedef, wadnum, lumpnum, lastlump);
        }
        else
        {
            // search in the normal sprite tables
            char **name;
            bool found = false;
            for(name = sprnames;*name;name++)
                if( strcmp(*name, sprname) == 0 )
                {
                    found = true;
                    skins[numskins].spritedef = sprites[sprnames-name];
                }

            // not found so make a new one
            if( !found )
                R_AddSingleSpriteDef (sprname, &skins[numskins].spritedef, wadnum, 0, MAXINT);
        }

        CONS_Printf ("added skin '%s'\n", skins[numskins].name);
#ifdef SKINVALUES
        skin_cons_t[numskins].value=numskins;
        skin_cons_t[numskins].strvalue=skins[numskins].name;
#endif

        numskins++;
        free(buf);
    }
    return;
  */
}


//==========================================================================
//                              sprite_t
//==========================================================================


sprite_t::sprite_t(const char *n)
  : cacheitem_t(n)
{
  iname = numframes = 0;
  spriteframes = NULL;
}

sprite_t::~sprite_t()
{
  /*
  for (int i = 0; i < numframes; i++)
    {
      spriteframe_t *frame = &spriteframes[i];
      // TODO Release() the textures of each frame...
    }
  */
  if (spriteframes)
    Z_Free(spriteframes);
}


//==========================================================================
//                           spritecache_t
//==========================================================================

spritecache_t sprites;


// used when building a sprite from lumps
static spriteframe_t sprtemp[29];
static int maxframe;

static void R_InstallSpriteLump(const char *name, int frame, int rot, bool flip)
{
  // uses sprtemp array, maxframe
  if (frame >= 29 || rot > 8)
    I_Error("R_InstallSpriteLump: Bad frame characters in lump %s", name);

  if (frame > maxframe)
    maxframe = frame;

  Material *mat = materials.Get(name, TEX_sprite);

  if (rot == 0)
    {
      // the lump should be used for all rotations
      if (sprtemp[frame].rotate == 0 && devparm)
        CONS_Printf ("R_InitSprites: Sprite %s frame %c has "
                     "multiple rot=0 lump\n", name, 'A'+frame);

      if (sprtemp[frame].rotate == 1 && devparm)
        CONS_Printf ("R_InitSprites: Sprite %s frame %c has rotations "
                     "and a rot=0 lump\n", name, 'A'+frame);

      sprtemp[frame].rotate = 0;
      for (int r = 0; r < 8; r++)
        {
          sprtemp[frame].tex[r] = mat;
          sprtemp[frame].flip[r] = flip;
        }
    }
  else
    {
      // the lump is only used for one rotation
      if (sprtemp[frame].rotate == 0 && devparm)
        CONS_Printf ("R_InitSprites: Sprite %s frame %c has rotations "
                     "and a rot=0 lump\n", name, 'A'+frame);

      sprtemp[frame].rotate = 1;
      rot--; // make 0 based

      if (sprtemp[frame].tex[rot] != (Material *)(-1) && devparm)
        CONS_Printf ("R_InitSprites: Sprite %s : %c : %c "
                     "has two lumps mapped to it\n",
                     name, 'A'+frame, '1'+rot);

      // lumppat & lumpid are the same for original Doom, but different
      // when using sprites in pwad : the lumppat points the new graphics
      sprtemp[frame].tex[rot] = mat;
      sprtemp[frame].flip[rot] = flip;
    }
}



// We assume that the sprite is in Doom sprite format
sprite_t *spritecache_t::Load(const char *p)
{
  // seeks a S_START lump in each file, adds any
  // consequental lumps starting with p to the sprite

  int i, l;
  int intname = *reinterpret_cast<const Sint32 *>(p);

  memset(sprtemp, -1, sizeof(sprtemp));
  maxframe = -1;

  //CONS_Printf(" Sprite: %s  |  ", p);

  int nwads = fc.Size();
  for (i=0; i<nwads; i++)
    {
      // find the sprites section in this file
      // we need at least the S_END
      // (not really, but for speedup)

      int start = fc.FindNumForNameFile("S_START", i, 0);
      if (start == -1)
        start = fc.FindNumForNameFile("SS_START", i, 0); //deutex compatib.

      if (start == -1)
        start = 0;      // search frames from start of wad (lumpnum low word is 0)
      else
        start++;   // just after S_START

      start &= 0xFFFF;    // 0 based in waddir

      int end = fc.FindNumForNameFile("S_END", i, 0);
      if (end == -1)
        end = fc.FindNumForNameFile("SS_END", i, 0);     //deutex compatib.

      if (end == -1)
        continue; // no S_END, no sprites accepted
      end &= 0xFFFF;

      const char *fullname;

      // scan the lumps, filling in the frames for whatever is found
      l = fc.FindPartialName(intname, i, start, &fullname);
      while (l != -1 && l < end)
        {
          int lump = (i << 16) + l;
          int frame = fullname[4] - 'A';
          int rotation = fullname[5] - '0';

          // skip NULL sprites from very old dmadds pwads
          if (fc.LumpLength(lump) <= 8)
            continue;

          const char *name = fc.FindNameForNum(lump);

          R_InstallSpriteLump(name, frame, rotation, false);
          if (fullname[6])
            {
              frame = fullname[6] - 'A';
              rotation = fullname[7] - '0';
              R_InstallSpriteLump(name, frame, rotation, true);
            }

          l = fc.FindPartialName(intname, i, l+1, &fullname);
        }
    }

  // if no frames found for this sprite
  if (maxframe == -1)
    return NULL;

  maxframe++;

  //  some checks to help development
  for (i = 0; i < maxframe; i++)
    switch (sprtemp[i].rotate)
      {
      case -1:
        // no rotations were found for that frame at all
        I_Error("No patches found for sprite %s frame %c", p, i+'A');
        break;

      case 0:
        // only the first rotation is needed
        break;

      case 1:
        // must have all 8 frames
        for (l = 0; l < 8; l++)
          // we test the patch lump, or the id lump whatever
          // if it was not loaded the two are -1
          if (sprtemp[i].tex[l] == (Material *)(-1))
            I_Error("Sprite %s frame %c is missing rotations", p, i+'A');
        break;
      }

  sprite_t *t = new sprite_t(p);

  // allocate this sprite's frames
  t->iname = intname;
  t->numframes = maxframe;
  t->spriteframes = (spriteframe_t *)Z_Malloc(maxframe*sizeof(spriteframe_t), PU_STATIC, NULL);
  memcpy(t->spriteframes, sprtemp, maxframe*sizeof(spriteframe_t));

  //CONS_Printf("frames = %d\n", maxframe);

  return t;
}



/*

//
// Search for sprites replacements in a wad whose names are in namelist
//
static void R_AddSpriteDefs (char** namelist, int wadnum)
{
  int         i;

  // find the sprites section in this pwad
  // we need at least the S_END
  // (not really, but for speedup)

  int start = fc.FindNumForNamePwad("S_START", wadnum, 0);
  if (start == -1)
    start = fc.FindNumForNamePwad("SS_START", wadnum, 0); //deutex compatib.

  if (start == -1)
    start = 0;      // search frames from start of wad (lumpnum low word is 0)
  else
    start++;   // just after S_START

  start &= 0xFFFF;    // 0 based in lumpinfo

  int end = fc.FindNumForNamePwad("S_END",wadnum,0);
  if (end == -1)
    end = fc.FindNumForNamePwad("SS_END",wadnum,0);     //deutex compatib.

  if (end == -1)
    {
      if (devparm)
        CONS_Printf ("no sprites in pwad %d\n", wadnum);
      return;
      //I_Error ("R_AddSpriteDefs: S_END, or SS_END missing for sprites "
      //         "in pwad %d\n",wadnum);
    }
  end &= 0xFFFF;

  //
  // scan through lumps, for each sprite, find all the sprite frames
  //
  int addsprites = 0;
  for (i=0 ; i<numsprites ; i++)
    {
      spritename = namelist[i];

      if (R_AddSingleSpriteDef (spritename, &sprites[i], wadnum, start, end) )
        {
          // if a new sprite was added (not just replaced)
          addsprites++;
          if (devparm)
            CONS_Printf ("sprite %s set in pwad %d\n", namelist[i], wadnum);//Fab
        }
    }

  CONS_Printf ("%d sprites added from file %d\n", addsprites, wadnum);//Fab
  //CONS_Error ("press enter\n");
}
*/



//=================================================================
//                      presentation_t 
//=================================================================


void *presentation_t::operator new(size_t size)
{
  return Z_Malloc(size, PU_LEVSPEC, NULL); // same tag as with thinkers
}


void presentation_t::operator delete(void *mem)
{
  Z_Free(mem);
}


void spritepres_t::Pack(BitStream *s)
{
  s->writeInt(info->GetMobjType(), 16); // DActorInfo defines the spritepres
  s->writeInt(color, 8);
  PackAnim(s);
  // flags and lastupdate need not be sent
}

/// replaces Unpack()
spritepres_t::spritepres_t(BitStream *s)
{
  int temp = s->readInt(16);
  info = aid[mobjtype_t(temp)];
  color = s->readInt(8);

  spr = NULL;
  UnpackAnim(s);
}


void spritepres_t::PackAnim(BitStream *s)
{
  s->writeInt(animseq, 8);
  s->writeInt(state - states, 16);
}


void spritepres_t::UnpackAnim(BitStream *s)
{
  animseq = animseq_e(s->readInt(8));
  SetFrame(&states[s->readInt(16)]);
}


spritepres_t::spritepres_t(const ActorInfo *inf, int col)
{
  info = inf;
  spr = NULL;
  color = col;
  animseq = Idle;

  if (info && info->spawnstate)
    SetFrame(info->spawnstate); // corresponds to Idle animation
  else
    {
      state = &states[S_NULL]; // SetFrame or SetAnim fixes this
      flags = 0;
      lastupdate = -1;
    }
}


spritepres_t::~spritepres_t()
{
  if (spr)
    spr->Release();
}


/// This function does all the necessary frame changes for DActors and other state machines
/// (but what about 3d models, which have no frames???)
void spritepres_t::SetFrame(const state_t *st)
{
  // FIXME for now the name of SPR_NONE is "NONE", fix it when we have the default sprite

  // some sprites change name during animation (!!!)
  const char *name = spritenames[st->sprite];

  if (!spr || spr->iname != *reinterpret_cast<const Sint32 *>(name))
    {
      if (spr)
	spr->Release();
      spr = sprites.Get(name);
    }

  state = st;
  flags = st->frame & ~TFF_FRAMEMASK; // the low 15 bits are wasted, but so what
  lastupdate = -1; // hack, since it will be Updated on this same tic TODO lastupdate should be set to nowtic
}


/// This is needed for PlayerPawns.
void spritepres_t::SetAnim(animseq_e seq)
{
  /*
  const state_t *st;

  if (animseq == seq)
    return; // do not interrupt the animation

  animseq = seq;
  switch (seq)
    {
    case Idle:
    default:
      st = info->spawnstate;
      break;

    case Run:
      st = info->seestate;
      break;

    case Pain:
      st = info->painstate;
      break;

    case Melee:
      st = info->meleestate;
      if (st)
        break;
      // if no melee anim, fallthrough to shoot anim

    case Shoot:
      st = info->missilestate;
      break;

    case Death1:
      st = info->deathstate;
      break;

    case Death2:
      st = info->xdeathstate;
      break;

    case Death3:
      st = info->crashstate;
      break;

    case Raise:
      st = info->raisestate;
      break;
    }

  SetFrame(st);
  */
}


bool spritepres_t::Update(int advance)
{
  // TODO replace "tics elapsed since last update" with "nowtic"...
  // the idea is to keep the count inside the presentation, not outside
  advance += lastupdate; // how many tics to advance, lastupdate is the remainder from last update

  const state_t *st = state;
  while (st->tics >= 0 && advance >= st->tics)
    {
      advance -= st->tics;
      st = st->nextstate;
    }

  if (st != state)
    SetFrame(st);

  lastupdate = advance;

  if (state == info->spawnstate)
    animseq = Idle; // another HACK, since higher animations often end up here

  return true;
}


spriteframe_t *spritepres_t::GetFrame()
{
  return &spr->spriteframes[state->frame && TFF_FRAMEMASK];
}



bool spritepres_t::Draw(const Actor *p)
{
  int frame = state->frame & TFF_FRAMEMASK;

  if (frame >= spr->numframes)
    {
      frame = spr->numframes - 1; // Thing may require more frames than the defaultsprite has...
      //I_Error("spritepres_t::Project: invalid sprite frame %d (%d)\n", frame, spr->numframes);
    }
    
  spriteframe_t *sprframe = &spr->spriteframes[frame];

  Material *mat;
  bool flip;

  // decide which patch to use for sprite relative to player
  if (sprframe->rotate)
    {
      // choose a different rotation based on player view
      angle_t ang = R_PointToAngle2(fixed_t(oglrenderer->x), fixed_t(oglrenderer->y), p->pos.x, p->pos.y);
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

  // TODO color translation maps, TFF_FULLBRIGHT
  int flags = OGLRenderer::BLEND_CONST;
  float alpha = 1.0;
  if (state->frame & TFF_TRANSMASK)
    {
      const float convert[] = {1, 0.5, 0.2, 0.1, 0.5, 1}; // HACK, not perfect
      int tr_table = (state->frame & TFF_TRANSMASK) >> TFF_TRANSSHIFT;
      alpha = convert[tr_table];

      if (tr_table >= tr_transfir)
	flags = OGLRenderer::BLEND_ADD;
    }
  else if (state->frame & TFF_SMOKESHADE)
    alpha = 0.5;
  else if (p->flags & (MF_SHADOW | MF_ALTSHADOW))
    alpha = 0.1; // TODO ALTSHADOW reverses src and dest (transposes the transmap)

  if (flip)
    flags |= OGLRenderer::FLIP_X;

  // hardware renderer part
  oglrenderer->DrawSpriteItem(p->pos, mat, flags, alpha);
  return true;
}
