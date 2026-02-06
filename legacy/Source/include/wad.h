// Emacs style mode select   -*- C++ -*- 
//--------------------------------------------------------------
//
// $Id: wad.h 449 2007-05-05 20:45:25Z smite-meister $
//
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
//--------------------------------------------------------------

/// \file
/// \brief Wad, Wad2, Wad3, PACK and PK3/ZIPfile formats

#ifndef wad_h
#define wad_h 1

#include "doomtype.h"
#include "vfile.h"

using namespace std;


/// \brief Handles all IWAD/PWAD I/O and keeps a cache for the data lumps.
///
/// (almost) no knowledge of a game type is assumed.

class Wad : public VDataFile
{
  friend class FileCache;
protected:
  struct waddir_t *directory;  ///< wad directory

  virtual int Internal_ReadItem(int item, void *dest, uint32_t size, uint32_t offset = 0);

public:
  // constructor and destructor
  Wad();
  virtual ~Wad();

  /// open a new wadfile
  virtual bool Open(const char *fname);
  virtual bool Create(const char *fname, const char *lumpname); ///< create a single-lump wad from a file

  // query data item properties
  virtual const char *GetItemName(int i);
  virtual int  GetItemSize(int i);
  virtual void ListItems();

  // search
  virtual int FindNumForName(const char* name, int startlump = 0);
  virtual int FindPartialName(Uint32 iname, int startlump, const char **fullname);

  /// process any DeHackEd lumps in this wad
  
  void LoadDehackedLumps();

  //virtual bool OpenFromMemory(byte* data, size_t datasize, const char* virtual_name = nullptr);
};

/* Marty */
class WadFromMemory : public Wad
{
    friend class FileCache;
    
protected:
  byte* memory_data;
  size_t memory_size;
  struct waddir_t *directory;  ///< wad directory
  
  // Muss implementiert werden, weil pure virtual in VDataFile/VFile
  virtual int Internal_ReadItem(int item, void *dest, uint32_t size, uint32_t offset = 0);
  
public:
  WadFromMemory();
  virtual ~WadFromMemory();

  virtual bool Open(byte* data, size_t datasize, const char* virtual_name = nullptr);
  virtual bool Create(byte* data, size_t datasize, const char* virtual_name, const char *lumpname);
  // query data item properties
  virtual const char *GetItemName(int i);
  virtual int  GetItemSize(int i);
  virtual void ListItems();
  
  // search
  virtual int FindNumForName(const char* name, int startlump = 0);
  virtual int FindPartialName(Uint32 iname, int startlump, const char **fullname);

  /// process any DeHackEd lumps in this wad
  
  void LoadDehackedLumps();
  
  void Lump_Extract(int numitems, waddir_t *directory, const char* lump_ext, const char* destinationpath);
  

};


//========================================================================

/// \brief Handles all WAD2/WAD3 I/O and keeps a cache for the data lumps.
///
/// WAD2 (Quake, Hexen 2 etc.) and WAD3 (Half-Life) are
/// nearly identical file formats (really!). Therefore only one class.

class Wad3 : public VDataFile
{
  friend class FileCache;
protected:
  struct wad3dir_t *directory;  ///< wad directory

  virtual int Internal_ReadItem(int item, void *dest, unsigned size, unsigned offset = 0);

public:
  // constructor and destructor
  Wad3();
  virtual ~Wad3();

  /// open a new wadfile
  virtual bool Open(const char *fname);

  // query data item properties
  virtual const char *GetItemName(int i);
  virtual int  GetItemSize(int i);
  virtual void ListItems();

  /// search
  virtual int FindNumForName(const char* name, int startlump = 0);
};


//========================================================================

/// \brief PACK files from Quake et al.

class Pak : public VDataFile
{
protected:
  struct pakdir_t *directory; ///< item directory

  virtual int Internal_ReadItem(int item, void *dest, unsigned size, unsigned offset = 0);

public:
  // constructor and destructor
  Pak();
  virtual ~Pak();

  // open a new file
  virtual bool Open(const char *fname);

  // query data item properties
  virtual const char *GetItemName(int i);
  virtual int  GetItemSize(int i);
  virtual void ListItems();
};

//========================================================================

/// \brief PK3/ZIP files from Quake3 etc.

class ZipFile : public VDataFile
{
    friend class FileCache;
    friend class Wad;
    
protected:
  struct zipdir_t *directory; ///< item directory

  virtual int Internal_ReadItem(int item, void *dest, uint32_t size, uint32_t offset = 0);

public:
  // constructor and destructor
  ZipFile();
  virtual ~ZipFile();

  /// open a new wadfile
  virtual bool Open(const char *fname);

  // query data item properties
  virtual const char *GetItemName(int i);
  virtual int  GetItemSize(int i);
  virtual void ListItems();

  // search
  //virtual int FindPartialName(Uint32 iname, int startlump, const char **fullname);
  
  /* Marty */
  virtual int  GetItemListFromMemory();
  
};

#endif
