// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: w_wad.cpp 504 2007-12-16 17:58:26Z smite-meister $
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
/// \brief FileCache: Container of VFiles

#include <string>
#include <vector>
#include <algorithm>    // für std::transform
#include <cctype>       // für std::tolower

#ifndef __WIN32__
# include <unistd.h>
#endif

#include "doomdef.h"

#include "i_system.h"
#include "i_video.h"

#include "vfile.h" // VFile class
#include "wad.h"
#include "w_wad.h" // FileCache Class
#include "z_zone.h"
#include "m_misc.h"


//====================================================================
// FileCache class implementation
//
// this replaces all W_* functions of old Doom code. W_* -> fc.*
//====================================================================

FileCache fc;


// destroys (closes) all open VFiles
// If not done on a Mac then open wad files
// can prevent removable media they are on from
// being ejected
FileCache::~FileCache()
{
  for (int i = vfiles.size()-1; i>=0; i--)
    {
      delete vfiles[i];
    }
  vfiles.clear();
  return;
}



//======================
// opening wad files
//======================

void FileCache::SetPath(const char *p)
{
  datapath = p;
}


// Not like libc access(). If file exists, returns the path+filename where it was found, otherwise NULL.
const char *FileCache::Access(const char *f)
{ 
  if (!access(f, F_OK))
  {
    //CONS_Printf(" File Added:[Loose]  %s\n", f);
    return f; // first try the current dir
  }


  // Extrahiere NUR den Dateinamen (ohne Pfad)
  const char *filename_only = strrchr(f, '\\');
  if (!filename_only)
      filename_only = strrchr(f, '\\');
  if (!filename_only)
      filename_only = f;  // kein Pfad vorhanden
  else
      filename_only++;    // nach dem letzten \ oder /
      
  //// needs to be static so that our return value stays valid      
  static string p;      
  static string n;
  
  n.assign(datapath);
  n.push_back('\\'); 
  n.append(filename_only);   // ← nur Dateiname! //n.append(f);
  
  p.assign(ProgrammPath());
  p.push_back('\\');
  p.append(filename_only);   // ← nur Dateiname!  

  if (!access(n.c_str(), F_OK))
  {
    //CONS_Printf(" File Added:[DataP] %s\n", n.c_str());
    return n.c_str();
  }
  else if (!access(p.c_str(), F_OK))
  {
    //CONS_Printf(" File Added:[ProgP] %s\n", p.c_str());      
    return p.c_str();
  }

  return NULL;
}

bool GWA_Skip_Files(const char* filename)
{
    if (!filename) return true;

    string name = filename;
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    // Typische WADs ohne Maps / ohne GL-Nodes-Bedarf
    if (name.find("legacy.wad") != string::npos)
    {
        // Da können noch mehr verankert werden mit ||
        return true;  // Überspringen
    }

    return false;
}

