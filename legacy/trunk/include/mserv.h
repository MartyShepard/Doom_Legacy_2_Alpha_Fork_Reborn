// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: mserv.h 306 2006-02-11 16:26:27Z jussip $
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
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
//      Header file for the master server routines
//
//-----------------------------------------------------------------------------

#ifndef _MSERV_H_
#define _MSERV_H_

// I want that structure 8 bytes aligned (current size is 80)
typedef struct
{
    char    header[16];     // information such as password
    char    ip[16];
    char    port[8];
    char    name[32];       
    char    version[8];     // format is: x.yy.z (like 1.30.2 or 1.31)
} msg_server_t;


// ================================ GLOBALS ===============================

char *GetMasterServerPort(void);
char *GetMasterServerIP(void);

void RegisterServer(int, int);
void UnregisterServer(void);

void SendPingToMasterServer(void);

msg_server_t *GetShortServersList(void);

#endif
