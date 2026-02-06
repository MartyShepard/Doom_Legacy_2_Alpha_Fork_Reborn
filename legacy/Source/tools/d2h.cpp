// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: d2h.cpp 309 2006-02-11 17:02:49Z jussip $
//
// Copyright (C) 2003-2004 by DooM Legacy Team.
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
/// \brief Creates a binary Doom => Hexen linedef conversion table

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "p_setup.h"


int main(int argc, char *argv[])
{
  if (argc != 3)
    {
      printf("This program converts a tab-delimited Doom => Hexen linedef\n"
	     "conversion table into a binary lump to be inserted into legacy.wad\n");
      printf("Usage: d2h <conversiontable> <outfile>\n");
      return -1;
    }


  FILE *input = fopen(argv[1], "rb");
  if (!input)
    {
      printf("File '%s' not found!\n", argv[1]);
      return -1;
    }

  int i, j, expected = 0, count = 0;
  char trig[8];
  int dtype, htype, a1, a2, a3, a4, a5;
  xtable_t t, zero;
  memset(&zero, 0, sizeof(xtable_t));

  FILE *output = fopen(argv[2], "wb");

  for (;;)
    {
      i = fscanf(input, "%d %d %d %d %d %d %d %4s", &dtype, &htype,
		 &a1, &a2, &a3, &a4, &a5, trig);      
      if (i != 8)
	break;

      if (dtype < expected)
	{
	  printf("Idiot! In linedef %d the ordering breaks!\n", dtype);
	  break;
	}

      while (dtype > expected)
	{
	  // fill the missing linedeftypes with zero
	  fwrite(&zero, sizeof(xtable_t), 1, output);
	  expected++;
	  count++;
	}

      if (htype > 255)
	{
	  printf("WTF?! In linedef %d the Hexen type is > 255!\n", dtype);
	  break;
	}

      t.type = htype;
      t.args[0] = a1;
      t.args[1] = a2;
      t.args[2] = a3;
      t.args[3] = a4;
      t.args[4] = a5;
      t.trigger = 0; // zero all bits
      for (j = 0; trig[j]; j++)
	{
	  // NOTE! this only works if only one trigger type is given
	  switch (toupper(trig[j]))
	    {
	    case 'R':
	      t.trigger |= T_REPEAT;
	      break;

	    case 'M':
	      t.trigger |= T_ALLOWMONSTER;
	      break;

	    case 'C':
	      t.trigger |= (T_CROSS << T_ASHIFT);
	      break;

	    case 'U':
	      t.trigger |= (T_USE << T_ASHIFT);
	      break;

	    case 'I':
	      t.trigger |= (T_IMPACT << T_ASHIFT);
	      break;

	    case '1':
	      // do nothing
	      break;

	    default:
	      break;
	    }
	}

      if (((t.trigger & T_AMASK) >> T_ASHIFT) > 3)
	{
	  printf("Dummkopf! In linedef %d, the trigger '%s' is bad, mmm'kay?\n",
		 dtype, trig);
	  break;
	}

      fwrite(&t, sizeof(xtable_t), 1, output);
      expected++; // next expected linedeftype
    }

  fclose(input);
  fclose(output);

  if (expected > 0)
    printf("\nDone. Linedef-types 0--%d written.\n%d undetermined linedef-types set to zero.\n", expected-1, count);

  return 0;
}
