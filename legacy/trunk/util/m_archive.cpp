// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: m_archive.cpp 449 2007-05-05 20:45:25Z smite-meister $
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
// DESCRIPTION:
//   Archive class implementation
//
//-----------------------------------------------------------------------------

#include <string>
#include <zlib.h>

#include "doomdef.h"
#include "m_archive.h"
#include "m_swap.h"
#include "m_menu.h" // for message boxes
#include "r_data.h"
#include "z_zone.h"


/*
#include <dirent.h>
void SearchSaveDirectory(const char *name)
{
  // scandir() would be an alternative

  DIR *ds = opendir(name);

  if (!ds)
    I_Error("Could not open directory %s!\n", name);

  dirent *d;
  while (d = readdir(ds))
    {
      printf("%s", d->d_name);
      // if extension is ok, read the description and snapshot and put the name into the list
    }
  closedir(ds);
  // sort the list, return it
}
*/

static char Savegame_id[] = "Doom Legacy savegame\n";

// constructor
LArchive::LArchive()
{
  storing = false;
  m_buf = m_buf_end = m_pos = NULL;
  active_map = NULL;
}


LArchive::~LArchive()
{
  if (m_buf)
    Z_Free(m_buf); // only necessary if the archive is retrieving
}


// returns the current uncompressed size of the archive
int LArchive::Size() const
{
  if (storing)
    return m_sbuf.size();
  else
    return m_buf_end - m_buf;
}



// prepares the archive for storing data
void LArchive::Create(const char *descr)
{
  storing = true;
  pointermap[NULL] = 0;

  strncpy(header.id_string, Savegame_id, 24);
  sprintf(header.version_string, "v.%d\n", LEGACY_VERSION);
  strncpy(header.description, descr, 32);
  header.version = LONG(LEGACY_VERSION);

  m_sbuf.reserve(32*1024); // default size 32 kB
}


// uncompresses an archive and prepares it for reading
bool LArchive::Open(byte *buffer, size_t length)
{
  storing = false;

  memcpy(&header, buffer, sizeof(savegame_header_t));

  // check the magic string
  if (strncmp(header.id_string, Savegame_id, 24))
    {
      M_StartMessage("Unknown savegame format\n\nPress ESC\n");
      return false;
    }
      
  int i = LONG(header.version);

  if (i > LEGACY_VERSION || i < LEGACY_VERSION) // here you can do backwards save compatibility
    {
      M_StartMessage("Savegame from different version\n\nPress ESC\n");
      return false;
    }

  pointervec.resize(LONG(header.num_objects), NULL);

  // the actual archive starts after the header
  buffer += sizeof(savegame_header_t);
  length -= sizeof(savegame_header_t);

  unsigned long size = LONG(header.uncompressed_size);
  m_buf = (byte *)Z_Malloc(size, PU_STATIC, NULL);

  if (LONG(header.compression_method) == 0)
    {
      // no compression
      memcpy(m_buf, buffer, size);
    }
  else
    {
      switch (uncompress(m_buf, &size, buffer, length))
	{
	case Z_MEM_ERROR:
	case Z_BUF_ERROR:
	  CONS_Printf("Out of memory while uncompressing savegame\n");
	  Z_Free(m_buf);
	  m_buf = NULL;
	  return false;

	case Z_DATA_ERROR:
	  CONS_Printf("Savegame archive is corrupted\n");
	  Z_Free(m_buf);
	  m_buf = NULL;
	  return false;

	default:
	  CONS_Printf("Savegame uncompressed okay!\n");
	}
    }

  m_buf_end = m_buf + size;
  m_pos = m_buf;

  return true;
}


// Compresses the archive, Z_Malloc'ing the necessary buffer to 'result'.
// Returns the compressed size.
int LArchive::Compress(byte **result, int method)
{
  if (!storing)
    I_Error("Tried to compress a read-only archive!\n");

  header.num_objects = LONG(pointermap.size());
  unsigned size = m_sbuf.size();
  header.uncompressed_size = LONG(size);

  // TODO if you only could get the vector data out as a char array...
  byte *uncomp, *comp, *p;
  uncomp = p = (byte *)Z_Malloc(size, PU_STATIC, NULL);
  // translate the data
  vector<byte>::iterator i;
  for (i = m_sbuf.begin(); i != m_sbuf.end(); i++, p++)
    *p = *i;

  unsigned long comp_size;

  if (method == 0)
    {
      // no compression
      header.compression_method = 0;
      comp_size = size;
      comp = uncomp;
    }
  else
    {
      header.compression_method = LONG(1);
      comp_size = unsigned(size * 1.01) + 1 + 12; // maximum possible compressed size
      comp = (byte *)Z_Malloc(comp_size, PU_STATIC, NULL);

      switch (compress(comp, &comp_size, uncomp, size))
	{
	case Z_MEM_ERROR:
	case Z_BUF_ERROR:
	  CONS_Printf("Out of memory while compressing savegame\n");
	  Z_Free(uncomp);
	  Z_Free(comp);
	  return -1;

	default:
	  CONS_Printf("Savegame compressed okay!\n");
	}

      Z_Free(uncomp); // free the uncompressed data
      // (it still stays in the vector until this object is deleted)
    }

  // Now comp_size contains the actual size of the compressed data,
  // we should add in the header and "realloc" the comp buffer to this (smaller) size:
  *result = (byte *)Z_Malloc(sizeof(savegame_header_t) + comp_size, PU_STATIC, NULL);
  memcpy(*result, &header, sizeof(savegame_header_t));
  memcpy(*result + sizeof(savegame_header_t), comp, comp_size);

  Z_Free(comp); // free the temporary compressed buffer

  return (comp_size + sizeof(savegame_header_t));
}



