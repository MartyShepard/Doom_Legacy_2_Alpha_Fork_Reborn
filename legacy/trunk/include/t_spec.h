// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// $Id: t_spec.h 306 2006-02-11 16:26:27Z jussip $
//
// Copyright(C) 2000 Simon Howard
// Copyright(C) 2001-2003 Doom Legacy Team
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


#ifndef t_spec_h
#define t_spec_h 1

void spec_brace();

int  spec_if();  //SoM: returns weather or not the if statement was true.
int  spec_elseif(bool lastif);
void spec_else(bool lastif);
void spec_while();
void spec_for();
void spec_goto();

// variable types

bool spec_variable();

void spec_script();     // in t_script.c btw

#endif