// Pass a null terminated list of files to use.
// All files are optional, but at least one file must be found.
// The name searcher looks backwards, so a later file does override all earlier ones.
// Also adds GWA files if they exist.
/*
bool FileCache::InitMultipleFiles(const char *const*filenames)
{
  CONS_Printf("Loading resource files...\n");
  bool result = true;

  for ( ; *filenames != NULL; filenames++)
  {
      const char *curfile = *filenames;
      if (AddFile(curfile) == -1)
        result = false;

        // try finding corresponding GWA file (GL-nodes data)
        string gwafile(curfile);
        // Try lower case.
        gwafile.replace(gwafile.length()-3, 3, "gwa");
        if (AddFile(gwafile.c_str(), true) == -1)
        {
          // not found, try upper case
          gwafile.replace(gwafile.length()-3, 3, "GWA");
          if (AddFile(gwafile.c_str(), true) == -1)
          {
            // CONS_Printf(" No GL information for file %s.\n", curfile);
            continue; // not found
          }
        }

      CONS_Printf(" Added GL information from file %s.\n", gwafile.c_str());
    }

  if (vfiles.size() == 0)
    I_Error("FileCache::InitMultipleFiles: no files found");

  // result = false : at least one file was missing
  return result;
}
*/
/*
 * w_wad.cpp: In member function 'bool FileCache::InitMultipleFiles(const char* const*)':
 * w_wad.cpp:105:7: warning: this 'if' clause does not guard... [-Wmisleading-indentation]
 * if (AddFile(curfile) == -1)
 *  ^~
 * w_wad.cpp:109:9: note: ...this statement, but the latter is misleadingly indented as
 * if it were guarded by the 'if'
 *  string gwafile(curfile);
 *  ^~~~~~
 *
 * Konsequenz:
 * Selbst wenn AddFile(curfile) fehlschlägt (z. B. WAD nicht gefunden) → wird trotzdem
 * versucht, eine .gwa-Datei zu laden.
 * Das kann zu unnötigen Fehlermeldungen führen oder sogar zu falschen GL-Nodes, wenn
 * eine GWA zu einer nicht-geladenen WAD gefunden wird.
 */
 bool FileCache::InitMultipleFiles(const char *const* filenames)
{
  CONS_Printf("Loading resource files...\n");
  bool result = true;

  string extensions[] = {"gwa", "GWA", "Gwa", "gWa", "GwA", "gWA", "GWa", "GwA"};

  for ( ; *filenames != NULL; filenames++)
  {
    const char *curfile = *filenames;

    // NEU: Prüfen, ob die Datei schon intern geladen ist
    bool already_loaded = false;
    for (auto vf : vfiles)
    {
      // Vergleich case-insensitive (Dateinamen)
      if (stricmp(vf->filename.c_str(), curfile) == 0)
      {
        //CONS_Printf("  %s already loaded internally (ZIP/Memory) – skip disk check\n", curfile);
        already_loaded = true;
        break;
      }
    }

    if (!already_loaded)
    {
      // Nur physisch laden, wenn nicht schon da
      if (AddFile(curfile) == -1)
      {
        result = false;
        CONS_Printf("  Failed to add file: %s\n", curfile);
      }
    }

    // GWA-Prüfung nur bei physischen Dateien (oder wenn nötig anpassen)
    if (rendermode == 1) // OpenGL
    {
      if (GWA_Skip_Files(curfile))
        continue;

      string gwafile(curfile);
      string base = gwafile.substr(0, gwafile.length() - 3);
      bool gwa_found = false;

      for (const string& ext : extensions)
      {
        string candidate = base + ext;
        if (AddFile(candidate.c_str(), true) != -1)
        {
          CONS_Printf(" - Added GL information from file %s.\n", candidate.c_str());
          gwa_found = true;
          break;
        }
      }
      if (!gwa_found)
        CONS_Printf(" - GWA Node File not found for: \"%s\"\n", curfile);
    }
  }

  // Entferne den alten Check – er ist zu streng
  // if (vfiles.size() == 0)
  //   I_Error("FileCache::InitMultipleFiles: no files found");

  CONS_Printf("InitMultipleFiles finished – %zu files in cache\n", vfiles.size());
  return result;
}

