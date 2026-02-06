// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: dehacked.cpp 704 2010-07-12 20:09:32Z smite-meister $
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
/// \brief DeHackEd and BEX support

#include <stdarg.h>
#include <ctype.h>

#include "dehacked.h"
#include "parser.h"
#include "mnemonics.h"

#include "g_game.h"
#include "g_actor.h"
#include "g_pawn.h"

#include "dstrings.h"
#include "d_items.h"
#include "info.h"
#include "sounds.h"

#include "w_wad.h"
#include "z_zone.h"
#include "a_functions.h" // action function prototypes


dehacked_t DEH; // one global instance

extern char orig_sprnames[NUMSPRITES][5];
static char save_sprnames[NUMSPRITES][5];
static actionf_p1 *d_actions;
static actionf_p2 *w_actions;


weapon_mnemonic_t BEX_WeaponMnemonics[] = 
{
#define WEAPON(x) {#x, A_ ## x},
#define DACTOR(x)
#include "a_functions.h"
  {"NULL", NULL},
  {NULL, NULL}
};

dactor_mnemonic_t BEX_DActorMnemonics[] = 
{
#define WEAPON(x)
#define DACTOR(x) {#x, A_ ## x},
#include "a_functions.h"
  {"NULL", NULL},
  {NULL, NULL}
};



//========================================================================
//  Mappings for various tables
//========================================================================

enum
{
  NUM_DOOM_STATES = 967, // all states in Doom (with null)
  NUM_LEGACY_STATES = 10,
  NUM_HERETIC_STATES = 1205, // with null
  NUM_HEXEN_STATES = 2846, // with null

  NUM_DOOM_THINGS = 137,
  NUM_LEGACY_THINGS = 12,
  NUM_HERETIC_THINGS = 160,
  NUM_HEXEN_THINGS = 398,

  // Offsets for both things and states. These must remain in order (D < H < X)
  OFS_HERETIC  = 10000,
  OFS_HEXEN    = 20000,
};


// reads an index like H232 and converts it into a number (for internal use only)
static int ReadTableIndex(const char *p)
{
  if (isdigit(p[0]))
    return atoi(p);
  else switch (p[0])
    {
    case 'H':
      return OFS_HERETIC + atoi(p+1);

    case 'X':
      return OFS_HEXEN + atoi(p+1);

    default:
      return atoi(p+1);
    }
}


// Our mobj tables have no gaps!
static int ThingMap(const char *thref)
{
  int num = ReadTableIndex(thref) - 1; // stupid DEH thing numbering starts with 1 and not 0

  if (num < 0)
    goto err;

  if (num < OFS_HERETIC)
    {
      if (num < NUM_DOOM_THINGS)
	return MT_DOOM + num;
    }
  else if (num < OFS_HEXEN)
    {
      if (num - OFS_HERETIC < NUM_HERETIC_THINGS)
	return MT_HERETIC + num - OFS_HERETIC;
    }
  else if (num - OFS_HEXEN < NUM_HEXEN_THINGS)
    return MT_HEXEN + num - OFS_HEXEN;

 err:
  DEH.error("Thing %d doesn't exist\n", num+1);
  return MT_DEFAULT_THING;
}


