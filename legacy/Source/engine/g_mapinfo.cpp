// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: g_mapinfo.cpp 505 2007-12-18 02:57:12Z smite-meister $
//
// Copyright (C) 2002-2007 by Doom Legacy Team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Implementation of MapInfo class.
/// MapInfo lump parser. Hexen/ZDoom MAPINFO lump parser.

#include <stdio.h>
#include <unistd.h>

#include "doomdef.h"
#include "command.h"
#include "cvars.h"
#include "parser.h"

#include "g_mapinfo.h"
#include "g_level.h"

#include "g_game.h"
#include "g_map.h"
#include "g_player.h"
#include "g_pawn.h"

#include "m_misc.h"
#include "m_archive.h"

#include "w_wad.h"
#include "z_zone.h"


//=================================
//    MapInfo class methods
//=================================

/// default constructor
MapInfo::MapInfo()
{
  state = MAP_UNLOADED;
  me = NULL;
  found = false;
  nicename = "Unnamed Map";

  mapnumber = 0;
  cluster = 100; // this is so that maps with no cluster are placed in a cluster of their own
  hub = false;

  scripts = 0;
  partime = 0;
  gravity = 1.0f;

  warptrans = warpnext = nextlevel = secretlevel = -1;
  //nextmaplump[0] = secretmaplump[0] = '\0';

  doublesky = lightning = false;
  sky1 = "SKY1";
  sky1sp = sky2sp = 0;

  cdtrack = 1;
  BossDeathKey = 0;

  fadetablelump = "COLORMAP";

  interpic = "INTERPIC"; // fallback for lazy map authors
  intermusic = "-";
}


/// destructor
MapInfo::~MapInfo()
{
  Close(-1);
}


/// ticks the map forward
void MapInfo::Ticker()
{
  //CONS_Printf("[%s][%d]MapInfo::Ticker\n",__FILE__,__LINE__);
  if (me && state != MAP_INSTASIS)
    {
      me->Ticker();

      // check fraglimit cvar TODO how does this work? when are the scores (or teamscores) zeroed?
      if (game.server && cv_fraglimit.value && game.CheckScoreLimit())
	{
	  // go on to the next map
	  Close(-1);
	}

      if (state == MAP_FINISHED)
	{
	  // TODO spectators have no destinations set, they should perhaps follow the last exiting player...
	  if (hub && game.server)
	    HubSave();
	  else
	    Close(-1);
	}
      else if (state == MAP_RESET)
	{
	  if (!savename.empty())
	    {
	      delete me; // the current map goes
	      state = MAP_SAVED;
	      HubLoad(); // revert to last hubsave
	    }
	  else
	    Close(-1);
	}
    }
}


/// if this returns true, "me" must be valid
bool MapInfo::Activate(PlayerInfo *p)
{
  const char *temp;
  //CONS_Printf("Activating map %d\n", mapnumber);

  switch (state)
    {
    case MAP_UNLOADED:
      temp = lumpname.c_str();
      if (fc.FindNumForName(temp) == -1)
	{
	  CONS_Printf("\2Map '%s' not found!\n", temp);
	  return false;
	}

      me = new Map(this);
      if (me->Setup(game.tic))
	state = MAP_RUNNING;
      else
	{
	  delete me;
	  me = NULL;
	  I_Error("Error during map setup.\n");
	  return false;
	}

      if (game.server)
	me->ACS_StartDeferredScripts(); // execute waiting scripts
      break;

    case MAP_RUNNING:
    case MAP_RESET:
    case MAP_FINISHED:
      break;

    case MAP_INSTASIS:
      state = MAP_RUNNING;
      break;

    case MAP_SAVED:
      if (!HubLoad())
	return false;
    }

  if (!p)
    return true; // map activated without players

  me->AddPlayer(p);
  return true;
}