int LArchive::Write(const byte *source, size_t length)
{
  m_sbuf.insert(m_sbuf.end(), source, source+length);
  return length;
}


int LArchive::Read(byte *dest, size_t length)
{
  if (m_pos + length > m_buf_end)
    return -1; // ran out of data

  for (unsigned i=0; i<length; i++, m_pos++)
    dest[i] = *m_pos;

  return length;
}


LArchive & LArchive::operator<<(byte& c)
{
  if (storing)
    m_sbuf.push_back(c);
  else if (m_pos < m_buf_end)
    c = *m_pos++;

  return *this;
}

LArchive & LArchive::operator<<(Uint16& c)
{
  Uint16 temp = 0;

  if (storing)
    {
      temp = SHORT(c);
      Write(reinterpret_cast<byte *>(&temp), sizeof(Uint16));
    }
  else
    {
      Read(reinterpret_cast<byte *>(&temp), sizeof(Uint16));
      c = SHORT(temp);
    }

  return *this;
}

LArchive & LArchive::operator<<(Uint32& c)
{
  Uint32 temp = 0;

  if (storing)
    {
      temp = LONG(c);
      Write(reinterpret_cast<byte *>(&temp), sizeof(Uint32));
    }
  else
    {
      Read(reinterpret_cast<byte *>(&temp), sizeof(Uint32));
      c = LONG(temp);
    }

  return *this;
}


LArchive& LArchive::operator<<(fixed_t& c)
{
  Uint32 temp = 0;

  if (storing)
    {
      temp = LONG(c.value());
      Write(reinterpret_cast<byte *>(&temp), sizeof(Uint32));
    }
  else
    {
      Read(reinterpret_cast<byte *>(&temp), sizeof(Uint32));
      c.setvalue(LONG(temp));
    }

  return *this;
}



LArchive & LArchive::operator<<(string &s)
{
  if (storing)
    {
      Write(reinterpret_cast<const byte *>(s.c_str()), s.length() + 1); // write the NUL too
    }
  else
    {
      // a small hack
      s = reinterpret_cast<char *>(m_pos); // there better be a terminating '\0' !
      m_pos += s.length() + 1;
    }

  return *this;
}

// archives a c-string. Z_Mallocs the memory when retrieving.
LArchive & LArchive::operator<<(char *&s)
{
  if (storing)
    {
      Write(reinterpret_cast<const byte *>(s), strlen(s) + 1); // write the NUL too
    }
  else
    {
      s = Z_Strdup(reinterpret_cast<char *>(m_pos), PU_STATIC, NULL); // there better be a terminating '\0' !
      m_pos += strlen(s) + 1;
    }

  return *this;
}

// archives a Material name
LArchive & LArchive::operator<<(Material *&m)
{
  if (storing)
    {
      const char *s = m->GetName();
      Write(reinterpret_cast<const byte *>(s), strlen(s) + 1); // write the NUL too
    }
  else
    {
      // use the archive buffer directly
      m = materials.Get(reinterpret_cast<char *>(m_pos), TEX_wall); // there better be a terminating '\0' !
      m_pos += strlen(reinterpret_cast<char *>(m_pos)) + 1;
    }

  return *this;
}


// returns true if the object 'p' is already in the pointermap, otherwise adds it there.
// 'n' gets the archive ID corresponding to 'p'
bool LArchive::HasStored(void *p, unsigned &id)
{
  if (!storing)
    I_Error("Asked for archive object ID while retrieving!\n");

  map<void *, int>::iterator i = pointermap.find(p);
  if (i == pointermap.end())
    {
      // not yet in the map
      id = pointermap.size(); // give next free index
      pointermap[p] = id;
      return false;
    }

  // already in the map
  id = (*i).second;
  return true;
}


void LArchive::SetPtr(unsigned id, void *p)
{
  if (storing)
    I_Error("Tried to set archive ID->pointer relationship while storing!\n");

  if (id >= pointervec.size())
    I_Error("Invalid archive object ID!\n");

  pointervec[id] = p;
}


bool LArchive::GetPtr(unsigned id, void *&p)
{
  if (storing)
    I_Error("Tried to get archive ID->pointer relationship while storing!\n");

  if (id >= pointervec.size())
    I_Error("Unknown archive object ID!\n");

  if (id > 0 && !pointervec[id])
    {
      // not extracted yet, zero means NULL
      return false;
    }

  p = pointervec[id];
  return true;
}


// A helper function for testing the integrity of the archive.
// It is used to set and check markers between data segments.
bool LArchive::Marker(int mark)
{
  if (storing)
    operator<<(mark);
  else
    {
      int check;
      operator<<(check);
      if (check != mark)
	{
	  // TODO after debugging, replace this with CONS_Printf
	  I_Error("Corrupted archive! Expected %d, got %d!\n", mark, check);
	  return false;
	}
    }

  return true;
}
