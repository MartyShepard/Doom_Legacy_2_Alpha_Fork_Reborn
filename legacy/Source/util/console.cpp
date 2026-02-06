// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: console.cpp 616 2010-03-23 05:15:47Z smite-meister $
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
/// \brief Console for Doom LEGACY

#include <stdarg.h>

#include "doomdef.h"
#include "d_event.h"

#include "command.h"
#include "console.h"

#include "g_game.h"
#include "g_input.h"
#include "hud.h"

#include "r_data.h"
#include "sounds.h"
#include "screen.h"

#include "v_video.h"
#include "i_video.h"
#include "z_zone.h"
#include "i_system.h"
#include "w_wad.h"


/// returns the number of bytes in the UTF-8 char beginning at p
static int utf8_numbytes(const char *p)
{
  //const byte *p = reinterpret_cast<const byte*>(q);

  if (!(p[0] & 0x80))
    return 1; // ASCII

  int n;
  for (n=1; (p[n] & 0xc0) == 0x80; n++);

  return n; // multibyte encoding
}

static int utf8_sync(const char *p)
{
  int n = 0;
  while ((p[n] & 0xc0) == 0x80) // if two high bits are 10, this is not a lead byte
    n++;

  return n;
}

/// Converts a single UCS-4 wide char into an UTF-8 string, returns the string length.
static int ucs4_to_utf8(int c, char *p)
{
  if (c < 0x80) // max 7 bits
    {
      p[0] = c;
      return 1;
    }
  else if (c < 0x800) // max 11 bits
    {
      p[0] = 0xc0 | (c >> 6);
      p[1] = 0x80 | (c & 0x3f);
      return 2;
    }
  else if (c < 0x10000) // max 16 bits
    {
      p[0] = 0xe0 | (c >> 12);
      p[1] = 0x80 | ((c >> 6) & 0x3f);
      p[2] = 0x80 | (c & 0x3f);
      return 3;
    }
  else if (c < 0x200000) // max 21 bits
    {
      p[0] = 0xf0 | (c >> 18);
      p[1] = 0x80 | ((c >> 12) & 0x3f);
      p[2] = 0x80 | ((c >> 6) & 0x3f);
      p[3] = 0x80 | (c & 0x3f);
      return 4;
    }

  return 0; // fail
}


/// Converts a single UCS-4 wide char into an UTF-8 string, returns the string length.
static int ucs4_to_utf8(int c, string &out)
{
  if (c < 0x80) // max 7 bits
    {
      out.push_back(c);
      return 1;
    }
  else if (c < 0x800) // max 11 bits
    {
      out.push_back(0xc0 | (c >> 6));
      out.push_back(0x80 | (c & 0x3f));
      return 2;
    }
  else if (c < 0x10000) // max 16 bits
    {
      out.push_back(0xe0 | (c >> 12));
      out.push_back(0x80 | ((c >> 6) & 0x3f));
      out.push_back(0x80 | (c & 0x3f));
      return 3;
    }
  else if (c < 0x200000) // max 21 bits
    {
      out.push_back(0xf0 | (c >> 18));
      out.push_back(0x80 | ((c >> 12) & 0x3f));
      out.push_back(0x80 | ((c >> 6) & 0x3f));
      out.push_back(0x80 | (c & 0x3f));
      return 4;
    }

  return 0; // fail
}




#define CON_PROMPT ">"
#define CON_margin 4 // pixels

Console con;


// communication with HUD
int  con_clearlines; // top screen lines to refresh with background image when 3D view is reduced
bool con_hudupdate;  // when messages scroll, we need a backgrnd refresh

int  con_clipviewtop;// SW renderer:
                     // clip value for planes & sprites, so that the
                     // part of the view covered by the console is not
                     // drawn when not needed, this must be -1 when
                     // console is off


//======================================================================
//   Global colormaps
//======================================================================

// Prepare a colormap for GREEN ONLY translucency over background

byte *whitemap;
byte *greenmap;
byte *graymap;