/// throws out all the players from the map
int MapInfo::EvictPlayers(int next, int ep, bool force)
{
  if (!me)
    return 0; // not active, no players

  if (next < 0)
    next = nextlevel;

  int n = me->players.size();
  for (int i = 0; i < n; i++)
    {
      PlayerInfo *p = me->players[i];
      if (force)
	{
	  p->requestmap = 0;
	  p->Reset(true, true); // and everything goes.
	}

      p->ExitLevel(next, ep); // puts them in the PST_LEAVINGMAP state
    }

  me->HandlePlayers(); // kicks them out, starts intermissions

  return n;
}


/// shuts the map down
void MapInfo::Close(int next, int ep, bool force)
{
  // delete the save file
  if (!savename.empty())
    {
      unlink(savename.c_str());
      savename.clear();
    }

  if (me)
    {
      EvictPlayers(next, ep, force);
      delete me; // and then the map goes
      me = NULL;
    }
  state = MAP_UNLOADED;
}


/// for making hub saves
bool MapInfo::HubSave()
{
  if (!me)
    return false;

  EvictPlayers(-1);
  CONS_Printf("Making a hubsave...");

  char fname[256];
  sprintf(fname, hubsavename, mapnumber);
  savename = fname;

  LArchive a;
  a.Create("Hubsave");
  me->Serialize(a);

  byte *buffer;
  unsigned length = a.Compress(&buffer, 1);

  FIL_WriteFile(savename.c_str(), buffer, length);
  CONS_Printf("done. %d bytes written.\n", length);
  Z_Free(buffer);

  delete me;
  me = NULL;

  state = MAP_SAVED;
  return true;
}


/// well, loading the hub saves
bool MapInfo::HubLoad()
{
  if (!game.server || state != MAP_SAVED)
    return false;

  CONS_Printf("Loading a hubsave...");

  byte *buffer;
  int length = FIL_ReadFile(savename.c_str(), &buffer);
  if (!length)
    {
      I_Error("\nCouldn't open hubsave file '%s'", savename.c_str());
      return false;
    }
  
  LArchive a;
  if (!a.Open(buffer, length))
    I_Error("\nCouldn't uncompress hubsave!\n");

  Z_Free(buffer);

  // dearchive the map
  me = new Map(this);
  if (me->Unserialize(a))
    I_Error("\nHubsave corrupted!\n");

  CONS_Printf("done.");

  state = MAP_RUNNING;
  return true;
}



//==================================================================
//   Legacy MapInfo parser
//==================================================================

//  Level vars: level variables in the [level info] section.
//
//  Takes the form:
//     [variable name] = [value]
//
//  '=' sign is optional: all equals signs are internally turned to spaces
//


// FIXME NOTE:
// This sucks. We need offsets of data fields inside the MapInfo, MapCluster and Episode classes, but there is no 
// universal way of getting them since g++ feels they are not POD objects, and its behavior depends on version.
// I see no reason why this shouldn't work since we have no virtual tables, access restrictions or anything weird in them...

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#if GCC_VERSION >= 30400
# define MI_offset(field) offsetof(MapInfo, field)
# define CD_offset(field) offsetof(MapCluster, field)
# define EP_offset(field) offsetof(Episode, field)
#else
# define MI_offset(field) (size_t(&MapInfo::field))
# define CD_offset(field) (size_t(&MapCluster::field))
# define EP_offset(field) (size_t(&Episode::field))
#endif

//#define MI_offset(field) ((char*)&(((MapInfo*)0)->field) - (char*)0)
//#define MI_offset(field) (size_t(&((MapInfo *)0)->field))

static parsercmd_t MapInfo_commands[]=
{
  {P_ITEM_STR, "levelname",  MI_offset(nicename)},
  {P_ITEM_STR, "name",       MI_offset(nicename)},
  {P_ITEM_STR, "version",    MI_offset(version)},
  {P_ITEM_STR, "creator",    MI_offset(author)},
  {P_ITEM_STR, "author",     MI_offset(author)},
  {P_ITEM_STR, "description", MI_offset(description)},
  {P_ITEM_INT, "partime",    MI_offset(partime)},
  {P_ITEM_STR, "music",      MI_offset(musiclump)},

  {P_ITEM_PERCENT_FLOAT, "gravity",  MI_offset(gravity)}, 
  {P_ITEM_STR, "skyname",    MI_offset(sky1)},

  //{P_ITEM_STR, "nextlevel",  MI_offset(nextlevel)},
  //{P_ITEM_STR, "nextsecret", MI_offset(nextsecret)},

  {P_ITEM_STR, "interpic", MI_offset(interpic)},
  {P_ITEM_STR, "intermusic", MI_offset(intermusic)},
  /*
    {P_ITEM_STR,    "levelpic",     &info_levelpic},
    {P_ITEM_STR,    "inter-backdrop",&info_backdrop},
    {P_ITEM_STR,    "defaultweapons",&info_weapons},
  */
  {P_ITEM_IGNORE, "consolecmd", 0}, // TODO at least for now
  {P_ITEM_IGNORE, NULL, 0} // terminator
};