// Loads a wad file into memory, adds it into vfiles vector.
// returns the number of the wadfile (0 is the first one)
// or -1 in the case of error
// Files with a .wad extension are idlink files with multiple lumps.
// Other files are single lumps with the base filename for the lump name.
// Lump names can appear multiple times.
int FileCache::AddFile(const char *fname, bool silent)
{
  int nfiles = vfiles.size();
  
  if (nfiles >= MAX_WADFILES)
    {
      CONS_Printf(" FileCache: Maximum number of resource files reached\n");
      return -1;
    }

  const char *name = Access(fname);

  // Doppel-Öffnen verhindern
  for (size_t i = 0; i < vfiles.size(); i++)
  {
    if (vfiles[i]->filename == fname)
    {
      CONS_Printf(" FileCache: Datei schon geladen: %s – überspringe\n", fname);
      return i;
    }
  }
  
  if (!name && !silent)
    {
      CONS_Printf(" FileCache: AddFile> Can't access file %s (path %s)\n", fname, datapath.c_str());
      return -1;
    }

  VFile *vf = NULL;
  int len = strlen(fname);
  bool ok = false;

  if (len >= 4)
  {
      if (!strcasecmp(&fname[len - 4], ".deh"))
      {
        // detect dehacked file with the "deh" extension
        vf = new Wad();
        ok = vf->Create(name, "DEHACKED");
        goto done;
      }
      else if (!strcasecmp(&fname[len - 4], ".hhe"))
      {
        // detect dehacked file with the "deh" extension
        vf = new Wad();
        ok = vf->Create(name, "DEHACKED");
        goto done;
      }      
      else if (!strcasecmp(&fname[len - 4], ".lmp"))
      {
        // a single lump file
        const char *lname = FIL_StripPath(fname);
        char lumpname[9];
        strncpy(lumpname, lname, 8);
        lumpname[min(int(strlen(lname))-4, 8)] = '\0';
        vf = new Wad();
        ok = vf->Create(name, lumpname);
        goto done;
      }
  }
  
  if (fname[len-1] == '/')
  {
      // directory
      vf = new VDir();
      ok = vf->Open(name);
  }
  else
  {
      union
      {
        char magic[5];
        int  imagic;
      };
      
      // open the file to read the magic number
      FILE *str = fopen(name, "rb");
      if (!str)
        return -1;

          fread(magic, 4, 1, str);
          magic[4] = 0;
          
      // check file type
      if (imagic == *reinterpret_cast<const int *>("IWAD")||
          imagic == *reinterpret_cast<const int *>("PWAD"))
      {
        vf = new Wad();
      }
      
      else if (imagic == *reinterpret_cast<const int *>("WAD2") ||
               imagic == *reinterpret_cast<const int *>("WAD3"))
      {
        vf = new Wad3();
      }
      
      else if (imagic == *reinterpret_cast<const int *>("PACK"))
      {
        vf = new Pak();
      }
      
      //else if (imagic == *reinterpret_cast<const int *>("PK\3\4"))
      /* Marty */
      else if (imagic == *reinterpret_cast<const int *>("PK\3\4") ||
              (len >= 4 && (!strcasecmp(&fname[len-4], ".zip")    ||
              !strcasecmp(&fname[len-4], ".pk3"))))    
      {
        vf = new ZipFile(); // ZIP/PK3 file
        if (ok)// ok kommt von vf->Open(name); weiter oben              
            CONS_Printf(" ZIP/PK3 geladen: %s (%d Lumps)\n", fname, vf->numitems);       
      }
      
      else
      {
        CONS_Printf(" FileCache: AddFile> Unknown file signature '%4c'\n", magic);
        fclose(str);
        return -1;
      }
      
      fclose(str);
      ok = vf->Open(name);
    }

 done:

  if (ok)
  {
    vfiles.push_back(vf); 
    nfiles = CacheArchiveFile_Remove();
    //CacheListIndex();    
    return nfiles;
  }

  delete vf;
  return -1;
}


//=============================
// basic info on open wadfiles
//=============================


const char *FileCache::Name(int i)
{
  return vfiles[i]->filename.c_str();
}


unsigned int FileCache::GetNumLumps(int filenum)
{
  return vfiles[filenum]->numitems;
}



//==============
//  searching 
//==============


// Returns -1 if name not found.
// scanforward: this is normally always false, so external pwads take precedence.
int FileCache::FindNumForName(const char* name, bool scanforward)
{
  int i, n = vfiles.size();
  int res;

  if (!scanforward)
    {
      // scan wad files backwards so patch lump files take precedence
      for (i = n-1; i >= 0; i--)
	{
	  res = vfiles[i]->FindNumForName(name);
	  // high word is the wad file number, low word the lump number
	  if (res != -1)
	    return ((i<<16) + res);
	}
    }
  else
    {  
      // scan wad files forward, when original wad resources
      //  must take precedence
      for (i = 0; i<n; i++)
	{
	  res = vfiles[i]->FindNumForName(name);
	  if (res != -1)
	    return ((i<<16) + res);
	}
    }
  // not found.
  return -1;
}


// Calls FindNumForName, but bombs out if not found.
int FileCache::GetNumForName(const char *name, bool scanforward)
{
  int i = FindNumForName(name, scanforward);
  if (i == -1)
    I_Error("FileCache::GetNumForName: %s not found!\n", name);
  
  return i;
}


// Same as the original, but checks in one wad only
// 'filenum' is a wad number (Used for sprites loading)
// 'startlump' is the lump number to start the search
int FileCache::FindNumForNameFile(const char* name, unsigned filenum, int startlump)
{
  // start at 'startlump', useful parameter when there are multiple
  // resources with the same name

  if (filenum >= vfiles.size())
    I_Error("FileCache::FindNumForNamePwad: %i >= numvfiles(%i)\n", filenum, vfiles.size());
 
  int res = vfiles[filenum]->FindNumForName(name, startlump);

  if (res != -1) return ((filenum<<16) + res);
  
  // not found.
  return -1;
}


