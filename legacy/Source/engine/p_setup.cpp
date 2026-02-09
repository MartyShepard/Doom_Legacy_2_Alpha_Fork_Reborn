// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: p_setup.cpp 570 2009-11-27 23:57:47Z smite-meister $
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
/// \brief Map loading and setup
///
/// Loads the map lumps from the WAD, sets up the runtime structures,
/// spawns static Thinkers and the initial Actors.

#include <math.h>
#include <string.h>

#include "doomdef.h"
#include "doomdata.h"
#include "command.h"
#include "console.h"
#include "console_log.h" // für CONS_Printf
#include "cvars.h"

#include "g_game.h"
#include "g_player.h"
#include "g_pawn.h"
#include "g_map.h"
#include "g_blockmap.h"
#include "g_mapinfo.h"
#include "g_level.h"
#include "g_decorate.h"

#include "p_setup.h"
#include "p_effects.h"
#include "p_spec.h"
#include "p_camera.h"
#include "m_bbox.h"
#include "m_swap.h"
#include "m_misc.h"
#include "m_argv.h"
#include "tables.h"

#include "i_sound.h" //for I_PlayCD()..

#include "r_render.h"
#include "r_data.h"
#include "r_main.h"
#include "r_sky.h"

#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"
#include "r_splats.h"

#include "t_parse.h"

#include "i_video.h"            //rendermode


void ConvertLineDef(line_t *ld);

static Material *skyflat_mat;

extern vector<mapthing_t *> polyspawn;  // for spawning polyobjects
extern vector<mapthing_t *> polyanchor; // for spawning polyobjects


/// Editor numbers for certain special mapthings
enum
{
  EN_START1       =    1, ///< normal playerstarts (1-4)
  EN_START4       =    4,
  EN_START5       = 4001, ///< extra playerstarts (5-32)
  EN_START32      = 4028,
  EN_DM_START     =   11, ///< deathmatch start

  EN_DOOM_BRAINTARGET     =   87, ///< Boss Brain spawncube target spot

  EN_HERETIC_BOSSSPOT     =   56, ///< D'Sparil teleport spot
  EN_HERETIC_AMBIENTSND1  = 1200, ///< ambient sound spawners
  EN_HERETIC_AMBIENTSND10 = 1209,
  EN_HERETIC_MACESPOT     = 2002, ///< Firemace spot

  EN_HEXEN_SNDSEQ1        = 1400, ///< sector sound sequences
  EN_HEXEN_SNDSEQ10       = 1409,
  EN_HEXEN_PO_ANCHOR      = 3000, ///< polyobjects
  EN_HEXEN_PO_SPAWN       = 3001,
  EN_HEXEN_PO_SPAWNCRUSH  = 3002,
  EN_HEXEN_START5         = 9100, ///< extra playerstarts (5-8)
  EN_HEXEN_START8         = 9103,
};



void Map::LoadVertexes(int lump)
{
  // Determine number of lumps: total lump length / vertex record length.
  numvertexes = fc.LumpLength(lump) / sizeof(mapvertex_t);
  //CONS_Printf("vertices: %d, ", numvertexes);

  // Allocate zone memory for buffer.
  vertexes = (vertex_t*)Z_Malloc(numvertexes*sizeof(vertex_t), PU_LEVEL, 0);

  // Load data into cache.
  byte *data = (byte *)fc.CacheLumpNum(lump, PU_STATIC);

  mapvertex_t *mv = (mapvertex_t *)data;
  vertex_t *v = vertexes;
  root_bbox.Clear(); // we also build the root bounding box here

  // Copy and convert vertex coordinates,
  // internal representation as fixed.
  for (int i=0 ; i<numvertexes ; i++, v++, mv++)
    {
      v->x = SHORT(mv->x);
      v->y = SHORT(mv->y);
      root_bbox.Add(v->x, v->y);
    }

  // Free buffer memory.
  Z_Free(data);
}



void Map::LoadSegs(int lump)
{
  numsegs = fc.LumpLength(lump) / sizeof(mapseg_t);
  segs = (seg_t *)Z_Malloc(numsegs*sizeof(seg_t), PU_LEVEL, NULL);
  memset(segs, 0, numsegs*sizeof(seg_t)); // clear everything
  byte *data = (byte *)fc.CacheLumpNum(lump, PU_STATIC);

  mapseg_t *ms = (mapseg_t *)data;
  seg_t    *seg = segs;
  for (int i = 0; i < numsegs; i++, seg++, ms++)
    {
      seg->v1 = &vertexes[SHORT(ms->v1)];
      seg->v2 = &vertexes[SHORT(ms->v2)];

      line_t *ldef = &lines[SHORT(ms->linedef)];
      seg->linedef = ldef;
      seg->side = SHORT(ms->side);
      // NOTE: partner_seg is NULL, length is 0 (not needed in software)

      seg->sidedef = ldef->sideptr[seg->side];
      seg->offset = SHORT(ms->offset);
      seg->angle = SHORT(ms->angle) << 16;

      seg->frontsector = ldef->sideptr[seg->side]->sector;
      seg->backsector = (ldef->flags & ML_TWOSIDED) ? ldef->sideptr[seg->side^1]->sector : NULL;
    }

  Z_Free(data);
}



void Map::LoadSubsectors(int lump)
{
  numsubsectors = fc.LumpLength(lump) / sizeof(mapsubsector_t);
  subsectors = (subsector_t *)Z_Malloc(numsubsectors * sizeof(subsector_t), PU_LEVEL, 0);
  memset(subsectors, 0, numsubsectors * sizeof(subsector_t));
  byte *data = (byte *)fc.CacheLumpNum(lump, PU_STATIC);

  mapsubsector_t *mss = (mapsubsector_t *)data;
  subsector_t *ss = subsectors;

  for (int i=0; i<numsubsectors; i++, ss++, mss++)
    {
      ss->num_segs = SHORT(mss->numsegs);
      ss->first_seg = SHORT(mss->firstseg);
    }

  Z_Free(data);
}



void Map::LoadSectors1(int lump)
{
  // allocate and zero the sectors
  numsectors = fc.LumpLength(lump) / sizeof(mapsector_t);
  sectors = (sector_t *)Z_Malloc(numsectors*sizeof(sector_t), PU_LEVEL, 0);
  memset(sectors, 0, numsectors*sizeof(sector_t));
}



void Map::LoadSectors2(int lump)
{
  byte *data = (byte *)fc.CacheLumpNum(lump, PU_STATIC);
  // NOTE: The sector_t structs have been initialized with zeroes in LoadSectors1()

  mapsector_t *ms = (mapsector_t *)data;
  sector_t *ss = sectors;
  for (int i=0; i < numsectors; i++, ss++, ms++)
    {
      ss->floorheight = SHORT(ms->floorheight);
      ss->ceilingheight = SHORT(ms->ceilingheight);

      ss->floorpic = materials.Get8char(ms->floorpic, TEX_floor);
      ss->ceilingpic = materials.Get8char(ms->ceilingpic, TEX_floor);

      // avoid dummy sky flats (when the Material of a surface is NULL, use fake backdrop)
      if (ss->floorpic == skyflat_mat)
	ss->floorpic = NULL;
      if (ss->ceilingpic == skyflat_mat)
	ss->ceilingpic = NULL;

      ss->lightlevel = SHORT(ms->lightlevel);

      ss->tag = SHORT(ms->tag);

      ss->thinglist = NULL;
      ss->touching_thinglist = NULL; //SoM: 4/7/2000

      ss->stairlock = 0;
      ss->nextsec = -1;
      ss->prevsec = -1;

      ss->heightsec = -1; //SoM: 3/17/2000: This causes some real problems
      ss->heightsec_type = sector_t::CS_boom;
      ss->floorlightsec = -1;
      ss->ceilinglightsec = -1;
      ss->ffloors = NULL;
      ss->lightlist = NULL;
      ss->numlights = 0;
      ss->attached = NULL;
      ss->numattached = 0;
      ss->moved = true;
      ss->floor_xoffs = ss->ceiling_xoffs = ss->floor_yoffs = ss->ceiling_yoffs = 0;

      // ----- for special tricks with HW renderer -----
      ss->pseudoSector = false;
      ss->virtualFloor = false;
      ss->virtualCeiling = false;
      ss->sectorLines = NULL;
      ss->stackList = NULL;
      ss->lineoutLength = -1.0;
      // ----- end special tricks -----

      ss->gravity = 1.0f;
      ss->special = SHORT(ms->special);

      // set floortype-dependent attributes
      ss->SetFloorType(ms->floorpic);
    }

  Z_Free(data);

  // whoa! there is usually no more than 25 different flats used per level!!
  //CONS_Printf("%d flats found\n", numlevelflats);
}