static void CON_SetupColormaps()
{
  //  setup the green translucent background colormap
  //  setup the white and gray text colormap

  greenmap = static_cast<byte*>(Z_Malloc(256,PU_STATIC,NULL));
  whitemap = static_cast<byte*>(Z_Malloc(256,PU_STATIC,NULL));
  graymap  = static_cast<byte*>(Z_Malloc(256,PU_STATIC,NULL));

  RGB_t *pal = static_cast<RGB_t*>(fc.CacheLumpName("PLAYPAL", PU_DAVE));

  switch (game.mode)
    {
    case gm_hexen:
      for (int k=0; k<256; k++)
	{
	  float c = (pal[k].r + pal[k].g + pal[k].b)/(3*255.0);

          greenmap[k] = 11*16+10 + byte(c*15);   //remaps to greens(209-224)
          graymap[k]  =       byte(c*35);   //remaps to grays(0-35)
          whitemap[k] = 145 + byte(c*15);   //remaps to reds(145-168)
	}
      break;

    case gm_heretic:
      for (int k=0; k<256; k++)
	{
	  float c = (pal[k].r + pal[k].g + pal[k].b)/(3*255.0);

          greenmap[k] = 209 + byte(c*15);   //remaps to greens(209-224)
          graymap[k]  =       byte(c*35);   //remaps to grays(0-35)
          whitemap[k] = 145 + byte(c*15);   //remaps to reds(145-168)
	}
      break;

    default:
      for (int k=0; k<256; k++)
	{
	  int j = pal[k].r + pal[k].g + pal[k].b;
	  greenmap[k] = 127 - (j>>6);
          graymap[k]  = k; // remap each color to itself...
          whitemap[k] = k;
	}

      for (int k=168; k<192; k++)
        {
          whitemap[k] = k-88;     //remaps reds(168-192) to whites(80-104)
          graymap[k]  = k-80;      //remaps reds(168-192) to gray(88-...)
        }
      whitemap[45] = 190-88; // the color[45]=color[190] !
      graymap [45] = 190-80;
      whitemap[47] = 191-88; // the color[47]=color[191] !
      graymap [47] = 191-80;
      break;
    }

  Z_Free(pal);
}



//======================================================================
//                   CONSOLE VARS AND COMMANDS
//======================================================================


CV_PossibleValue_t CV_Positive[] = {{1,"MIN"}, {999999999,"MAX"}, {0,NULL}};

// how many seconds the hud messages lasts on the screen
consvar_t   cons_msgtimeout = {"con_hudtime","5", CV_SAVE,CV_Unsigned};

// number of lines console move per frame
consvar_t   cons_speed = {"con_speed","8", CV_SAVE,CV_Positive};

// percentage of screen height to use for console
consvar_t   cons_height = {"con_height","50", CV_SAVE,CV_Positive};

CV_PossibleValue_t backpic_cons_t[]={{0,"translucent"},{1,"picture"},{0,NULL}};
// whether to use console background picture, or translucent mode
consvar_t   cons_backpic = {"con_backpic","0", CV_SAVE,backpic_cons_t};





//  Clear console text buffer
void Command_Clear_f()
{
  con.Clear();
}



void Command_Bind_f()
{
  int na = COM.Argc();
  int key;

  if (na != 2 && na != 3)
    {
      CONS_Printf ("bind <keyname> [<command>]\n");
      CONS_Printf("\2bind table :\n");
      na = 0;
      for (key=0; key<NUMINPUTS; key++)
        if (con.bindtable[key])
          {
            CONS_Printf("%s : \"%s\"\n", G_KeynumToString(key), con.bindtable[key]);
            na = 1;
          }
      if (!na)
        CONS_Printf("Empty\n");
      return;
    }

  key = G_KeyStringtoNum(COM.Argv(1));
  if (!key)
    {
      CONS_Printf("Invalid key name\n");
      return;
    }

  if (con.bindtable[key] != NULL)
    {
      Z_Free(con.bindtable[key]);
      con.bindtable[key] = NULL;
    }

  if (na == 3)
    con.bindtable[key] = Z_StrDup(COM.Argv(2));
}


//======================================================================
//   Console class implementation
//======================================================================

Console::Console()
{
  refresh = true;
  recalc = true;

  active = true; // start with console active, but unseen
  graphic = false;
  con_tick = 0;

  input_browse = 0;
  input_history.push_front(""); // this way history is never empty

  con_cx = con_cy = 0;
  con_line = NULL;
  con_scrollup = 0;

  // initialize output buffer
  for (int i=0; i<CON_LINES; i++)
    con_buffer[i][0] = '\0';

  con_height = con_destheight = 0;
  con_width = 0; // first time resize
  con_lineheight = 0;

  for (int i=0;i<NUMINPUTS;i++)
    bindtable[i] = NULL;
}


