// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: z_zone.cpp 504 2007-12-16 17:58:26Z smite-meister $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2007 by DooM Legacy Team.
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
/// \brief Zone Memory Allocation. Neat. Obsolete.

#include <vector>

#include "doomdef.h"
#include "z_zone.h"
#include "i_system.h"
#include "i_video.h"


using namespace std;

struct mem_extra_t
{
  int    size;
  int    tag;
  void **user;
};

static unsigned int mem_usage[PU_NUMTAGS];

void Z_Init()
{
  //CONS_Printf("Z_Init: Init zone memory allocation daemon.\n");
  for (int i=0; i<PU_NUMTAGS; i++)
    mem_usage[i] = 0;
}


unsigned Z_TagUsage(unsigned tagnum)
{
  if (tagnum >= PU_NUMTAGS)
    return 0;

  return mem_usage[tagnum];
}


int Z_GetTag(void *ptr)
{
  if (!ptr)
    {
      I_Error("Z_GetTag: NULL given!\n");
      return -1;
    }

  mem_extra_t *p = reinterpret_cast<mem_extra_t*>(static_cast<byte*>(ptr) - sizeof(mem_extra_t));
  return p->tag;
}


void *Z_Malloc(int size, int tag, void **user)
{
  mem_extra_t *p = static_cast<mem_extra_t*>(malloc(sizeof(mem_extra_t) + size));
  p->size = size;
  p->tag = tag;
  p->user = user;

  if (tag < PU_NUMTAGS)
    mem_usage[tag] += size;

  byte *ret = reinterpret_cast<byte*>(p) + sizeof(mem_extra_t);
  if (user)
    *user = ret;
  return ret;
}


void Z_Free(void *ptr)
{
  if (!ptr)
    {
      I_Error("Z_Free: NULL given!\n");
      return;
    }

  mem_extra_t *p = reinterpret_cast<mem_extra_t*>(static_cast<byte*>(ptr) - sizeof(mem_extra_t));

  if (p->tag < PU_NUMTAGS)
    mem_usage[p->tag] -= p->size;

  if (p->user)
    *p->user = NULL;
  free(p);
}


char *Z_Strdup(const char *s, int tag, void **user)
{
  return strcpy(static_cast<char*>(Z_Malloc(strlen(s)+1, tag, user)), s);
}


void Command_Meminfo_f()
{
  CONS_Printf("\2Memory Info\n");

  CONS_Printf("Unspecified:  %8d kB\n", Z_TagUsage(PU_STATIC) >> 10);
  CONS_Printf("Sounds:       %8d kB\n", Z_TagUsage(PU_SOUND) >> 10);
  CONS_Printf("Music:        %8d kB\n", Z_TagUsage(PU_MUSIC) >> 10);
  CONS_Printf("Textures:     %8d kB\n", Z_TagUsage(PU_TEXTURE) >> 10);
  CONS_Printf("Sprites:      %8d kB\n", Z_TagUsage(PU_SPRITE) >> 10);
  CONS_Printf("3D models:    %8d kB\n", Z_TagUsage(PU_MODEL) >> 10);
  CONS_Printf("\nMap data:     %8d kB\n", Z_TagUsage(PU_LEVEL) >> 10);
  CONS_Printf("Thinkers:     %8d kB\n", Z_TagUsage(PU_LEVSPEC) >> 10);
  CONS_Printf("Dave demands: %8d kB\n", Z_TagUsage(PU_DAVE) >> 10);
  //CONS_Printf("OpenGL stuff: %8d kB\n", Z_TagUsage(PU_OPENGL_GEOMETRY) >> 10);

  int total = 0;
  for (int i=0; i<PU_NUMTAGS; i++)
    total += mem_usage[i];
  CONS_Printf("\nTotal:        %8d kB\n", total >> 10);

  CONS_Printf("\2\nSystem Memory Info\n");
  Uint32 freebytes, totalbytes;
  freebytes = I_GetFreeMem(&totalbytes);
  CONS_Printf("Total     physical memory: %8d kB\n", totalbytes>>10);
  CONS_Printf("Available physical memory: %8d kB\n", freebytes>>10);
}