// BSP 'child is subsector' from original/v2 to v5.
static void ConvertGLFlags16_32(Uint32 &in)
{
  if (in & CHILD_IS_SUBSECTOR_OLD)
    {
      in &= ~CHILD_IS_SUBSECTOR_OLD;
      in |= NF_SUBSECTOR;
    }
}

void Map::LoadNodes(int lump)
{
  numnodes = fc.LumpLength(lump) / sizeof(mapnode_t);
  nodes = (node_t *)Z_Malloc(numnodes*sizeof(node_t),PU_LEVEL,0);
  byte *data = (byte*)fc.CacheLumpNum(lump,PU_STATIC);

  mapnode_t *mn = (mapnode_t *)data;
  node_t *no = nodes;

  for (int i=0 ; i<numnodes ; i++, no++, mn++)
    {
      no->x = SHORT(mn->x);
      no->y = SHORT(mn->y);
      no->dx = SHORT(mn->dx);
      no->dy = SHORT(mn->dy);
      for (int j=0 ; j<2 ; j++)
        {
          no->children[j] = SHORT(mn->children[j]);
	  ConvertGLFlags16_32(no->children[j]); // needed here too (internally we use GLv5 nodes)
          for (int k=0 ; k<4 ; k++)
            no->bbox[j].box[k] = SHORT(mn->bbox[j][k]);
        }
    }

  Z_Free(data);
}



void Map::LoadThings(int lump, bool heed_spawnflags)
{
  TID_map.clear();

  if (hexen_format)
    nummapthings = fc.LumpLength(lump)/sizeof(hex_mapthing_t);
  else
    nummapthings = fc.LumpLength(lump)/sizeof(doom_mapthing_t);

  mapthings    = static_cast<mapthing_t*>(Z_Malloc(nummapthings*sizeof(mapthing_t), PU_LEVEL, NULL));
  // this memset is crucial, it initializes the mapthings to all zeroes.
  memset(mapthings, 0, nummapthings*sizeof(mapthing_t));

  NumPolyobjs  = 0;

  char *data   = static_cast<char*>(fc.CacheLumpNum(lump, PU_STATIC));

  doom_mapthing_t *mt = reinterpret_cast<doom_mapthing_t*>(data);
  hex_mapthing_t  *ht = reinterpret_cast<hex_mapthing_t *>(data);

  mapthing_t *t = mapthings;

  // flag checks
  int ffail  = 0;
  int fskill;
  int fmode  = -1; // all bits on
  int fclass = -1;

  // check skill
  if (game.skill == sk_baby)
    fskill = 1;
  else if (game.skill == sk_nightmare)
    fskill = 4;
  else
    fskill = 1 << (game.skill-1);

  if (hexen_format)
    {
      // Hexen flags
      if (!game.multiplayer)
        fmode = MTF_GSINGLE;
      else if (cv_deathmatch.value)
        fmode = MTF_GDEATHMATCH;
      else
        fmode = MTF_GCOOP;

      // If there are _any_ clerics in the game, MTF_CLERIC stuff should be spawned etc.
      //fclass = (MTF_FIGHTER | MTF_CLERIC | MTF_MAGE);
      fclass = 0;
      for (GameInfo::player_iter_t k = game.Players.begin(); k != game.Players.end(); k++)
        switch (pawn_aid[k->second->options.ptype]->pclass)
          {
          case PCLASS_FIGHTER:
            fclass |= MTF_FIGHTER;
            break;
          case PCLASS_CLERIC:
            fclass |= MTF_CLERIC;
            break;
          case PCLASS_MAGE:
            fclass |= MTF_MAGE;
            break;
          default:
            fclass |= (MTF_FIGHTER | MTF_CLERIC | MTF_MAGE);
            break;
          }
    }
  else
    {
      // Doom / Boom flags
      // multiplayer only thing flag
      // "not deathmatch"/"not coop" thing flags
      if (!game.multiplayer)
        ffail |= MTF_MULTIPLAYER;
      else if (cv_deathmatch.value)
        ffail |= MTF_NOT_IN_DM;
      else
        ffail |= MTF_NOT_IN_COOP;
    }


  int i, ednum;
  for (i=0 ; i<nummapthings ; i++, t++)
    {
      if (hexen_format)
        {
          t->tid = SHORT(ht->tid);
          t->x = SHORT(ht->x);
          t->y = SHORT(ht->y);
          t->height = SHORT(ht->height);
          t->angle  = SHORT(ht->angle);
          ednum     = SHORT(ht->type);
          t->flags  = SHORT(ht->flags);

          t->special = ht->special;
          for (int j=0; j<5; j++)
            t->args[j] = ht->args[j];
          ht++;
        }
      else
        {
          t->x = SHORT(mt->x);
          t->y = SHORT(mt->y);
          t->angle = SHORT(mt->angle);
          ednum    = SHORT(mt->type);
          t->flags = SHORT(mt->flags);
          mt++;
        }


      // convert editor number to mobjtype_t number right now
      if (!ednum)
        continue; // Ignore type-0 things as NOPs

      //======== first we spawn some common THING types not affected by spawning flags ========

      // deathmatch start positions
      if (ednum == EN_DM_START)
        {
          if (dmstarts.size() < MAX_DM_STARTS)
            dmstarts.push_back(t);
          continue;
        }

      // normal playerstarts (normal 4 + 28 extra)
      if ((ednum >= EN_START1 && ednum <= EN_START4) || (ednum >= EN_START5 && ednum <= EN_START32))
        {
          if (ednum >= EN_START5)
            ednum += 5 - EN_START5;

          playerstarts.insert(pair<int, mapthing_t *>(ednum, t));

#warning FIXME TEST
	  t->tid = -100; // respawn timer
          continue;
        }

      // common polyobjects
      if (ednum == EN_PO_ANCHOR || ednum == EN_PO_SPAWN || ednum == EN_PO_SPAWNCRUSH)
	{
	  t->height = ednum; // HACK, internally we use the same numbers
	  if (ednum == EN_PO_ANCHOR)
	    polyanchor.push_back(t);
	  else
	    polyspawn.push_back(t);

	  continue;
	}

      //======== then game-specific things not affected by spawning flags ========

      if (game.mode <= gm_doom2) // Doom
        {
          // DoomII braintarget list
          if (ednum == EN_DOOM_BRAINTARGET)
            braintargets.push_back(t);
	  // NOTE: also spawned
        }
      else if (game.mode == gm_heretic) // Heretic
        {
          // Ambient sound sequences
          if (ednum >= EN_HERETIC_AMBIENTSND1 && ednum <= EN_HERETIC_AMBIENTSND10)
            {
              AmbientSeqs.push_back(ednum - EN_HERETIC_AMBIENTSND1);
              continue;
            }

          // D'Sparil teleport spot (no Actor spawned)
          if (ednum == EN_HERETIC_BOSSSPOT)
            {
              BossSpots.push_back(t);
              continue;
            }

          // Mace spot (no Actor spawned)
          if (ednum == EN_HERETIC_MACESPOT)
            {
              MaceSpots.push_back(t);
              continue;
            }
        }
      else // Hexen
        {
          // The polyobject system is pretty stupid, since the mapthings are not always in
          // any particular order. Polyobjects have to be picked apart
          // from other things  => polyspawn vector
          if (ednum == EN_HEXEN_PO_ANCHOR || ednum == EN_HEXEN_PO_SPAWN || ednum == EN_HEXEN_PO_SPAWNCRUSH)
            {
              t->height = ednum - EN_HEXEN_PO_ANCHOR + EN_PO_ANCHOR; // HACK
              if (ednum == EN_HEXEN_PO_ANCHOR)
		polyanchor.push_back(t);
	      else
		polyspawn.push_back(t);

              continue;
            }

          // Check for player starts 5 to 8
          if (ednum >= EN_HEXEN_START5 && ednum <= EN_HEXEN_START8)
            {
              ednum += 5 - EN_HEXEN_START5;
              playerstarts.insert(pair<int, mapthing_t *>(ednum, t));
              continue;
            }

          // sector sound sequences
          if (ednum >= EN_HEXEN_SNDSEQ1 && ednum <= EN_HEXEN_SNDSEQ10)
            {
              GetSubsector(t->x, t->y)->sector->seqType = ednum - EN_HEXEN_SNDSEQ1;
              continue;
            }
        }

      //======== then some more common things not affected by spawning flags ========

      // find which type to spawn
      ActorInfo *ai = aid.FindDoomEdNum(ednum);
      if (ai && ai->spawn_always)
	{
	  // cameras etc.
	  t->ai = ai;
	  continue; // was found
	}

      // NOTE: Spawning flags don't apply to playerstarts, teleport exits or polyobjs! Why, pray, is that?
      // wrong flags?
      if (heed_spawnflags)
	if ((t->flags & ffail) || !(t->flags & fskill) || !(t->flags & fmode) || !(t->flags & fclass))
	  continue;

      //======== finally things affected by spawning flags ========

      if (ai)
	{
	  t->ai = ai;
	  continue; // was found
	}
      else
	CONS_Printf("\2Map::LoadThings: Unknown DoomEdNum %d at (%d, %d)\n", ednum, t->x, t->y);
    }

  Z_Free(data);
}



