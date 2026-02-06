// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: g_think.cpp 307 2006-02-11 16:38:54Z jussip $
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
/// \brief TypeInfo and Thinker class implementation

#include "g_think.h"
#include "m_archive.h"
#include "z_zone.h"


map<unsigned, TypeInfo*>& TypeInfo::id_map()
{
  // This is to overcome the "static initialization order fiasco" :/
  // We MUST make sure that id_map is constructed before it is used during
  // the construction of the TypeInfo instances, so a normal static
  // member object is not enough.

  // When this function is first executed, the map is constructed.
  static map<unsigned, TypeInfo*> *temp = new map<unsigned, TypeInfo*>;
  return *temp;
} 


TypeInfo::TypeInfo(const char *n, thinker_factory_t f, TypeInfo *p)
{
  static map<unsigned, TypeInfo*>& id_ref = TypeInfo::id_map();

  type_id = id_ref.size() + 1; // get the next available id (zero is reserved for NULL)
  id_ref[type_id] = this;  // store the mapping
  name = n;
  factory = f;
  parent = p;
}


TypeInfo *TypeInfo::Find(unsigned c)
{
  static map<unsigned, TypeInfo*>& id_ref = TypeInfo::id_map();
  map<unsigned, TypeInfo*>::iterator i;

  i = id_ref.find(c);
  if (i == id_ref.end())
    return NULL; // not found
  return (*i).second;
}


// Since this class has no parent, we must implement it like this:
TypeInfo Thinker::_type("Thinker", Thinker::Create, NULL);


Thinker::Thinker()
{
  mp = NULL;
}

Thinker::~Thinker()
{}

int Thinker::Marshal(LArchive &a)
{
  return 0;
}

int Thinker::Serialize(Thinker *p, LArchive &a)
{
  unsigned id;
  // is it already there?
  if (a.HasStored(p, id))
    {
      // handles the case p == NULL as well
      a << id;
      return id;
    }

  a << id;
  a << p->Type()->type_id; // hmm. basically an unnecessary call, but less trouble this way?
  p->Marshal(a);
  return id;
}


Thinker *Thinker::Unserialize(LArchive &a)
{
  unsigned id;
  a << id;

  void *v = NULL;
  if (a.GetPtr(id, v))
    return static_cast<Thinker*>(v); // already unserialized, just return the appropriate pointer

  // not unserialized yet, so let's do it now:
  unsigned type;
  a << type;
  TypeInfo *t = TypeInfo::Find(type);
  if (!t)
    {
      return NULL; // TODO: How to handle errors? using exceptions?
    }
  Thinker *p = t->factory();
  a.SetPtr(id, p); // order is important: consider the case a -> b -> a, where -> is a pointer

  p->mp = a.active_map; // a small kludge, some Marshal functions need to have a valid Map*
  p->Marshal(a);
  return p;
}


void *Thinker::operator new (size_t size)
{
  return Z_Malloc(size, PU_LEVSPEC, NULL);    
}

void Thinker::operator delete (void *mem)
{
  Z_Free(mem);
}