#if 0 // the rest is obsolete


// Use malloc instead of zone memory to detect leaks. This way we always get a SIGSEGV.
#define MEMDEBUG 1

// =========================================================================
//                        ZONE MEMORY ALLOCATION
// =========================================================================
//
// There is never any space between memblocks,
//  and there will never be two contiguous free memblocks.
// 'base' can be left pointing at a non-empty block.
//
// It is of no value to free a cachable block,
//  because it will get overwritten automatically if needed.
//

#define ZONEID  0x1d4a11

// New memory allocation system, by Jussi Pakkanen, written into a class by smite-meister
//
// Instead of one zone, we have several zones, which are stored in a vector.
// memzone_t methods work just like they do in Doom and the
// Z_* versions work as wrappers.
//
// So functions such as Z_Malloc find the appropriate zone to use and
// call memzone_t::Malloc, which does the actual work.
//
// class memzone_t definition is here, because it is not meant to be used directly.

struct memblock_t
{
  int    size;   // including the header and possibly tiny fragments
  void **user;   // NULL if a free block
  int    tag;    // purgelevel
  int    id;     // should be ZONEID
  memblock_t *next;
  memblock_t *prev;

#ifdef ZDEBUG
  char     *ownerfile;
  int       ownerline;
#endif
};


class memzone_t
{
  friend void Z_Free(void *ptr);

public:
  static int mb_used;
  static int mb_increment; // At least how large should the second, third etc. blocks be.

private:
  // total bytes malloced, including header
  int size;

  // start / end cap for linked list
  memblock_t  blocklist;
  memblock_t *base;
public:

  static memzone_t *NewZone(int size); // allocates a new memzone of size 'size' and activates it

  void  Clear(); // clears and initializes the zone
  void *Malloc(int size, int tag, void **user, int alignbits); // allocate zone memory
  void  Free(void* ptr); // free zone memory
  void  FreeTags(int lowtag, int hightag); // frees tags between lowtag and hightag within this zone

  int  TagUsage(int tagnum);  // returns the number of bytes currently allocated in the zone for the given tag

  // Calculates the amount of free, purgable and used memory in the zone. Returns also the largest consecutive
  // memory block available.
  void FreeMemory(int *realfree, int *cachemem, int *usedmem, int *largestfreeblock);

  // analysis and debugging
  void CheckHeap(int i); // makes sure that the zone block structure is valid. 'i' means apparently nothing(!)
  void DumpHeap(int lowtag, int hightag); // like CheckHeap but for a range of tags and results are shown on console.
  void FileDumpHeap(FILE* f); // arg. Like CheckHeap, but results are written if file 'f'
};

static vector<memzone_t *> zones; // The new zones are stored here.

int memzone_t::mb_used = 0;
int memzone_t::mb_increment = 6; // This value is a guesstimate.


//-------------------------------------------
// Z_Init
//
void Z_Init()
{
  CONS_Printf("Z_Init: Init zone memory allocation daemon.\n");

#ifndef MEMDEBUG
  Uint32 free, total;
  int mb_alloc = memzone_t::mb_increment;

  if (M_CheckParm("-mb"))
    {
      if (M_IsNextParm())
        mb_alloc = atoi(M_GetNextParm());
      else
        I_Error("usage : -mb <number of megabytes for the heap>");
    }
  else
    {
      free = I_GetFreeMem(&total) >> 20;
      total >>= 20; // Megabytes
      CONS_Printf("System memory %d MB free %d MB\n", total, free);
      // we assume that system uses a lot of memory for disk cache
      if (free < 6)
        free = total / 2;

      if (mb_alloc < free)
        mb_alloc = free;
      if (mb_alloc > 20)
        mb_alloc = 20; // min 6 MB max 20 MB
    }

  memzone_t *newzone = memzone_t::NewZone(mb_alloc << 20);
  zones.push_back(newzone);
#endif
}