/// Reads a MapInfo lump for a map.
char *MapInfo::Read(int lump)
{
  Parser p;
  string scriptblock;

  if (p.Open(lump))
    {
      CONS_Printf("Reading MapInfo...\n");

      enum {PS_CLEAR, PS_SCRIPT, PS_INTERTEXT, PS_MAPINFO} parsestate = PS_CLEAR;
      char line[40];

      p.RemoveComments('/'); // TODO can we also remove other types of comments?
      p.DeleteChars('\r');
      while (p.NewLine())
	{
	  if (parsestate != PS_SCRIPT) // not for scripts
	    p.LineReplaceChars('=', ' ');
	  // unprintable chars to whitespace?

	  if (p.Peek() == '[')  // a new section seperator
	    {
	      p.GetStringN(line, 12);
	      if (!strncasecmp(line, "[level info]", 12))
		parsestate = PS_MAPINFO;
	      else if (!strncasecmp(line, "[scripts]", 9))
		{
		  parsestate = PS_SCRIPT;
		  scripts++; // has scripts
		}
	      else if(!strncasecmp(line, "[intertext]", 11))
		parsestate = PS_INTERTEXT;
	    }
	  else switch (parsestate)
	    {
	    case PS_MAPINFO:
	      p.ParseCmd(MapInfo_commands, (char *)this);
	      break;

	    case PS_SCRIPT:
	      scriptblock += p.Pointer(); // add the new (NUL-terminated!) line to the current data
	      break;

	    case PS_INTERTEXT: // TODO
	      //intertext += '\n';
	      //intertext += s;
	      break;

	    case PS_CLEAR:
	      break;
	    }
	}
    }

  // FS script data
  return (scriptblock.size() > 0) ? Z_Strdup(scriptblock.c_str(), PU_LEVEL, NULL) : NULL;
}



//==============================================
//   Hexen/ZDoom MAPINFO parser.
//==============================================

/// MAPINFO map commands
static parsercmd_t MAPINFO_MAP_commands[] =
{
  {P_ITEM_INT, "cluster", MI_offset(cluster)},

  {P_ITEM_INT, "warptrans", MI_offset(warptrans)}, // Damnation!
  {P_ITEM_INT, "next", MI_offset(warpnext)}, // Hexen 'next' refers to warptrans numbers, not levelnums!

  // TODO implement ZDoom lumpname-fields?
  //{P_ITEM_STR16, "next", MI_offset(nextmaplump)},
  //{P_ITEM_STR16, "secretnext", MI_offset(secretmaplump)},

  // our own solution (because the Hexen way SUCKS and ZDoom is not much better :)
  {P_ITEM_INT_INT, "nextlevel", MI_offset(nextlevel), MI_offset(secretlevel)}, // refers to levelnums

  {P_ITEM_BOOL, "doublesky", MI_offset(doublesky)},
  {P_ITEM_STR16_FLOAT, "sky1", MI_offset(sky1), MI_offset(sky1sp)},
  {P_ITEM_STR16_FLOAT, "sky2", MI_offset(sky2), MI_offset(sky2sp)},
  {P_ITEM_BOOL, "lightning", MI_offset(lightning)},
  {P_ITEM_STR, "fadetable", MI_offset(fadetablelump)},
  {P_ITEM_INT, "cdtrack", MI_offset(cdtrack)},
  {P_ITEM_STR, "music", MI_offset(musiclump)},
  {P_ITEM_INT, "par", MI_offset(partime)},
  {P_ITEM_INT, "bossdeath", MI_offset(BossDeathKey)},

  {P_ITEM_STR, "interpic", MI_offset(interpic)},
  {P_ITEM_STR, "intermusic", MI_offset(intermusic)},

  {P_ITEM_STR, "picname", MI_offset(namepic)},

  {P_ITEM_IGNORE, "cd_start_track", 0},
  {P_ITEM_IGNORE, "cd_end1_track", 0},
  {P_ITEM_IGNORE, "cd_end2_track", 0},
  {P_ITEM_IGNORE, "cd_end3_track", 0},
  {P_ITEM_IGNORE, "cd_intermission_track", 0},
  {P_ITEM_IGNORE, "cd_title_track", 0},
  {P_ITEM_IGNORE, NULL, 0}
};
#undef MI_offset


