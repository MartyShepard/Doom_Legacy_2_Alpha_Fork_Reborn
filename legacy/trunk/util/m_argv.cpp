// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: m_argv.cpp 508 2007-12-18 21:00:41Z jussip $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2004 by DooM Legacy Team.
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
/// \brief Commandline argument processing

#include <stdlib.h>
#include <string.h>

#include "doomdef.h"
#include "doomtype.h"
#include "command.h"

int             myargc;
char**          myargv;
static int      found = 0;

//
// M_CheckParm
// Checks for the given parameter
// in the program's command line arguments.
// Returns the argument number (1 to argc-1)
// or 0 if not present
int M_CheckParm(const char *check)
{
  int         i;

  for (i = 1;i<myargc;i++)
    {
      if ( !strcasecmp(check, myargv[i]) )
        {
	  found = i;
	  return i;
        }
    }
  found = 0;
  return 0;
}

// return true if there is available parameters
// called after M_CheckParm
bool M_IsNextParm()
{
  if(found > 0 && found+1 < myargc && myargv[found+1][0] != '-' && myargv[found+1][0] != '+')
    return true;
  return false;
}

// return the next parameter after a M_CheckParm
// NULL if not found use M_IsNext to find if there is a parameter
char *M_GetNextParm()
{
  if(M_IsNextParm())
    {
      found++;
      return myargv[found];
    }
  return NULL;
}

// push all parameters begining by '+'
void M_PushSpecialParameters()
{
  int     i;
  char    s[256];
  bool onetime=false;

  for (i = 1;i<myargc;i++)
    {
      if ( myargv[i][0]=='+' )
        {
	  strcpy(s,&myargv[i][1]);
	  i++;

	  // get the parameter of the command too
	  for(;i<myargc && myargv[i][0]!='+' && myargv[i][0]!='-' ;i++)
            {
	      strcat(s," ");
	      if(!onetime) { strcat(s,"\"");onetime=true; }
	      strcat(s,myargv[i]);
            }
	  if( onetime )    { strcat(s,"\"");onetime=false; }
	  strcat(s,"\n");

	  // push it
	  COM.AppendText(s);
	  i--;
        }
    }
}

//
// Find a Response File
//
void M_FindResponseFile()
{
#define MAXARGVS        256

  int             i;

  for (i = 1;i < myargc;i++)
    if (myargv[i][0] == '@') {
      FILE    *handle;
      int      size;
      int      j, k;
      int      indexinfile;
      bool  inquote = false;
      byte    *infile;
      //char    *file;
      char    *moreargs[20];
      char    *firstargv;

      // READ THE RESPONSE FILE INTO MEMORY
      handle = fopen (&myargv[i][1],"rb");
      if (handle == NULL) I_Error ("\nResponse file %s not found !",&myargv[i][1]);

      CONS_Printf("Found response file %s!\n",&myargv[i][1]);
      fseek (handle, 0, SEEK_END);
      size = ftell(handle);
      fseek (handle, 0, SEEK_SET);
      infile = (byte *)malloc(size);
      fread (infile, size, 1, handle);
      fclose (handle);
	
      // KEEP ALL CMDLINE ARGS FOLLOWING @RESPONSEFILE ARG
      for (j = 0, k = i+1; k < myargc; k++)
	moreargs[j++] = myargv[k];
	
      firstargv = myargv[0];
      myargv = (char **)malloc(sizeof(char *) * MAXARGVS);
      if(myargv == NULL) I_Error("\nNot enough memory for cmdline args");
      memset(myargv, 0, sizeof(char *) * MAXARGVS);
      myargv[0] = firstargv;
      
      //infile = file;
      indexinfile = k = 0;
      indexinfile++;  // SKIP PAST ARGV[0] (KEEP IT)
      do {
	inquote = infile[k] == '"';
	if (inquote) k++; // strip enclosing double-quote
	
	myargv[indexinfile++] = (char *)&infile[k];
	while (k < size && ((inquote && infile[k]!='"') || (!inquote && infile[k] > ' '))) k++;
	infile[k] = 0; // can cause crash (responsefile with size 0 or just one ")
	while(k < size && (infile[k] <= ' ')) k++;
      } while(k < size);
	
      for (k = 0; k < j; k++) myargv[indexinfile++] = moreargs[k];
      myargc = indexinfile;

      // DISPLAY ARGS
      CONS_Printf("%d command-line args:\n", myargc);
      for (k = 1; k < myargc; k++)
	CONS_Printf("%s\n",myargv[k]);
	
      break;
    }
}