// Return the first lump in a file whose name starts with the given four bytes, also returns the full name of the lump.
int FileCache::FindPartialName(Uint32 iname, unsigned filenum, int startlump, const char **fullname)
{
  if (filenum >= vfiles.size())
    I_Error("FileCache::FindNumForNamePwad: %i >= numvfiles(%i)\n", filenum, vfiles.size());
 
  return vfiles[filenum]->FindPartialName(iname, startlump, fullname);
}


//====================
// retrieve lump info
//====================

const char *FileCache::FindNameForNum(int lump)
{
  unsigned int file = lump >> 16;
  int item = lump & 0xffff;

  if (file >= vfiles.size())
    I_Error("FileCache::FindNameForNum: %i >= numvfiles(%i)\n", file, vfiles.size());
  if (item >= vfiles[file]->numitems)
    //I_Error("FileCache::FindNameForNum: %i >= numitems", item);
    return NULL;

  return vfiles[file]->GetItemName(item);
}


// Returns the buffer size needed to load the given lump.
int FileCache::LumpLength(int lump)
{
  int item = lump & 0xffff;
  unsigned int file = lump >> 16;
  
  if (file >= vfiles.size())
    CONS_Printf/*I_Error*/("FileCache::LumpLength: %i >= numvfiles(%i)\n", file, vfiles.size());
  if (item >= vfiles[file]->numitems)
    CONS_Printf/*I_Error*/("FileCache::LumpLength: %i >= numitems", item);

  return vfiles[file]->GetItemSize(item);
}




//================================
//    reading and caching
//================================


// read 'size' bytes of lump. If size == 0, read the entire lump
// sometimes just the header is needed
int FileCache::ReadLumpHeader(int lump, void *dest, unsigned size, unsigned offset)
{
  int item = lump & 0xffff;
  unsigned int file = lump >> 16;

  if (file >= vfiles.size())
    I_Error("FileCache::ReadLumpHeader: %i >= numvfiles(%i)\n", file, vfiles.size());
  if (item >= vfiles[file]->numitems)
    I_Error("FileCache::ReadLumpHeader: %i >= numitems", item);

  return vfiles[file]->ReadItem(item, dest, size, offset);
}
// caches the given lump

void *FileCache::CacheLumpNum(int lump, int tag, bool add_NUL)
{
  int item = lump & 0xffff;
  unsigned int file = lump >> 16;

  if (file >= vfiles.size())
    I_Error("FileCache::CacheLumpNum: %i >= numvfiles(%i)\n", file, vfiles.size());
  if (item >= vfiles[file]->numitems)
    I_Error("FileCache::CacheLumpNum: %i >= numitems", item);

  if (add_NUL)
    {
      int size = vfiles[file]->GetItemSize(item);
      char *p = (char *)Z_Malloc(size + 1, tag, NULL);
      vfiles[file]->ReadItem(item, p, size);
      p[size] = '\0';
      return p;
    }
  else
    return vfiles[file]->CacheItem(item, tag);
}


void FileCache::CacheArchiveFile(VFile* vf)
{
  vfiles.push_back(vf);
}

Uint16 FileCache::CacheArchiveFile_Remove()
{
    for (size_t k = 0; k < vfiles.size(); )  // Kein k++ im Header
    {
        const char *ext = strrchr(vfiles[k]->filename.c_str(), '.');
        if (ext) {
            ext++;  // Nach dem Punkt
            if (strcasecmp(ext, "zip") == 0)
            {
                CONS_Printf("*ZIP/PK3 Entferne: %s aus der internen VfileList\n", vfiles[k]->filename.c_str());
                delete vfiles[k];  // Füge das hinzu, falls nicht schon in ~FileCache!
                vfiles.erase(vfiles.begin() + k);
                // Kein k++, da nächstes jetzt an k ist
            }
            else
            {
                k++;
            }
        }
        else
        {
            k++;
        }
    }
    return vfiles.size();
}
  
Uint8 FileCache::CacheListIndex(void)
{  
    
    CONS_Printf("\n [%s][%d] vFileList Check\n",__FILE__,__LINE__);
    CONS_Printf("------------------------------------------------------------\n");  
    for (size_t i = 0; i < vfiles.size(); i++)
    {
       CONS_Printf("  > Hold:[%2d] %s\n",(unsigned int)i,vfiles[i]->filename.c_str());
    }      
    CONS_Printf("------------------------------------------------------------\n\n");
    
    return 0;
    
}