void line_t::SetDims()
{
  dx = v2->x - v1->x;
  dy = v2->y - v1->y;

  slopetype = !dx ? ST_VERTICAL :
    !dy ? ST_HORIZONTAL :
    (dy / dx > 0) ? ST_POSITIVE : ST_NEGATIVE;

  //bbox.Add(v1->x, v1->y);
  //bbox.Add(v2->x, v2->y);

  if (v1->x < v2->x)
    {
      bbox.box[BOXLEFT] = v1->x;
      bbox.box[BOXRIGHT] = v2->x;
    }
  else
    {
      bbox.box[BOXLEFT] = v2->x;
      bbox.box[BOXRIGHT] = v1->x;
    }

  if (v1->y < v2->y)
    {
      bbox.box[BOXBOTTOM] = v1->y;
      bbox.box[BOXTOP] = v2->y;
    }
  else
    {
      bbox.box[BOXBOTTOM] = v2->y;
      bbox.box[BOXTOP] = v1->y;
    }
}


void Map::LoadLineDefs(int lump)
{
  vertex_t *v1, *v2;

  if (hexen_format)
    numlines = fc.LumpLength(lump)/sizeof(hex_maplinedef_t);
  else
    numlines = fc.LumpLength(lump)/sizeof(doom_maplinedef_t);

  //CONS_Printf("lines: %d\n", numlines);

  lines = static_cast<line_t*>(Z_Malloc(numlines*sizeof(line_t), PU_LEVEL, 0));
  memset(lines, 0, numlines*sizeof(line_t)); // NOTE: important, initialize fields to zero
  byte *data = static_cast<byte*>(fc.CacheLumpNum(lump, PU_STATIC));

  doom_maplinedef_t *mld = reinterpret_cast<doom_maplinedef_t*>(data);
  hex_maplinedef_t *hld = reinterpret_cast<hex_maplinedef_t*>(data);

  line_t *ld = lines;

  for (int i=0 ; i<numlines ; i++, ld++)
    {
      int temp[2];
      if (hexen_format)
        {
          ld->flags = SHORT(hld->flags);
          ld->special = hld->special;

          for (int j=0; j<5; j++)
            ld->args[j] = hld->args[j];

	  // use 16-bit tags
	  if (ld->special == LINE_LEGACY_EXT)
	    {
	      // NOTE: this Legacy-specific linedef type uses 16-bit tags
	      ld->tag = ld->args[3] + (ld->args[4] << 8); // Hexen style
	    }
	  else
	    ld->tag = ld->args[0];

	  // Set the Hexen line id's (corresponding to Boom line-line pairing tags)
	  switch (ld->special)
	    {
	    case 121: // Line_SetIdentification
	      ld->lineid = ld->args[0] + (ld->args[4] << 8); // high byte is a ZDoom extension
	      ld->special = 0;
	      break;

	    case 215: // ZDoom: Teleport_Line
	    case 222: // ZDoom: Scroll_Texture_Model
	      ld->lineid = ld->args[0];
	      break;

	    default:
	      break;
	    }

          v1 = ld->v1 = &vertexes[SHORT(hld->v1)];
          v2 = ld->v2 = &vertexes[SHORT(hld->v2)];

	  /*
          if (SHORT(hld->v1) > numvertexes)
            CONS_Printf("v1 > numverts: %d\n", SHORT(hld->v1));
          if (SHORT(hld->v2) > numvertexes)
            CONS_Printf("v2 > numverts: %d\n", SHORT(hld->v2));
	  */

	  temp[0] = SHORT(hld->sidenum[0]);
	  temp[1] = SHORT(hld->sidenum[1]);
          hld++;
        }
      else
        {
          ld->flags = SHORT(mld->flags);
          ld->special = SHORT(mld->special);
          ld->tag = SHORT(mld->tag);
	  ld->lineid = ld->tag; // NOTE: in Doom format, lineid's are equivalent to tags, in Hexen they are different.

	  // Doom => Hexen conversion using the conversion table.
	  ConvertLineDef(ld);

          v1 = ld->v1 = &vertexes[SHORT(mld->v1)];
          v2 = ld->v2 = &vertexes[SHORT(mld->v2)];

	  temp[0] = SHORT(mld->sidenum[0]);
	  temp[1] = SHORT(mld->sidenum[1]);
          mld++;
        }

      // index -1 means no sidedef
      ld->sideptr[0] = (temp[0] == NULL_INDEX) ? NULL : &sides[temp[0]];
      ld->sideptr[1] = (temp[1] == NULL_INDEX) ? NULL : &sides[temp[1]];

      ld->SetDims();

      // set line references to the sidedefs
      if (ld->sideptr[0])
        ld->sideptr[0]->line = ld;
      if (ld->sideptr[1])
        ld->sideptr[1]->line = ld;

      ld->transmap = -1; // no transmap by default
    }

  Z_Free(data);
}


void Map::LoadLineDefs2()
{
  line_t* ld = lines;
  for (int i=0; i < numlines; i++, ld++)
    {
      ld->frontsector = ld->sideptr[0] ? ld->sideptr[0]->sector : NULL;
      ld->backsector =  ld->sideptr[1] ? ld->sideptr[1]->sector : NULL;
    }
}


/*
void P_LoadSideDefs(int lump)
  {
  byte*               data;
  int                 i;
  mapsidedef_t*       msd;
  side_t*             sd;

  numsides = fc.LumpLength(lump) / sizeof(mapsidedef_t);
  sides = Z_Malloc(numsides*sizeof(side_t),PU_LEVEL,0);
  memset (sides, 0, numsides*sizeof(side_t));
  data = fc.CacheLumpNum(lump,PU_STATIC);

  msd = (mapsidedef_t *)data;
  sd = sides;
  for (i=0 ; i<numsides ; i++, msd++, sd++)
  {
  sd->textureoffset = SHORT(msd->textureoffset)<<FRACBITS;
  sd->rowoffset = SHORT(msd->rowoffset)<<FRACBITS;
  sd->toptexture = R_TextureNumForName(msd->toptexture);
  sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
  sd->midtexture = R_TextureNumForName(msd->midtexture);

  sd->sector = &sectors[SHORT(msd->sector)];
  }

  Z_Free(data);
  }
*/


void Map::LoadSideDefs(int lump)
{
  numsides = fc.LumpLength(lump) / sizeof(mapsidedef_t);
  sides = (side_t *)Z_Malloc(numsides*sizeof(side_t), PU_LEVEL, 0);
  memset(sides, 0, numsides*sizeof(side_t));
}


