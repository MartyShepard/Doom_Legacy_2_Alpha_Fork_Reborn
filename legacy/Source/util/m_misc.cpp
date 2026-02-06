// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: m_misc.cpp 616 2010-03-23 05:15:47Z smite-meister $
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
/// \brief Default configfile, screenshots, file I/O.

#include <ctype.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>


#include "doomdef.h"
#include "doomdata.h"   // IWAD recognition table
#include "command.h"

#include "m_misc.h"
#include "m_argv.h"
#include "m_swap.h"

#include "g_input.h"
#include "screen.h"
#include "i_video.h"

#include "w_wad.h"
#include "z_zone.h"

#include "parser.h"

#include "hardware/oglrenderer.hpp"


// ==========================================================================
//                         FILE INPUT / OUTPUT
// ==========================================================================

//
// FIL_WriteFile
//

bool FIL_WriteFile(const char *name, void *source, int length)
{
  int handle = open(name, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);

  if (handle == -1)
    return false;

  int count = write(handle, source, length);
  close(handle);

  if (count < length)
    return false;

  return true;
}


//
// FIL_ReadFile : return length, 0 on error
//
int FIL_ReadFile(const char *name, byte **buffer)
{
  struct stat fileinfo;

  int handle = open(name, O_RDONLY | O_BINARY, 0666);

  if (handle == -1)
    return 0;

  if (fstat(handle, &fileinfo) == -1)
    return 0;

  int length = fileinfo.st_size;
  byte *buf = (byte *)Z_Malloc(length+1, PU_STATIC, 0);

  int count = read(handle, buf, length);
  close(handle);

  if (count < length)
    return 0;

  //Fab:26-04-98:append 0 byte for script text files
  buf[length] = 0;

  *buffer = buf;
  return length;
}


//
// checks if needed, and add default extension to filename
//
void FIL_DefaultExtension(char *path, const char *extension)
{
  // search for '.' from end to begin, add .EXT only when not found
  const char *src = path + strlen(path) - 1;

  while (*src != '/' && src != path)
    {
      if (*src == '.')
        return;                 // it has an extension
      src--;
    }

  strcat(path, extension);
}


//  Creates a resource name (max 8 chars 0 padded) from a file path
//
void FIL_ExtractFileBase(char *path, char *dest)
{
  char *src = path + strlen(path) - 1;

  // back up until a \ or the start
  while (src != path && *(src-1) != '\\' && *(src-1) != '/')
    src--;

  // copy up to eight characters
  memset(dest, 0, 8);
  int length = 0;

  while (*src && *src != '.')
    {
      if (++length == 9)
        I_Error("Filename base of %s >8 chars",path);

      *dest++ = toupper((int)*src++);
    }
}


//  Returns true if a filename extension is found
//  There are no '.' in wad resource name
//
bool FIL_CheckExtension(const char *in)
{
  while (*in++)
    if (*in=='.')
      return true;

  return false;
}


// returns a pointer to the "filename-part" of a pathname
const char *FIL_StripPath(const char *s)
{
  for (int j = strlen(s) - 1; j >= 0; j--)
    if ((s[j] == '\\') || (s[j] == ':') || (s[j] == '/'))
      return &s[j+1];

  return s;
}



// ==========================================================================
//                        CONFIGURATION FILE
// ==========================================================================

#define MAX_CONFIGNAME 128

char configfile[MAX_CONFIGNAME];


char savegamename[256]; // path + name template
char hubsavename[256];  // path + name template

// ==========================================================================
//                          CONFIGURATION
// ==========================================================================
bool         gameconfig_loaded = false;      // true once config.cfg loaded
                                                //  AND executed


void Command_SaveConfig_f()
{
    char tmpstr[MAX_CONFIGNAME];

    if (COM.Argc()!=2)
    {
        CONS_Printf("saveconfig <filename[.cfg]> : save config to a file\n");
        return;
    }
    strcpy(tmpstr,COM.Argv(1));
    FIL_DefaultExtension(tmpstr,".cfg");

    M_SaveConfig(tmpstr);
    CONS_Printf("config saved as %s\n",configfile);
}

void Command_LoadConfig_f()
{
    if (COM.Argc()!=2)
    {
        CONS_Printf("loadconfig <filename[.cfg]> : load config from a file\n");
        return;
    }

    strcpy(configfile,COM.Argv(1));
    FIL_DefaultExtension(configfile,".cfg");
/*  for create, don't check

    if ( access(tmpstr,F_OK) )
    {
        CONS_Printf("Error reading file %s (not exist ?)\n",tmpstr);
        return;
    }
*/
    COM.PrependText(va("exec \"%s\"\n",configfile));

}

