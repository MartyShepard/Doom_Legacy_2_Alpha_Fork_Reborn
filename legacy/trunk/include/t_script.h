// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// $Id: t_script.h 306 2006-02-11 16:26:27Z jussip $
//
// Copyright(C) 2000 Simon Howard
// Copyright(C) 2001-2004 Doom Legacy Team
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
/// \brief Main FS interface, running scripts

#ifndef t_script_h
#define t_script_h 1

#define VARIABLESLOTS 16


/// \brief FS wait state
enum fs_wait_e
{
  wt_none,        // not waiting
  wt_delay,       // wait for a set amount of time
  wt_tagwait,     // wait for sector to stop moving
  wt_scriptwait,  // wait for script to finish
};


/// \brief stores the state of a paused FS script
struct runningscript_t
{
  struct script_t     *script;  ///< script definition
  char      *savepoint;  ///< saved rover position
  fs_wait_e  wait_type;  ///< what are we waiting for?
  int        wait_data;  ///< data for wait: tagnum, counter, script number etc
	
  struct svariable_t *variables[VARIABLESLOTS]; ///< saved variables
  class Actor *trigger;  

  runningscript_t *prev, *next;  // for chain


  static runningscript_t *freelist; ///< maintain a freelist for speed
  void *operator new(size_t size);
  void  operator delete(void *mem);
};


#endif