void Map::LoadSideDefs2(int lump)
{
  byte *data = (byte *)fc.CacheLumpNum(lump,PU_STATIC);

  for (int i=0; i<numsides; i++)
    {
      mapsidedef_t *msd = (mapsidedef_t *)data + i;
      side_t *sd = sides + i;
      sector_t *sec;

      sd->textureoffset = SHORT(msd->textureoffset);
      sd->rowoffset = SHORT(msd->rowoffset);

      // shorthand
      char *ttex = msd->toptexture;
      char *mtex = msd->midtexture;
      char *btex = msd->bottomtexture;

      // refined to allow colormaps to work as wall
      // textures if invalid as colormaps but valid as textures.

      sd->sector = sec = &sectors[SHORT(msd->sector)];

      // specials where texture names might be something else
      line_t *ld = sd->line;
      if (ld && ld->special == LINE_LEGACY_EXT) // TODO shouldn't all sidedefs have a corresponding linedef?
	{
	  if (ld->args[0] == LINE_LEGACY_EXOTIC_TEXTURE)
	    switch (ld->args[1])
	      {
	      case 1:  // BOOM: 242 fake ceiling/floor, variable colormaps
	      case 3:  // Legacy: swimmable water, colormaps
		sd->toptexture = materials.GetMaterialOrColormap(ttex, sec->topmap);
		sd->midtexture = materials.GetMaterialOrColormap(mtex, sec->midmap);
		sd->bottomtexture = materials.GetMaterialOrColormap(btex, sec->bottommap);
		break;

	      case 2: // BOOM: 260 make middle texture translucent
		sd->toptexture = materials.Get8char(ttex);
		sd->midtexture = materials.GetMaterialOrTransmap(mtex, ld->transmap); // can also be a transmap lumpname
		sd->bottomtexture = materials.Get8char(btex);
		break;

	      case 4: // Legacy: create a colormap
		if (ttex[0] == '#' || btex[0] == '#')
		  {
		    sec->midmap = R_CreateColormap(ttex, mtex, btex);
		    sd->toptexture = sd->bottomtexture = 0;
		    
		    if (rendermode != render_soft) // FIXME is this necessary?
		      sec->extra_colormap = sec->midmap;
		  }
		else
		  {
		    sd->toptexture = materials.Get8char(ttex);
		    sd->midtexture = materials.Get8char(mtex);
		    sd->bottomtexture = materials.Get8char(btex);
		  }
		break;

	      default:
		I_Error("Unknown linedef type extension (%d)\n.", ld->args[1]);
	      }
	  else if (ld->args[0] == LINE_LEGACY_FAKEFLOOR)
	    switch (ld->args[1])
	      {
		// Legacy: alpha value for translucent 3D-floors/water, TODO not good
	      case 19:
	      case 20:
		if (ttex[0] == '#')
		  sd->toptexture = sd->bottomtexture = 0; // FIXME 3D floor translucency ((ttex[1] - '0')*100 + (ttex[2] - '0')*10 + ttex[3] - '0') + 1;
		else
		  sd->toptexture = sd->bottomtexture = 0;
		sd->midtexture = materials.Get8char(mtex);
		break;

	      default:
		goto normal;
	      }
	  else
	    goto normal;
	}
      else
	{
	normal:
	  // normal texture names
          sd->toptexture = materials.Get8char(ttex);
          sd->midtexture = materials.Get8char(mtex);
          sd->bottomtexture = materials.Get8char(btex);
        }
    }

  Z_Free(data);
}



blockmap_t::~blockmap_t()
{
  Z_Free(cells);
  Z_Free(lists);
}

// Load a blockmap from a lump.
blockmap_t::blockmap_t(int lump)
{
  lists = NULL;
  cells = NULL;

  int size = fc.LumpLength(lump)/2;

  Uint16 *blockmap_lump = static_cast<Uint16*>(fc.CacheLumpNum(lump, PU_LEVEL));
  Uint16 *blockmap_index = blockmap_lump + 4; // the offsets array

  // Endianness: everything in blockmap is expressed in 2-byte shorts
  for (int i=0; i < size; i++)
    blockmap_lump[i] = SHORT(blockmap_lump[i]);

  // read the header
  blockmapheader_t *bm = reinterpret_cast<blockmapheader_t*>(blockmap_lump);
  orgx = bm->origin_x;
  orgy = bm->origin_y;
  width = bm->width;
  height = bm->height;

  // init the index
  int numcells = width * height;

  // check the blockmap for errors
  int errors = 0;
  int first = 4 + numcells; // first possible blocklist offset (in shorts)
  int list_size = size - first; // blocklist size

  if (list_size < 2) // one empty blocklist (two shorts) is the minimal size
    {
      Z_Free(blockmap_lump);
      CONS_Printf("Blockmap is corrupted (blocklist is too short).\n");
      throw -1;
    }

  lists = static_cast<Uint16 *>(Z_Malloc(list_size * sizeof(Uint16), PU_LEVEL, 0));
  memcpy(lists, &blockmap_lump[first], 2*list_size); // copy the lists

  // Build a new blockmap index using pointers
  cells = static_cast<blockmapcell_t *>(Z_Malloc(numcells * sizeof(blockmapcell_t), PU_LEVEL, 0));
  memset(cells, 0, numcells * sizeof(blockmapcell_t));

  for (int i=0; i < numcells && errors < 50; i++)
    {
      int offs = blockmap_index[i] - first;
      if (offs < 0)
	{
	  CONS_Printf(" Invalid blocklist offset for cell %d: %d (possible offset overflow).\n",
		      i, blockmap_index[i]);
	  errors++;
	  // TEST: assume that (short) offset has overflowed, fix
	  offs += 0x10000;
	}

      if (offs >= list_size)
	{
	  CONS_Printf(" Cell %d blocklist offset points past the lump!\n", i);
	  errors++;
	  continue;
	}

      // build new blockmap index
      cells[i].blocklist = &lists[offs+1]; // skip the zero marker

      if (lists[offs] != 0x0000)
	{
	  CONS_Printf(" Cell %d blocklist does not start with zero!\n", i);
	  errors++;
	}

      while (offs < list_size && lists[offs] != MAPBLOCK_END)
	offs++;

      if (offs >= list_size)
	{
	  CONS_Printf(" Cell %d blocklist is unterminated!\n", i);
	  errors++;
	  continue;
	}
    }

  Z_Free(blockmap_lump);

  if (errors)
    {
      Z_Free(lists); lists = NULL;
      Z_Free(cells); cells = NULL;
      CONS_Printf("Blockmap (%dx%d cells, %d bytes) had some errors.\n", width, height, 2*size);
      throw -1;
    }
}


