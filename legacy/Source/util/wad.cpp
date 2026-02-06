// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2002-2007 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Wad, Wad3, Pak and ZipFile classes: datafile I/O

#include <stdio.h>
#include <sys/stat.h>
#include <zlib.h>

#include <unistd.h>
#include <sys/stat.h>   // mkdir (Linux/MSYS) oder _mkdir (Windows)
#include <direct.h>     // _mkdir Windows
#include <dirent.h>

#include "doomdef.h"

#include "m_swap.h"
#include "parser.h"
#include "dehacked.h"


#include "wad.h"
#include "w_wad.h"
#include "z_zone.h"

// Hilfsfunktionen für Little-Endian (sicher auf allen Systemen)
inline uint32_t read_le32(const void* ptr)
{
    const uint8_t* p = static_cast<const uint8_t*>(ptr);
    return p[0] |
          (p[1] << 8) |
          (p[2] << 16)|
          (p[3] << 24);
}

inline uint16_t read_le16(const void* ptr)
{
    const uint8_t* p = static_cast<const uint8_t*>(ptr);
    return p[0] |
          (p[1] << 8);
}

static char *ProgrammPath(void)
{
  static char dosroot[MAX_PATH] = {0}; 	
  static char exepath[MAX_PATH] = {0};  // static = nur einmal initialisiert

  if (exepath[0] == '\0')  // Nur einmal berechnen
  {
       GetModuleFileNameA(NULL, exepath, MAX_PATH);
       char *last = strrchr(exepath, '\\');
       if (last) *(last /*+ 1*/) = '\0';  // Nur Ordner und entferne '\' den auch.
  }

  if (dosroot[0] == '\0')  // Nur einmal berechnen  
       getcwd(dosroot, MAX_PATH);

  if (strcmp(exepath, dosroot ) == 0)
  {
  //    printf(, "ProgrammPath [exedir]:[getcwd] sind identisch\n");
  }
  else
  {
      printf("ProgrammPath [exedir]: %s\n", exepath);
      printf("ProgrammPath [getcwd]: %s\n", dosroot);
      printf("Main->Argv hat eine anderes Arbeitsverzeichnis bekommen...\n");  
  }

  return exepath; 
}

static byte DirectoryCheck_isPath(const char *path)
{
      byte Result = 1;
      
      DIR *dir = opendir(path);
      if (!dir) // Kein Verzeichnis? Dann normale Datei?     
          Result = 0;
      
      closedir(dir);
/*
      printf("[%s][%d] Directory Check: is Path = %s\n"
             "        : %s\n",__FILE__,__LINE__,
                      (Result==1)?"True":"False",path);
*/                                    
      return Result;
}

static bool TestPadding(char *name, int len)
{
  // TEST padding of lumpnames
  int j;
  bool warn = false;
  for (j=0; j<len; j++)
    if (name[j] == 0)
      {
	for (j++; j<len; j++)
	  if (name[j] != 0)
	    {
	      name[j] = 0; // fix it
	      warn = true;
	    }
	if (warn)
	  CONS_Printf("Warning: Lumpname %s not padded with zeros!\n", name);
	break;
      }
  return warn;
}


//=============================
//  Wad class implementation
//=============================

// All WAD* files use little-endian byte ordering (LSB)

/// WAD file header
struct wadheader_t 
{
  char   magic[4];   ///< "IWAD", "PWAD", "WAD2" or "WAD3"
  Uint32 numentries; ///< number of entries in WAD
  Uint32 diroffset;  ///< offset to WAD directory
} __attribute__((packed));

/// WAD file directory entry
struct waddir_file_t
{
  Uint32 offset;  ///< file offset of the resource
  Uint32 size;    ///< size of the resource
  char   name[8]; ///< name of the resource (NUL-padded)
} __attribute__((packed));


// a runtime WAD directory entry
struct waddir_t
{
  unsigned int offset;  // file offset of the resource
  unsigned int size;    // size of the resource
  union
  {
    char name[9]; // name of the resource (NUL-terminated)
    Uint32 iname[2];
  };
};


// constructor
Wad::Wad()
{
  directory = NULL;
}

Wad::~Wad()
{
  if (directory)
    Z_Free(directory);
}


// Creates a WAD that encapsulates a single .lmp and .deh file
bool Wad::Create(const char *fname, const char *lumpname)
{
  // this code emulates a wadfile with one lump
  // at position 0 and size of the whole file
  // this allows deh files to be treated like wad files,
  // copied by network and loaded at the console

  // common to all files
  if (!VDataFile::Open(fname))
    return false;

  numitems = 1;
  directory = static_cast<waddir_t*>(Z_Malloc(sizeof(waddir_t), PU_STATIC, NULL));
  directory->offset = 0;
  directory->size = size;
  strncpy(directory->name, lumpname, 8);
  directory->name[8] = '\0'; // terminating NUL to be sure

  cache = static_cast<lumpcache_t*>(Z_Malloc(numitems * sizeof(lumpcache_t), PU_STATIC, NULL));
  memset(cache, 0, numitems * sizeof(lumpcache_t));

  LoadDehackedLumps();
  CONS_Printf(" Added single-lump file %s\n", filename.c_str());
  return true;
}



// Loads a WAD file, sets up the directory and cache.
// Returns false in case of problem
bool Wad::Open(const char *fname)
{
  // common to all files
  if (!VDataFile::Open(fname))
    return false;

  // read header
  wadheader_t h;
  rewind(stream);
  fread(&h, sizeof(wadheader_t), 1, stream);
  // endianness swapping
  numitems = LONG(h.numentries);
  int diroffset = LONG(h.diroffset);

  // set up caching
  cache = (lumpcache_t *)Z_Malloc(numitems * sizeof(lumpcache_t), PU_STATIC, NULL);
  memset(cache, 0, numitems * sizeof(lumpcache_t));

  // read wad file directory
  fseek(stream, diroffset, SEEK_SET);
  waddir_file_t *temp = (waddir_file_t *)Z_Malloc(numitems * sizeof(waddir_file_t), PU_STATIC, NULL); 
  fread(temp, sizeof(waddir_file_t), numitems, stream);  

  directory = (waddir_t *)Z_Malloc(numitems * sizeof(waddir_t), PU_STATIC, NULL);
  memset(directory, 0, numitems * sizeof(waddir_t));

  // endianness conversion and NUL-termination for directory
  for (int i = 0; i < numitems; i++)
    {
      directory[i].offset = LONG(temp[i].offset);
      directory[i].size   = LONG(temp[i].size);
      TestPadding(temp[i].name, 8);
      strncpy(directory[i].name, temp[i].name, 8);
    }

  Z_Free(temp);

  h.numentries = 0; // what a great hack!
  CONS_Printf(" Added %s file %s (%i lumps)\n", h.magic, filename.c_str(), numitems);
  LoadDehackedLumps();
  return true;
}