// Our state tables have lots of gaps due to the mobj/weaponstate separation:(
// State number remapping: weapon states get negated numbers.
static int StateMap(const char *stref)
{
  if (!stref)
    {
      DEH.error("Missing state reference\n");
      return 0;
    }

  int num = ReadTableIndex(stref);

  /// Describes one gap in our state table.
  struct stategap_t
  {
    statenum_t       gap_start;    // gap begins after this state
    weaponstatenum_t first_wstate; // the first weaponstate constituting the gap
  };

  int n;
  stategap_t *gap;

  if (num < 0)
    goto err;
  else if (num < OFS_HERETIC)
    {
      if (num < NUM_DOOM_STATES)
	{
	  static stategap_t Doom_gaps[] =
	  {
	    {S_NULL, S_LIGHTDONE},
	    {S_DOOM_END, S_HLIGHTDONE},
	    {S_DOOM_END, S_HLIGHTDONE} // the last row must be duplicated...
	  };
	  // Doom:
	  // 0 null state
	  // 1-89 doom weapons
	  // 90-966 doom things
	  // (967-976 legacy additions) (cannot be accessed using DeHackEd)

	  n = sizeof(Doom_gaps)/sizeof(stategap_t) - 1;
	  gap = Doom_gaps;
	}
      else
	goto err;
    }
 else if (num < OFS_HEXEN)
    {
      if (num - OFS_HERETIC < NUM_HERETIC_STATES)
	{
	  num += S_HERETIC - OFS_HERETIC;

	  // Heretic: 9 separate groups of weapon states
 	  static stategap_t Heretic_gaps[] =
	  {
	    {S_HTFOG13, S_HLIGHTDONE},
	    {S_STAFFPUFF2_6, S_BEAKREADY},
	    {S_WGNT, S_GAUNTLETREADY},
	    {S_BLSR, S_BLASTERREADY},
	    {S_WMCE, S_MACEREADY},
	    {S_WSKL, S_HORNRODREADY},
	    {S_RAINAIRXPLR4_3, S_GOLDWANDREADY},
	    {S_WPHX, S_PHOENIXREADY},
	    {S_WBOW, S_CRBOW1},
	    {S_HERETIC_END, S_XLIGHTDONE},
	    {S_HERETIC_END, S_XLIGHTDONE},
	  };

	  n = sizeof(Heretic_gaps)/sizeof(stategap_t) - 1;
	  gap = Heretic_gaps;
	}
      else
	goto err;
    }
  else if (num - OFS_HEXEN < NUM_HEXEN_STATES)
    {
      num += S_HEXEN - OFS_HEXEN;

      // Hexen: 13 separate groups of weapon states 
      static stategap_t Hexen_gaps[] =
      {
	{S_TELESMOKE26, S_XLIGHTDONE},
	{S_AXE, S_FAXEREADY},
	{S_HAMM, S_FHAMMERREADY},
	{S_HAMMERPUFF5, S_FSWORDREADY},
	{S_FSWORD_FLAME10, S_CMACEREADY},
	{S_CSTAFF, S_CSTAFFREADY},
	{S_CFLAME8, S_CFLAMEREADY1},
	{S_CFLAME_MISSILE_X, S_CHOLYREADY},
	{S_HOLY_MISSILE_P5, S_MWANDREADY},
	{S_MW_LIGHTNING8, S_MLIGHTNINGREADY},
	{S_LIGHTNING_ZAP_X8, S_MSTAFFREADY},
	{S_MSTAFF3, S_SNOUTREADY},
	{S_COS3, S_CONEREADY},
	{S_HEXEN_END, NUMWEAPONSTATES},
	{S_HEXEN_END, NUMWEAPONSTATES},
      };

      n = sizeof(Hexen_gaps)/sizeof(stategap_t) - 1;
      gap = Hexen_gaps;
    }
  else
    goto err;

  for (int i=0; i<n; i++)
    {
      int len = gap[i+1].first_wstate - gap[i].first_wstate; // gap lenght
      if (num <= gap[i].gap_start)
	return num;
      else if (num <= gap[i].gap_start + len)
	return -(gap[i].first_wstate + num - gap[i].gap_start - 1);
      num -= len;
    }
  
  I_Error("DEH: should never arrive here!\n");

 err:
  DEH.error("Frame %d doesn't exist!\n", num);
  return S_DEFAULT_STATE;
}



// Mapping for Doom sound id's.
static int SoundMap(int id)
{
  // The original sound id enum from Doom, used as a mapping.
  // Not quite perfect, since we have splitted some sounds
  // (and this table only covers one of them).
  static int doom_sounds[] =
  {
    sfx_None,
    sfx_pistol,
    sfx_shotgn,
    sfx_sgcock,
    sfx_dshtgn,
    sfx_dbopn,
    sfx_dbcls,
    sfx_dbload,
    sfx_plasma,
    sfx_bfg,
    sfx_sawup,
    sfx_sawidl,
    sfx_sawful,
    sfx_sawhit,
    sfx_rlaunc,
    sfx_rxplod,
    sfx_firsht,
    sfx_firxpl,
    sfx_platstart, //sfx_pstart,
    sfx_platstop,  //sfx_pstop,
    sfx_doropn,
    sfx_dorcls,
    sfx_floormove, //sfx_stnmov,
    sfx_switchon,  //sfx_swtchn,
    sfx_switchoff, //sfx_swtchx,
    sfx_plpain,
    sfx_dmpain,
    sfx_popain,
    sfx_vipain,
    sfx_mnpain,
    sfx_pepain,
    sfx_gib, //sfx_slop,
    sfx_itemup,
    sfx_weaponup, //sfx_wpnup,
    sfx_grunt,    //sfx_oof,
    sfx_teleport, //sfx_telept,
    sfx_posit1,
    sfx_posit2,
    sfx_posit3,
    sfx_bgsit1,
    sfx_bgsit2,
    sfx_sgtsit,
    sfx_cacsit,
    sfx_brssit,
    sfx_cybsit,
    sfx_spisit,
    sfx_bspsit,
    sfx_kntsit,
    sfx_vilsit,
    sfx_mansit,
    sfx_pesit,
    sfx_sklatk,
    sfx_sgtatk,
    sfx_skepch,
    sfx_vilatk,
    sfx_claw,
    sfx_skeswg,
    sfx_pldeth,
    sfx_pdiehi,
    sfx_podth1,
    sfx_podth2,
    sfx_podth3,
    sfx_bgdth1,
    sfx_bgdth2,
    sfx_sgtdth,
    sfx_cacdth,
    sfx_usefail, //sfx_skldth,
    sfx_brsdth,
    sfx_cybdth,
    sfx_spidth,
    sfx_bspdth,
    sfx_vildth,
    sfx_kntdth,
    sfx_pedth,
    sfx_skedth,
    sfx_posact,
    sfx_bgact,
    sfx_dmact,
    sfx_bspact,
    sfx_bspwlk,
    sfx_vilact,
    sfx_usefail, //sfx_noway,
    sfx_barexp,
    sfx_punch,
    sfx_hoof,
    sfx_metal,
    sfx_chgun,
    sfx_message, //sfx_tink,
    sfx_bdopn,
    sfx_bdcls,
    sfx_itemrespawn, //sfx_itmbk,
    sfx_flame,
    sfx_flamst,
    sfx_powerup, //sfx_getpow,
    sfx_bospit,
    sfx_boscub,
    sfx_bossit,
    sfx_bospn,
    sfx_bosdth,
    sfx_manatk,
    sfx_mandth,
    sfx_sssit,
    sfx_ssdth,
    sfx_keenpn,
    sfx_keendt,
    sfx_skeact,
    sfx_skesit,
    sfx_skeatk,
    sfx_message, //sfx_radio
  };

  if (id >= 0 && id <= 108)
    return doom_sounds[id];

  DEH.error("Sound id %d doesn't exist.\n", id);
  return sfx_None;
}