/// ZDoom clusterdef commands
static parsercmd_t MAPINFO_CLUSTERDEF_commands[] =
{
  {P_ITEM_INT, "finale", CD_offset(episode)},
  {P_ITEM_STR, "entertext", CD_offset(entertext)},
  {P_ITEM_STR, "exittext", CD_offset(exittext)},
  {P_ITEM_STR, "flat", CD_offset(finalepic)},
  {P_ITEM_STR, "music", CD_offset(finalemusic)},

  {P_ITEM_BOOL, "hub", CD_offset(hub)},
  {P_ITEM_IGNORE, NULL, 0}
};
#undef CD_offset



/// MAPINFO episode commands
static parsercmd_t MAPINFO_EPISODE_commands[] =
{
  {P_ITEM_STR, "picname", EP_offset(namepic)},
  {P_ITEM_STR, "name", EP_offset(name)}, // ZDoom support
  {P_ITEM_IGNORE, NULL, 0}
};
#undef EP_offset


/// Reads the MAPINFO lump, filling mapinfo and clustermap with data
int GameInfo::Read_MAPINFO()
{
  int lump = fc.FindNumForName(mapinfo_lump.c_str());
  if (lump < 0)
    {
      I_Error("MAPINFO lump '%s' not found.\n", mapinfo_lump.c_str());
      return -1;
    }

  CONS_Printf("Reading MAPINFO...\n");

  Parser p;
  if (!p.Open(lump))
    return -1;

  Clear_mapinfo_clustermap();

  enum {PS_CLEAR, PS_MAP, PS_CLUSTERDEF, PS_EPISODE} parsestate = PS_CLEAR;
  int i, n;
  vector<MapInfo *> tempinfo;

  Episode    *ep = NULL;
  MapCluster *cl = NULL;
  MapInfo  *info = NULL;
  MapInfo   def; // default map

  p.RemoveComments(';');
  p.DeleteChars('\r');
  while (p.NewLine())
    {
      char line[61], ln[17]; // read in max. 60 (16) character strings
      char *start = p.Pointer(); // small hack, store location
      i = p.GetString(line, 30); // pass whitespace, read first word

      // block starters?
      if (!strcasecmp(line, "DEFAULTMAP"))
	{
	  // default map definition block begins
	  parsestate = PS_MAP;
	  info = &def;
	}
      else if (!strcasecmp(line, "MAP"))
	{
	  parsestate = PS_MAP;
	  // map definition block begins. accepted formats:

	  // standard:  map xxx   nicename 2
	  // shorthand: map MAP02 nicename
	  // old Hexen: map 2     nicename

	  i = sscanf(p.Pointer(), "%16s \"%60[^\"]\" %d", ln, line, &n);
	  if (i == 1) // nicename not quoted
	    i = sscanf(p.Pointer(), "%16s %60s %d", ln, line, &n);

	  info = new MapInfo(def); // copy construction
	  tempinfo.push_back(info);

	  if (i == 3)
	    {
	      info->lumpname = ln;
	      if (n >= 0 && n <= 99)
		info->mapnumber = n;
	    }
	  else if (i == 2)
	    {
	      // no mapnumber given, deduce it
	      if (IsNumeric(ln))
		{
		  // assume deprecated Hexen format
		  n = atoi(ln);
		  char temp[10];
		  if (n >= 0 && n <= 99)
		    {
		      sprintf(temp, "MAP%02d", n);
		      info->lumpname = temp;
		      info->mapnumber = n;
		    }
		}
	      else
		{
		  // lumpname must be in MAPxx format
		  i = sscanf(ln, "MAP%2d", &n);
		  info->lumpname = ln;
		  info->mapnumber = n;
		}
	    }
	  else
	    CONS_Printf(" Bad map definition at char %d!\n", p.Location());

	  info->nicename = line;

	  // check that the map can be found
	  const char *temp = info->lumpname.c_str();
	  if (fc.FindNumForName(temp) == -1)
	    {
	      CONS_Printf(" Map '%s' not present!\n", temp);
	    }
	  else
	    info->found = true;
	}
      else if (!strcasecmp(line, "CLUSTERDEF"))
	{
	  // cluster definition block begins (ZDoom)
	  parsestate = PS_CLUSTERDEF;

	  n = p.GetInt();

	  if (clustermap.count(n))
	    {
	      CONS_Printf(" Cluster number %d defined more than once!\n", n);
	      cl = clustermap[n]; // already there, update the existing cluster
	    }
	  else
	    clustermap[n] = cl = new MapCluster(n);

	  if (sscanf(p.Pointer(), " \"%60[^\"]\"", line) == 1)
	    cl->clustername = line;
	}
      else if (!strcasecmp(line, "EPISODE"))
	{
	  // game entrypoint definition
	  i = sscanf(p.Pointer(), "%16s \"%60[^\"]\" %d", ln, line, &n);

	  if (i >= 1)
	    {
	      parsestate = PS_EPISODE;

	      if (i == 1)
		line[0] = '\0'; // ZDoom support, get episode name later

	      ep = new Episode(ln, line);
	      if (i == 3)
		ep->entrypoint = n;

	      episodes.push_back(ep);
	    }
	  else
	    CONS_Printf("Bad episode definition at char %d!\n", p.Location());
	}
      else switch (parsestate)
	{
	case PS_CLEAR:
	  CONS_Printf(" Unknown MAPINFO block at char %d!\n", p.Location());
	  break;

	case PS_MAP:
	  p.SetPointer(start);
	  p.ParseCmd(MAPINFO_MAP_commands, (char *)info);
	  break;

	case PS_CLUSTERDEF:
	  p.SetPointer(start);
	  p.ParseCmd(MAPINFO_CLUSTERDEF_commands, (char *)cl);
	  break;

	case PS_EPISODE:
	  p.SetPointer(start);
	  p.ParseCmd(MAPINFO_EPISODE_commands, (char *)ep);
	  break;
	}
    }


  // now "remap" tempinfo into mapinfo
  n = tempinfo.size();
  for (i=0; i<n; i++)
    {
      info = tempinfo[i];
      int j = info->mapnumber;

      if (mapinfo.count(j)) // already there
	{
	  CONS_Printf(" Map number %d found more than once!\n", j);
	  delete mapinfo[j]; // later one takes precedence
	}

      mapinfo[j] = info;
    }

  mapinfo_iter_t r;

  // generate the missing clusters and fill them all with maps
  for (r = mapinfo.begin(); r != mapinfo.end(); r++)
    {
      info = r->second;
      if (info->mapnumber <= 0)
	I_Error("Map numbers must be positive (%s)!\n", info->lumpname.c_str());

      if (!info->found)
	continue; // non-present maps are not assigned to clusters

      n = info->cluster;
      if (!clustermap.count(n))
	{
	  CONS_Printf(" New cluster %d (map %d)\n", n, info->mapnumber);
	  // create the cluster
	  cl = clustermap[n] = new MapCluster(n);
	  cl->hub = cl->keepstuff = true; // for original Hexen
	}
      else
	cl = clustermap[n];

      cl->maps.push_back(info);
      info->hub = cl->hub; // TEST copy the hub info to MapInfos
    }

  // and then check that all clusters have at least one map
  cluster_iter_t t, s;
  for (s = clustermap.begin(); s != clustermap.end(); )
    {
      t = s++; // erase will invalidate t

      cl = t->second;
      if (cl->maps.empty())
	{
	  CONS_Printf(" Cluster %d has no maps!\n", cl->number);
	  clustermap.erase(t);
	  delete cl;
	}
    }

  // time to unravel the warptrans numbering.
  map<int, MapInfo *> warptransmap;
  for (r = mapinfo.begin(); r != mapinfo.end(); r++)
    {
      info = r->second;
      warptransmap[info->warptrans] = info;
    }

  // now just put the correct exit data in the MapInfos
  for (r = mapinfo.begin(); r != mapinfo.end(); r++)
    {
      info = r->second;

      // set normal and secret exits
      // 'nextlevel' overrides 'next'
      if (info->nextlevel < 0 && info->warpnext > 0)
	info->nextlevel = warptransmap[info->warpnext]->mapnumber;
    }

  // backwards compatibility
  if (episodes.empty() && !mapinfo.empty())
    {
      info = mapinfo.begin()->second;
      episodes.push_back(new Episode(info->lumpname.c_str(), "Episode"));
    }

  // find direct pointers for convenience
  n = episodes.size();
  for (i=0; i<n; i++)
    {
      ep = episodes[i];
      ep->minfo = FindMapInfo(ep->maplump.c_str());
      if (ep->minfo)
	ep->active = ep->minfo->found;
    }

  n = mapinfo.size();
  CONS_Printf(" %d maps found.\n", n);
  return n;
}



