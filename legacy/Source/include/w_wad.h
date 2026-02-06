// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: w_wad.h 463 2007-05-21 23:32:04Z smite-meister $
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
//-----------------------------------------------------------------------------

/// \file
/// \brief FileCache, a class for storing VFiles.

#ifndef w_wad_h
#define w_wad_h 1

#include <vector>
#include <string>

#include "doomtype.h"

using namespace std;
namespace TNL { class BitStream; };

#define MAX_WADPATH   128
#define MAX_WADFILES  32       // maximum of wad files used at the same time


//=========================================================================
/// \brief First-level cache for data files
///
/// A class for holding open VFile objects and giving out information about them
/// This class also handles game dependent operations concerning wad files.
/// The data items (lumps) inside the files are numbered using a 16.16 scheme,
/// where the first short is the file number and the second short the number of the lump
/// inside the file.

class FileCache
{
private:
  string datapath;              ///< absolute path for searching files
  vector<class VFile *> vfiles; ///< open virtual files

public:
  ~FileCache();

  /// Set the default path.
  void SetPath(const char *path);
  /// Try to find the given file, return path where found.
  const char *Access(const char *filename);
  /// Open a new VFile, return -1 on error.
  int  AddFile(const char *filename, bool silent = false);
  /// Return true if everything is OK, false if at least one file was missing.
  bool InitMultipleFiles(const char *const*filenames);

  // These methods return info about the open VFiles.

  /// Number of open VFiles.
  unsigned int Size() const { return vfiles.size(); };
  /// Name of the VFile.
  const char *Name(int filenum);
  /// Number of lumps in the VFile
  unsigned int GetNumLumps(int filenum);
  /// Writes miscellaneous info about the VFile into the BitStream.
  void WriteNetInfo(TNL::BitStream &s);

  // Search methods return a lump number. Return -1 if nothing is found.

  /// Search a lump by name.
  int FindNumForName(const char *name, bool scanforward = false);
  /// Search a lump by name, generate an error if nothing is found.
  int GetNumForName(const char *name, bool scanforward = false);
  /// Search a lump by name inside a specific file, starting from a given lump.
  int FindNumForNameFile(const char *name, unsigned filenum, int startlump = 0);
  /// Return the first lump in a file whose name starts with the given four bytes, also returns the full name of the lump.
  int FindPartialName(Uint32 iname, unsigned filenum, int startlump, const char **fullname);

  // Lump info.

  /// Name of a lump.
  const char *FindNameForNum(int lump);
  /// Length of a lump in bytes.
  int LumpLength(int lump);

  // Reading and caching of lumps.

  /// Read size bytes from the lump (starting from the given offset) into dest. Returns the number of bytes actually read.
  int ReadLumpHeader(int lump, void *dest, unsigned size, unsigned offset = 0);
  /// Read the entire lump into memory, allocating the space. Returns a pointer to the buffer.
  void *CacheLumpNum(int lump, int tag, bool add_NUL = false);
  /// Read the entire lump into dest without allocating any memory.
  inline int ReadLump(int lump, void *dest) { return ReadLumpHeader(lump, dest, 0); };
  /// Shorthand for caching lumps by name.
  inline void *CacheLumpName(const char* name, int tag) { return CacheLumpNum(GetNumForName(name), tag); };
  
  void CacheArchiveFile(VFile* vf);
  Uint16 CacheArchiveFile_Remove();
  Uint8 CacheListIndex(void);
};


extern FileCache fc;


#endif
