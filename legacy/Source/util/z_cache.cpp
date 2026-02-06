// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: z_cache.cpp 520 2008-01-07 01:37:12Z smite-meister $
//
// Copyright (C) 2003-2007 by DooM Legacy Team.
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
/// \brief Abstract cache system with reference counting.

#include "doomdef.h"
#include "z_cache.h"
#include "z_zone.h"


//=================================================================================

cacheitem_t::cacheitem_t(const char *n)
{
  strncpy(name, n, CACHE_NAME_LEN); // we make a copy so it stays intact as long as this cacheitem lives
  name[CACHE_NAME_LEN] = '\0';      // NUL-terminated to be safe
  refcount = 0;
  usefulness = 0;
}


void *cacheitem_t::operator new(size_t size)
{
  return Z_Malloc(size, PU_STATIC, NULL);
}


void cacheitem_t::operator delete(void *mem)
{
  Z_Free(mem);
}


bool cacheitem_t::Release()
{
  if (--refcount < 0)
    I_Error("cacheitem_t '%s': Too many releases!\n", name);

  return (refcount == 0);
}

bool cacheitem_t::FreeIfUnused()
{
  if (refcount == 0)
    {
      delete this; // delete the cacheitem itself
      return true;
    }

  return false;
}



//=================================================================================


/// Pure virtual, renew. This is a sample implementation.
/*
T *cache_t::Load(const char *n)
{
  int lump = fc.FindNumForName(n, false);
  if (lump == -1)
    return NULL;

  T *p = new T(n);

  p->lumpnum = lump;
  p->data = fc.CacheLumpNum(lump, tagtype);
  p->length = fc.LumpLength(lump);
  return p;
}
*/
