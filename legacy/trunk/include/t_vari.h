// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// $Id: t_vari.h 515 2007-12-22 14:51:46Z jussip $
//
// Copyright(C) 2000 Simon Howard
// Copyright(C) 2001-2005 Doom Legacy Team
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
//
//--------------------------------------------------------------------------

/// \file
/// \brief FS variables

#ifndef t_vari_h
#define t_vari_h 1

#include "m_fixed.h"

/// FS variable types
enum svartype_t
{
  svt_string,
  svt_int,
  svt_fixed,
  svt_actor,        // a map object
  svt_pString,      // pointer to game string
  svt_pInt,
  svt_pFixed,
  svt_pActor,
  svt_function,     // functions are stored as variables
  svt_label,        // labels for goto calls are variables
  svt_const,        // temporary value type: adapts when first set
};


#define FIXED_TO_FLOAT(x) (float(x) / 65536.0f)


/// \brief Value of an FS variable
struct svalue_t
{
  int type;
  union
  {
    const char *s;
    int i; // also fixed
    class Actor *mobj;
    char *labelptr; // goto() label
  } value;
};

extern svalue_t nullvar;


inline int intvalue(svalue_t v)
{
  return (v.type == svt_string) ? atoi(v.value.s) :
    (v.type == svt_fixed) ? (v.value.i >> fixed_t::FBITS) :
    (v.type == svt_actor) ? (v.value.mobj ? 1 : 0) :
    v.value.i;
}

inline fixed_t fixedvalue(svalue_t v)
{
  fixed_t res;

  if (v.type == svt_fixed)
    res.setvalue(v.value.i);
  else if (v.type == svt_string)
    res = float(atof(v.value.s));
  else if (v.type == svt_actor)
    res = v.value.mobj ? 1 : 0;
  else
    res = v.value.i;

  return res;
}

const char *stringvalue(svalue_t v);
Actor *MobjForSvalue(svalue_t v);


/// \brief FS variable
struct svariable_t
{
  char *name;
  int   type;
  union
  {
    char    *s;
    int      i; // also fixed
    Actor   *mobj;
    // pointers to the same
    char    **pS;
    int      *pI;
    fixed_t  *pFixed;
    Actor   **pMobj;
    
    void (*handler)();   // for functions
    char *labelptr;      // for labels
  } value;
  svariable_t *next;     // for hashing

public:
  void setvalue(svalue_t newvalue);
  svalue_t getvalue();

  int Serialize(class LArchive &a);
  int Unserialize(LArchive &a);
};



svariable_t *find_variable(const char *name);

// creating global variables
svariable_t *add_game_int(const char *name, int *var);
svariable_t *add_game_string(char *name, char **var);
svariable_t *add_game_mobj(const char *name, Actor **mo);

// functions
svariable_t *new_function(const char *name, void (*handler)() );
svalue_t evaluate_function(int start, int stop);   // actually run a function

// arguments to handler functions
#define MAXARGS 128
extern int t_argc;
extern svalue_t *t_argv;
extern svalue_t  t_return;

#endif
