// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: z_cache.h 536 2009-06-29 06:46:13Z smite-meister $
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
/// \brief Abstract cache class with reference counting.

#ifndef z_cache_h
#define z_cache_h 1

#include <string.h>
#include "doomdef.h"
#include "dictionary.h"


/// \brief BC for cache items
class cacheitem_t
{
protected:
#define CACHE_NAME_LEN 63 // was 8
  char  name[CACHE_NAME_LEN+1]; ///< name of the item, NUL-terminated
  int   usefulness; ///< how many times has it been used?
  int   refcount;   ///< reference count, number of current users

public:
  cacheitem_t(const char *name);
  virtual ~cacheitem_t() {}; // TODO unnecessary virtualization???

  inline void MakeLink()
  {
    usefulness = -usefulness - 1; // HACK negative usefulness marks them as links, -1 denotes link with zero usefulness etc.
  }

  /// Mark item used by incrementing refcount. Return true if item is a link, i.e. usefulness is negated.
  inline bool AddRef()
  {
    refcount++;
    if (usefulness < 0)
      {
	usefulness--;
	return true;
      }
    else
      {
	usefulness++;
	return false;
      }
  }

  /// releases the item by decrementing the refcount
  bool Release();

  /// Deletes the cacheitem (and returns true) if refcount is zero.
  bool FreeIfUnused();

  /// returns the name of the item
  inline const char *GetName() { return name; }

  /// change the name of the item
  inline void SetName(const char *n) { strncpy(name, n, CACHE_NAME_LEN); }

  /// Could virtualize this to print child class info too.
  void Print()
  {
    CONS_Printf("- %s: rc = %d, use = %d\n", name, refcount, usefulness);
  }

  void *operator new(size_t size);
  void  operator delete(void *mem);
};



/// \brief Data source within a cache_t.
///
/// This is an extra layer of abstraction for complex caches with multiple data sources
/// which can have different priorities depending on the query. See material_cache_t.
template<typename T>
class cachesource_t : public HashDictionary<T>
{
  typedef HashDictionary<T> parent;
public:
  /// Prints current contents
  void Inventory()
  {
    for (typename parent::dict_iter_t s = parent::dict_map.begin(); s != parent::dict_map.end(); s++)
      s->second->Print();
  }

  /// Deletes unused items (refcount == 0)
  int  Cleanup()
  {
    int k = 0;
    for (typename parent::dict_iter_t s = parent::dict_map.begin(); s != parent::dict_map.end(); )
      {
	cacheitem_t *p = s->second;
	typename parent::dict_iter_t t = s++; // first copy s to t, then increment s

	if (p->FreeIfUnused())
	  {
	    parent::dict_map.erase(t); // erase it from the hash_map
	    // Once an iterator is erased, it becomes invalid
	    // and cannot be incremented! Therefore we have both s and t.
	    k++;
	  }
      }
    return k;
  }
};


/// \brief Template for different types of simple caches.
///
/// Data used through a cache_t must not be used anywhere
/// else, because Z_Free and Z_ChangeTag will cause problems.
/// T must be descendant of cacheitem_t.
template<typename T>
class cache_t
{
protected:
  cachesource_t<T> source; ///< a simple cache has only one source
  T      *default_item; ///< the default data item

  /// Creates a new cacheitem_t, does the actual loading and conversion of the data during a Get() operation.
  virtual T *Load(const char *name) = 0;

public:
  /// cache constructor
  cache_t() { default_item = NULL; }

  /// cache destructor
  virtual ~cache_t()
  {
    // source takes care of itself
    if (default_item)
      delete default_item;
  }

  /// Inserts an item into the cache. Manual alternative to Get().
  bool Insert(T *t) { return source.Insert(t); }

  /// Returns true if the named data item exists in the cache, false otherwise.
  bool Exists(const char *name) const
  {
    if (!name)
      return false;

    return source.Count(name);
  }


  /// Returns the requested data item (incrementing refcount) if it is in cache, otherwise NULL.
  T *Find(const char *name)
  {
    if (!name)
      return NULL;

    T *p = source.Find(name);
    if (!p)
      return NULL;

    if (p->AddRef())
      {
	// a "link" to default_item
	default_item->AddRef();
	return default_item;
      }
    else
      return p;
  }


  /// Caches and returns the requested data item, or, if not found, the default_item.
  /// Checks if item is already in cache. If so, increments refcount and returns it.
  /// If not, tries to Load() it. If succesful, returns the item.
  /// If not, returns a link to the defaultitem.
  T *Get(const char *name)
  {
    T *p;

    if (name == NULL)
      p = default_item;
    else
      {
	p = source.Find(name);
	if (!p)
	  {
	    // Not found in source.
	    p = Load(name);
	    if (!p)
	      {
		// Item not found at all.
		// Some nonexistant items are asked again and again.
		// We use a special cacheitem_t to link their names to the default item.
		p = new T(name);
		p->MakeLink();
	      }
	    source.Insert(p);
	  }
      }

    if (p->AddRef())
      {
	// a "link" to default_item
	default_item->AddRef();
	return default_item;
      }
    else
      return p;
  }

  /// Defines the default data item for the cache.
  /// Has to be separate from constructor. Static class instances are constructed when
  /// program starts, and at that time we do not yet have a working FileCache.
  void SetDefaultItem(const char *name)
  {
    if (default_item)
      CONS_Printf("cache: Replacing default_item!\n");

    default_item = Load(name);
    if (!default_item)
      I_Error("cache: New default_item '%s' not found!\n", name);
    // TODO delete the old default item?
  }

  /// Prints current cache contents.
  void Inventory()
  {
    if (default_item)
      {
	CONS_Printf("Default item: ");
	default_item->Print();
      }

    source.Inventory();
  }

  /// Returns the number of items in the cache.
  inline unsigned Size() const { return source.Size(); }

  /// Removes unused data items from cache
  inline int Cleanup() { return source.Cleanup(); };
};


#endif