void Command_ChangeConfig_f()
{
    if (COM.Argc()!=2)
    {
        CONS_Printf("changeconfig <filaname[.cfg]> : save current config and load another\n");
        return;
    }

    COM.AppendText(va("saveconfig \"%s\"\n",configfile));
    COM.AppendText(va("loadconfig \"%s\"\n",COM.Argv(1)));
}

//
// Load the default config file
//
void M_FirstLoadConfig()
{
  // load default control
  G_Controldefault();

  // load config, make sure those commands doesnt require the screen..
  CONS_Printf("\n");
  COM.PrependText(va("exec \"%s\"\n",configfile));
  COM.BufExecute();       // make sure initial settings are done

  // make sure I_Quit() will write back the correct config
  // (do not write back the config if it crash before)
  gameconfig_loaded = true;
}


void G_SavePlayerPrefs(FILE *f);
void G_SaveKeySetting(FILE *f);
void G_SaveJoyAxisBindings(FILE *f);

//  Save all game config here
//
void M_SaveConfig(char *filename)
{
  FILE *f;

  // make sure not to write back the config until
  //  it's been correctly loaded
  if (!gameconfig_loaded)
    return;

  // can change the file name
  if (filename)
    {
      f = fopen(filename, "w");
      // change it only if valide
      if (f)
	strcpy(configfile,filename);
      else
        {
	  CONS_Printf("Couldn't save game config file %s\n",filename);
	  return;
        }
    }
  else
    {
      f = fopen(configfile, "w");
      if (!f)
        {
	  CONS_Printf("Couldn't save game config file %s\n",configfile);
	  return;
        }
    }

  // header message
  fprintf(f, "// Doom Legacy configuration file.\n");

  //FIXME: save key aliases if ever implemented..

  consvar_t::SaveVariables(f);
  G_SavePlayerPrefs(f);
  G_SaveKeySetting(f);
  G_SaveJoyAxisBindings(f);

  fclose(f);
}



// ==========================================================================
//                            SCREEN SHOTS
// ==========================================================================


struct pcx_t
{
  char                manufacturer;
  char                version;
  char                encoding;
  char                bits_per_pixel;

  unsigned short      xmin, ymin, xmax, ymax;
  unsigned short      hres, vres;

  unsigned char       palette[48];

  char                reserved;
  char                color_planes;
  unsigned short      bytes_per_line;
  unsigned short      palette_type;

  char                filler[58];
  unsigned char       data;           // unbounded
};


//
// WritePCXfile
//
bool WritePCXfile(const char*   filename,
                  byte*         data,
                  int           width,
                  int           height,
                  byte*         palette)
{
  pcx_t *pcx = (pcx_t *)Z_Malloc(width*height*2+1000, PU_STATIC, NULL);

  pcx->manufacturer = 0x0a;           // PCX id
  pcx->version = 5;                   // 256 color
  pcx->encoding = 1;                  // uncompressed
  pcx->bits_per_pixel = 8;            // 256 color
  pcx->xmin = 0;
  pcx->ymin = 0;
  pcx->xmax = SHORT(width-1);
  pcx->ymax = SHORT(height-1);
  pcx->hres = SHORT(width);
  pcx->vres = SHORT(height);
  memset(pcx->palette, 0, sizeof(pcx->palette));
  pcx->color_planes = 1;              // chunky image
  pcx->bytes_per_line = SHORT(width);
  pcx->palette_type = SHORT(1);       // not a grey scale
  memset(pcx->filler, 0, sizeof(pcx->filler));


  // pack the image
  byte *pack = &pcx->data;

  int i;
  for (i=0; i < width*height; i++)
    {
      if ((*data & 0xc0) != 0xc0)
           *pack++ = *data++;
      else
        {
	  *pack++ = 0xc1;
	  *pack++ = *data++;
        }
    }

  // write the palette
  *pack++ = 0x0c;     // palette ID byte
  for (i=0; i < 768; i++)
    *pack++ = *palette++;

  // write output file
  int length = pack - (byte *)pcx;
  i = FIL_WriteFile(filename, pcx, length);

  Z_Free(pcx);
  return i;
}