Console::~Console()
{
}


/// Client init (not executed on dedicated server!): Setup the console text buffer
void Console::Init()
{
  if (devparm)
    CONS_Printf("Starting up the console.\n");

  // set console full screen for game startup
  con_destheight = con_height = vid.height;

  CON_SetupColormaps();

  con_clipviewtop = -1;     // -1 do not clip SW 3D view

  // load console background pic & borders
  con_backpic = materials.Get("CONSBACK");
  con_lborder = materials.Get("CBLEFT");
  con_rborder = materials.Get("CBRIGHT");

  con_lineheight = hud_font->Height();

  cons_msgtimeout.Reg();
  cons_speed.Reg();
  cons_height.Reg();
  cons_backpic.Reg();

  graphic = true;

  // make sure it is ready for the loading screen
  RecalcSize();
}


//
//  Called after a screen size change to set the rows and line size of the
//  console text buffer and rewrap it.
//
void Console::RecalcSize()
{
  recalc = false;

  float cw = vid.width -vid.fdupx * (con_lborder->worldwidth + con_rborder->worldwidth); // printing space between borders
  if (cw < 100) // minimal reasonable width
    cw = 100;

  // check for change of video width
  if (cw == con_width)
    return; // didn't change

  if (con_width == 0)
    {
      // first time
      con_width = cw;
      Clear();
      return; 
    }

  con_width = cw;

  int old_cy = con_cy;
  char tmp_buffer[CON_LINES][CON_MAXLINECHARS+1];
  memcpy(tmp_buffer, con_buffer, sizeof(con_buffer));

  // initialize output buffer
  Clear();

  // re-arrange console text buffer to keep text
  for (int i = old_cy+1; i < old_cy + CON_LINES; i++)
    Print(tmp_buffer[i % CON_LINES]);
}



// Toggle console on and off
void Console::Toggle(bool forceoff)
{
  if (game.dedicated)
    {
      I_Error("Some idiot tried to close the console\n");
      return; // dedicated server cannot close console!
    }

  // console key was pushed
  if (active || forceoff)
    {
      // toggle console off
      ClearHUD();
      con_destheight = 0;
      active = false;

      if (forceoff)
        {
          // close it NOW!
          con_height = 0;
          con_clipviewtop = -1;       //remove console clipping of view
        }
    }
  else
    {
      // toggle console on
      con_destheight = (cons_height.value * vid.height)/100;
      if (con_destheight < 20)
        con_destheight = 20;
      else if (con_destheight > vid.height - hud.stbarheight)
        con_destheight = vid.height - hud.stbarheight;

      con_destheight = int(CON_margin + (con_destheight / con_lineheight) * con_lineheight); // multiple of text row height
      active = true;
    }
}



void Console::Clear()
{
  con_cx = 0;
  con_cy = 0;
  con_scrollup = 0;

  // initialize output buffer
  for (int i=0; i<CON_LINES; i++)
    con_buffer[i][0] = '\0';

  con_line = con_buffer[con_cy];
}



//  Clear time of console heads up messages
void Console::ClearHUD()
{
  for (int i=0; i<CON_HUDLINES; i++)
    con_hudtime[i] = 0;
}



//  Console ticker : handles console move in/out, cursor blinking
void Console::Ticker()
{
  extern int viewwindowy;

  // cursor blinking
  con_tick++;
  con_tick &= 7;

  // console movement
  if (con_destheight != con_height)
    {
      // up/down move to dest
      if (con_height < con_destheight)
        {
          con_height += cons_speed.value;
          if (con_height > con_destheight)
            con_height = con_destheight;
        }
      else if (con_height > con_destheight)
        {
          con_height -= cons_speed.value;
          if (con_height < con_destheight)
            con_height = con_destheight;
        }
    }

  // clip the view, so that the part under the console is not drawn
  con_clipviewtop = -1;
  if (cons_backpic.value)   // clip only when using an opaque background
    {
      if (con_height > 0)
        con_clipviewtop = con_height - viewwindowy - 1 - 10;
//NOTE: BIG HACK::SUBTRACT 10, SO THAT WATER DON'T COPY LINES OF THE CONSOLE
//      WINDOW!!! (draw some more lines behind the bottom of the console)
      if (con_clipviewtop < 0)
        con_clipviewtop = -1;   //maybe not necessary, provided it's <0
    }

  // make overlay messages disappear after a while
  for (int i=0; i<CON_HUDLINES; i++)
    {
      if (--(con_hudtime[i]) < 0)
        con_hudtime[i] = 0;
    }
}