//---------------------------------------------
// NewZone creates a new memory zone
//
memzone_t *memzone_t::NewZone(int size)
{
  int size_mb = size >> 20;
  CONS_Printf("%d MB requested for a new memory zone.\n", size_mb);

  memzone_t *newzone = (memzone_t *)calloc(1, size);
  if (!newzone)
    I_Error("Could not allocate %d megabytes.\n", size_mb);

  mb_used += size_mb;

  // if (M_checkParm("-lock"))
  //   I_LockMemory(newzone);

  newzone->size = size;
  newzone->Clear();

  return newzone;
}


//---------------------------------------------
// was Z_ClearZone
//
// Clears and initializes a memory zone. zone->size must already be set.
void memzone_t::Clear()
{
  memblock_t *block = (memblock_t *)((byte *)this + sizeof(memzone_t));

  // set the entire zone to one free block
  // block is the only free block in the zone
  blocklist.next = blocklist.prev = block;
  blocklist.size = sizeof(memzone_t);
  blocklist.user = (void **)this; // just a marker
  blocklist.tag = PU_STATIC;
  blocklist.id = 0;
  base = block;

  block->prev = block->next = &blocklist;
  block->size = size - sizeof(memzone_t);
  // NULL indicates a free block.
  block->user = NULL;
}



//---------------------------------------------
// Z_Free
//
#ifdef ZDEBUG
void Z_Free2(void *ptr, char *file, int line)
#else
void Z_Free(void *ptr)
#endif
{
  // this function is somewhat redundant.
  // We could do without if we stored the proper memzone_t* into each block header
  // and made memzone_t::Free again Z_Free
  if (ptr == NULL)
    {
      //CONS_Printf("Z_Free: NULL given!\n");
      I_Error("Z_Free: NULL given!\n");
      return;
    }

#ifdef MEMDEBUG
  memblock_t *block = (memblock_t *)((byte *)ptr - sizeof(memblock_t));

  if (block->user > (void **)0x100)
    *block->user = NULL;
  free(block);
  return;
#endif

#ifdef ZDEBUG
  // SoM: HARDERCORE debuging
  // Write all Z_Free's to a debug file
  if (debugfile)
    fprintf(debugfile, "ZFREE@File: %s, line: %i\n", file, line);
  //BP: hardcore debuging
  // check if there is not a user in this zone
  for (other = mainzone->blocklist.next; other->next != &mainzone->blocklist; other = other->next)
    {
      if ((other != block) && (other->user > (void **)0x100) &&
          (other->user >= (void **)block) &&
          (other->user <= (void **)((byte *)block) + block->size))
        {
          //I_Error("Z_Free: Pointer in zone\n");
          I_Error("Z_Free: Pointer %s:%d in zone at %s:%i",other->ownerfile,other->ownerline,file,line);
        }
    }
#endif

  // Find the correct block in which to do the free'ing.
  for (unsigned int i=0; i < zones.size(); i++)
    if (ptr >= zones[i] && ptr <= zones[i] + zones[i]->size)
      {
        zones[i]->Free(ptr);
        return;
      }
  I_Error("Attempt to free zone memory outside the allocated area.\n");
}

void memzone_t::Free(void* ptr)
{
  memblock_t *block = (memblock_t *)((byte *)ptr - sizeof(memblock_t));

  if (block->id != ZONEID)
    I_Error("Z_Free: freed a pointer without ZONEID");

#ifdef PARANOIA
  // get direct a segv when using a pointer that isn't right FIXME is this necessary?
  //memset(ptr, 0, block->size - sizeof(memblock_t));
#endif

  if (block->user > (void **)0x100)
    {
      // smaller values are not pointers
      // Note: OS-dependend?

      // clear the user's pointer
      *block->user = NULL;
    }

  // mark as free
  block->user = NULL;
  block->tag = 0;
  block->id = 0;

  memblock_t *other = block->prev;
  if (!other->user)
    {
      // merge with previous free block
      other->size += block->size;
      other->next = block->next;
      other->next->prev = other;

      if (block == base)
        base = other;

      block = other;
    }

  other = block->next;
  if (!other->user)
    {
      // merge the next free block onto the end
      block->size += other->size;
      block->next = other->next;
      block->next->prev = block;

      if (other == base)
        base = block;
    }
}



