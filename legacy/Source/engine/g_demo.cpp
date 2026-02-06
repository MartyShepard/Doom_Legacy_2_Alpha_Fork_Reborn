// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
/// \brief GameInfo class implementation:
/// Demo recording and playback related part

#include "doomdef.h"
#include "d_ticcmd.h"
#include "command.h"
#include "cvars.h"
#include "g_actor.h"

#include "i_system.h"

#include "r_main.h"

#include "g_game.h"
#include "g_player.h"
#include "g_map.h" // Map
#include "g_level.h"

#include "p_fab.h"
#include "m_argv.h"
#include "m_misc.h"

#include "w_wad.h"
#include "z_zone.h"


#warning The demo system is completely inoperational.

extern bool singletics;


bool         timingdemo;             // if true, exit with report on completion
tic_t        demostarttime;              // for comparative timing purposes

char         demoname[32];
bool         demorecording;
bool         demoplayback;
byte*        demobuffer;
byte*        demo_p;
byte*        demoend;
bool         singledemo;             // quit after playing a demo from cmdline

bool         precache = true;        // if true, load all graphics at start


//added:03-02-98:
//
// was G_Downgrade
//  'Downgrade' the game engine so that it is compatible with older demo
//   versions. This will probably get harder and harder with each new
//   'feature' that we add to the game. This will stay until it cannot
//   be done a 'clean' way, then we'll have to forget about old demos..
//   NOTE! The downgrading only affects players that already are in the game.
//
bool GameInfo::Downgrade(int version)
{
  extern int boomsupport;
  extern int allow_pushers;
  extern int variable_friction;


  if (version<120)
    return false;

  if( version<130 )
    {
      mobjinfo[MT_BLOOD].radius = 20;
      mobjinfo[MT_BLOOD].height = 16;
      mobjinfo[MT_BLOOD].flags  = MF_NOBLOCKMAP;
    }
  else
    {
      mobjinfo[MT_BLOOD].radius = 3;
      mobjinfo[MT_BLOOD].height = 0;
      mobjinfo[MT_BLOOD].flags  = 0;
    }

  // smoke trails for skull head attack since v1.25
  if (version<125)
    {
      states[S_ROCKET].action = NULL;

      states[S_SKULL_ATK3].action = NULL;
      states[S_SKULL_ATK4].action = NULL;
    }
  else
    {
      //activate rocket trails by default
      states[S_ROCKET].action     = A_SmokeTrailer;

      // smoke trails behind the skull heads
      states[S_SKULL_ATK3].action = A_SmokeTrailer;
      states[S_SKULL_ATK4].action = A_SmokeTrailer;
    }

  //SoM: 3/17/2000: Demo compatability
  if(version < 129)
    {
      boomsupport = 0;
      allow_pushers = 0;
      variable_friction = 0;
    }
  else
    {
      boomsupport = 1;
      allow_pushers = 1;
      variable_friction = 1;
    }

  // always true now, might be false in the future, if couldn't
  // go backward and disable all the features...
  demoversion = version;
  return true;
}


void G_DoneLevelLoad()
{
  CONS_Printf("Load Level in %f sec\n",(float)(I_GetTics()-demostarttime)/TICRATE);
  framecount = 0;
  demostarttime = I_GetTics ();
}

//
// DEMO RECORDING
//

#define ZT_FWD          0x01
#define ZT_SIDE         0x02
#define ZT_ANGLE        0x04
#define ZT_BUTTONS      0x08
#define ZT_AIMING       0x10
#define ZT_CHAT         0x20    // no more used
#define ZT_EXTRADATA    0x40
#define DEMOMARKER      0x80    // demoend

//ticcmd_t oldcmd[MAXPLAYERS];
vector<ticcmd_t> oldcmd;

// was G_ReadDemoTiccmd
//

  // HACK
inline Sint8 READCHAR(byte *p)
{
  Sint8 t = *(reinterpret_cast<Sint8*>(p));
  p++;
  return t;
}
inline Sint16 READSHORT(byte *p)
{
  Sint16 t = *(reinterpret_cast<Sint16*>(p));
  p += 2;
  return t;
}