//  Handles console key input
bool Console::Responder(event_t *ev)
{
  if (hud.chat_on)
    return false;

  // let go keyup events, don't eat them
  if (ev->type != ev_keydown)
    return false;

  int key = ev->data1;

  // check for console toggle key
  if (key == commoncontrols[gk_console][0] || key == commoncontrols[gk_console][1])
    {
      Toggle();
      return true;
    }

  //  check other keys only if console prompt is active
  if (!active)
    {
      if (key < NUMINPUTS && bindtable[key])
        {
          COM.AppendText(bindtable[key]);
          COM.AppendText("\n");
          return true;
        }
      return false;
    }

  // eat shift if console is active
  if (key == KEY_RSHIFT || key == KEY_LSHIFT)
    return true;

  // escape key toggles off console
  if (key == KEY_ESCAPE)
    {
      Toggle();
      return true;
    }


  // sequential completions a la 4dos
  static char completion[80];
  static int  comskips, varskips;

  // command completion forward (tab) and backward (shift-tab)
  if (key == KEY_TAB)
    {
      // TOTALLY UTTERLY UGLY NIGHT CODING BY FAB!!! :-)
      //
      // sequential command completion forward and backward
      string &input_line = input_history[input_browse];

      int n = input_line.length();

      // remember typing for several completions (à-la-4dos)
      if (n > 0 && input_line[n-1] != ' ')
        {
          if (n < 80)
            strcpy(completion, input_line.c_str());
          else
            completion[0] = 0;

          comskips = varskips = 0;
        }
      else
        {
          if (shiftdown)
            {
              if (comskips<0)
                {
                  if (--varskips<0)
                    comskips = -(comskips+2);
                }
              else
                if (comskips>0)
                  comskips--;
            }
          else
            {
              if (comskips<0)
                varskips++;
              else
                comskips++;
            }
        }

      const char *cmd;
      if (comskips>=0)
        {
          cmd = COM.CompleteCommand(completion, comskips);
          if (!cmd)
            // dirty:make sure if comskips is zero, to have a neg value
            comskips = -(comskips+1);
        }
      if (comskips<0)
        cmd = consvar_t::CompleteVar(completion, varskips);

      if (cmd)
        {
	  input_line = cmd;
	  input_line.push_back(' ');
        }
      else
        {
          if (comskips>0)
            comskips--;
          else if (varskips>0)
            varskips--;
        }

      return true;
    }

  // move up (backward) in console textbuffer
  if (key == KEY_PGUP)
    {
      if (con_scrollup < CON_LINES - int((con_height - CON_margin) / con_lineheight) + 1)
        con_scrollup += 5;
      return true;
    }
  else if (key == KEY_PGDN)
    {
      if (con_scrollup >= 5)
        con_scrollup -= 5;
      else
        con_scrollup = 0;
      return true;
    }

  // oldset text in buffer
  if (key == KEY_HOME)
    {
      con_scrollup = CON_LINES - int((con_height - CON_margin) / con_lineheight) + 1;
      return true;
    }
  else if (key == KEY_END)
    {
      // most recent text in buffer
      con_scrollup = 0;
      return true;
    }

  // enter current input line to command buffer
  if (key == KEY_ENTER)
    {
      string &input_line = input_history[input_browse];

      if (input_line.length() < 1)
        return true;

      // push the command
      COM.AppendText(input_line.c_str());
      COM.AppendText("\n");

      // print the command
      CONS_Printf(CON_PROMPT "%s\n", input_line.c_str());

      // update history
      if (input_browse != 0)
	input_history.front() = input_line; // copy only if necessary

      input_history.push_front(""); // add an empty line
      if (input_history.size() > CON_HISTORY)
	input_history.pop_back();

      input_browse = 0; // start editing the newly added empty line
      return true;
    }

  // erase last char from input line
  if (key == KEY_BACKSPACE)
    {
      string &input_line = input_history[input_browse];

      string::iterator start = input_line.begin();
      string::iterator t = input_line.end();
      while (t != start)
	{
	  // go back until we get a byte beginning with either 0 or 11
	  t--;
	  if (!(*t & 0x80) || (*t & 0xc0) == 0xc0)
	    break;
	}

      input_line.erase(t, input_line.end());
      return true;
    }

  // move back in input history
  if (key == KEY_UPARROW)
    {
      // copy one of the previous inputlines to the current
      int n = input_history.size();
      input_browse = (input_browse + 1) % n;
      return true;
    }

  // move forward in input history
  if (key == KEY_DOWNARROW)
    {
      if (input_browse > 0)
	input_browse--;

      return true;
    }

  // interpret it as input char
  int c = ev->data2;

  if (key >= KEY_KEYPAD0 && key <= KEY_PLUSPAD)
    {
      // allow people to use keypad in console (good for typing IP addresses) - Calum
      const char keypad_translation[] = {'0','1','2','3','4','5','6','7','8','9','.','/','*','-','+'};
      c = keypad_translation[key - KEY_KEYPAD0];
    }

  // enter a char into the command prompt
  if (c < 32) // ignore control characters
    return false;

  // add key to cmd line here
  string &input_line = input_history[input_browse];

  if (input_line.length() < CON_MAXINPUT)
    ucs4_to_utf8(c, input_line);

  return true;
}