int Wad::GetItemSize(int i)
{
  return directory[i].size;
}

const char *Wad::GetItemName(int i)
{
  return directory[i].name;
}


int Wad::Internal_ReadItem(int lump, void *dest, unsigned size, unsigned offset)
{
  waddir_t *l = directory + lump;
  fseek(stream, l->offset + offset, SEEK_SET);
  return fread(dest, 1, size, stream); 
}


void Wad::ListItems()
{
  waddir_t *p = directory;
  for (int i = 0; i < numitems; i++, p++)
    printf("%-8s\n", p->name);
}



// Searches the wadfile for lump named 'name', returns the lump number
// if not found, returns -1
int Wad::FindNumForName(const char *name, int startlump)
{
  union
  {
    char s[9];
    Uint32 x[2];
  };

  // make the name into two integers for easy compares
  strncpy(s, name, 8);

  // in case the name was 8 chars long
  s[8] = 0;
  // case insensitive TODO make it case sensitive if possible
  strupr(s);

  // FIXME doom.wad and doom2.wad PNAMES lumps have exactly ONE (1!) patch
  // entry with a lowcase name: w94_1. Of course the actual lump is
  // named W94_1, so it won't be found if we have case sensitive search! damn!
  // heretic.wad and hexen.wad have no such problems.
  // The right way to fix this is either to fix the WADs (yeah, right!) or handle
  // this special case in the texture loading routine.

  waddir_t *p = directory + startlump;

  // a slower alternative could use strncasecmp()
  for (int j = startlump; j < numitems; j++, p++)
    if (p->iname[0] == x[0] && p->iname[1] == x[1])
      return j;

  // not found
  return -1;
}


int Wad::FindPartialName(Uint32 iname, int startlump, const char **fullname)
{
  // checks only first 4 characters, returns full name
  // a slower alternative could use strncasecmp()

  waddir_t *p = directory + startlump;

  for (int j = startlump; j < numitems; j++, p++)
    if (p->iname[0] == iname)
      {
	*fullname = p->name;
	return j;
      }

  // not found
  return -1;
}


// LoadDehackedLumps
// search for DEHACKED lumps in a loaded wad and process them
void Wad::LoadDehackedLumps()
{
  // just the lump number, nothing else
  int clump = 0;

  while (1)
    { 
      clump = FindNumForName("DEHACKED", clump);
      if (clump == -1)
	break;
      CONS_Printf(" Loading DEHACKED lump %d from %s\n", clump, filename.c_str());

      DEH.LoadDehackedLump(clump);
      clump++;
    }
}


// OpenFromMemory
// Readin a wad from Memory from a Archive
WadFromMemory::WadFromMemory()

  : memory_data(nullptr), memory_size(0)
{directory = NULL;}

void WadFromMemory::Lump_Extract(int numitems, waddir_t *directory, const char* lump_ext, const char* destinationpath)
{
    
      CONS_Printf("=== DEBUG: Extrahiere Lumps nach Maps in 'Lump_Extract' ===\n");

    // Basis-Verzeichnis einmalig anlegen
    const char* base_dir = "Lump_Extract";

    #ifdef WIN32
      _mkdir(base_dir);
    #else
      mkdir(base_dir, 0755);
    #endif

    bool in_map = false;
    char current_map_dir[MAX_PATH] = "";

    for (int i = 0; i < numitems; i++)
    {
      waddir_t *l = &directory[i];
      //if (l->size == 0) continue;

      char lump_name[9];
      strncpy(lump_name, l->name, 8);
      lump_name[8] = '\0';

      // Ist das ein Map-Start-Lump? (MAPxx)
      if (strncmp(lump_name, "MAP", 3) == 0 && 
                      isdigit(lump_name[3]) &&
                      isdigit(lump_name[4]) )
                      //  lump_name[5] == ' ' &&
                      //  lump_name[6] == ' ' &&
                      //  lump_name[7] == ' '  )
      {
        // Neues Map-Verzeichnis beginnen
        in_map = true;
        snprintf(current_map_dir, sizeof(current_map_dir), "%s\\%s\\%s", destinationpath, base_dir, lump_name);

        #ifdef WIN32
          _mkdir(current_map_dir);
        #else
          mkdir(current_map_dir, 0755);
        #endif

        CONS_Printf("  Map-Verzeichnis angelegt: %s\n", current_map_dir);
      }
      else if (in_map)
      {
        // Lump in aktuelles Map-Verzeichnis speichern
        char filepath[256];
        snprintf(filepath, sizeof(filepath), "%s\\%s%s", current_map_dir, lump_name, lump_ext);

        FILE *f = fopen(filepath, "wb");
        if (f)
        {
          byte *buf = (byte*)Z_Malloc(l->size, PU_STATIC, NULL);
          if (buf)
          {
            uint32_t read = Internal_ReadItem(i, buf, l->size, 0);
            if (read == l->size)
            {
              fwrite(buf, 1, l->size, f);
              CONS_Printf("    Extrahiert: %s (%d bytes)\n", filepath, l->size);
            }
            else
            {
              CONS_Printf("    FEHLER: %s – nur %d von %d Bytes gelesen\n", filepath, read, l->size);
            }
            Z_Free(buf);
          }
          fclose(f);
        }
        else
        {
          CONS_Printf("    Kann %s nicht schreiben\n", filepath);
        }
      }
      else
      {
        // Lump außerhalb von Maps → direkt in Basis-Ordner
        char filepath[256];
        snprintf(filepath, sizeof(filepath), "%s\\%s\\%s%s", destinationpath, base_dir, lump_name, lump_ext);

        FILE *f = fopen(filepath, "wb");
        if (f)
        {
          byte *buf = (byte*)Z_Malloc(l->size, PU_STATIC, NULL);
          if (buf)
          {
            uint32_t read = Internal_ReadItem(i, buf, l->size, 0);
            if (read == l->size)
            {
              fwrite(buf, 1, l->size, f);
              CONS_Printf("  Extrahiert (global): %s (%d bytes)\n", filepath, l->size);
            }
            Z_Free(buf);
          }
          fclose(f);
        }
      }
    }

    CONS_Printf("=== DEBUG: Extraktion fertig ===\n");
}