//==============================================================
//   GameInfo utilities related to MapInfo and MapCluster
//==============================================================

void GameInfo::Clear_mapinfo_clustermap()
{
  // delete old mapinfo and clusterdef
  mapinfo_iter_t s;
  for (s = mapinfo.begin(); s != mapinfo.end(); s++)
    delete s->second;
  mapinfo.clear(); 

  cluster_iter_t t;
  for (t = clustermap.begin(); t != clustermap.end(); t++)
    delete t->second;
  clustermap.clear();

  currentcluster = NULL;

  for (unsigned i=0; i < episodes.size(); i++)
    delete episodes[i];
  episodes.clear();
}


MapCluster *GameInfo::FindCluster(int c)
{
  cluster_iter_t i = clustermap.find(c);
  if (i == clustermap.end())
    return NULL;

  return i->second;
}


MapInfo *GameInfo::FindMapInfo(int c)
{
  mapinfo_iter_t i = mapinfo.find(c);

  if (i == mapinfo.end())
    return NULL;

  return i->second;
}


MapInfo *GameInfo::FindMapInfo(const char *name)
{ 
  char *tail;
  int n = strtol(name, &tail, 10);

  if (tail != name)
    return FindMapInfo(n); // by number

  for (mapinfo_iter_t i = mapinfo.begin(); i != mapinfo.end(); i++)
  {
    
    //CONS_Printf(" [%s][%d]::FindMapInfo Find=\"%s\"? [%2d]:Found LumpName = \"%8s\" \n",
    //            __FILE__, __LINE__, name, i->first, i->second->lumpname.c_str());
                
    if (i->second->lumpname == name)
      return i->second;
  }
  
  return NULL;
}


MapInfo *GameInfo::FindNextMap(int mapnum, int incr)
{
  mapinfo_iter_t i = mapinfo.find(mapnum);
  if (i == mapinfo.end())
    i = mapinfo.begin();

  if (incr == -1)
    {
      if (i != mapinfo.begin())
	i--;
      else
	return mapinfo.rbegin()->second;
    }
  else if (incr == 1)
    {
      i++;
      if (i == mapinfo.end())
	i = mapinfo.begin();
    }

  return i->second;
}


const Episode *GameInfo::GetEpisode(int n)
{
  int num = episodes.size();
  if (!num)
    return NULL;
  n = (n >= num) ? num-1 : ((n < 0) ? 0 : n);
  return episodes[n];
}