void GameInfo::ReadDemoTiccmd(ticcmd_t* cmd, int playernum)
{
  if (*demo_p == DEMOMARKER)
    {
      // end of demo data stream
      CheckDemoStatus();
      return;
    }

  char ziptic=*demo_p++;

  if(ziptic & ZT_FWD)
    oldcmd[playernum].forward = READCHAR(demo_p);
  if(ziptic & ZT_SIDE)
    oldcmd[playernum].side = READCHAR(demo_p);
  if(ziptic & ZT_ANGLE)
    {
      if(demoversion<125)
	oldcmd[playernum].yaw = (*demo_p++)<<8;
      else
	oldcmd[playernum].yaw = READSHORT(demo_p);
    }
  if(ziptic & ZT_BUTTONS)
    oldcmd[playernum].buttons = *demo_p++;
  if(ziptic & ZT_AIMING)
    {
      if(demoversion<128)
	oldcmd[playernum].yaw = READCHAR(demo_p);
      else
	oldcmd[playernum].yaw = READSHORT(demo_p);
    }
  if(ziptic & ZT_CHAT)
    demo_p++;
      /*
      if(ziptic & ZT_EXTRADATA)
        ReadLmpExtraData(&demo_p,playernum);
      else
        ReadLmpExtraData(0,playernum);
      */
  memcpy(cmd,&(oldcmd[playernum]),sizeof(ticcmd_t));

}

// was G_WriteDemoTiccmd

void GameInfo::WriteDemoTiccmd(ticcmd_t* cmd, int playernum)
{
  char ziptic=0;
  byte *ziptic_p;

  ziptic_p=demo_p++;  // the ziptic
  // write at the end of this function

  if(cmd->forward != oldcmd[playernum].forward)
    {
      *demo_p++ = cmd->forward;
      oldcmd[playernum].forward = cmd->forward;
      ziptic|=ZT_FWD;
    }

  if(cmd->side != oldcmd[playernum].side)
    {
      *demo_p++ = cmd->side;
      oldcmd[playernum].side=cmd->side;
      ziptic|=ZT_SIDE;
    }

  if(cmd->yaw != oldcmd[playernum].yaw)
    {
      *(short *)demo_p = cmd->yaw;
      demo_p +=2;
      oldcmd[playernum].yaw=cmd->yaw;
      ziptic|=ZT_ANGLE;
    }

  if(cmd->buttons != oldcmd[playernum].buttons)
    {
      *demo_p++ = cmd->buttons;
      oldcmd[playernum].buttons=cmd->buttons;
      ziptic|=ZT_BUTTONS;
    }

  if(cmd->yaw != oldcmd[playernum].yaw)
    {
      *(short *)demo_p = cmd->yaw;
      demo_p+=2;
      oldcmd[playernum].yaw=cmd->yaw;
      ziptic|=ZT_AIMING;
    }

  /*
  if (AddLmpExtradata(&demo_p, playernum))
    ziptic|=ZT_EXTRADATA;
  */

  *ziptic_p=ziptic;
  //added:16-02-98: attention here for the ticcmd size!
  // latest demos with mouse aiming byte in ticcmd
  // FIXME! doesn't make much sense.
  //if (ziptic_p > demoend - (5*MAXPLAYERS))
  if (ziptic_p > demoend)
    {
      CheckDemoStatus();   // no more space
      return;
    }

  //  don't work in network the consistency is not copyed in the cmd
  //    demo_p = ziptic_p;
  //    G_ReadDemoTiccmd (cmd,playernum);         // make SURE it is exactly the same
}



//
// G_RecordDemo
//
void G_RecordDemo (char* name)
{
  int             i;
  int             maxsize = 0x20000;

  strcpy (demoname, name);
  strcat (demoname, ".lmp");
  i = M_CheckParm ("-maxdemo");
  if (i && i<myargc-1)
    maxsize = atoi(myargv[i+1])*1024;
  demobuffer = (byte *)Z_Malloc (maxsize,PU_STATIC,NULL);
  demoend = demobuffer + maxsize;

  demorecording = true;
}