WadFromMemory::~WadFromMemory()
{
  if (directory)
    Z_Free(directory);
  
  if (memory_data)
    Z_Free(memory_data);
  
  memory_data = nullptr;
}

bool WadFromMemory::Open(byte* data, size_t datasize, const char* virtual_name)
{
  if (!data || datasize < 12)
  {
      return false;
  }

  filename = virtual_name ? virtual_name : "memory wad";
  
  // === DEEP COPY – das ist der Fix! ===
  memory_data = (byte*)Z_Malloc(datasize, PU_STATIC, NULL);
  if (!memory_data)
  {
      CONS_Printf("WadFromMemory::Open: Z_Malloc failed for %zu bytes in '%s'\n",
                  datasize, filename.c_str());
      return false;
  }
  
  memcpy(memory_data, data, datasize);
  memory_size = datasize;
  
  // Header prüfen
  wadheader_t h;
  memcpy(&h, data, sizeof(wadheader_t));
   
  Uint8 isIWAD = -1;
  if (memcmp(h.magic, "IWAD", 4) == 0) isIWAD = 0;
  if (memcmp(h.magic, "PWAD", 4) == 0) isIWAD = 1;

  if (isIWAD == -1) {
      Z_Free(memory_data);
      memory_data = nullptr;
      memory_size = 0;
      CONS_Printf("'%s' is not a valid IWAD/PWAD\n", filename.c_str());
      return false;
  }
    
  numitems = LONG(h.numentries);
  int diroffset = LONG(h.diroffset);

  // Directory kopieren
  waddir_file_t *temp = (waddir_file_t *)Z_Malloc(numitems * sizeof(waddir_file_t), PU_STATIC, NULL);
  memcpy(temp, data + diroffset, numitems * sizeof(waddir_file_t));

  directory = (waddir_t *)Z_Malloc(numitems * sizeof(waddir_t), PU_STATIC, NULL);
  for (int i = 0; i < numitems; i++)
  {
    directory[i].offset   = read_le32(&temp[i].offset);
    directory[i].size     = read_le32(&temp[i].size);
    TestPadding(temp[i].name, 8);
    strncpy(directory[i].name, temp[i].name, 8);
    directory[i].name[8]  = '\0';
    
    // Union iname setzen (little-endian)
    directory[i].iname[0] = (Uint32)directory[i].name[0] |
                            ((Uint32)directory[i].name[1] << 8) |
                            ((Uint32)directory[i].name[2] << 16) |
                            ((Uint32)directory[i].name[3] << 24);

    directory[i].iname[1] = (Uint32)directory[i].name[4] |
                            ((Uint32)directory[i].name[5] << 8) |
                            ((Uint32)directory[i].name[6] << 16) |
                            ((Uint32)directory[i].name[7] << 24);

    CONS_Printf(" Added Memory: [%-2d] Name: %-8s, Size = %-6d, Offset = %08X (iname[0]=%08X, iname[1]=%08X)\n",i,
                                                                                                directory[i].name,
                                                                                                directory[i].size,
                                                                                                directory[i].offset,
                                                                                                directory[i].iname[0],
                                                                                                directory[i].iname[1]);
  
  }
  Z_Free(temp);
  
  // 6. Cache allokieren
  cache = (lumpcache_t*)Z_Malloc(numitems * sizeof(lumpcache_t), PU_STATIC, NULL);
  if (cache)
  {
        memset(cache, 0, numitems * sizeof(lumpcache_t));
  }
 
  /* Lump_Extraction für den Vergleich ob der Deflate läuft und die CRC summe stimmt
   * PS: Vergleich fand mit Total commander statt dem WAD PLugin
   * Lump Endung. '""' Kann dann geändert werden
   */
  // Lump_Extract(numitems, directory, "", ProgrammPath());
  
  LoadDehackedLumps();
  
  CONS_Printf(" ZIP/PK3 Added %s file %s (%i lumps from memory)\n", ((isIWAD==0)?"IWAD":"PWAD"),filename.c_str(), numitems);  
  return true;
}

int WadFromMemory::GetItemSize(int i)
{
  return directory[i].size;
}

const char *WadFromMemory::GetItemName(int i)
{
  return directory[i].name;
}


// Searches the wadfile for lump named 'name', returns the lump number
// if not found, returns -1
int WadFromMemory::FindNumForName(const char *name, int startlump)
{
  union
  {
    char s[9];
    Uint32 x[2];
  };

  // make the name into two integers for easy compares
  strncpy(s, name, 8);

  // in case the name was 8 chars long
  s[8] = 0;
  // case insensitive TODO make it case sensitive if possible
  strupr(s);

  // FIXME doom.wad and doom2.wad PNAMES lumps have exactly ONE (1!) patch
  // entry with a lowcase name: w94_1. Of course the actual lump is
  // named W94_1, so it won't be found if we have case sensitive search! damn!
  // heretic.wad and hexen.wad have no such problems.
  // The right way to fix this is either to fix the WADs (yeah, right!) or handle
  // this special case in the texture loading routine.

  waddir_t *p = directory + startlump;

  // a slower alternative could use strncasecmp()
  for (int j = startlump; j < numitems; j++, p++)
    if (p->iname[0] == x[0] && p->iname[1] == x[1])
      return j;

  // not found
  return -1;
}


int WadFromMemory::FindPartialName(Uint32 iname, int startlump, const char **fullname)
{
  // checks only first 4 characters, returns full name
  // a slower alternative could use strncasecmp()

  waddir_t *p = directory + startlump;

  for (int j = startlump; j < numitems; j++, p++)
    if (p->iname[0] == iname)
      {
	*fullname = p->name;
	return j;
      }

  // not found
  return -1;
}