//========================================================================
//  The DeHackEd class
//========================================================================


dehacked_t::dehacked_t()
{
  loaded = false;
  num_errors = 0;

  idfa_armor = 200;
  idfa_armorfactor = 0.5;
  idkfa_armor = 200;
  idkfa_armorfactor = 0.5;
  god_health = 100;

  // TODO these are not yet used...
  max_health = 200;
  max_soul_health = 200;
  initial_bullets = 50;
}


void dehacked_t::error(const char *first, ...)
{
#define BUF_SIZE 1024
  char buffer[BUF_SIZE];
  va_list ap;
 
  va_start(ap, first);
  vsnprintf(buffer, BUF_SIZE, first, ap);
  va_end(ap);

  CONS_Printf("DEH: %s", buffer);
  num_errors++;
}


// A small hack for retrieving values following a '=' sign.
// Does not change the parser state.
int dehacked_t::FindValue()
{
  char *temp = p.Pointer(); // save the current location

  // find the first occurrence of str
  char *res = strstr(temp, "=");
  if (res)
    p.SetPointer(++res); // pass the = sign
  else
    {
      error("Missing equality sign!\n");
      return 0;
    }

  int value;
  if (!p.MustGetInt(&value))
    {
      error("No value found\n");
      value = 0;
    }

  p.SetPointer(temp); // and go back
  return value;
}


// Bigger HACK for retrieving special state numbers and mapping them.
// Does not change the parser state.
int dehacked_t::FindState()
{
  char *temp = p.Pointer(); // save the current location

  // find the first occurrence of "="
  char *res = strstr(temp, "=");
  if (res)
    p.SetPointer(++res); // pass the = sign
  else
    {
      error("Missing equality sign!\n");
      return 0;
    }

  res = p.GetToken(" \t");
  p.SetPointer(temp); // restore parser

  return StateMap(res);
}


bool dehacked_t::ReadFlags(mobjinfo_t *m)
{
  m->flags = 0;
  m->flags2 = 0;

  char *word = p.GetToken("=+| \t");

  // we allow bitwise-ORed combinations of BEX mnemonics and numeric values
  while (word)
    {
      if (isdigit(word[0]))
	{
	  // old-style numeric entry, just do the conversion
	  int temp = atoi(word);
	  for (int i=0; i<26; i++)
	    if (temp & OriginalFlags[i].old_flag)
	      {
		int f = OriginalFlags[i].new_flag;
		if (f == 0)
		  error("NOTE: Flag %d has no in-game effect.\n", OriginalFlags[i].old_flag);
		m->flags |= f;
	      }

	  word = p.GetToken("+| \t"); // next token
	  continue;
	}

      
      // must be a mnemonic
      flag_mnemonic_t *b = BEX_FlagMnemonics;

      for ( ; b->name; b++)
	if (!strcasecmp(word, b->name))
	  {
	    switch (b->flagword)
	      {
	      case 1:
		m->flags |= b->flag;
		break;

	      case 2:
	      default:
		m->flags2 |= b->flag;
		break;
	      }

	    break;
	  }

      if (!b->name)
	{
	  error("Unknown bit mnemonic '%s'\n", word);
	}
		
      word = p.GetToken("+| \t"); // next token
    }

  return true;
}




// Utility for setting codepointers / action functions.
// Accepts both numeric and BEX mnemonic references.
// The 'to' state must already be mapped.
static void SetAction(int to, const char *mnemonic)
{
  if (isdigit(mnemonic[0]) || isdigit(mnemonic[1])) // no mnemonic has a digit as the first or second char
    {
      // this must also handle strings like H111
      int from = StateMap(mnemonic);
      if (to > 0)
	if (from > 0)
	  states[to].action = d_actions[from];
	else
	  DEH.error("Tried to use a weapon codepointer '%s' in a thing frame!\n", mnemonic);
      else
	if (from < 0)
	  weaponstates[-to].action = w_actions[-from];
	else
	  DEH.error("Tried to use a thing codepointer '%s' in a weapon frame!\n", mnemonic);

      return;
    }

  if (to > 0)
    {
      dactor_mnemonic_t *m;
      for (m = BEX_DActorMnemonics; m->name && strcasecmp(mnemonic, m->name); m++);
      if (!m->name)
	DEH.error("[CODEPTR]: Unknown mnemonic '%s'\n", mnemonic);
      states[to].action = m->ptr;
    }
  else
    {
      weapon_mnemonic_t *w;
      for (w = BEX_WeaponMnemonics; w->name && strcasecmp(mnemonic, w->name); w++);
      if (!w->name)
	DEH.error("[CODEPTR]: Unknown mnemonic '%s'\n", mnemonic);
      weaponstates[-to].action = w->ptr;
    }
}



