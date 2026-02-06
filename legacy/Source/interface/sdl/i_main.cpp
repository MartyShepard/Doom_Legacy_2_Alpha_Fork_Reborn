// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_main.cpp 514 2007-12-21 16:07:36Z smite-meister $
//
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
/// \brief Main program, simply calls D_DoomMain and the main game loop D_DoomLoop.

// in m_argv.h
  #include <SDL.h>
  #include <string.h>
  #include <ctype.h>
  #include <stdio.h>
  #include <iostream>
  #include <locale>
  #include <stdint.h>

  #include "doomtype.h"

  #include "i_system.h"
  #include "m_misc.h"


 
extern  int     myargc;
extern  char**  myargv;

int Console_Requested;



static void  Console_Request(int console_request); //Marty

void D_DoomLoop();
bool D_DoomMain();

int main(int argc, char **argv)
{ 

  SetCurrentDirectoryA(ProgrammPath());  // Jetzt ist CWD = Ordner der exe   
  
  myargc = argc; 
  myargv = argv; 
 


    /* Marty
     * Damit man on the fly auch in der CMD sieht was das abgeht
     */
    int   i;
    Console_Requested = 0;

    for (i = 1; i < myargc; i++)
    {
      //#ifdef DRAGFILE
      if (isFullFilePath(argv[i]) && isWadFile(argv[i]))
      {        
          if (Get_DragFile(argv[i]))
          {
              DrgFile_AutoStart = argv[i]; // liegt in d_main
              //Console_Requested = 1;
              break;  // Erste gefundene WAD nehmen
          }
      }
      //#endif      
      if (strcmp(argv[i], "-v" ) == 0)
      {
        Console_Requested = 1;
       break;
      }
      
      if ((strcmp(argv[i], "-h"       ) == 0)||
          (strcmp(argv[i], "-help"    ) == 0)||
          (strcmp(argv[i], "--version") == 0))
      {
        Console_Requested = 2;
       break;
      }      
    }
  
  Console_Request(Console_Requested);
  
  if (D_DoomMain())
    D_DoomLoop();

  return 0;
}

/* Marty
 * Öffne die Konsole wenn man in der Eingabeaufoderung ist oder
 * eben ein kommando übergibt. sonst sollte sie geschlossen bleiben
 */
static void  Console_Request(int console_request)
{
  if (console_request == 1)
  {
    AllocConsole();                           // Öffnet Konsole   
    SetConsoleTitle("Doom Legacy v2.0.0 Console");
  }
  else if (console_request == 2)
  {
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    freopen("CONIN$",  "r", stdin);   // optional    
  }  
  else
    FreeConsole();  // Falls doch eine offen war, schließen   
}