int WadFromMemory::Internal_ReadItem(int item, void *dest, unsigned size, unsigned offset)
{
  if (item < 0 || item >= numitems || !memory_data) 
  {
    CONS_Printf("WadFromMemory::Internal_ReadItem ERROR: item %d ungültig (numitems=%d)\n", item, numitems);
    return 0;
  }

  waddir_t *l = &directory[item];

  unsigned lump_start = l->offset;
  unsigned lump_size = l->size;

  if (offset >= lump_size) 
  {
    CONS_Printf("WadFromMemory::Internal_ReadItem WARN: offset %u >= size %u\n", offset, lump_size);
    return 0;
  }

  unsigned read_size = size;
  if (offset + read_size > lump_size) read_size = lump_size - offset;

  memcpy(dest, memory_data + lump_start + offset, read_size);
  return read_size;
}

void WadFromMemory::ListItems()
{
  waddir_t *p = directory;
  for (int i = 0; i < numitems; i++, p++)
    printf("%-8s\n", p->name);
}

// LoadDehackedLumps
// search for DEHACKED lumps in a loaded wad and process them
void WadFromMemory::LoadDehackedLumps()
{
  // just the lump number, nothing else
  int clump = 0;

  while (1)
    { 
      clump = FindNumForName("DEHACKED", clump);
      if (clump == -1)
	break;
      CONS_Printf(" Loading DEHACKED lump %d from %s\n", clump, filename.c_str());

      DEH.LoadDehackedLump(clump);
      clump++;
    }
}



//==============================
//  Wad3 class implementation
//==============================

/// a WAD2 or WAD3 directory entry
struct wad3dir_t
{
  Uint32  offset; ///< offset of the data lump
  Uint32  dsize;  ///< data lump size in file (compressed)
  Uint32  size;   ///< data lump size in memory (uncompressed)
  char type;      ///< type (data format) of entry. not needed.
  char compression; ///< kind of compression used. 0 means none.
  char padding[2];  ///< unused
  union
  {
    char name[16]; // name of the entry, padded with '\0'
    Uint32 iname[4];
  };
} __attribute__((packed));


// constructor
Wad3::Wad3()
{
  directory = NULL;
}

// destructor
Wad3::~Wad3()
{
  Z_Free(directory);
}



// Loads a WAD2 or WAD3 file, sets up the directory and cache.
// Returns false in case of problem
bool Wad3::Open(const char *fname)
{
  // common to all files
  if (!VDataFile::Open(fname))
    return false;

  // read header
  wadheader_t h;
  rewind(stream);
  fread(&h, sizeof(wadheader_t), 1, stream);
  // endianness swapping
  numitems = LONG(h.numentries);
  int diroffset = LONG(h.diroffset);

  // set up caching
  cache = (lumpcache_t *)Z_Malloc(numitems * sizeof(lumpcache_t), PU_STATIC, NULL);
  memset(cache, 0, numitems * sizeof(lumpcache_t));

  // read in directory
  fseek(stream, diroffset, SEEK_SET);
  wad3dir_t *p = directory = (wad3dir_t *)Z_Malloc(numitems * sizeof(wad3dir_t), PU_STATIC, NULL);
  fread(directory, sizeof(wad3dir_t), numitems, stream);

  // endianness conversion for directory
  for (int i = 0; i < numitems; i++, p++)
    {
      p->offset = LONG(p->offset);
      p->dsize  = LONG(p->dsize);
      p->size   = LONG(p->size);
      TestPadding(p->name, 16);
    }
    
  h.numentries = 0; // what a great hack!

  CONS_Printf(" Added %s file %s (%i lumps)\n", h.magic, filename.c_str(), numitems);
  return true;
}


int Wad3::GetItemSize(int i)
{
  return directory[i].size;
}

const char *Wad3::GetItemName(int i)
{
  return directory[i].name;
}

void Wad3::ListItems()
{
  wad3dir_t *p = directory;
  for (int i = 0; i < numitems; i++, p++)
    printf("%-16s\n", p->name);
}


int Wad3::Internal_ReadItem(int lump, void *dest, unsigned size, unsigned offset)
{
  wad3dir_t *l = directory + lump;
  if (l->compression)
    return -1;
  
  fseek(stream, l->offset + offset, SEEK_SET);
  return fread(dest, 1, size, stream);
}



// FindNumForName
// Searches the wadfile for lump named 'name', returns the lump number
// if not found, returns -1
int Wad3::FindNumForName(const char *name, int startlump)
{
  union
  {
    char s[16];
    Uint32 x[4];
  };

  // make the name into 4 integers for easy compares
  // strncpy pads the target string with zeros if needed
  strncpy(s, name, 16);

  // comparison is case sensitive

  wad3dir_t *p = directory + startlump;

  int j;
  for (j = startlump; j < numitems; j++, p++)
    {
      if (p->iname[0] == x[0] && p->iname[1] == x[1] &&
	  p->iname[2] == x[2] && p->iname[3] == x[3])
	return j; 
    }
  // not found
  return -1;
}



//=============================
//  Pak class implementation
//=============================

/// PACK file header
struct pakheader_t
{
  char   magic[4];   ///< "PACK"
  Uint32 diroffset;  ///< offset to directory
  Uint32 dirsize;    ///< numentries * sizeof(pakdir_t) == numentries * 64
} __attribute__((packed));


/// PACK directory entry
struct pakdir_t
{
  char   name[56]; ///< item name, NUL-padded
  Uint32 offset;   ///< file offset for the item
  Uint32 size;     ///< item size
} __attribute__((packed));


// constructor
Pak::Pak()
{
  directory = NULL;
}

// destructor
Pak::~Pak()
{
  Z_Free(directory);
}



