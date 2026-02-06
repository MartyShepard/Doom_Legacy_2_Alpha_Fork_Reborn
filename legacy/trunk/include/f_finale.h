// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: f_finale.h 306 2006-02-11 16:26:27Z jussip $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
//
//
// DESCRIPTION:
//
//    
//-----------------------------------------------------------------------------


#ifndef f_finale_h
#define f_finale_h 1

class clusterdef_t;

//
// FINALE
//

// Called by main loop.
//boolean F_Responder (event_t* ev);

// Called by main loop.
void F_Ticker();

// Called by main loop.
void F_Drawer();

void F_StartFinale(const class MapCluster *c, bool enter, bool endgame);


#endif