// Build a new blockmap
blockmap_t::blockmap_t(Map *mp)
{
  parent_map = mp;

  orgx = mp->root_bbox[BOXLEFT];
  orgy = mp->root_bbox[BOXBOTTOM];
  width = BlockX(mp->root_bbox[BOXRIGHT]) + 1;
  height = BlockY(mp->root_bbox[BOXTOP]) + 1;

  int numcells = width * height;
  CONS_Printf("Generating blockmap (%dx%d blocks)...", width, height);

  vector<Uint16> xxx[numcells];

  for (int i=0; i < mp->numlines; i++)
    {
      line_t *li = &mp->lines[i];
      double x1 = (li->v1->x - orgx).Float() / MAPBLOCKUNITS;
      double y1 = (li->v1->y - orgy).Float() / MAPBLOCKUNITS;
      double x2 = (li->v2->x - orgx).Float() / MAPBLOCKUNITS;
      double y2 = (li->v2->y - orgy).Float() / MAPBLOCKUNITS;

      double dx = x2-x1;
      double dy = y2-y1;

      double x_intercept = x1, y_intercept = y1;

      // split (nonnegative) block coords to integer and fractional parts
      double dummy;
      int bx1 = int(x1); x1 = modf(x1, &dummy);
      int by1 = int(y1); y1 = modf(y1, &dummy);
      int bx2 = int(x2); x2 = modf(x2, &dummy);
      int by2 = int(y2); y2 = modf(y2, &dummy);

      int bdx, bdy;
      double xstep, ystep;

      if (bx2 > bx1)
	{
	  bdx = 1;
	  ystep = dy / fabs(dx);
	  y_intercept += (1 - x1) * ystep;
	}
      else if (bx2 < bx1)
	{
	  bdx = -1;
	  ystep = dy / fabs(dx);
	  y_intercept += x1 * ystep;
	}
      else
	{
	  // vertical run of blocks
	  bdx = 0;
	  ystep = 0;
	  y_intercept += 1000000; // must not hit it ever
	}

      if (by2 > by1)
	{
	  bdy = 1;
	  xstep = dx / fabs(dy);
	  x_intercept += (1 - y1) * xstep;
	}
      else if (by2 < by1)
	{
	  bdy = -1;
	  xstep = dx / fabs(dy);
	  x_intercept += y1 * xstep;
	}
      else
	{
	  // horizontal run of blocks
	  bdy = 0;
	  xstep = 0;
	  x_intercept += 1000000;
	}

      // Step through map blocks.
      // Count is present to prevent a round off error causing us to miss the end block.
      for (int count = abs(bx2-bx1) + abs(by2-by1); ; count--)
	{
	  // store the line number
	  xxx[by1*width + bx1].push_back(i);

	  if (count <= 0)
	    break;

	  // intercepts can not become negative until count has expired
	  if (int(y_intercept) == by1)
	    {
	      y_intercept += ystep;
	      bx1 += bdx;
	    }
	  else if (int(x_intercept) == bx1)
	    {
	      x_intercept += xstep;
	      by1 += bdy;
	    }
	}
    }

  // convert the vector array into old-style blocklists (no packing for now...)

  // find out blocklist size
  int list_size = 1; // "empty block" list (only end marker)
  for (int i=0; i<numcells; i++)
    {
      int temp = xxx[i].size();
      if (temp)
	list_size += temp + 1; // line numbers + end marker
    }

  lists = static_cast<Uint16 *>(Z_Malloc(list_size * sizeof(Uint16), PU_LEVEL, 0));

  // Build a new blockmap index using pointers
  cells = static_cast<blockmapcell_t *>(Z_Malloc(numcells * sizeof(blockmapcell_t), PU_LEVEL, 0));
  memset(cells, 0, numcells * sizeof(blockmapcell_t));

  int idx = 0; // next free slot
  lists[idx++] = MAPBLOCK_END; // "empty block" list
  for (int i=0; i<numcells; i++)
    {
      int temp = xxx[i].size();
      if (!temp)
	cells[i].blocklist = &lists[0];
      else
	{
	  // add a new blocklist
	  cells[i].blocklist = &lists[idx];
	  for (int k=0; k<temp; k++)
	    lists[idx++] = xxx[i][k];
	  lists[idx++] = MAPBLOCK_END;
	}
    }

  if (idx != list_size)
    I_Error("FUCK!\n");

  CONS_Printf("done. %d entries, %d bytes.\n", list_size, 2*(4 + numcells + list_size));
}



void Map::LoadBlockMap(int lump)
{
  int size = fc.LumpLength(lump)/2;

  if (M_CheckParm("-blockmap") ||
      size < 6 || // smallest possible blockmap
      size > 0x10000+1) // largest, always fully addressable blockmap
    blockmap = new blockmap_t(this); // blockmap lump is invalid, we must build our own
  else
    {
      // use the blockmap from the wad
      try
	{
	  blockmap = new blockmap_t(lump);
	  blockmap->parent_map = this;
	}
      catch(int i)
	{
	  CONS_Printf("Building a blockmap from scratch.\n");
	  blockmap = new blockmap_t(this); // blockmap lump is invalid, we must build our own
	}
    }
}


//
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
void Map::GroupLines()
{
  int i, j;

  // Look up sector number for each subsector. This is made more complex by the
  // presence of minisegs from GL-subsectors (they have no associated lines!)
  subsector_t *ss = subsectors;
  for (i=0 ; i<numsubsectors ; i++, ss++)
    {
      if (ss->sector) // This subsector is already processed.
	continue;

      // Go through segs one by one. Stop when any assignation is done.
      for (unsigned csnum = ss->first_seg; csnum < ss->first_seg + ss->num_segs; csnum++)
	{
	  seg_t *seg = &segs[csnum];
	  if (seg->sidedef)
	    {
	      ss->sector = seg->sidedef->sector;
	      break;
	    }
	}

      // Every gl subsector should have at least one non-miniseg.
      if (!ss->sector)
	CONS_Printf("GL subsector %d consists entirely of minisegs. Unpredictable things may happen at any time.\n", i);
    }

  // count number of lines in each sector
  line_t *li = lines;
  int total = 0;
  for (i=0 ; i<numlines ; i++, li++)
    {
      total++;
      li->frontsector->linecount++;

      if (li->backsector && li->backsector != li->frontsector)
        {
          li->backsector->linecount++;
          total++;
        }
    }

  // build line tables for each sector
  line_t **lb = linebuffer = (line_t **)Z_Malloc(total*sizeof(line_t *), PU_LEVEL, NULL);
  sector_t *sector = sectors;
  for (i=0 ; i<numsectors ; i++, sector++)
    {
      bbox_t bb; // temporary bounding box
      bb.Clear();

      sector->lines = lb;
      li = lines;
      for (j=0 ; j<numlines ; j++, li++)
        {
          if (li->frontsector == sector || li->backsector == sector)
            {
              *lb++ = li;
              bb.Add(li->v1->x, li->v1->y);
              bb.Add(li->v2->x, li->v2->y);
            }
        }
      if (lb - sector->lines != sector->linecount)
        I_Error("Map::GroupLines: miscounted");

      // set the degenmobj_t to the middle of the bounding box
      sector->soundorg.x = (bb[BOXRIGHT]+bb[BOXLEFT])/2;
      sector->soundorg.y = (bb[BOXTOP]+bb[BOXBOTTOM])/2;
      sector->soundorg.z = sector->floorheight-10;

      /*
      // adjust bounding box to map blocks
      int block = bmap.BlockY(bb[BOXTOP] + MAXRADIUS);
      block = block >= bmap.height ? bmap.height-1 : block;
      sector->blockbox[BOXTOP]=block;

      block = bmap.BlockY(bb[BOXBOTTOM] - MAXRADIUS);
      block = block < 0 ? 0 : block;
      sector->blockbox[BOXBOTTOM]=block;

      block = bmap.BlockX(bb[BOXRIGHT] + MAXRADIUS);
      block = block >= bmap.width ? bmap.width-1 : block;
      sector->blockbox[BOXRIGHT]=block;

      block = bmap.BlockX(bb[BOXLEFT] - MAXRADIUS);
      block = block < 0 ? 0 : block;
      sector->blockbox[BOXLEFT]=block;
      */
    }

}

/*
TODO
static char *levellumps[] =
{
  "label",        // LUMP_LABEL,    A separator, name, ExMx or MAPxx
  "THINGS",       // LUMP_THINGS,   Monsters, items..
  "LINEDEFS",     // LUMP_LINEDEFS, LineDefs, from editing
  "SIDEDEFS",     // LUMP_SIDEDEFS, SideDefs, from editing
  "VERTEXES",     // LUMP_VERTEXES, Vertices, edited and BSP splits generated
  "SEGS",         // LUMP_SEGS,     LineSegs, from LineDefs split by BSP
  "SSECTORS",     // LUMP_SSECTORS, SubSectors, list of LineSegs
  "NODES",        // LUMP_NODES,    BSP nodes
  "SECTORS",      // LUMP_SECTORS,  Sectors, from editing
  "REJECT",       // LUMP_REJECT,   LUT, sector-sector visibility
  "BLOCKMAP",     // LUMP_BLOCKMAP  LUT, motion clipping, walls/grid element
  "BEHAVIOR"      // LUMP_BEHAVIOR, ACS scripts
};
*/