// Loads a PACK file, sets up the directory and cache.
// Returns false in case of problem
bool Pak::Open(const char *fname)
{
  // common to all files
  if (!VDataFile::Open(fname))
    return false;

  // read header
  pakheader_t h;
  rewind(stream);
  fread(&h, sizeof(pakheader_t), 1, stream);
  // endianness swapping
  numitems = LONG(h.dirsize) / sizeof(pakdir_t);
  int diroffset = LONG(h.diroffset);

  // set up caching
  cache = (lumpcache_t *)Z_Malloc(numitems * sizeof(lumpcache_t), PU_STATIC, NULL);
  memset(cache, 0, numitems * sizeof(lumpcache_t));

  // read in directory
  fseek(stream, diroffset, SEEK_SET);
  pakdir_t *p = directory = (pakdir_t *)Z_Malloc(numitems * sizeof(pakdir_t), PU_STATIC, NULL);
  fread(directory, sizeof(pakdir_t), numitems, stream);

  // endianness conversion for directory
  for (int i = 0; i < numitems; i++, p++)
    {
      p->offset = LONG(p->offset);
      p->size   = LONG(p->size);
      TestPadding(p->name, 56);
      p->name[55] = 0; // precaution
      imap.insert(pair<const char *, int>(p->name, i)); // fill the name map
    }
    
  CONS_Printf(" Added PACK file %s (%i lumps)\n", filename.c_str(), numitems);
  return true;
}


int Pak::GetItemSize(int i)
{
  return directory[i].size;
}

const char *Pak::GetItemName(int i)
{
  return directory[i].name;
}

void Pak::ListItems()
{
  pakdir_t *p = directory;
  for (int i = 0; i < numitems; i++, p++)
    printf("%-56s\n", p->name);
}


int Pak::Internal_ReadItem(int item, void *dest, unsigned size, unsigned offset)
{
  pakdir_t *l = directory + item;
  fseek(stream, l->offset + offset, SEEK_SET);
  return fread(dest, 1, size, stream);
}



//================================
//  ZipFile class implementation
//================================

#pragma pack(push, 1)
struct zip_central_directory_end_t
{
  char   signature[4]; // 0x50, 0x4b, 0x05, 0x06
  Uint16 disk_number;  // ZIP archives can consist of several files, but we do not support this feature.
  Uint16 num_of_disk_with_cd_start;
  Uint16 num_entries_on_this_disk;
  Uint16 total_num_entries;
  Uint32 cd_size;
  Uint32 cd_offset;
  Uint16 comment_size;
  //char   comment[0]; // from here to end of file
};
#pragma pack(pop)

#pragma pack(push, 1)
struct zip_file_header_t
{
  char   signature[4]; // 0x50, 0x4b, 0x01, 0x02
  Uint16 version_made_by;
  Uint16 version_needed;
  Uint16 flags;
  Uint16 compression_method;
  Uint16 last_modified_time;
  Uint16 last_modified_date;
  Uint32 crc32;
  Uint32 compressed_size;
  Uint32 size;
  Uint16 filename_size;
  Uint16 extrafield_size;
  Uint16 comment_size;
  Uint16 disk_number;
  Uint16 internal_attributes;
  Uint32 external_attributes;
  Uint32 local_header_offset;
  char   filename[0];
  // followed by filename and other variable-length fields
};
#pragma pack(pop)
static_assert(sizeof(zip_file_header_t) == 46,"ZIP Central Directory Header muss genau 46 Bytes groß sein!");

#pragma pack(push, 1)
struct zip_local_header_t {
    uint8_t  signature[4];          // PK\x03\x04 (0x50, 0x4b, 0x03, 0x04)
    uint16_t version_needed;
    uint16_t flags;
    uint16_t compression_method;
    uint16_t last_modified_time;
    uint16_t last_modified_date;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t size;                //uncompressed_size
    uint16_t filename_size;       //filename_length
    uint16_t extrafield_size;     //extra_field_length
    // Dateiname und Extra folgen danach
};
#pragma pack(pop)
static_assert(sizeof(zip_local_header_t) == 30, "Local Header muss 30 Byte sein!");


/// Runtime ZipFile directory entry.
#define ZIP_NAME_LENGTH 64
#pragma pack(push, 1)
struct zipdir_t
{
  uint32_t     offset;
  uint32_t     compressed_size;
  uint32_t     size;
  char         name[ZIP_NAME_LENGTH + 1];
  uint8_t      deflated;  // statt bool – 1 Byte garantiert
};
#pragma pack(pop)
static_assert(sizeof(zipdir_t) == 4+4+4+65+1, "zipdir_t muss 78 Bytes sein!");

ZipFile::ZipFile()
{
  directory = NULL;
}


ZipFile::~ZipFile()
{
  
  if (directory)
    Z_Free(directory);
}



byte   ZIP_FlagCheck(int ZipFlags, const char *ZipFileName, char *LumpName, zip_file_header_t *ZipFileHeader)
{
  if (ZipFlags & 0x1)
  {
    CONS_Printf(" Lump '%s' in ZIP file '%s' is encrypted.\n", LumpName, ZipFileName);
    return 1;
  }

  if (ZipFlags & 0x8)
  {
    CONS_Printf(" Lump '%s' in ZIP file '%s' has a data descriptor (unsupported).\n",LumpName, ZipFileName);
    return 1;
  }

  if (ZipFileHeader->compression_method != 0 && read_le16(&ZipFileHeader->compression_method) != Z_DEFLATED)
  {
    CONS_Printf(" Lump '%s' in ZIP file '%s' uses an unsupported compression algorithm.\n",
    LumpName, ZipFileName);
    return 1;
  }

  if (ZipFileHeader->compression_method == 0 && ZipFileHeader->size != ZipFileHeader->compressed_size)
  {
    CONS_Printf(" Uncompressed lump '%s' in ZIP file '%s' has unequal compressed and uncompressed sizes.\n",
    LumpName, ZipFileName);
    return 1;
  }
  return 0;
  
}
byte   ZIP_isCorrupted (Uint32 Central_Directoy_End, Uint32 DirSize, Uint32 DirOffset, const char *ZipFileName)
{
  if (Central_Directoy_End < DirOffset + DirSize)
  {
    // corrupt file
    CONS_Printf(" ZIP ERROR: file \"%s\" is corrupted.\n", ZipFileName);
    return 1;
  }
  return 0;
}