//========================================================================
// Load a dehacked file format 6. I (BP) don't know other format
//========================================================================

/*
  Thing sample:
Thing 1 (Player)
ID # = 3232              -1,             // doomednum
Initial frame = 32       S_PLAY,         // spawnstate
Hit points = 3232        100,            // spawnhealth
First moving frame = 32  S_PLAY_RUN1,    // seestate
Alert sound = 32         sfx_None,       // seesound
Reaction time = 3232     0,              // reactiontime
Attack sound = 32        sfx_None,       // attacksound
Injury frame = 32        S_PLAY_PAIN,    // painstate
Pain chance = 3232       255,            // painchance
Pain sound = 32          sfx_plpain,     // painsound
Close attack frame = 32  S_NULL,         // meleestate
Far attack frame = 32    S_PLAY_ATK1,    // missilestate
Death frame = 32         S_PLAY_DIE1,    // deathstate
Exploding frame = 32     S_PLAY_XDIE1,   // xdeathstate
Death sound = 32         sfx_pldeth,     // deathsound
Speed = 3232             0,           // speed   These three are given in 16.16 fixed point
Width = 211812352        16,          // radius    *
Height = 211812352       56,          // height    *
Mass = 3232              100,            // mass
Missile damage = 3232    0,              // damage
Action sound = 32        sfx_None,       // activesound
Bits = 3232              MF_SOLID|MF_SHOOTABLE|MF_DROPOFF|MF_PICKUP|MF_NOTDMATCH,
Respawn frame = 32       S_NULL          // raisestate
*/

void dehacked_t::Read_Thing(const char *str)
{
  int t = ThingMap(str);

  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      char *word = p.GetToken(" \t"); // get first word

      // special handling for mnemonics
      if (!strcasecmp(word, "Bits"))
	{
	  ReadFlags(&mobjinfo[t]); continue;
	}

      int value = FindValue();

      // set the value in appropriate field
      if (!strcasecmp(word, "ID"))            mobjinfo[t].doomednum    = value;
      else if (!strcasecmp(word,"Hit"))       mobjinfo[t].spawnhealth  = value;
      else if (!strcasecmp(word,"Alert"))     mobjinfo[t].seesound     = SoundMap(value);
      else if (!strcasecmp(word,"Reaction"))  mobjinfo[t].reactiontime = value;
      else if (!strcasecmp(word,"Attack"))    mobjinfo[t].attacksound  = SoundMap(value);
      else if (!strcasecmp(word,"Pain"))
	{
	  word = p.GetToken(" \t");
	  if (!strcasecmp(word,"chance"))     mobjinfo[t].painchance = value;
	  else if (!strcasecmp(word,"sound")) mobjinfo[t].painsound  = SoundMap(value);
	}
      else if (!strcasecmp(word,"Death"))
	{
	  word = p.GetToken(" \t");
	  if (!strcasecmp(word,"frame"))
	    {
	      value = FindState();
	      if (value < 0)
		{
		  error("Thing %s : Weapon states must not be used with Things!\n", str);
		  continue;
		}
	      mobjinfo[t].deathstate  = &states[value];
	    }
	  else if (!strcasecmp(word,"sound")) mobjinfo[t].deathsound  = SoundMap(value);
	}
      else if (!strcasecmp(word,"Speed"))     mobjinfo[t].speed       = float(value)/int(fixed_t::UNIT);
      else if (!strcasecmp(word,"Width"))     mobjinfo[t].radius      = float(value)/int(fixed_t::UNIT);
      else if (!strcasecmp(word,"Height"))    mobjinfo[t].height      = float(value)/int(fixed_t::UNIT);
      else if (!strcasecmp(word,"Mass"))      mobjinfo[t].mass        = value;
      else if (!strcasecmp(word,"Missile"))   mobjinfo[t].damage      = value;
      else if (!strcasecmp(word,"Action"))    mobjinfo[t].activesound = SoundMap(value);
      else
	{
	  value = FindState();
	  if (value < 0)
	    {
	      error("Thing %s : Weapon states must not be used with Things!\n", str);
	      continue;
	    }

	  if (!strcasecmp(word,"Initial"))        mobjinfo[t].spawnstate   = &states[value];
	  else if (!strcasecmp(word,"First"))     mobjinfo[t].seestate     = &states[value];
	  else if (!strcasecmp(word,"Injury"))    mobjinfo[t].painstate    = &states[value];
	  else if (!strcasecmp(word,"Close"))     mobjinfo[t].meleestate   = &states[value];
	  else if (!strcasecmp(word,"Far"))       mobjinfo[t].missilestate = &states[value];
	  else if (!strcasecmp(word,"Exploding")) mobjinfo[t].xdeathstate  = &states[value];
	  else if (!strcasecmp(word,"Respawn"))   mobjinfo[t].raisestate   = &states[value];
	  else error("Thing %s : Unknown field '%s'\n", str, word);
	}
    }
}


/*
  Frame sample:
Sprite number = 10
Sprite subnumber = 32968
Duration = 200
Next frame = 200
Codep = 111 // Legacy addition
*/