//---------------------------------------------
// Z_Malloc
// You can pass a NULL user if the tag is < PU_PURGELEVEL.
//
#define MINFRAGMENT int(sizeof(memblock_t))

#ifdef ZDEBUG
void *Z_Malloc2 (int size, int tag, void **user, int alignbits, char *file, int line)
#else
void *Z_MallocAlign(int size, int tag, void **user, int alignbits)
#endif
{
#ifdef MEMDEBUG
  size += sizeof(memblock_t);
  memblock_t *temp = (memblock_t *)malloc(size);
  temp->size = size;
  temp->id = ZONEID;
  temp->tag = tag;
  temp->user = user;
  byte *ret = reinterpret_cast<byte*>(temp) + sizeof(memblock_t);
  if (user)
    *user = ret;
  return ret;
#endif

  void *allocation;
  for (unsigned int i=0; i < zones.size(); i++)
    {
      allocation = zones[i]->Malloc(size, tag, user, alignbits);
      if (allocation != NULL)
        {
          //memset(allocation, 0, size); // FIXME TEST. Crashes if bad return value.
          return allocation;
        }
    }

  // No free block was found. Allocate a new zone from the system and
  // allocate user's request from there.
  if ((size >> 20) > memzone_t::mb_increment)
    zones.push_back(memzone_t::NewZone(size + 1<<20));
  else
    zones.push_back(memzone_t::NewZone(memzone_t::mb_increment << 20));

  allocation = zones.back()->Malloc(size, tag, user, alignbits);
  if (allocation == NULL)
    I_Error("Memory allocation function serious failure.\n");

  //memset(allocation, 0, size); // FIXME TEST. Crashes if bad return value.
  return allocation;
}