byte   ZIP_isMultiFiles(zip_central_directory_end_t Central_Directoy_End, const char *ZipFileName)
{
 
  // ZIP files are little endian, but zeros are zeros...
  if (Central_Directoy_End.disk_number               != 0 ||
      Central_Directoy_End.num_of_disk_with_cd_start != 0 ||
      Central_Directoy_End.num_entries_on_this_disk  != Central_Directoy_End.total_num_entries)
  {
      // we do not support multi-file ZIPs
      CONS_Printf(" ZIP file \"%s\" spans several disks, this is not supported.\n", ZipFileName);
      //CONS_Printf(" ZIP Warn: file \"%s\" Erstreckt sich über mehrere Festplatten, dies wird nicht unterstützt.\n", ZipFileName);                
      return 1;
  }
  
  return 0;  
}

byte   ZIP_Get_Signature_PK12(zip_file_header_t *ZipFileHeader, const char *ZipFileName)
{
    if (ZipFileHeader->signature[0] != 'P' ||
        ZipFileHeader->signature[1] != 'K' ||    
        ZipFileHeader->signature[2] != '\1'||
        ZipFileHeader->signature[3] != '\2' )
    {
      // corrupted directory
      CONS_Printf(" ZIP ERROR: Central directory in ZIP file \"%s\" is corrupted.\n", ZipFileName);
      return 1;
    }
 
    //CONS_Printf("\n");
    //CONS_Printf(" [%s][%d] Zip Signature Search: ( PK\\01\\02 ) Gefunden\n",__FILE__,__LINE__);
    
    return 0;
}

byte   ZIP_Get_Signature_PK34(zip_local_header_t ZipLocalHeader, const char *ZipFileName, char *LumpName)
{
    if (ZipLocalHeader.signature[0] != 'P' ||
        ZipLocalHeader.signature[1] != 'K' ||
        ZipLocalHeader.signature[2] != '\3'||
        ZipLocalHeader.signature[3] != '\4' )
    {
      
      CONS_Printf(" ZIP ERROR: Could not find local header for lump \"%s\" in ZIP file \"%s\".\n",
		  LumpName, ZipFileName);
      return 1;
    }
    /* 
    CONS_Printf("\n");
    CONS_Printf(" [%s][%d] Zip Signature Search: ( PK\\01\\02 ) Gefunden\n",__FILE__,__LINE__);
    CONS_Printf("         Local Header for Lump: \"%s\"\n",LumpName);
    */
    return 0;
}   