void dehacked_t::Read_Frame(const char *str)
{
  int s = StateMap(str);

  if (s == 0)
    {
      DEH.error("You must not modify frame 0.\n");
      return;
    }

  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      int value = FindValue();

      // set the value in appropriate field
      char *word = p.GetToken(" \t");

      if (s > 0)
	{
	  state_t *state = &states[s];
	  if (!strcasecmp(word,"Sprite"))
	    {
	      word = p.GetToken(" \t");
	      if (!strcasecmp(word,"number"))         state->sprite = spritenum_t(value);
	      else if (!strcasecmp(word,"subnumber")) state->frame  = value;
	    }
	  else if (!strcasecmp(word,"Duration"))      state->tics      = value;
	  else if (!strcasecmp(word,"Next"))
	    {
	      value = FindState();
	      if (value < 0)
		{
		  error("Frame %s: A mobj frame cannot be followed by a weapon frame!\n", str);
		  continue;
		}

	      state->nextstate = &states[value];
	    }
	  else if (!strcasecmp(word,"Codep"))
	    {
	      word = p.GetToken(" \t=");
	      SetAction(s, word);
	    }
	  else error("Frame %s : Unknown field '%s'\n", str, word);
	}
      else
	{
	  weaponstate_t *wstate = &weaponstates[-s];
	  if (!strcasecmp(word,"Sprite"))
	    {
	      word = p.GetToken(" \t");
	      if (!strcasecmp(word,"number"))         wstate->sprite = spritenum_t(value);
	      else if (!strcasecmp(word,"subnumber")) wstate->frame  = value;
	    }
	  else if (!strcasecmp(word,"Duration"))      wstate->tics      = value;
	  else if (!strcasecmp(word,"Next"))
	    {
	      value = FindState();
	      if (value > 0)
		{
		  error("Weaponframe %s: A weapon frame cannot be followed by a mobj frame!\n", str);
		  continue;
		}

	      wstate->nextstate = &weaponstates[-value];
	    }
	  else if (!strcasecmp(word,"Codep"))
	    {
	      word = p.GetToken(" \t=");
	      SetAction(s, word);
	    }
	  else error("Weaponframe %s : Unknown field '%s'\n", str, word);
	}
    }
}


// deprecated
void dehacked_t::Read_Sound(int num)
{
  error("Sound command has been deprecated. Use SNDINFO.\n");
  return;

  if (num >= NUMSFX || num < 0)
    {
      error("Sound %d doesn't exist\n");
      return;
    }
  
  while (p.NewLine(false))
    {
      // TODO dehacked sound commands
      /*

      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      int value = FindValue();
      char *word = p.GetToken(" ");

	if (!strcasecmp(word,"Offset"))
	  {
	    value -= 150360;
	    if (value<=64)
	      value/=8;
	    else if (value<=260)
	      value=(value+4)/8;
	    else value=(value+8)/8;

	    if (value>=-1 && value < NUMSFX-1)
	      strcpy(S_sfx[num].lumpname, savesfxnames[value+1]);
	    else
	      error("Sound %d : offset out of bound\n",num);
	  }
	else if (!strcasecmp(word,"Zero/One"))
	  S_sfx[num].multiplicity = value;
	else if (!strcasecmp(word,"Value"))
	  S_sfx[num].priority   =value;
	else
	  error("Sound %d : unknown word '%s'\n",num,word);
      */
    }
}


// this part of dehacked really sucks, but it still partly supported
void dehacked_t::Read_Text(int len1, int len2)
{
  char s[2001];
  int i;

  // dehacked text
  // It is hard to change all the text in Doom.
  // Here we implement only the vital, easy things.
  // Yes, "text" can change some tables like music, sound and sprite names
  if (len1+len2 > 2000)
    {
      error("Text too long\n");
      return;
    }
  
  if (p.ReadChars(s, len1 + len2) != len1 + len2)
    {
      error("Text reading failed\n");
      return;
    }

  // sound table (not supported anymore)
  /*
    for (i=0;i<NUMSFX;i++)
      if (!strncmp(savesfxname[i],s,len1))
      {
        strncpy(S_sfx[i].lumpname,&(s[len1]),len2);
        S_sfx[i].lumpname[len2]='\0';
        return;
      }
  */

  // sprite table
  if (len1 == 4 && len2 == 4)
    {
      for (i=0; i<NUMSPRITES; i++)
	if (!strncmp(save_sprnames[i], s, 4))
	  {
	    strncpy(orig_sprnames[i], &s[len1], 4);
	    return;
	  }
    }


  // music table
  extern char* MusicNames[NUMMUSIC];
  for (i=1; i<NUMMUSIC; i++)
    if (MusicNames[i] && !strncmp(MusicNames[i], s, len1))
      {
        strncpy(MusicNames[i], &(s[len1]), len2);
        MusicNames[i][len2]='\0';
        return;
      }

  // text table
  for (i=0; i<NUMTEXT; i++)
    {
      int temp = strlen(text[i]);
      if (temp == len1 && !strncmp(text[i], s, len1))
	{
	  // FIXME odd. If I remove this comment, DEH crashes with a segfault.
	  // can't you write to static tables??
	  //if (temp < len2)  // increase size of the text
	    {
	      text[i] = (char *)malloc(len2 + 1);
	      if (!text[i])
		I_Error("Read_Text : Out of memory");
	    }

	  // FIXME, JussiP: we should not change these strings, as
	  // they are constants and new C standards specifically
	  // prohibit it, but Dehacked needs to patch the binary on
	  // the fly. Yes, it sucks and may break at any time in the
	  // future.
	  strncpy(const_cast<char*>(text[i]), s + len1, len2);
	  const_cast<char*>(text[i])[len2] = '\0';
	  return;
	}
    }

  s[len1] = '\0';
  error("Text not changed :%s\n", s);
}


