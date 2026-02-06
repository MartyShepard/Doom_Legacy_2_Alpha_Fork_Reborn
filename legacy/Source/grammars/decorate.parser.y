// Emacs style mode select   -*- C++ -*-
//---------------------------------------------------------------------
//
// $Id: $
//
// Copyright (C) 2006-2007 by DooM Legacy Team.
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
//---------------------------------------------------------------------

/// \file
/// \brief Lemon parser for DECORATE lumps.


%name DECORATE_Parse
%extra_argument {decorate_driver *d}

%parse_accept {
  //ActorInfo::Error("Parsing complete!\n");
}

%parse_failure {
  ActorInfo::Error("Giving up. Parser is hopelessly lost...\n");
}

%stack_overflow {
  ActorInfo::Error("Giving up. Parser stack overflow\n");
}

%token_type {yy_t}
%token_destructor { if (yymajor == STR) Z_Free(const_cast<char*>($$.stype)); }

%type str {const char *}
%type int {int}
%type num {float}

%include {
#include "z_zone.h"
#include "g_decorate.h"
#include "sounds.h"
#include "parser_driver.h"
#include "decorate.parser.h"

static ActorInfo *t; // replaces the unnecessary parser driver
}

//============================================================================

start ::= definitions.

// sequence of 0-N actor definitions
definitions ::= . // empty
definitions ::= definitions actor. // left recursion

actor ::= actor_init construction doomednum NL L_BRACE NL actor_properties R_BRACE NL.
  {
    t = NULL; // done updating it
  }
actor ::= NL.
actor ::= SEMICOLON. // HACK


// initialize temp variables
actor_init ::= ACTOR. { t = NULL; }


// after this we must have a valid ActorInfo pointer
construction ::= str(A).
  {
    t = aid.Find(A); // see if it already exists
    if (!t)
    {
      t = new ActorInfo(A);
      t->SetMobjType(aid.GetFreeMT());
      aid.Insert(t);
    }
  }
construction ::= str(A) COLON str(P). // inheritance
  {
    t = aid.Find(A); // see if it already exists
    if (!t)
    {
      ActorInfo *parent = aid.Find(P);
      if (!parent)
	{
	  ActorInfo::Error("Parent class '%s' is unknown.\n", P);
	  t = new ActorInfo(A);
	}
      else
	{
	  // copy of parent
	  t = new ActorInfo(*parent);
	  t->SetName(A);
	}

      t->SetMobjType(aid.GetFreeMT());
      aid.Insert(t);
    }
    else
    {
      ActorInfo::Error("Class '%s' already exists, and thus cannot inherit.\n", A);
    }
  }


doomednum ::= . // empty
doomednum ::= int(A).
  {
    t->doomednum = A;
    aid.InsertDoomEd(t, true);
  }


// list of 0-N properties
actor_properties ::= . // empty
actor_properties ::= actor_properties actor_property NL.

actor_property ::= .
actor_property ::= OBITUARY str(A).    { t->obituary = A; }
actor_property ::= HITOBITUARY str(A). { t->hitobituary = A; }
actor_property ::= MODEL str(A).    { t->modelname = A; }

actor_property ::= HEALTH int(A).       { t->spawnhealth = A; }
actor_property ::= REACTIONTIME int(A). { t->reactiontime = A; }
actor_property ::= PAINCHANCE int(A).   { t->painchance = A; }
actor_property ::= SPEED num(A).        { t->speed = A; }
actor_property ::= DAMAGE int(A).       { t->damage = A; }

actor_property ::= RADIUS num(A). { t->radius = A; }
actor_property ::= HEIGHT num(A). { t->height = A; }
actor_property ::= MASS num(A).   { t->mass = A; }

actor_property ::= SEESOUND str(A).    { t->seesound = S_GetSoundID(A); }
actor_property ::= ATTACKSOUND str(A). { t->attacksound = S_GetSoundID(A); }
actor_property ::= PAINSOUND str(A).   { t->painsound = S_GetSoundID(A); }
actor_property ::= DEATHSOUND str(A).  { t->deathsound = S_GetSoundID(A); }
actor_property ::= ACTIVESOUND str(A). { t->activesound = S_GetSoundID(A); }


// flags
actor_property ::= CLEARFLAGS.   { t->flags = 0; t->flags2 = 0; }
actor_property ::= MONSTER.      { t->SetFlag("MONSTER", true); }
actor_property ::= PROJECTILE.   { t->SetFlag("PROJECTILE", true); }
actor_property ::= PLUS  str(A). { t->SetFlag(A, true); }
actor_property ::= MINUS str(A). { t->SetFlag(A, false); }


// states
actor_property ::= STATES NL L_BRACE NL state_seqs R_BRACE. { t->UpdateSequences(); }

state_seqs ::= . // empty
state_seqs ::= state_seqs state_labeled_seq.

state_labeled_seq ::= state_label state_defs state_jump NL.  // labeled sequence of states

state_label ::= str(L) COLON NL. { t->AddLabel(L); }

state_defs ::= . // empty
state_defs ::= state_defs state_def NL.

state_def ::= str(S) str(F) int(T). { t->AddStates(S, F, T, NULL); } // no action func
state_def ::= str(S) str(F) int(T) str(A). { t->AddStates(S, F, T, A); } // with action func

state_jump ::= LOOP.        { t->FinishSequence(NULL, 0); }   // loop to beginning of sequence
state_jump ::= STOP.        { t->FinishSequence("", 0); }     // go to S_NULL
state_jump ::= GOTO str(L). { t->FinishSequence(L, 0); }      // go to another label
state_jump ::= GOTO str(L) PLUS int(A). { t->FinishSequence(L, A); }



// string
str(A) ::= STR(B). { A = B.stype; }

// int
int(A) ::= INT(B). { A = B.itype; }

// any real number
num(A) ::= FLOAT(B). { A = B.ftype; }
num(A) ::= INT(B).   { A = B.itype; } // make it a float