byte   ZIP_CentralDirectory_Match(zip_file_header_t *ZipFileHeader , zip_local_header_t ZipLocalHeader, const char *ZipFileName, char *LumpName)
{
    if (ZipLocalHeader.flags              != ZipFileHeader->flags ||
        ZipLocalHeader.compression_method != ZipFileHeader->compression_method ||
        ZipLocalHeader.compressed_size    != ZipFileHeader->compressed_size ||
        ZipLocalHeader.size               != ZipFileHeader->size)
    {
      CONS_Printf(" ZIP ERROR: Local header for lump \"%s\" in ZIP file \"%s\" does not match the central directory.\n",
		  LumpName, ZipFileName);
      return 1;
    }
    
    //CONS_Printf("\n");
    //CONS_Printf(" [%s][%d] Central Directory Successfully Matched\n",__FILE__,__LINE__);
    //CONS_Printf("          Local Header for Lump: \"%s\": OK\n",LumpName);
    
    return 0;
}
Uint32 ZIP_Get_CentralDirectoyEnd(byte *Buffer, Uint32 Central_Directoy_End, unsigned int size, int max_csize )
{
  int k;
  
  // Rückwärts suchen – vom Ende der eingelesenen Daten aus
  for (k = max_csize - sizeof(zip_central_directory_end_t); k >= 0; k--)
  {
    if (Buffer[k]   ==  'P' &&
        Buffer[k+1] ==  'K' &&
        Buffer[k+2] == '\5' &&
        Buffer[k+3] == '\6'  )
    { 
        Central_Directoy_End = size - max_csize + k;
        /*
        CONS_Printf("\n");
        CONS_Printf(" [%s][%d] Zip Header Search   : ( PK\\05\\06 ) Gefunden\n"
                    "        CentralDirectoy Offset: %d\n",
                      __FILE__,__LINE__,Central_Directoy_End);
        */                                  
        break;
    }
   }

  if (Central_Directoy_End == 0)
  {
    // could not find cd end
    //CONS_Printf("\n");
    //CONS_Printf(" [%s][%d] ZIP ERROR: Header Search ( PK0506 ): Not Found\n",__FILE__,__LINE__);
  }
  
  Z_Free(Buffer);   
  return Central_Directoy_End;   
}
bool ZipFile::Open(const char *fname)
{

  // common to all files
  if (!VDataFile::Open(fname))
    return false;

  CONS_Printf(" ZipFile -> Open: \"%s\"\n",fname);
  CONS_Printf("------------------------------------------------------------\n"); 
  // Find and read the central directory end.
  // We have to go through this because of the stupidly-placed ZIP comment field...
  
  Uint32 cd_end_pos = 0;
  {
    const int SEARCH_RANGE = 1024 * 256;
    //int max_csize = min(size, 0xFFFF); // max. comment size is 0xFFFF    
    int max_csize = (size > SEARCH_RANGE) ? SEARCH_RANGE : size;    
        
    byte *buf = static_cast<byte*>(Z_Malloc(max_csize, PU_STATIC, NULL));
    if (!buf) return false;    
    
    fseek(stream, size - max_csize, SEEK_SET);
    if (fread(buf, max_csize, 1, stream) != 1)
    {
       Z_Free(buf);
       return false;
    } 
    
    cd_end_pos = ZIP_Get_CentralDirectoyEnd(buf, cd_end_pos, size, max_csize);
    if (buf) Z_Free(buf);     
  }

  if (cd_end_pos == 0)
    return false;

  zip_central_directory_end_t cd_end;
  
  fseek(stream, cd_end_pos, SEEK_SET);
  fread(&cd_end, sizeof(zip_central_directory_end_t), 1, stream);
 
  if (ZIP_isMultiFiles(cd_end, fname) != 0)
    return false;
    
  // Dann bei cd_end lesen:
  numitems   = read_le16(&cd_end.total_num_entries);
  Uint32 dir_size   = read_le32(&cd_end.cd_size);
  Uint32 dir_offset = read_le32(&cd_end.cd_offset);

  bool WadFile = false; 
  if (ZIP_isCorrupted (cd_end_pos, dir_size, dir_offset, fname) != 0)
    return false;  


  // read central directory
  directory = static_cast<zipdir_t*>(Z_Malloc(numitems * sizeof(zipdir_t), PU_STATIC, NULL));

  byte *tempdir = static_cast<byte*>(Z_Malloc(dir_size, PU_STATIC, NULL));
  fseek(stream, dir_offset, SEEK_SET);
  fread(tempdir, dir_size, 1, stream);

  byte *p = tempdir;
  int item = 0; // since items may be ignored
 
  // Schleife =============================================================================
  for (int k=0; k<numitems; k++)
  {
    zip_file_header_t *fh = reinterpret_cast<zip_file_header_t*>(p);

    /* Get Singnature PK \1 \2 */
    if (ZIP_Get_Signature_PK12(fh, fname) != 0)
      return false;     

    // go to next record
    int n_size = read_le16(&fh->filename_size);
    p += sizeof(zip_file_header_t) + n_size + read_le16(&fh->extrafield_size) + read_le16(&fh->comment_size);

    if (p > tempdir + dir_size)
    {
      CONS_Printf(" Central directory in ZIP file \"%s\" is too long.\n", fname);
      break;
    }

    if (fh->filename[n_size-1] == '/')
      continue; // ignore directories

    // copy the lump name
    n_size = min(n_size, ZIP_NAME_LENGTH);
    strncpy(directory[item].name, fh->filename, n_size);
    directory[item].name[n_size] = '\0'; // NUL-termination


    int flags = read_le16(&fh->flags);
    if (ZIP_FlagCheck(flags, fname,  directory[item].name, fh) != 0)
      continue;
    
    // copy relevant fields to our directory - Alles ok – Felder kopieren
    directory[item].offset          = read_le32(&fh->local_header_offset);
    directory[item].size            = read_le32(&fh->size);
    directory[item].compressed_size = read_le32(&fh->compressed_size);
    directory[item].deflated        = (SHORT(fh->compression_method) == Z_DEFLATED); // boolean
    
    // check if the local file header matches the central directory entry
    zip_local_header_t lh;
    fseek(stream, directory[item].offset, SEEK_SET);
    fread(&lh, sizeof(zip_local_header_t), 1, stream);

    /* Get Singnature PK \3 \4 */
    if (ZIP_Get_Signature_PK34(lh, fname, directory[item].name ) != 0)
      continue;

    /* Central Directory Match */
    if (ZIP_CentralDirectory_Match(fh, lh, fname, directory[item].name)!=0)
      continue;

    // make offset point directly to the data
    directory[item].offset += sizeof(zip_local_header_t) + SHORT(lh.filename_size) + SHORT(lh.extrafield_size);
         
    if (item < 10)
    {
      CONS_Printf(" [%s][%d] Contains\n",__FILE__,__LINE__);
      CONS_Printf("   Filename             :\"%s\"\n",               directory[item].name);
      CONS_Printf("   Local-Header-Offset  : 0x%08X (%u decimal)\n", directory[item].offset,
                                                                     directory[item].offset);
      CONS_Printf("   File Size Compressed : %8d Bytes\n",           directory[item].compressed_size);                                                            
      CONS_Printf("   File Size Original   : %8d Bytes\n",           directory[item].size);
      CONS_Printf("   Compression Methode  : %s\n", (fh->compression_method==Z_DEFLATED)?"Deflate":"Stored");
    }
    
    const char *ext = strrchr(directory[item].name, '.');
    
    ext++; // Nach dem Punkt
    if (strcasecmp(ext, "wad") == 0)
    {
      WadFile = true;
    }
    
    imap.insert(pair<const char *, int>(directory[item].name, item)); // fill the name map
    
    item++;
    CONS_Printf("------------------------------------------------------------\n");      
  }   

  // NOTE: If lumps are ignored, there will be a few empty records at the end of directory. Let them be.
  numitems =item;
  
  Z_Free(tempdir);

  // set up caching (Cache allokieren (nur für akzeptierte Items) )
  cache = (lumpcache_t *)Z_Malloc(numitems * sizeof(lumpcache_t), PU_STATIC, NULL);
  memset(cache, 0, numitems * sizeof(lumpcache_t));
  
  // CONS_Printf(" Added ZIP file %s (%i lumps)\n", filename.c_str(), numitems); 
  CONS_Printf(" ZIP/PK3 Added from \"%s\" = %i File%s)\n", 
              filename.c_str(), numitems, numitems==1?"":"s");
              
  if (WadFile)
  {
    int nAdded = GetItemListFromMemory();


  }
  
  return true;
}


/* Marty */


int ZipFile::GetItemListFromMemory()
{
  //CONS_Printf(" ZIP/PK3 Get Item List From Memory\n");
  
  ListItems();
  
  Uint16 Added = -1;
  Uint8 isIWAD = -1; 
  if (directory == NULL || numitems <= 0)
  {
    CONS_Printf("  [%s][%d]Error::GetItemListFromMemory:  -> Kein Verzeichnis oder keine Items (numitems = %d)\n", numitems);
    return Added;
  }
      
    for (int i = 0; i < numitems; i++)
    {
       
      zipdir_t *l = &directory[i];
      if (l->size < 12) continue; // zu klein für WAD-Header

      const char *ext = strrchr(l->name, '.');
      if (!ext) continue;
    
      ext++; // Nach dem Punkt
      if ((strcasecmp(ext, "wad") == 0) ||  // Dateinamen Prüfung
         (strcasecmp(ext, "gwa") == 0)) 
      { 
    
        // Temporär laden
        byte* wad_data = (byte*)Z_Malloc(l->size, PU_STATIC, NULL);
        if (!wad_data) continue;

        uint32_t read = Internal_ReadItem(i, wad_data, l->size, 0);
        if (read != l->size)
        {
          Z_Free(wad_data); continue;
        }

        // Magic prüfen
        if (memcmp(wad_data, "IWAD", 4) == 0)
            isIWAD = 0;

        if (memcmp(wad_data, "PWAD", 4) == 0)
            isIWAD = 1;    
        
        if (isIWAD >= 0)
        {  
          //CONS_Printf(" ZIP/PK3 \"%s\" Magic Header = %s\n", l->name, (isIWAD==0)?"IWAD":"PWAD");
          // Neue Wad-Instanz erstellen und direkt parsen
          WadFromMemory* inner_wad = new WadFromMemory();
          if (inner_wad->Open(wad_data, l->size, l->name))
          {
              fc.CacheArchiveFile(inner_wad);
              //CONS_Printf(" ZIP/PK3 \"%s\" Added to vFiles (%d Lumps)\n", l->name,
              //                                           inner_wad->GetNumItems());
              Added++; 
          }
          else
          {
            delete inner_wad;
          }
          //fc.CacheArchiveFile_Remove(inner_wad);          
        }
        if (wad_data)
          Z_Free(wad_data);
        
        
      } // Ende - for (int i = 0; i < numitems; i++)
        
    } // Ende - // Dateinamen Prüfung

  
    
  return Added;
}