void GameInfo::BeginRecording()
{
  //extern byte gamemap;
  int n = Players.size();
  demo_p = demobuffer;

  *demo_p++ = LEGACY_VERSION;
  *demo_p++ = skill;
  //*demo_p++ = currentlevel->episode; //gameepisode;
  *demo_p++ = 1; //FIXME! no more gamemap;
  *demo_p++ = cv_deathmatch.value;     // just to be compatible with old demo (no more used)
  *demo_p++ = cv_respawnmonsters.value;// just to be compatible with old demo (no more used)
  *demo_p++ = cv_fastmonsters.value;   // just to be compatible with old demo (no more used)
  *demo_p++ = cv_nomonsters.value;
  //*demo_p++ = consoleplayer->number;
  *demo_p++ = cv_timelimit.value;      // just to be compatible with old demo (no more used)
  *demo_p++ = multiplayer;             // 1.31

  // FIXME! the demo format must be changed to allow for any number of players
  // like *demo_p++ = players.size();
  for (int i=0 ; i<32 ; i++)
    {
      if (i < n)
        *demo_p++ = 1;
      else
        *demo_p++ = 0;
    }

  //memset(oldcmd, 0, sizeof(oldcmd));
  oldcmd.clear();
  oldcmd.resize(n);
}


//
// G_PlayDemo
//

void G_DeferedPlayDemo (char* name)
{
  COM.AppendText("playdemo \"");
  COM.AppendText(name);
  COM.AppendText("\"\n");
}


// was G_DoPlayDemo
//  Start a demo from a .LMP file or from a wad resource (eg: DEMO1)
//
void GameInfo::PlayDemo(char *defdemoname)
{
#warning FIXME debug aid
#if 0

  //  skill_t skill;
  int i, n, episode, level;

  // load demo file / resource

  //it's an internal demo
  if ((i = fc.FindNumForName(defdemoname)) == -1)
    {
      FIL_DefaultExtension(defdemoname,".lmp");
      if (!FIL_ReadFile(defdemoname, &demobuffer))
        {
          CONS_Printf ("\2ERROR: couldn't open file '%s'.\n", defdemoname);
          goto no_demo;
        }
      demo_p = demobuffer;
    }
  else
    demobuffer = demo_p = (byte *)fc.CacheLumpNum(i, PU_STATIC);

// read demo header

  demoversion = *demo_p++;
  if (demoversion < 109)
    {
      CONS_Printf ("\2ERROR: demo version too old.\n");
      demoversion = LEGACY_VERSION;
      Z_Free (demobuffer);
no_demo:
      return;
    }

  if (demoversion < LEGACY_VERSION)
    CONS_Printf ("\2Demo is from an older game version\n");


  int j = *demo_p++;
  Players.clear();

  demoplayback = true;
#endif
}

//
// G_TimeDemo
//             NOTE: name is a full filename for external demos
//

void G_TimeDemo(char* name)
{
  extern bool nodrawers;
  nodrawers = M_CheckParm ("-nodraw");
  timingdemo = true;
  singletics = true;
  framecount = 0;
  demostarttime = I_GetTics ();
  G_DeferedPlayDemo(name);
}


// reset engine variable set for the demos
// called from stopdemo command, map command, and g_checkdemoStatus.
void GameInfo::StopDemo()
{
  Z_Free(demobuffer);
  demoplayback  = false;
  timingdemo = false;
  singletics = false;

  Downgrade(LEGACY_VERSION);

  state = GS_NULL;
  //SV_StopServer();
  //    SV_StartServer();
  //SV_ResetServer();
}


//===================
// was G_CheckDemoStatus
// Called after a death or level completion to allow demos to be cleaned up
// Returns true if a new demo loop action will take place

bool GameInfo::CheckDemoStatus()
{
  if (timingdemo)
    {
      int time;
      float f1,f2;
      time = I_GetTics () - demostarttime;
      if(!time) return true;
      StopDemo ();
      timingdemo = false;
      f1 = time;
      f2 = framecount*TICRATE;
      CONS_Printf ("timed %i gametics in %i realtics\n%f seconds, %f avg fps\n",
                   tic, time, f1/TICRATE, f2/f1);

      pagetic = 0;
      return true;
    }

  if (demoplayback)
    {
      if (singledemo)
        I_Quit ();
      StopDemo();
      pagetic = 0;
      return true;
    }

  if (demorecording)
    {
      *demo_p++ = DEMOMARKER;
      FIL_WriteFile(demoname, demobuffer, demo_p - demobuffer);
      Z_Free(demobuffer);
      demorecording = false;

      CONS_Printf("\2Demo %s recorded\n",demoname);
      return true;
      //I_Quit ();
    }

  return false;
}