/*
  Weapon sample:
Ammo type = 2
TODO ammopershoot
Deselect frame = 11
Select frame = 12
Bobbing frame = 13
Shooting frame = 17
Firing frame = 10
*/
void dehacked_t::Read_Weapon(int num)
{
  if (num >= NUMWEAPONS || num < 0)
    {
      error("Weapon %d doesn't exist\n", num);
      return;
    }

  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      int value = FindValue();
      char *word = p.GetToken(" \t");

      if (!strcasecmp(word,"Ammo"))
	wpnlev1info[num].ammo = ammotype_t(value);
      else
	{
	  value = -FindState();
	  if (value < 0)
	    {
	      error("Weapon %d : Thing states must not be used with weapons!\n", num);
	      continue;
	    }

	  if (!strcasecmp(word,"Deselect"))      wpnlev1info[num].upstate    = weaponstatenum_t(value);
	  else if (!strcasecmp(word,"Select"))   wpnlev1info[num].downstate  = weaponstatenum_t(value);
	  else if (!strcasecmp(word,"Bobbing"))  wpnlev1info[num].readystate = weaponstatenum_t(value);
	  else if (!strcasecmp(word,"Shooting"))
	    wpnlev1info[num].atkstate = wpnlev1info[num].holdatkstate        = weaponstatenum_t(value);
	  else if (!strcasecmp(word,"Firing"))   wpnlev1info[num].flashstate = weaponstatenum_t(value);
	  else error("Weapon %d : unknown word '%s'\n", num, word);
	}
    }
}


/*
  Ammo sample:
Max ammo = 400
Per ammo = 40
*/
void dehacked_t::Read_Ammo(int num)
{
  if (num >= NUMAMMO || num < 0)
    {
      error("Ammo %d doesn't exist\n", num);
      return;
    }

  // support only Doom ammo with this command
  const mobjtype_t clips[4][4] =
  {
    {MT_CLIP, MT_SHELL, MT_CELL, MT_ROCKETAMMO},
    {MT_AMMOBOX, MT_SHELLBOX, MT_CELLPACK, MT_ROCKETBOX},
    {MT_CHAINGUN, MT_SHOTGUN, MT_PLASMA, MT_ROCKETLAUNCH},
    {MT_NONE, MT_SUPERSHOTGUN, MT_BFG, MT_NONE}
  };

  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      int value = FindValue();
      char *word = p.GetToken(" \t");

      if (!strcasecmp(word,"Max"))
	maxammo1[num] = value;
      else if (!strcasecmp(word,"Per"))
	{
	  mobjinfo[clips[0][num]].spawnhealth = value;
	  mobjinfo[clips[1][num]].spawnhealth = 5*value;
	}
      else if (!strcasecmp(word,"Perweapon"))
	{
	  mobjinfo[clips[2][num]].spawnhealth = value;
	  if (clips[3][num] > MT_NONE)
	    mobjinfo[clips[3][num]].spawnhealth = value;
	}
      else
	error("Ammo %d : unknown word '%s'\n", num, word);
    }
}


// miscellaneous one-liners
void dehacked_t::Read_Misc()
{
  extern int MaxArmor[];
  const double ac = 1.0/6;

  char *word1, *word2;
  int value;

  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      value = FindValue();
      word1 = p.GetToken(" \t");
      word2 = p.GetToken(" \t");

      if (!strcasecmp(word1,"Initial"))
	{
	  if (!strcasecmp(word2,"Health"))       mobjinfo[MT_PLAYER].spawnhealth = value;
	  else if (!strcasecmp(word2,"Bullets")) initial_bullets = value;
	}
      else if (!strcasecmp(word1,"Max"))
	{
	  if (!strcasecmp(word2,"Health"))          max_health = value;
	  else if (!strcasecmp(word2,"Armor"))      MaxArmor[0] = value;
	  else if (!strcasecmp(word2,"Soulsphere")) max_soul_health = value;
	}
      else if (!strcasecmp(word1,"Green"))
	{
	  mobjinfo[MT_GREENARMOR].spawnhealth = value*100;
	  mobjinfo[MT_GREENARMOR].speed = ac*(value+1);
	}
      else if (!strcasecmp(word1,"Blue"))
	{
	  mobjinfo[MT_BLUEARMOR].spawnhealth = value*100;
	  mobjinfo[MT_BLUEARMOR].speed = ac*(value+1);
	}
      else if (!strcasecmp(word1,"Soulsphere")) mobjinfo[MT_SOULSPHERE].spawnhealth = value;
      else if (!strcasecmp(word1,"Megasphere")) mobjinfo[MT_MEGA].spawnhealth = value;
      else if (!strcasecmp(word1,"God"))        god_health = value;
      else if (!strcasecmp(word1,"IDFA"))
	{
	  word2 = p.GetToken(" \t");
	  if (!strcasecmp(word2,"="))          idfa_armor = value;
	  else if (!strcasecmp(word2,"Class")) idfa_armorfactor = ac*(value+1);
	}
      else if (!strcasecmp(word1,"IDKFA"))
	{
	  word2 = p.GetToken(" \t");
	  if (!strcasecmp(word2,"="))          idkfa_armor = value;
	  else if (!strcasecmp(word2,"Class")) idkfa_armorfactor = ac*(value+1);
	}
      else if (!strcasecmp(word1,"BFG")) wpnlev1info[wp_bfg].ammopershoot = value;
      else if (!strcasecmp(word1,"Monsters"))      {} // TODO
      else error("Misc : unknown command '%s'\n", word1);
    }
}