int ZipFile::GetItemSize(int i)
{
  //CONS_Printf("\n ZipFile::GetItemSize\n");  
  return directory[i].size;
}


const char *ZipFile::GetItemName(int i)
{
  //CONS_Printf("\n ZipFile::GetItemName\n");
  return directory[i].name;
}


void ZipFile::ListItems()
{
 
  CONS_Printf("\n ZipFile -> List Directory\n");
  CONS_Printf("------------------------------------------------------------\n"); 
  if (directory == NULL || numitems <= 0)
  {
    CONS_Printf("  -> Kein Verzeichnis oder keine Items (numitems = %d)\n", numitems);
    return;
  }
  zipdir_t *p = directory;
    
  for (int i = 0; i < numitems; i++, p++)
  { 
    //printf("%-64s\n", p->name);
    CONS_Printf("  [%2d] %-12s | size=%-10u bytes | offset=0x%08X\n", 
            i, p->name, p->size, p->offset);    
  }
  //cache = (lumpcache_t *)Z_Malloc(numitems * sizeof(lumpcache_t), PU_STATIC, NULL);
  //memset(cache, 0, numitems * sizeof(lumpcache_t));  
}

int ZipFile::Internal_ReadItem(int item, void *dest, uint32_t size, uint32_t offset)
{
  zipdir_t *l = directory + item;
  CONS_Printf("\n-------------------------------------------------ZIP EXTRACT\n"); 
  if (item >= numitems || item < 0)
  {
      CONS_Printf("ERROR: Ungültiger ZIP-Item %d\n", item);
      return 0;
  }

  if (l->size == 0)
  {
      CONS_Printf("WARN: ZIP-Item %d hat Größe 0\n", item);
      return 0;
  }
    
  if (!l->deflated)
  {
      fseek(stream, l->offset + offset, SEEK_SET); // skip to correct offset within the uncompressed lump
      return fread(dest, 1, size, stream); // uncompressed lump
  }

  //CONS_Printf(" Debug - [%s][%d]::Internal_ReadItem %s\n",__FILE__,__LINE__,l->name);
    
  // DEFLATEd lump, uncompress it

  // NOTE: inflating compressed lumps can be expensive, so we transparently cache them at first use.
  // This way we only have to inflate the lump once. Uncompressed lumps are treated as usual.
  // NOTE: this function is only called when an item has not been found in the lumpcache.

  fseek(stream, l->offset, SEEK_SET); // seek to lump start within the file

  unsigned unpack_size = l->size;
  if (!cache[item])
    Z_Malloc(unpack_size, PU_STATIC, &cache[item]); // even if cache[item] is allocated, it is not yet filled with data

  z_stream zs; // stores the decompressor state

  // output buffer
  zs.avail_out = unpack_size;
  zs.next_out  = static_cast<byte*>(cache[item]);

  int chunksize = min(max(unpack_size, 256u), 8192u); // guesstimate
  byte in[chunksize];

  zs.zalloc   = Z_NULL;
  zs.zfree    = Z_NULL;
  zs.opaque   = Z_NULL;
  zs.avail_in = 0;
  zs.next_in  = Z_NULL;

  // TODO more informative error messages
  int ret = inflateInit2(&zs, -MAX_WBITS); // tell zlib not to expect any headers
  if (ret != Z_OK)
    I_Error(" [%s][%d]::Internal_ReadItem: Fatal zlib error.\n",__FILE__,__LINE__);

  // decompress until deflate stream ends or we have enough data
  do
    {
      // get some input
      // NOTE: we may fread past the end of the lump, but that should not be harmful.
      zs.avail_in = fread(in, 1, chunksize, stream);
      if (ferror(stream) || zs.avail_in == 0)
      {
        inflateEnd(&zs);
          I_Error(" [%s][%d]::Internal_ReadItem: Error decompressing a ZIP lump!\n",__FILE__,__LINE__);
      }

      zs.next_in = in;

      // decompress as much as possible
      ret = inflate(&zs, Z_SYNC_FLUSH);
    } while (ret == Z_OK && zs.avail_out != 0); // decompression can continue && not done yet

  inflateEnd(&zs);

  switch (ret)
  {
    case Z_STREAM_END:
    {
      // ran out of input
      if (zs.avail_out != 0)
        I_Error(" [%s][%d]::Internal_ReadItem: DEFLATE stream ended prematurely.\n",__FILE__,__LINE__);
    }
    // fallthru
    case Z_OK:
      break;

    case Z_NEED_DICT:
    case Z_DATA_ERROR:
    case Z_STREAM_ERROR:
    case Z_MEM_ERROR:
    case Z_BUF_ERROR:
    default:
      I_Error("Error while decompressing a ZIP lump!\n",__FILE__,__LINE__);
      break;
    }
  // successful
  // Magic prüfen
  Uint8 isIWAD = -1;
  
  if (memcmp(cache[item], "IWAD", 4)     == 0)
    isIWAD = 0;
  else if (memcmp(cache[item], "PWAD", 4) == 0)
  {
    isIWAD = 1;   
  }
  else 
  {
    CONS_Printf(" ZIP/PK3 Successful. Item Extracted but not a IWAD or PWAD\n"/*,cache[item]*/); 
    return 0;
  }
  
  CONS_Printf(" ZIP/PK3 Successful. Lump is Uncompressed & Cached as %s\n\n", (isIWAD==0)?"IWAD":"PWAD");  
  //// now the lump is uncompressed and cached
  memcpy(dest, static_cast<byte*>(cache[item]) + offset, size);
  return size;
}