//  Insert a new line in the console text buffer
void Console::Linefeed(int player)
{
  // set time for heads up messages
  con_hudtime[con_cy % CON_HUDLINES] = cons_msgtimeout.value * TICRATE;
  // for which local player is it meant?
  con_lineowner[con_cy % CON_HUDLINES] = player;

  con_line[con_cx] = '\0'; // end the line

  // move to next line in buffer, clear it
  con_cy = (con_cy + 1) % CON_LINES;
  con_cx = 0;

  con_line = con_buffer[con_cy];
  con_line[0] = '\0';

  // make sure the view borders are refreshed if hud messages scroll
  con_hudupdate = true; // see HU_Erase()
}



//  Outputs text into the console text buffer
void Console::Print(char *msg)
{
  int p = 1;
  float width = 0.0; // current line width

  while (*msg)
    {
      byte c = *msg;
      if (c <= ' ') // control chars or space
	{
	  if (c == ' ')
            {
              con_line[con_cx++] = ' ';
	      width += hud_font->StringWidth(" ", 1);
            }
	  else if (c == '\2')  // set white color
	    {
	      con_line[con_cx++] = '\2';
	    }
	  else if (c == '\a') // sound
	    {
	      S_StartLocalAmbSound(sfx_message);
	    }
	  else if (c == '\r') // carriage return
            {
              con_cy--;
              Linefeed(p);
            }
          else if (c == '\n')
	    {
	      con_line[con_cx++] = '\n';
	      Linefeed(p);
	    }
          else if (c == '\t')
            {
              // tabs for nice layout in console
              con_line[con_cx++] = '\t';
	      width = (floor(width / TABWIDTH)+1) * TABWIDTH;
            }

	  if (con_cx >= CON_MAXLINECHARS || width >= con_width)
	    Linefeed(p);

          msg++;
	  continue;
	}

      // printable character(s)

      int k; // count word length and width, keep them below max values for a single line
      float w;
      for (k=0, w=0.0; (c > ' ') && (k < CON_MAXLINECHARS) && (w < con_width); )
	{
	  w += hud_font->StringWidth(&msg[k], 1);
	  k += utf8_numbytes(&msg[k]); // pick full utf-8 chars
	  c = msg[k];
	}

      // word wrap
      if (con_cx + k >= CON_MAXLINECHARS || width + w > con_width)
        Linefeed(p);

      // a word at a time
      for ( ; k>0; k--)
        con_line[con_cx++] = *msg++;

      width += w;
    }

  con_line[con_cx] = '\0'; // line ends here (at least for now)
}



//======================================================================
//                          CONSOLE DRAW
//======================================================================


// draw the last lines of console text to the top of the screen
void Console::DrawHudlines()
{
  float y = hud.chat_on ? hud_font->Height() : 0; // leave place for chat input in the first row of text

  for (int i = con_cy - CON_HUDLINES + 1; i <= con_cy; i++)
    {
      if (i < 0)
	i += CON_LINES;

      if (con_hudtime[i % CON_HUDLINES] == 0)
        continue;

      // FIXME lineowner! use viewport locations!
      hud_font->DrawString(0, y, con_buffer[i % CON_LINES], 0);
      y += hud_font->Height();
    }

  // top screen lines that might need clearing when view is reduced
  con_clearlines = int(y)+1;      // this is handled by HU_Erase ();
}



