// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_net.cpp 308 2006-02-11 16:54:43Z jussip $
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
//      network interface
//      
//-----------------------------------------------------------------------------


#include <errno.h>

#include "doomdef.h"

#include "i_system.h"
#include "d_event.h"
#include "m_argv.h"

//#include "doomstat.h"

#include "i_net.h"

#include "z_zone.h"

#if 0
int I_InitTcpNetwork(void);
//
// NETWORKING
//

void Internal_Get(void)
{
     I_Error("Get without netgame\n");
}

void Internal_Send(void)
{
     I_Error("Send without netgame\n");
}

void Internal_FreeNodenum(int nodenum)
{}
#endif

//
// I_InitNetwork
// Only required for DOS, so this is more a dummy
//
bool I_InitNetwork (void)
{
  if( M_CheckParm ("-net") )
    {
      I_Error("-net not supported, use -server and -connect\n"
	      "see docs for more\n");
    }
  return false;
}