// Searches the given memory zone for free space. Then gives a
// pointer to the memory. Returns NULL on failure.
void* memzone_t::Malloc(int size, int tag, void **user, int alignbits)
{
#ifdef MEMDEBUG
  return NULL;
#else
  Uint32 alignmask = (1 << alignbits) - 1;

#define ALIGN(a) (((Uint32)(a)+alignmask) & ~alignmask)

  size = (size + 3) & ~3; // align size to next multiple of 4
  size += sizeof(memblock_t); // account for size of block header


  // if there is a free block behind the base, back up to it
  // added comment : base is used to point at the begin of a region in case
  //                 when there is two (or more) adjacent purgable block
  if (!base->prev->user)
    base = base->prev;

  // scan through the block list,
  // looking for the first free block
  // of sufficient size,
  // throwing out any purgable blocks along the way.

  memblock_t *rover = base;
  memblock_t *end = base->prev;
  int basedata;

  do {
    if (rover == end)
      {
        // There was not enough memory. Return NULL to signal
        // wrapper function to search next block or allocate more
        // memory.
        return NULL;
      }

    if (rover->user)
      {
        if (rover->tag < PU_PURGELEVEL)
          {
            // hit a block that can't be purged,
            //  so move base past it
            base = rover = rover->next;
          }
        else
          {
            // free the rover block (adding the size to base)
            // TODO: block are freed even though is is not yet certain that they _will_ be reused
            //  (it migh be that the purged area is still too small). Perhaps check first, purge later?

            // the rover can be the base block
            base = base->prev;
            Z_Free ((byte *)rover+sizeof(memblock_t));
            base = base->next;
            rover = base->next;
          }
      }
    else
      rover = rover->next;

    basedata = ALIGN((Uint32)base + sizeof(memblock_t));
  } while (base->user || (Uint32)base + base->size < basedata + size - sizeof(memblock_t));

  // aligning can leave free space in current block so make it really free
  if (alignbits)
    {
      memblock_t *newbase = (memblock_t *)basedata - 1;
      int sizediff = (byte*)newbase - (byte*)base;

      if (sizediff > MINFRAGMENT)
        {
          newbase->prev = base;
          newbase->next = base->next;
          newbase->next->prev = newbase;

          newbase->size = base->size - sizediff;
          base->next = newbase;
          base->size = sizediff;
        }
      else
        {
          // adjust size of previous block if adjacent (not cycling)
          if (base->prev < base)
            base->prev->size += sizediff;
          base->prev->next = newbase;
          base->next->prev = newbase;
          base->size -= sizediff;
          memmove(newbase, base, sizeof(memblock_t));
        }
      base = newbase;
    }

  // found a block big enough
  int extra = base->size - size;

  if (extra > MINFRAGMENT)
    {
      // there will be a free fragment after the allocated block
      memblock_t *newblock = (memblock_t *) ((byte *)base + size);
      newblock->size = extra;

      // NULL indicates free block.
      newblock->user = NULL;
      newblock->tag = 0;
      newblock->id = 0;
      newblock->prev = base;
      newblock->next = base->next;
      newblock->next->prev = newblock;

      base->next = newblock;
      base->size = size;
    }

  if (user)
    {
      // mark as an in use block
      base->user = user;
      *user = (void *)((byte *)base + sizeof(memblock_t));
    }
  else
    {
      if (tag >= PU_PURGELEVEL)
        I_Error("Z_Malloc: an owner is required for purgable blocks");

      // mark as in use, but unowned
      base->user = (void **)2;
    }

  base->tag = tag;
  base->id = ZONEID;

#ifdef ZDEBUG
  base->ownerfile = file;
  base->ownerline = line;
#endif

  rover = base;

  // next allocation will start looking here
  base = base->next;

  return (void *)((byte *)rover + sizeof(memblock_t));
#endif
}



//----------------------------------------------------------
// Z_FreeTags
//
void Z_FreeTags(int lowtag, int hightag)
{
#ifdef MEMDEBUG
  return;
#endif

  for (unsigned int i=0; i < zones.size(); i++)
    zones[i]->FreeTags(lowtag, hightag);
}

void memzone_t::FreeTags(int lowtag, int hightag)
{
  memblock_t *block, *next;

  for (block = blocklist.next; block != &blocklist; block = next)
    {
      // get link before freeing
      next = block->next;

      // free block?
      if (!block->user)
        continue;

      if (block->tag >= lowtag && block->tag <= hightag)
        Free((byte *)block + sizeof(memblock_t));
    }
}



//----------------------------------------------------------
// Z_DumpHeap
// Note: Z_DumpHeap(stream) could do both this and FileDumpHeap?
//  console should be rewritten as a stream?
void Z_DumpHeap(int lowtag, int hightag)
{
#ifdef MEMDEBUG
  return;
#endif

  for (unsigned int i=0; i < zones.size(); i++)
    {
      CONS_Printf("Z_DumpHeap info for zone %d/%d.\n\n", i, zones.size());
      zones[i]->DumpHeap(lowtag, hightag);
    }
}

void memzone_t::DumpHeap(int lowtag, int hightag)
{
  memblock_t* block;

  CONS_Printf("zone size: %i bytes, location: %p\n", size, this);
  CONS_Printf("tag range: %i to %i\n", lowtag, hightag);

  int n = 0;
  for (block = blocklist.next; ; block = block->next)
    {
      if ((block->tag < lowtag || block->tag > hightag) &&
          (block->next != &blocklist))
        continue;

      CONS_Printf("block:%p    size:%7i    user:%p    tag:%3i prev:%p next:%p\n",
                  block, block->size, block->user, block->tag, block->prev, block->next);

      if (block->next->prev != block)
        CONS_Printf("ERROR: next block doesn't have a proper back link\n");

      if ((block->user > (void **)0x100) &&
          (*(block->user) != (byte *)block + sizeof(memblock_t)))
        CONS_Printf("ERROR: block doesn't have a proper user\n");

      if (block->next == &blocklist)
        break; // all blocks have been hit

      n++;

      if ((byte *)block + block->size != (byte *)block->next)
        CONS_Printf("ERROR: block size does not touch the next block\n");

      if (!block->user && !block->next->user)
        CONS_Printf("ERROR: two consecutive free blocks\n");
    }

  CONS_Printf("\nTotal : %d blocks\n"
              "========================================================\n\n", n);
}