//
// P_CheckLevel
// Checks a lump and returns whether or not it is a level header lump.
/*
  bool P_CheckLevel(int lumpnum)
  {
  int  i;
  int  file, lump;

  for(i=LUMP_THINGS; i<=LUMP_BLOCKMAP; i++)
  {
  file = lumpnum >> 16;
  lump = (lumpnum & 0xffff) + i;
  if(file > numwadfiles || lump > wadfiles[file]->numlumps ||
  strncmp(wadfiles[file]->lumpinfo[lump].name, levellumps[i], 8) )
  return false;
  }
  return true;    // all right
  }
*/

// Convert short integers to long ones with byte swapping and sign
// extension. Maps -1 (0xFFFF) to -1 (0xFFFFFFFF).
//
// This should probably be somewhere else.

static int LONG_FROM_USHORT(short i)
{
  Uint32 result = SHORT(i);
  if(result == 65535)
    result = 0xFFFFFFFF;
  return result;
}


int Map::LoadGLVertexes(const int lump)
{
  const int glheaderlen = 4;
  int glversion;
  byte *data = (byte*)fc.CacheLumpNum(lump, PU_STATIC);

  if(strncmp(GL2_HEADER, (const char*)data, glheaderlen) == 0)
    glversion = 2;
  else if(strncmp(GL5_HEADER, (const char*)data, glheaderlen) == 0)
    glversion = 5;
  else {
    return -1; // We don't handle other versions.
  }

  numglvertexes = (fc.LumpLength(lump) - glheaderlen) / sizeof(mapglvertex_t);
  glvertexes = (vertex_t *)Z_Malloc(numglvertexes*sizeof(vertex_t),PU_LEVEL,0);

  mapglvertex_t *in = (mapglvertex_t *)(data+glheaderlen);
  vertex_t *out = glvertexes;

  for (int i=0 ; i<numglvertexes ; i++, in++, out++)
    {
      out->x.setvalue(LONG(in->x));
      out->y.setvalue(LONG(in->y));
    }

  Z_Free(data);

  return glversion;
}


static float SegLength(float dx, float dy)
{
  return sqrt(dx*dx + dy*dy);
}

/// GL segs always override the normal segs.
void Map::LoadGLSegs(const int lump, const int glversion)
{
  if (glversion == 2)
    numsegs = fc.LumpLength(lump) / sizeof(mapgl2seg_t);
  else // Assume version 5.
    numsegs = fc.LumpLength(lump) / sizeof(mapgl5seg_t);

  // Unload regular segs.
  if(segs)
    Z_Free(segs);

  segs = (seg_t *)Z_Malloc(numsegs*sizeof(seg_t), PU_LEVEL, 0);
  memset(segs, 0, numsegs*sizeof(seg_t)); // clear all fields to zero
  byte *data = (byte*)fc.CacheLumpNum(lump, PU_STATIC);

  seg_t *seg = segs;
  mapgl2seg_t *in2 = (mapgl2seg_t*)data;
  mapgl5seg_t *in  = (mapgl5seg_t*)data;

  for (int i=0; i<numsegs; i++, seg++)
    {
      line_t *ldef;
      if (glversion == 2)
	{
	  Uint16 v = SHORT(in2->start_vertex);
	  if (v & VERT_IS_GL_V2)
	    seg->v1 = &glvertexes[v & (~VERT_IS_GL_V2)];
	  else
	    seg->v1 = &vertexes[v];

	  v = SHORT(in2->end_vertex);
	  if(v & VERT_IS_GL_V2)
	    seg->v2 = &glvertexes[v & (~VERT_IS_GL_V2)];
	  else
	    seg->v2 = &vertexes[v];

	  v = SHORT(in2->linedef);
	  if (v == NULL_INDEX)
	    ldef = NULL;
	  else
	    ldef = &lines[v];

	  if (in2->side == 0)
	    seg->side = 0;
	  else
	    seg->side = 1;

	  v = in2->partner_seg;
	  if(v == NULL_INDEX)
	    seg->partner_seg = NULL;
	  else
	    seg->partner_seg = &segs[v];

	  in2++;
	}
      else
	{ // V5 again.
	  Uint32 v = LONG(in->start_vertex);

	  if (v & VERT_IS_GL_V5)
	    seg->v1 = &glvertexes[v&(~VERT_IS_GL_V5)];
	  else
	    seg->v1 = &vertexes[v];

	  v = LONG(in->end_vertex);
	  if (v & VERT_IS_GL_V5)
	    seg->v2 = &glvertexes[v&(~VERT_IS_GL_V5)];
	  else
	    seg->v2 = &vertexes[v];

	  Uint16 ld = SHORT(in->linedef);
	  if (ld == NULL_INDEX)
	    ldef = NULL;
	  else
	    ldef = &lines[ld];

	  if (SHORT(in->side))
	    seg->side = 1;
	  else
	    seg->side = 0;

	  v = LONG(in->partner_seg);
	  if (v == NULL_INDEX_32)
	    seg->partner_seg = NULL;
	  else
	    seg->partner_seg = &segs[v]; 

	  in++;
	}

      seg->linedef = ldef;

      // fill in the rest
      seg->sidedef = ldef ? ldef->sideptr[seg->side] : NULL;

      if (ldef)
	{ // not a miniseg
	  float sx = ((seg->side ? ldef->v2->x : ldef->v1->x) - seg->v1->x).Float();
	  float sy = ((seg->side ? ldef->v2->y : ldef->v1->y) - seg->v1->y).Float();
	  seg->offset = SegLength(sx, sy);

	  seg->frontsector = ldef->sideptr[seg->side]->sector;
	  seg->backsector = (ldef->flags & ML_TWOSIDED) ? ldef->sideptr[seg->side^1]->sector : NULL;
	}

      // make a vector (start at origin)
      float dx = (seg->v2->x - seg->v1->x).Float();
      float dy = (seg->v2->y - seg->v1->y).Float();

      if (dx == 0)
	seg->angle = (dy > 0) ? ANG90 : ANG270;
      else
	seg->angle = angle_t((atan2(dy, dx) * ANG180) / M_PI);

      seg->length = SegLength(dx, dy);
    }

  Z_Free(data);
}


void Map::LoadGLSubsectors(const int lump, const int glversion)
{
  if (glversion == 2)
    numsubsectors = fc.LumpLength(lump) / sizeof(mapgl2subsector_t);
  else // Assume version 5.
    numsubsectors = fc.LumpLength(lump) / sizeof(mapgl5subsector_t);

  // The code may have loaded regular subsectors. Release them.
  if(subsectors)
    Z_Free(subsectors);

  subsectors = (subsector_t *)Z_Malloc(numsubsectors * sizeof(subsector_t), PU_LEVEL, 0);
  memset(subsectors, 0, numsubsectors * sizeof(subsector_t));
  byte *data = (byte*)fc.CacheLumpNum(lump, PU_STATIC);

  subsector_t *ss = subsectors;
  if (glversion == 2)
    {
      mapgl2subsector_t *in = (mapgl2subsector_t*)data;

      for(int i=0; i<numsubsectors; i++, in++, ss++)
	{
	  ss->num_segs  = LONG_FROM_USHORT(in->count);
	  ss->first_seg = LONG_FROM_USHORT(in->first_seg);
	  ss->sector = NULL;
	}
    }
  else
    { // V5 again.
      mapgl5subsector_t *in = (mapgl5subsector_t*)data;

      for(int i=0; i<numsubsectors; i++, in++, ss++)
	{
	  ss->num_segs = LONG(in->count);
	  ss->first_seg = LONG(in->first_seg);
	  ss->sector = NULL;
	}
    }


  Z_Free(data);
}