extern byte cheat_mus_seq[];
extern byte cheat_choppers_seq[];
extern byte cheat_god_seq[];
extern byte cheat_ammo_seq[];
extern byte cheat_ammonokey_seq[];
extern byte cheat_noclip_seq[];
extern byte cheat_commercial_noclip_seq[];
extern byte cheat_powerup_seq[7][10];
extern byte cheat_clev_seq[];
extern byte cheat_mypos_seq[];
extern byte cheat_amap_seq[];

static void change_cheat_code(byte *old, byte *n)
{
  for ( ; *n && *n != 0xff; old++, n++)
    if (*old == 1 || *old == 0xff) // no more place in the cheat
      {
	DEH.error("Cheat too long\n");
	return;
      }
    else
      *old = *n;

  // newcheatseq < oldcheat
  n = old;
  // search special cheat with 100
  for ( ; *n != 0xff; n++)
    if (*n == 1)
      {
	*old++ = 1;
	*old++ = 0;
	*old++ = 0;
	break;
      }
  *old = 0xff;

  return;
}


// deprecated
void dehacked_t::Read_Cheat()
{
  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      // FIXME how does this work?
      p.GetToken("=");
      byte *value = (byte *)p.GetToken(" \n"); // skip the space
      p.GetToken(" \n");         // finish the string
      char *word = p.GetToken(" \t");

      if (!strcasecmp(word,"Change"))        change_cheat_code(cheat_mus_seq,value);
      else if (!strcasecmp(word,"Chainsaw")) change_cheat_code(cheat_choppers_seq,value);
      else if (!strcasecmp(word,"God"))      change_cheat_code(cheat_god_seq,value);
      else if (!strcasecmp(word,"Ammo"))
	{
	  word = p.GetToken(" \t");
	  if (word && !strcasecmp(word,"&")) change_cheat_code(cheat_ammo_seq,value);
	  else                           change_cheat_code(cheat_ammonokey_seq,value);
	}
      else if (!strcasecmp(word,"No"))
	{
	  word = p.GetToken(" \t");
	  if (word)
	    word = p.GetToken(" \t");

	  if (word && !strcasecmp(word,"1")) change_cheat_code(cheat_noclip_seq,value);
	  else                           change_cheat_code(cheat_commercial_noclip_seq,value);
	}
      /* // FIXME! this could be replaced with a possibility to create new cheat codes (easy)
      else if (!strcasecmp(word,"Invincibility")) change_cheat_code(cheat_powerup_seq[0],value);
      else if (!strcasecmp(word,"Berserk"))       change_cheat_code(cheat_powerup_seq[1],value);
      else if (!strcasecmp(word,"Invisibility"))  change_cheat_code(cheat_powerup_seq[2],value);
      else if (!strcasecmp(word,"Radiation"))     change_cheat_code(cheat_powerup_seq[3],value);
      else if (!strcasecmp(word,"Auto-map"))      change_cheat_code(cheat_powerup_seq[4],value);
      else if (!strcasecmp(word,"Lite-Amp"))      change_cheat_code(cheat_powerup_seq[5],value);
      else if (!strcasecmp(word,"BEHOLD"))        change_cheat_code(cheat_powerup_seq[6],value);
      */
      else if (!strcasecmp(word,"Level"))         change_cheat_code(cheat_clev_seq,value);
      else if (!strcasecmp(word,"Player"))        change_cheat_code(cheat_mypos_seq,value);
      else if (!strcasecmp(word,"Map"))           change_cheat_code(cheat_amap_seq,value);
      else error("Cheat : unknown word '%s'\n",word);
    }
}


// Parses the BEX [CODEPTR] section
void dehacked_t::Read_CODEPTR()
{
  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      char *word = p.GetToken(" \t");
      if (strcasecmp(word, "Frame"))
	{
	  error("[CODEPTR]: Unknown command '%s'\n", word);
	  continue;
	}

      // the "to" state number
      int s = StateMap(p.GetToken(" \t="));

      if (s == 0)
	{
	  DEH.error("You must not modify frame 0.\n");
	  continue;
	}

      SetAction(s, p.GetToken(" \t="));
    }
}


// Parses the BEX [STRINGS] section
void dehacked_t::Read_STRINGS()
{
  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      char *word = p.GetToken(" \t");
      string_mnemonic_t *m;
      for (m = BEX_StringMnemonics; m->name && strcasecmp(word, m->name); m++);
      if (!m->name)
	{	  
	  error("[STRINGS]: Unknown mnemonic '%s'\n", word);
	  continue;
	}

      // FIXME backslash-continued lines...
      char *newtext = p.GetToken("=");
      text[m->num] = Z_StrDup(newtext);
    }
}