//
// Z_FileDumpHeap
//
void Z_FileDumpHeap(FILE *f)
{
#ifdef MEMDEBUG
  return;
#endif

  for (unsigned int i=0; i < zones.size(); i++)
    {
      fprintf(f, "Z_FileDumpHeap info for zone %d/%ld.\n", i, zones.size());
      zones[i]->FileDumpHeap(f);
    }
}

void memzone_t::FileDumpHeap(FILE *f)
{
  int i = 0;

  fprintf(f, "zone size: %i     location: %p\n", size, this);

  for (memblock_t *block = blocklist.next ; ; block = block->next)
    {
      i++;
      fprintf (f,"block:%p size:%7i user:%p tag:%3i prev:%p next:%p id:%7i\n",
               block, block->size, block->user, block->tag, block->prev, block->next, block->id);

      if (block->next == &blocklist)
        {
          // all blocks have been hit
          break;
        }

      if ((block->user > (void **)0x100) &&
	  (*(block->user) != reinterpret_cast<byte*>(block) + sizeof(memblock_t)))
        fprintf (f,"ERROR: block don't have a proper user\n");

      if ( (byte *)block + block->size != (byte *)block->next)
        fprintf (f,"ERROR: block size does not touch the next block\n");

      if ( block->next->prev != block)
        fprintf (f,"ERROR: next block doesn't have proper back link\n");

      if (!block->user && !block->next->user)
        fprintf (f,"ERROR: two consecutive free blocks\n");
    }
  fprintf (f,"Total : %d blocks\n"
           "===============================================================================\n\n",i);
}



//----------------------------------------------------------
// Z_CheckHeap
//
void Z_CheckHeap(int mark)
{
#ifdef MEMDEBUG
  return;
#endif

  for (unsigned int i=0; i < zones.size(); i++)
    zones[i]->CheckHeap(mark);
}

void memzone_t::CheckHeap(int i)
{
  for (memblock_t *block = blocklist.next; ; block = block->next)
    {
      if ((block->user > (void **)0x100) &&
          (*(block->user) != (byte *)block + sizeof(memblock_t)))
        I_Error("Z_CheckHeap: block don't have a proper user %d\n", i);

      if (block->next->prev != block)
        I_Error("Z_CheckHeap: next block doesn't have proper back link %d\n", i);

      if (block->next == &blocklist)
        break;

      if ((byte *)block + block->size != (byte *)block->next)
        I_Error("Z_CheckHeap: block size does not touch the next block %d\n", i);

      if (!block->user && !block->next->user)
        I_Error("Z_CheckHeap: two consecutive free blocks %d\n", i);
    }
}



//----------------------------------------------------------
// Z_ChangeTag
//
void Z_ChangeTag(void *ptr, int tag)
{
#ifdef MEMDEBUG
  // no point in changing tag because it is never used
  return;
#endif

  memblock_t *block = (memblock_t *)((byte *)ptr - sizeof(memblock_t));

  if (block->id != ZONEID)
    I_Error("Z_ChangeTag: pointer without ZONEID");

  if (tag >= PU_PURGELEVEL && block->user < (void **)0x100)
    I_Error("Z_ChangeTag: an owner is required for purgable blocks");

  block->tag = tag;
}