void Map::LoadGLNodes(const int lump, const int glversion)
{
  if (glversion == 2)
    numnodes = fc.LumpLength(lump) / sizeof(mapnode_t);
  else // Assume version 5.
    numnodes = fc.LumpLength(lump) / sizeof(mapgl5node_t);

  // Unload regular nodes if they are loaded.
  if(nodes)
    Z_Free(nodes);

  nodes = (node_t *)Z_Malloc(numnodes*sizeof(node_t), PU_LEVEL, 0);
  byte *data = (byte*)fc.CacheLumpNum(lump, PU_STATIC);
  node_t *no = nodes;

  if (glversion == 2)
    {
      mapnode_t *in = (mapnode_t*)data;

      for (int i=0; i<numnodes; i++, in++, no++)
	{
	  no->x = SHORT(in->x);
	  no->y = SHORT(in->y);
	  no->dx = SHORT(in->dx);
	  no->dy = SHORT(in->dy);
	  for (int j=0 ; j<2 ; j++)
	    {
	      no->children[j] = SHORT(in->children[j]);
	      ConvertGLFlags16_32(no->children[j]);	  
	      for (int k=0 ; k<4 ; k++)
		no->bbox[j].box[k] = SHORT(in->bbox[j][k]);
	    }
	}
    }
  else
    { // V5 again.
      mapgl5node_t *in = (mapgl5node_t*)data;
      for (int i=0; i<numnodes; i++, in++, no++)
	{
	  no->x = SHORT(in->x);
	  no->y = SHORT(in->y);
	  no->dx = SHORT(in->dx);
	  no->dy = SHORT(in->dy);
	  for (int j=0 ; j<2 ; j++)
	    {
	      no->children[j] = LONG(in->children[j]);
	      for (int k=0 ; k<4 ; k++)
		no->bbox[j].box[k] = SHORT(in->bbox[j][k]);
	    }
	}
    }

  Z_Free(data);
}

// Load glVis data. If it does not exist, glvis is set to NULL.

void Map::LoadGLVis(const int lump)
{
  const char *lname = fc.FindNameForNum(lump);
  int vissize = fc.LumpLength(lump);

  // glVIS is not always present. Check for it.
  if (lname == NULL || !strcmp(lname, "GL_VIS"))
    CONS_Printf(" Map does not have GL_VIS data.\n");
  else if (vissize == 0)
    CONS_Printf(" Map has empty GL_VIS data.\n");
  else
    {
      // At this point we know that GL_VIS exists, and that it is
      // nonempty. Load it.
      glvis = static_cast<byte*>(fc.CacheLumpNum(lump, PU_STATIC));
      CONS_Printf(" Loaded %d bytes of glVIS data.\n", vissize);
    }

  if (!glvis && rendermode == render_opengl)
    CONS_Printf(" Automap will not work until you run glvis on this file.\n");
}


// Setup sky texture to use for the level
//
// The sky texture to be used instead of the F_SKY1 dummy.
void Map::SetupSky()
{
  // where does the sky texture come from?
  // 1. MapInfo skyname
  // 2. MAPINFO lump skyname
  // 3. if everything else fails, use "SKY1" and hope for the best

  // original DOOM determined the sky texture to be used
  // depending on the current episode, and the game version.

  if (info->sky1.empty())
    info->sky1 = "SKY1";

  if (info->sky2.empty())
    info->sky2 = "SKY1";

  skytexture = materials.Get(info->sky1.c_str(), TEX_wall);
  skybox_pov = NULL;

  // scale up the old skies, if needed
  R_SetupSkyDraw(skytexture);

  // set the dummy sky flat num (only used locally during map loading)
  if (hexen_format)
    skyflat_mat = materials.Get("F_SKY", TEX_floor);
  else
    skyflat_mat = materials.Get("F_SKY1", TEX_floor);
}


//
// (re)loads the map from an already opened wad
//
bool Map::Setup(tic_t start, bool spawnthings)
{
  extern  bool precache;

  CONS_Printf("Loading map %s...\n", lumpname.c_str());
  con.Drawer();     // let the user know what we are going to do
  I_FinishUpdate(); // page flip or blit buffer

  maptic = 0;
  starttic = start;
  kills = items = secrets = 0;

  // internal game map
  lumpnum = fc.GetNumForName(lumpname.c_str());

  InitThinkers();

  // NOTE Hexen map separators are not empty!!! They contain a version string "version 2.3\0"
  char *FS_data = info->Read(lumpnum); // load map separator lump info (map properties, FS...)

  // is the map in Hexen format?
  const char *acslumpname = fc.FindNameForNum(lumpnum + LUMP_BEHAVIOR);
  if (acslumpname && !strncmp(acslumpname, "BEHAVIOR", 8))
    {
      CONS_Printf(" Hexen format.\n");
      hexen_format = true;
    }
  else
    {
      hexen_format = false;
    }

  // If the map defines its music in MapInfo_t, use it.
  if (!info->musiclump.empty())
  {
    S.StartMusic(info->musiclump.c_str(), true);
 // I_PlayCD(info->mapnumber, true);  // FIXME cd music
  }

  // Set the gravity for the level
  if (cv_gravity.value != int(info->gravity * fixed_t::UNIT))
    {
      COM.AppendText(va("gravity %f\n", info->gravity));
      COM.BufExecute();
    }

  SetupSky();

  // NOTE: most of this ordering is important
  // Load GL data if it exists, otherwise use normal data
  string glname = "GL_";
  glname.append(lumpname, 0, 5);

  int gllump = fc.FindNumForName(glname.c_str(), false);
  int gl_version = 0;
  if (gllump != -1 && gllump > lumpnum)
    {
      gl_version = LoadGLVertexes(gllump+LUMP_GL_VERTEXES);
      CONS_Printf(" Map has v%d GL nodes.\n", gl_version);

      if (gl_version < 0)
	I_Error("Can not handle GL nodes that are not v2 or v5.");
    }
  else
    {
      CONS_Printf(" Map has no GL nodes.\n");
      // OpenGL renderer. TODO more friendly behavior (include glBSP, build nodes on the fly)
      if (rendermode == render_opengl)
	{
	  CONS_Printf("Trying to use OpenGL renderer without GL nodes. Exiting.\n");
	  CONS_Printf("Build GL nodes with glbsp and try again.\n");
	  return false;
	}

      gllump = -1;
    }

  LoadVertexes(lumpnum+LUMP_VERTEXES); // These are always needed.
  LoadSectors1(lumpnum+LUMP_SECTORS);  // allocates sectors
  LoadSideDefs(lumpnum+LUMP_SIDEDEFS); // allocates sidedefs
  LoadLineDefs(lumpnum+LUMP_LINEDEFS); // points to v, si(and back)
  LoadBlockMap(lumpnum+LUMP_BLOCKMAP); // loads (or builds) the blockmap, uses vertexes and lines
  LoadSideDefs2(lumpnum+LUMP_SIDEDEFS); // points to se, uses l, processes some linedef specials
  LoadLineDefs2(); // uses si, points to se

  if (gllump != -1)
    {
      LoadGLSubsectors(gllump+LUMP_GL_SSECT, gl_version);
      LoadGLNodes(gllump+LUMP_GL_NODES, gl_version);
      LoadGLSegs(gllump+LUMP_GL_SEGS, gl_version);
      LoadGLVis(gllump+LUMP_GL_PVS);
    }
  else
    {
      LoadSubsectors(lumpnum+LUMP_SSECTORS); // just loads them
      LoadNodes(lumpnum+LUMP_NODES); // loads nodes
      LoadSegs(lumpnum+LUMP_SEGS); // points to v, l, si, se, uses l
    }

  LoadSectors2(lumpnum+LUMP_SECTORS); // rest of secs, uses nothing!!!
  rejectmatrix = static_cast<byte*>(fc.CacheLumpNum(lumpnum+LUMP_REJECT,PU_LEVEL));
  GroupLines();



  // lights, scrollers, sectordamage...
  for (int i=0; i<numsectors; i++)
    SpawnSectorSpecial(sectors[i].special, &sectors[i]);

  R_SetFadetable(info->fadetablelump.c_str());

  // fix renderer to this map TODO why?
  R.SetMap(this);

  // NOTE: in loading a game, we ignore spawnflags so (almost) every mapthing gets a type.
  // This is because the players have not been unserialized yet and hence the fclass mask cannot be set.
  LoadThings(lumpnum + LUMP_THINGS, spawnthings);

  if (!polyspawn.empty())
    PO_Init(); // create the polyobjs, clear their mapthings (before spawning other things!)

  // spawn the THINGS (Actors) if needed (not needed on clients or when loading savegames)
  if (spawnthings && game.server)
    {
      for (int i=0; i<nummapthings; i++)
        if (mapthings[i].ai)
          mapthings[i].ai->Spawn(this, &mapthings[i], true);

      PlaceWeapons(); // Heretic mace
    }

  RemoveAllActiveCeilings(); // TODO unnecessary?
  RemoveAllActivePlats(); // TODO unnecessary?
  InitTagLists(); // Create hash tables for tags and lineid's
  SpawnLineSpecials(); // spawn Thinkers created by linedefs, confer properties given by linedefs

  if (game.server)
    {
      if (hexen_format)
	ACS_LoadScripts(lumpnum + LUMP_BEHAVIOR);

      FS_LoadScripts(FS_data); // load and preprocess FraggleScript scripts (needs already added players)
    }

  if (info->lightning)
    effects = new MapEffect(this); // Hexen lightning effect

  if (precache)
    PrecacheMap();



  int minisegs = 0;
  int botnodes2 = 0;
  for (int i=0; i<numsegs; i++)
    {
      seg_t *seg = &segs[i];
      
      if (!seg->linedef)
	{
	  minisegs++;
	  if (!seg->partner_seg)
	    CONS_Printf("miniseg %d w/o partner seg\n", i);
	  else if (seg->partner_seg->linedef)
	    {
	      CONS_Printf("miniseg %d partner (%d-sided) not a miniseg\n", i, seg->partner_seg->linedef->flags & ML_TWOSIDED ? 2 : 1);
	    }
	}
      else if (seg->linedef->flags & ML_TWOSIDED)
	botnodes2++;
    }
  CONS_Printf(" %d vertices, %d subsectors, %d segs, %d + %d botnodes\n", numvertexes, numsubsectors, numsegs, minisegs, botnodes2);

  return true;
}


