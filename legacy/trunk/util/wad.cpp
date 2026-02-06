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

#include "doomdef.h"

#include "m_swap.h"
#include "parser.h"
#include "dehacked.h"

#include "wad.h"
#include "z_zone.h"


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
} __attribute__((packed));


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
} __attribute__((packed));


struct zip_local_header_t
{
  char   signature[4]; // 0x50, 0x4b, 0x03, 0x04
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
  // file name and other variable length info follows
} __attribute__((packed));


/// Runtime ZipFile directory entry.
struct zipdir_t
{
  unsigned int offset;          ///< offset of the data from the beginning of the file
  unsigned int compressed_size; ///< data lump size in file (compressed)
  unsigned int size;            ///< data lump size in memory (uncompressed)
  bool         deflated;        ///< either uncompressed or DEFLATE-compressed

#define ZIP_NAME_LENGTH 64
  char name[ZIP_NAME_LENGTH+1]; // name of the entry, NUL-terminated
};



ZipFile::ZipFile()
{
  directory = NULL;
}


ZipFile::~ZipFile()
{
  if (directory)
    Z_Free(directory);
}


bool ZipFile::Open(const char *fname)
{
  // common to all files
  if (!VDataFile::Open(fname))
    return false;

  // Find and read the central directory end.
  // We have to go through this because of the stupidly-placed ZIP comment field...
  int cd_end_pos = 0;
  {
    int max_csize = min(size, 0xFFFF); // max. comment size is 0xFFFF
    byte *buf = static_cast<byte*>(Z_Malloc(max_csize, PU_STATIC, NULL));

    fseek(stream, size - max_csize, SEEK_SET);
    fread(buf, max_csize, 1, stream);

    for (int k = max_csize - sizeof(zip_central_directory_end_t); k >= 0; k--)
      if (buf[k] == 'P' && buf[k+1] == 'K' && buf[k+2] == '\5' && buf[k+3] == '\6')
	{
	  cd_end_pos = size - max_csize + k;
	  break;
	}

    Z_Free(buf);
  }

  if (cd_end_pos == 0)
    {
      // could not find cd end
      CONS_Printf("Could not find central directory in ZIP file '%s'.\n", fname);
      return false;
    }

  zip_central_directory_end_t cd_end;
  fseek(stream, cd_end_pos, SEEK_SET);
  fread(&cd_end, sizeof(zip_central_directory_end_t), 1, stream);

  // ZIP files are little endian, but zeros are zeros...
  if (cd_end.disk_number != 0 ||
      cd_end.num_of_disk_with_cd_start != 0 ||
      cd_end.num_entries_on_this_disk != cd_end.total_num_entries)
    {
      // we do not support multi-file ZIPs
      CONS_Printf("ZIP file '%s' spans several disks, this is not supported.\n", fname);
      return false;
    }

  numitems = SHORT(cd_end.total_num_entries);
  int dir_size   = LONG(cd_end.cd_size);
  int dir_offset = LONG(cd_end.cd_offset);

  if (cd_end_pos < dir_offset + dir_size)
    {
      // corrupt file
      CONS_Printf("ZIP file '%s' is corrupted.\n", fname);
      return false;
    }

  // read central directory
  directory = static_cast<zipdir_t*>(Z_Malloc(numitems * sizeof(zipdir_t), PU_STATIC, NULL));

  byte *tempdir = static_cast<byte*>(Z_Malloc(dir_size, PU_STATIC, NULL));
  fseek(stream, dir_offset, SEEK_SET);
  fread(tempdir, dir_size, 1, stream);

  byte *p = tempdir;
  int item = 0; // since items may be ignored
  for (int k=0; k<numitems; k++)
    {
      zip_file_header_t *fh = reinterpret_cast<zip_file_header_t*>(p);
      if (fh->signature[0] != 'P' || fh->signature[1] != 'K' ||
	  fh->signature[2] != '\1' || fh->signature[3] != '\2')
	{
	  // corrupted directory
	  CONS_Printf("Central directory in ZIP file '%s' is corrupted.\n", fname);
	  return false;
	}

      // go to next record
      int n_size = SHORT(fh->filename_size);
      p += sizeof(zip_file_header_t) + n_size + SHORT(fh->extrafield_size) + SHORT(fh->comment_size);

      if (p > tempdir + dir_size)
	{
	  CONS_Printf("Central directory in ZIP file '%s' is too long.\n", fname);
	  break;
	}

      if (fh->filename[n_size-1] == '/')
	continue; // ignore directories

      // copy the lump name
      n_size = min(n_size, ZIP_NAME_LENGTH);
      strncpy(directory[item].name, fh->filename, n_size);
      directory[item].name[n_size] = '\0'; // NUL-termination

      int flags = SHORT(fh->flags);
      if (flags & 0x1)
	{
	  CONS_Printf("Lump '%s' in ZIP file '%s' is encrypted.\n", directory[item].name, fname);
	  continue;
	}

      if (flags & 0x8)
	{
	  CONS_Printf("Lump '%s' in ZIP file '%s' has a data descriptor (unsupported).\n", directory[item].name, fname);
	  continue;
	}

      if (fh->compression_method != 0 && SHORT(fh->compression_method) != Z_DEFLATED)
	{
	  CONS_Printf("Lump '%s' in ZIP file '%s' uses an unsupported compression algorithm.\n",
		      directory[item].name, fname);
	  continue;
	}

      if (fh->compression_method == 0 && fh->size != fh->compressed_size)
	{
	  CONS_Printf("Uncompressed lump '%s' in ZIP file '%s' has unequal compressed and uncompressed sizes.\n",
		      directory[item].name, fname);
	  continue;
	}

      // copy relevant fields to our directory
      directory[item].offset = LONG(fh->local_header_offset);
      directory[item].size   = LONG(fh->size);
      directory[item].compressed_size = LONG(fh->compressed_size);
      directory[item].deflated = fh->compression_method; // boolean

      // check if the local file header matches the central directory entry
      zip_local_header_t lh;
      fseek(stream, directory[item].offset, SEEK_SET);
      fread(&lh, sizeof(zip_local_header_t), 1, stream);

      if (lh.signature[0] != 'P' || lh.signature[1] != 'K' ||
	  lh.signature[2] != '\3' || lh.signature[3] != '\4')
	{
	  CONS_Printf("Could not find local header for lump '%s' in ZIP file '%s'.\n",
		      directory[item].name, fname);
	  continue;
	}

      if (lh.flags              != fh->flags ||
	  lh.compression_method != fh->compression_method ||
	  lh.compressed_size    != fh->compressed_size ||
	  lh.size               != fh->size)
	{
	  CONS_Printf("Local header for lump '%s' in ZIP file '%s' does not match the central directory.\n",
		      directory[item].name, fname);
	  continue;
	}

      // make offset point directly to the data
      directory[item].offset += sizeof(zip_local_header_t) + SHORT(lh.filename_size) + SHORT(lh.extrafield_size);

      // accepted
      imap.insert(pair<const char *, int>(directory[item].name, item)); // fill the name map
      item++;
    }
  // NOTE: If lumps are ignored, there will be a few empty records at the end of directory. Let them be.
  numitems = item;
  Z_Free(tempdir);

  // set up caching
  cache = (lumpcache_t *)Z_Malloc(numitems * sizeof(lumpcache_t), PU_STATIC, NULL);
  memset(cache, 0, numitems * sizeof(lumpcache_t));
    
  CONS_Printf(" Added ZIP file %s (%i lumps)\n", filename.c_str(), numitems);
  return true;
}


