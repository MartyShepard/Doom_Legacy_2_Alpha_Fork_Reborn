// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: m_dll.cpp 309 2006-02-11 17:02:49Z jussip $
//
// Copyright (C) 2002-2004 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief This zone is for loading and unloading of Doom Legacy DLLs only.


// Why oh why can't there be just one standard DLL system inteface?!?
#ifdef __WIN32__
  // windows
# include <windows.h>
#else
  // unices
# include <dlfcn.h>
#endif
#include <string.h>

#include "doomdef.h"
#include "m_dll.h"


// Dynamically loads a DLL file 
dll_handle_t OpenDLL(const char *dllname)
{
#ifdef __WIN32__
  return LoadLibrary(dllname);
#else
  return dlopen(dllname, RTLD_LAZY);
#endif
}

// Frees a DLL file loaded using OpenDLL
void CloseDLL(dll_handle_t handle)
{
#ifdef __WIN32__
  FreeLibrary(handle);
#else  
  dlclose(handle);
#endif
}

// Returns a pointer to 'symbol', which _must_ belong
// to the ones exported by the DLL in question.
// Otherwise returns NULL.
void *GetSymbol(dll_handle_t handle, const char *symbol)
{
#ifdef __WIN32__
  return (void *)GetProcAddress(handle, symbol);
#else
  return dlsym(handle, symbol);
#endif
}





LegacyDLL::LegacyDLL()
{
  handle = NULL;
}


LegacyDLL::~LegacyDLL()
{
  if (handle)
    CloseDLL(handle);
}


bool LegacyDLL::Open(const char *filename)
{
  handle = OpenDLL(filename);
  if (!handle)
    {
      I_Error("Could not load DLL %s!\n", filename);
      return false;
    }

  dll_info_t *info = (dll_info_t *)GetSymbol(handle, "dll_info");
  if (!info)
    {
      I_Error("DLL %s exports no dll_info!\n", filename);
      return false;
    }

  api_version = info->api_version;
  version = info->version;
  strcpy(name, info->name);
  
  return true;
}


void *LegacyDLL::FindSymbol(const char *symbol)
{
  if (handle)
    return GetSymbol(handle, symbol);

  return NULL;
}