// TEMP linedef conversion tables
xtable_t *linedef_xtable = NULL;
int  linedef_xtable_size = 0;

/// Does the Doom => Hexen linedef type conversion for the given linedef.
void ConvertLineDef(line_t *ld)
{
  int j, trig;
  bool passuse = ld->flags & ML_PASSUSE;
  bool alltrigger = ld->flags & ML_ALLTRIGGER;
  ld->flags &= 0x1ff; // only basic Doom flags are kept

  if (ld->special < linedef_xtable_size)
    {
      // we use a pregenerated binary lookup table in legacy.wad
      xtable_t *p = &linedef_xtable[ld->special];

      if (p->type)
	ld->special = p->type; // some specials are unaffected

      for (j=0; j<5; j++)
	ld->args[j] = p->args[j];

      trig = p->trigger;

      // flags
      ld->flags |= (trig & 0x0f) << (ML_SPAC_SHIFT-1); // activation and repeat

      if (passuse && (GET_SPAC(ld->flags) == SPAC_USE))
	{
	  ld->flags &= ~ML_SPAC_MASK;
	  ld->flags |= SPAC_PASSUSE << ML_SPAC_SHIFT;
	}

      if (trig & T_ALLOWMONSTER)
	ld->flags |= ML_MONSTERS_CAN_ACTIVATE;
    }
  else if (ld->special >= GenCrusherBase)
    {
      // Boom generalized linedefs
      ld->flags |= ML_BOOM_GENERALIZED;
      if (passuse)
	ld->flags |= SPAC_PASSUSE << ML_SPAC_SHIFT;
    }

  // put flags back
  if (alltrigger)
    ld->flags |= ML_MONSTERS_CAN_ACTIVATE;

}





/// Converts the relevant parts of a Doom/Heretic map to Hexen format,
/// writes the lumps on disk.
bool ConvertMapToHexen(int lumpnum)
{
  const char *maplumpname = fc.FindNameForNum(lumpnum);

  if (!maplumpname)
    return false;

  CONS_Printf("Converting map %s to Hexen format...\n", maplumpname);

  // is it a map?
  const char *lumpname = fc.FindNameForNum(lumpnum+LUMP_THINGS);
  if (!lumpname || strncmp(lumpname, "THINGS", 8))
    {
      CONS_Printf(" No THINGS lump found!\n");
      return false;
    }

  lumpname = fc.FindNameForNum(lumpnum+LUMP_LINEDEFS);
  if (!lumpname || strncmp(lumpname, "LINEDEFS", 8))
    {
      CONS_Printf(" No LINEDEFS lump found!\n");
      return false;
    }

  // is the map in Hexen format?
  lumpname = fc.FindNameForNum(lumpnum+LUMP_BEHAVIOR);
  if (lumpname && !strncmp(lumpname, "BEHAVIOR", 8))
    {
      CONS_Printf(" Map already is in Hexen format!\n");
      return false;
    }

  // first convert THINGS

  int lump = lumpnum + LUMP_THINGS;
  int numthings = fc.LumpLength(lump)/sizeof(doom_mapthing_t);

  byte *data = (byte *)fc.CacheLumpNum(lump, PU_STATIC);
  doom_mapthing_t *t = (doom_mapthing_t *)data;

  int length = numthings * sizeof(hex_mapthing_t);
  hex_mapthing_t *ht = (hex_mapthing_t *)Z_Malloc(length, PU_STATIC, NULL);
  memset(ht, 0, length); // no TID, height, special or args

  int bflags = MTF_FIGHTER | MTF_CLERIC | MTF_MAGE;

  for (int i=0; i < numthings; i++, t++)
    {
      ht[i].x = t->x;
      ht[i].y = t->y;
      ht[i].angle = t->angle;
      ht[i].type = t->type;

      int dflags = SHORT(t->flags); // since we use this data, it must be of correct endianness

      ht[i].flags = bflags | (dflags & 0xF); // lowest four flags are the same

      if (!(dflags & MTF_MULTIPLAYER))
	ht[i].flags |= MTF_GSINGLE;

      if (!(dflags & MTF_NOT_IN_COOP))
	ht[i].flags |= MTF_GCOOP;

      if (!(dflags & MTF_NOT_IN_DM))
	ht[i].flags |= MTF_GDEATHMATCH;

      ht[i].flags = SHORT(ht[i].flags); // and back to little-endian
    }

  Z_Free(data);

  string outfilename = string(maplumpname) + "_THINGS.lmp";
  FIL_WriteFile(outfilename.c_str(), ht, length);
  Z_Free(ht);

  // then convert LINEDEFS

  lump = lumpnum + LUMP_LINEDEFS;
  int numlines = fc.LumpLength(lump)/sizeof(doom_maplinedef_t);

  data = (byte *)fc.CacheLumpNum(lump, PU_STATIC);
  doom_maplinedef_t *ld = (doom_maplinedef_t *)data;

  length = numlines * sizeof(hex_maplinedef_t);
  hex_maplinedef_t *hld = (hex_maplinedef_t *)Z_Malloc(length, PU_STATIC, NULL);

  for (int i=0; i < numlines; i++, ld++)
    {
      // trivialities
      hld[i].v1 = ld->v1;
      hld[i].v2 = ld->v2;
      hld[i].sidenum[0] = ld->sidenum[0];
      hld[i].sidenum[1] = ld->sidenum[1];

      // flags, special, args
      line_t temp;

      temp.flags = SHORT(ld->flags);
      temp.special = SHORT(ld->special);
      ConvertLineDef(&temp);

      hld[i].flags = SHORT(temp.flags);
      hld[i].special = temp.special;
      for (int j=0; j<5; j++)
	hld[i].args[j] = temp.args[j];

      // tag
      unsigned tag = SHORT(ld->tag);

      if (hld[i].special == LINE_LEGACY_EXT) // different tag handling (full 16-bit tag stored)
	{
	  hld[i].args[3] = tag & 0xFF;
	  hld[i].args[4] = tag >> 8;
	}
      else
	{
	  // we must make do with 8-bit tags
	  if (tag > 255)
	    {
	      CONS_Printf(" Warning: Linedef %d: tag (%d) is above 255, set to zero.\n.", i, tag);
	      hld[i].args[0] = 0;
	    }
	  else
	    hld[i].args[0] = tag;
	}
    }

  Z_Free(data);

  outfilename = string(maplumpname) + "_LINEDEFS.lmp";
  FIL_WriteFile(outfilename.c_str(), hld, length);
  Z_Free(hld);

  CONS_Printf(" done. %d THINGS and %d LINEDEFS.\n", numthings, numlines);
  return true;
}
