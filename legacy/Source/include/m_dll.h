// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: m_dll.h 306 2006-02-11 16:26:27Z jussip $
//
// Copyright (C) 2002-2003 by DooM Legacy Team.
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

#ifndef m_dll_h
#define m_dll_h 1

#ifdef __WIN32__
# include <windows.h>
typedef HINSTANCE dll_handle_t;
#else
typedef void* dll_handle_t;
#endif

// macros for exporting data and functions in a DLL
#define DATAEXPORT __declspec(dllexport)
#define EXPORT extern "C" __declspec(dllexport)


/// \brief Handles all Doom Legacy DLL plugins
///
/// A Legacy DLL plugin exports usually just two symbols:
/// dll_info, the dll_info_t struct of the DLL, and
/// some way to get the actual function pointers to the DLL functions.

class LegacyDLL
{
private:
  dll_handle_t handle;

public:
  struct dll_info_t
  {
    int  api_version; ///< the interface version of the dll
    int  version;     ///< dll version number
    char name[64];    ///< official dll name
  };

  char name[64];
  int  api_version;
  int  version;


  LegacyDLL();
  ~LegacyDLL();

  /// loads the DLL, imports and checks dll_info
  bool  Open(const char *filename);

  /// looks for an exported symbol in the DLL
  void *FindSymbol(const char *symbol);
};



// common DLL interface
dll_handle_t OpenDLL(const char *dllname);
void  CloseDLL(dll_handle_t handle);
void *GetSymbol(dll_handle_t handle, const char *symbol);

#endif