// dehacked command parser
enum DEH_cmd_t
{
  DEH_Thing,
  DEH_Frame,
  DEH_Pointer,
  DEH_Sound,
  DEH_Sprite,
  DEH_Text,
  DEH_Weapon,
  DEH_Ammo,
  DEH_Misc,
  DEH_Cheat,
  DEH_Doom,
  DEH_Patch,
  DEH_CODEPTR,
  DEH_PARS,
  DEH_STRINGS,
  DEH_NUM
};

static const char *DEH_cmds[DEH_NUM + 1] =
{
  "thing", "frame", "pointer", "sound", "sprite", "text", "weapon", "ammo", "misc", "cheat", "doom", "patch",
  "[CODEPTR]", "[PARS]", "[STRINGS]", NULL
};


// Parse a DeHackEd lump
// (there is special trick for converting .deh files into WAD lumps)
bool dehacked_t::LoadDehackedLump(int lump)
{
  if (!p.Open(lump))
    return false;

  p.DeleteChars('\r'); // annoying cr's.

  num_errors = 0;

  // original values
  d_actions = (actionf_p1 *)Z_Malloc(int(NUMSTATES) * sizeof(actionf_p1), PU_STATIC, NULL);
  w_actions = (actionf_p2 *)Z_Malloc(int(NUMWEAPONSTATES) * sizeof(actionf_p2), PU_STATIC, NULL);

  int i;

  // save original values
  for (i=0; i<NUMSTATES; i++)
    d_actions[i] = states[i].action;
  for (i=0; i<NUMWEAPONSTATES; i++)
    w_actions[i] = weaponstates[i].action;

  memcpy(save_sprnames, orig_sprnames, sizeof(save_sprnames));


  p.RemoveComments('#', true);
  while (p.NewLine())
    {
      char *word1, *word2;

      if ((word1 = p.GetToken(" \t")))
	{
	  word2 = p.GetToken(" \t");
	  if (word2)
	    i = atoi(word2);
	  else
	    {
	      i = 0;
	      //error("Warning: missing argument for '%s'\n", word1);
	    }

	  switch (P_MatchString(word1, DEH_cmds))
	    {
	    case DEH_Thing:
	      Read_Thing(word2);
	      break;

	    case DEH_Frame:
	      Read_Frame(word2);
	      break;

	    case DEH_Pointer:
	      /*
		Syntax:
		pointer uuu (frame xxx)
		codep frame = yyy
	      */
	      p.GetToken(" \t"); // get rid of "(frame"
	      if ((word1 = p.GetToken(")")))
		{
		  int s = StateMap(word1);
		  if (p.NewLine())
		    {
		      p.GetToken(" \t");
		      p.GetToken(" \t");
		      word2 = p.GetToken(" \t=");
		      SetAction(s, word2);
		    }
		}
	      else
		error("Pointer %d : (Frame xxx) missing\n", i);
	      break;

	    case DEH_Sound:
	      Read_Sound(i);
	      break;

	    case DEH_Sprite:
	      if (i < NUMSPRITES && i >= 0)
		{
		  if (p.NewLine())
		    {
		      int k;
		      k = (FindValue() - 151328) / 8;
		      if (k >= 0 && k < NUMSPRITES)
			strncpy(orig_sprnames[i], save_sprnames[k], 4);
		      else
			error("Sprite %i : offset out of bound\n", i);
		    }
		}
	      else
		error("Sprite %d doesn't exist\n", i);
	      break;

	    case DEH_Text:
	      if ((word1 = p.GetToken(" \t")))
		{
		  int j = atoi(word1);
		  Read_Text(i, j);
		}
	      else
		error("Text : missing second number\n");
	      break;

	    case DEH_Weapon:
	      Read_Weapon(i);
	      break;

	    case DEH_Ammo:
	      Read_Ammo(i);
	      break;

	    case DEH_Misc:
	      Read_Misc();
	      break;

	    case DEH_Cheat:
	      Read_Cheat();
	      break;

	    case DEH_Doom:
	      i = FindValue();
	      if (i != 19)
		error("Warning : Patch from a different Doom version (%d), only version 1.9 is supported\n", i);
	      break;

	    case DEH_Patch:
	      if (word2 && !strcasecmp(word2, "format"))
		{
		  if (FindValue() != 6)
		    error("Warning : Patch format not supported");
		}
	      break;

	      // BEX stuff
	    case DEH_CODEPTR:
	      Read_CODEPTR();
	      break;
	    case DEH_PARS:
	      // TODO support PARS?
	      error("BEX [PARS] block currently unsupported.\n");
	      break;
	    case DEH_STRINGS:
	      Read_STRINGS();
	      break;
	    default:
	      error("Unknown command : %s\n", word1);
	    }
	}
      else
        error("No word in this line:\n%s\n", p.GetToken("\0"));
    }

  if (num_errors > 0)
    {
      CONS_Printf("DEH: %d warning(s).\n", num_errors);
    }

  loaded = true;
  p.Clear();

  // HACK fix for phobia.wad
  mobjinfo[MT_TELEPORTMAN].flags &= ~MF_NOSECTOR; // teleport destinations must have sector links!
  return true;
}