//
// M_ScreenShot
//
void M_ScreenShot()
{
  bool WritePNGScreenshot(FILE *fp, byte *lfb, int width, int height, RGB_t *pal);

  char lbmname[MAX_CONFIGNAME];

  // find a file name to save it to
  if(rendermode == render_opengl)
    strcpy(lbmname, "DOOM000.bmp");
  else
    strcpy(lbmname, "DOOM000.png");

  for (int i=0; i <= 999; i++)
    {
      lbmname[4] = i/100 + '0';
      lbmname[5] = i/10  + '0';
      lbmname[6] = i%10  + '0';
      if (access(lbmname, F_OK) == -1)
	break;
    }

  bool ret = false;

  if (rendermode == render_opengl)
    {
      ret = oglrenderer->WriteScreenshot(lbmname);
    }
  else
    {
      RGB_t *pal = vid.GetCurrentPalette();
      FILE *fp = fopen(lbmname, "wb");
      if (fp)
	{
	  ret = WritePNGScreenshot(fp, vid.screens[0], vid.width, vid.height, pal);
	  fclose(fp);
	}

      // Save the pcx
      //ret = WritePCXfile(lbmname, vid.screens[0], vid.width, vid.height, (byte *)fc.CacheLumpName("PLAYPAL", PU_CACHE));
    }

  if (ret)
    CONS_Printf("Screen shot %s saved.\n", lbmname);
  else
    CONS_Printf("Couldn't create a screen shot.\n");
}


// ==========================================================================
//                        MISC STRING FUNCTIONS
// ==========================================================================


// Variable arguments handler for printing (sort of inline sprintf)
char *va(const char *format, ...)
{
#define BUF_SIZE 1024
  va_list      ap;
  static char  buffer[BUF_SIZE];
 
  va_start(ap, format);
  vsnprintf(buffer, BUF_SIZE, format, ap);
  va_end(ap);

  return buffer;
}


// creates a copy of a string, null-terminated
// returns ptr to the new duplicate string
//
char *Z_StrDup(const char *in)
{
  char *out = (char *)ZZ_Alloc(strlen(in)+1);
  strcpy(out, in);
  return out;
}


// s1=s2+s3+s1
void strcatbf(char *s1,char *s2,char *s3)
{
  char tmp[1024];

  strcpy(tmp,s1);
  strcpy(s1,s2);
  strcat(s1,s3);
  strcat(s1,tmp);
}

string string_to_upper(const char *c) {
  string result;
  char *newc;
  int len;

  if(c == NULL)
    return result;

  len = strlen(c);
  if(len == 0)
    return result;

  newc = new char[len];
  strcpy(newc, c);
  strupr(newc);
  result = newc;
  delete []newc;

  return result;
}

// ==========================================================================
//                        VARIOUS UTIL FUNCTIONS
// ==========================================================================

byte CheckMainWad(const char *MainWadFile)
{
    int i;

    for (i = 0; i < NUM_IWADS; i++)
    {
        if (strcasecmp(MainWadFile, iwads[i].wadname) == 0)
        {
            CONS_Printf("Main WAD match: %s -> mode=%d, mission=%d\n",
                        iwads[i].wadname, iwads[i].mode, iwads[i].mission);
            return 1; // gefunden
        }
    }

    CONS_Printf("No IWAD match for: %s\n", MainWadFile);
    return 0;
}


byte isFullFilePath(const char *str)
{
    if (!str || !*str) return 0;

    int has_slash = 0;
    int has_extension = 0;

    const char *p = str;

    // Hat Pfadtrenner (\ oder /)?
    while (*p)
    {
        if (*p == '\\' || *p == '/') has_slash = 1;
        p++;
    }

    // Hat Dateiendung? (mindestens . + 3 Zeichen, z. B. .wad)
    const char *dot = strrchr(str, '.');
    if (dot && strlen(dot) >= 4) has_extension = 1;

    // Muss mindestens einen Trenner UND eine Endung haben
    return has_slash && has_extension;
}

// Prüft, ob es eine WAD/ZIP/DEH-Datei ist (case-insensitive)
//#ifdef DRAGFILE
  byte  DrgFile_Requested = 0;
  char *DrgFile_AutoStart = NULL;
//#endif
byte isWadFile(const char *path)
{
    if (!path) return 0;

    const char *ext = strrchr(path, '.');
    if (!ext) return 0;

    ext++; // Nach dem Punkt

    DrgFile_Requested = 1;       // liegt in d_main 

    CONS_Printf("DrgFile_Requested: %d\n", DrgFile_Requested);
    if (CheckMainWad( path ) == 0)
    {
      CONS_Printf("DrgFile_Requested: strcasecmp\n");
      return  strcasecmp(ext, "wad") == 0 ||
              strcasecmp(ext, "zip") == 0 ||
              strcasecmp(ext, "deh") == 0  ;
    }
    CONS_Printf("DrgFile_Requested: return 0\n");
    return 0;
}
//#ifdef DRAGFILE
byte Get_DragFile(char *dragged_file)
{
    if (dragged_file)
      return 1;// Optional: Spiel sofort starten     

   return 0;// Normaler Start ohne Drag & Drop
}
//#endif

char *ProgrammPath(void)
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

byte DirectoryCheck_isPath(const char *path)
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