//----------------------------------------------------------
// Z_FreeMemory
//
void Z_FreeMemory(int *realfree,int *cachemem,int *usedmem,int *largestfreeblock)
{
  *largestfreeblock = 0;
  *usedmem = 0;
  *cachemem = 0;
  *realfree = 0;

#ifdef MEMDEBUG
  return;
#endif

  int tmpfree, tmpcache, tmpused, tmplargest;
  for (unsigned int i=0; i < zones.size(); i++)
    {
      zones[i]->FreeMemory(&tmpfree, &tmpcache, &tmpused, &tmplargest);
      *realfree += tmpfree;
      *cachemem += tmpcache;
      *usedmem += tmpused;
      if (tmplargest > *largestfreeblock)
        *largestfreeblock = tmplargest;
    }
}

void memzone_t::FreeMemory(int *realfree, int *cachemem, int *usedmem, int *largestfreeblock)
{
  memblock_t *block;
  int freeblock = 0;

  *realfree = 0;
  *cachemem = 0;
  *usedmem  = 0;
  *largestfreeblock = 0;

  for (block = blocklist.next; block != &blocklist; block = block->next)
    {
      if (block->user == NULL)
        {
          // free memory
          *realfree += block->size;
          freeblock += block->size;
        }
      else if (block->tag >= PU_PURGELEVEL)
        {
          // purgable memory (cache)
          *cachemem += block->size;
          freeblock += block->size;
        }
      else
        {
          // used block
          *usedmem += block->size;

          if (freeblock > *largestfreeblock)
            *largestfreeblock = freeblock;
          freeblock = 0;
        }
    }
}


//----------------------------------------------------------
// Z_TagUsage
// - return number of bytes currently allocated in the heap for the given tag
int Z_TagUsage(int tagnum)
{
#ifdef MEMDEBUG
  return 0;
#endif

  int total = 0;
  for (unsigned int i=0; i < zones.size(); i++)
    total += zones[i]->TagUsage(tagnum);
  return total;
}

int memzone_t::TagUsage(int tagnum)
{
  memblock_t *block;
  int bytes = 0;

  for (block = blocklist.next; block != &blocklist; block = block->next)
    {
      if (block->user != NULL && block->tag == tagnum)
        bytes += block->size;
    }

  return bytes;
}


//----------------------------------------------------------
//
void Command_Meminfo_f()
{
  int   free, cache, used, largefreeblock;
  Uint32 freebytes, totalbytes;

  Z_CheckHeap(-1);
  Z_FreeMemory(&free, &cache, &used, &largefreeblock);

  CONS_Printf("\2Zone Memory Info\n");
  CONS_Printf("# of memory zones  : %7d\n", zones.size());
  CONS_Printf("Total heap size    : %7d kB\n", memzone_t::mb_used<<10);
  CONS_Printf("used  memory       : %7d kB\n", used>>10);
  CONS_Printf("free  memory       : %7d kB\n", free>>10);
  CONS_Printf("purgable memory    : %7d kB\n", cache>>10);
  CONS_Printf("largest free block : %7d kB\n", largefreeblock>>10);

  // FIXME: Hurdler: this is probably not accurate with the new renderer
  if (rendermode != render_soft)
    {
      CONS_Printf("Patch info headers : %7d kB\n", Z_TagUsage(PU_HWRPATCHINFO)>>10);
      CONS_Printf("HW Texture cache   : %7d kB\n", Z_TagUsage(PU_HWRCACHE)>>10);
      CONS_Printf("Plane polygon      : %7d kB\n", Z_TagUsage(PU_HWRPLANE)>>10);
      //CONS_Printf("HW Texture used    : %7d kB\n", HWR.GetTextureUsed()>>10);
    }

  CONS_Printf("\2System Memory Info\n");
  freebytes = I_GetFreeMem(&totalbytes);
  CONS_Printf("Total     physical memory: %6d kB\n", totalbytes>>10);
  CONS_Printf("Available physical memory: %6d kB\n", freebytes>>10);
}

#endif