int ZipFile::GetItemSize(int i)
{
  return directory[i].size;
}


const char *ZipFile::GetItemName(int i)
{
  return directory[i].name;
}


void ZipFile::ListItems()
{
  zipdir_t *p = directory;
  for (int i = 0; i < numitems; i++, p++)
    printf("%-64s\n", p->name);
}


int ZipFile::Internal_ReadItem(int item, void *dest, unsigned size, unsigned offset)
{
  zipdir_t *l = directory + item;

  if (!l->deflated)
    {
      fseek(stream, l->offset + offset, SEEK_SET); // skip to correct offset within the uncompressed lump
      return fread(dest, 1, size, stream); // uncompressed lump
    }

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

  zs.zalloc = Z_NULL;
  zs.zfree = Z_NULL;
  zs.opaque = Z_NULL;
  zs.avail_in = 0;
  zs.next_in = Z_NULL;

  // TODO more informative error messages
  int ret = inflateInit2(&zs, -MAX_WBITS); // tell zlib not to expect any headers
  if (ret != Z_OK)
    I_Error("Fatal zlib error.\n");

  // decompress until deflate stream ends or we have enough data
  do
    {
      // get some input
      // NOTE: we may fread past the end of the lump, but that should not be harmful.
      zs.avail_in = fread(in, 1, chunksize, stream);
      if (ferror(stream) || zs.avail_in == 0)
	{
	  inflateEnd(&zs);
	  I_Error("Error decompressing a ZIP lump!\n");
	}

      zs.next_in = in;

      // decompress as much as possible
      ret = inflate(&zs, Z_SYNC_FLUSH);
    } while (ret == Z_OK && zs.avail_out != 0); // decompression can continue && not done yet

  inflateEnd(&zs);

  switch (ret)
    {
    case Z_STREAM_END:
      // ran out of input
      if (zs.avail_out != 0)
	I_Error("DEFLATE stream ended prematurely.\n");

      // fallthru
    case Z_OK:
      break;

    case Z_NEED_DICT:
    case Z_DATA_ERROR:
    case Z_STREAM_ERROR:
    case Z_MEM_ERROR:
    case Z_BUF_ERROR:
    default:
      I_Error("Error while decompressing a ZIP lump!\n");
      break;
    }
  // successful

  // now the lump is uncompressed and cached
  memcpy(dest, static_cast<byte*>(cache[item]) + offset, size);
  return size;
}