// draw the console background, text, and prompt if enough place
void Console::DrawConsole()
{
  //FIXME: refresh borders only when console bg is translucent
  con_clearlines = con_height;    // clear console draw from view borders
  con_hudupdate = true;           // always refresh while console is on

  // draw console background
  float x = 0;
  float y = con_height - vid.height;
  if (cons_backpic.value)
    con_backpic->Draw(0, y, V_SSIZE);
  else
    {
      float wl = con_lborder->worldwidth * vid.fdupx;
      float wr = con_rborder->worldwidth * vid.fdupx;
  
      x = vid.width - wr;
      con_lborder->Draw(0, y, V_SSIZE);
      con_rborder->Draw(x, y, V_SSIZE);

      V_DrawFadeConsBack(wl, 0, x, con_height); // translucent background
      x = wl; // console printing area starts here
    }

  // draw console text lines from bottom to top
  // (going backward in console buffer text)

  float minheight = 2*con_lineheight + CON_margin;

  if (con_height < minheight)
    return;

  int i = con_cy - con_scrollup;

  // skip the last empty line due to the cursor being at the start
  // of a new line
  if (!con_scrollup && !con_cx)
    i--;

  for (y = con_height - minheight; y >= 0; y -= con_lineheight, i--)
    {
      if (i < 0)
        i += CON_LINES; // wrap

      hud_font->DrawString(x, y, con_buffer[i % CON_LINES], 0);
    }


  // draw prompt if enough space (not while game startup)
  if (con_height == con_destheight && !refresh)
    {
      string &input_line = input_history[input_browse];
      int n = input_line.length();
      const char *p = input_line.c_str();

      // TODO scrolling based on text width, not num of chars
      // input line scrolls left if it gets too long
      if (n >= CON_MAXLINECHARS)
        p += n - CON_MAXLINECHARS + 1;

      y = con_height - CON_margin - con_lineheight;
      x += hud_font->DrawString(x, y, CON_PROMPT, 0);
      x += hud_font->DrawString(x, y, p, 0);

      // draw the blinking cursor
      if (con_tick < 4)
        hud_font->DrawCharacter(x, y, '_', 0);
    }
}


//  Console refresh drawer, called each frame
void Console::Drawer()
{
  if (!graphic)
    return;

  if (recalc)
    RecalcSize();

  //Fab: bighack: patch 'I' letter leftoffset so it centers
  //hud.font['I'-HU_FONTSTART]->leftoffset = -2;

  if (con_height > 0)
    {
      DrawConsole();
      hud.RefreshStatusbar(); // since the console may be drawn over the statusbar
    }
  else if (game.state == GameInfo::GS_LEVEL)
    DrawHudlines();

  //hud.font['I'-HU_FONTSTART]->leftoffset = 0;
}







//======================================================================
//   Wrappers
//======================================================================

//
//  Console print! Wahooo! Lots o fun!
//
void CONS_Printf(const char *fmt, ...)
{
#define BUF_SIZE 1024
  va_list ap;
  char    txt[BUF_SIZE];
 
  va_start(ap, fmt);
  int nchars = vsnprintf(txt, BUF_SIZE, fmt, ap);
  va_end(ap);

  I_OutputMsg("%s", txt); // send copies of console messages to stdout for debugging

  if (!con.graphic)
    return;

  con.Print(txt); // write message in con text buffer

  // make sure new text is visible
  con.con_scrollup = 0;

  // if not in display loop, force screen update
  if (con.refresh)
    {
      /*
      // this #if structure should be replaced with a platform independent function call
#if !defined (SDL) && (defined( __WIN32__) || defined( __OS2__))
      // show startup screen and message using only 'software' graphics
      // (rendermode may be hardware accelerated, but the video mode is not set yet)
      CON_DrawBackpic (con_backpic, 0, vid.width);    // put console background
      I_LoadingScreen ( txt );  // draw the message on it using Win32 API?
#else
      */
      // here we display the console background and console text
      // (no hardware accelerated support for these versions)
      con.Drawer();
      I_FinishUpdate(); // page flip or blit buffer
    }
}


//  Print an error message, and wait for ENTER key to continue.
//  To make sure the user has seen the message
void CONS_Error(const char *msg)
{
  CONS_Printf("\2%s",msg);   // write error msg in different colour
  CONS_Printf("Press ENTER to continue\n");

  // dirty quick hack, but for the good cause
  while (I_GetKey() != KEY_ENTER)
    ;
}
