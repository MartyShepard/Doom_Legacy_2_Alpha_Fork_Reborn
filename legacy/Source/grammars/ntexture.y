// Emacs style mode select   -*- C++ -*-
// GNU Bison 2.2 grammar for the JDS NTEXTURE lumps.
// Generates a C++ parser class.
// Copyright (C) Ville Bergholm 2005-2006

%skeleton "lalr1.cc"
%require "2.2"
%define "parser_class_name" "ntexture_parser"

%{
// first prologue
#include "ntexture.h"
#include "doomdef.h"
#include "r_data.h"
%}

// token data types
%union
{
  float ftype;
  int   itype;
  const char *stype; // just a pointer, not a copy! thus no destructor!
}


%{
// second prologue
// Announce to Flex the prototype we want for lexing function,
//#define YY_DECL  int yylex(YYSTYPE *yylval, yy::location *yylloc, ntexture_driver& driver)
#define YY_DECL  int yylex(union yy::ntexture_parser::semantic_type *yylval, yy::location *yylloc)

// and declare it for the parser's sake.
YY_DECL;
%}


// The parsing context.
%parse-param { ntexture_driver& d }
//%lex-param   { ntexture_driver& d }

// data types
%token <itype> INT
%token <ftype> FLOAT
%token <stype> STR

// keywords
%token TEXTURE SPRITE
%token DATA WORLDSIZE SCALE TEXELOFFSETS OFFSET

//%left, %right, %nonassoc    // token precedence
//%destructor { delete $$; } STR

%type <ftype> num // typed nonterminal


%defines        // generate header xxx.tab.h
%verbose        // generate raport file xxx.output
%error-verbose
%debug

%locations
%pure-parser
%initial-action
{
  // Initialize the initial location.
  //@$.begin.filename = @$.end.filename = ;
};

%start definitions // start symbol


//============================================================================
%% // grammar rules
//============================================================================

// sequence of 0-N texture definitions
definitions
: // empty
| definitions texture // left recursion
;

// single texture declaration
texture
: tex_type STR '{'
    {
      d.texname = $2;
      d.t = NULL;
      d.texeloffsets = false;
    }
  tex_initialization tex_properties '}'
    {
      // texture definition finished, insert it into cache
      if (d.t != &d.dummy)
	{
	  CONS_Printf(" Built texture '%s'.\n", d.t->GetName());
	  d.t->Initialize();
	  if (d.is_sprite)
	    tc.InsertSprite(d.t);
	  else
	    tc.InsertTexture(d.t);
	}
    }
/*
| TEXTURE STR ':' STR '{'  // with inheritance, no initialization part allowed
    {
      // FIXME only one string can be used at a time! pointer!
      t = tc.GetPtr($4, TEX_wall);
      if (t == tc.default_item)
	{
	  CONS_Printf(" Texture '%s' not found.\n", $4);
	}
      t = new XXX_Texture(*t); // copy construction
    }
  tex_properties '}'
*/
//| error { yyerrok; }
;

tex_type
: TEXTURE { d.is_sprite = false; }
| SPRITE  { d.is_sprite = true;  }
;

// one or the other, not both
tex_initialization
: DATA STR ';'
    {
      d.t = tc.Load($2);
      if (!d.t)
	{
	  CONS_Printf(" Texture lump '%s' not found.\n", $2);
	  d.t = &d.dummy;
	}
      d.t->SetName(d.texname.c_str());
    }
// | COMPOSE INT INT ';' { d.t = new ComposedTexture(d.texname, $2, $3); } 
;

// list of 0-N properties
tex_properties
: // empty
| tex_properties tex_property
;

// now we know that t points to a valid texture object, let's just fill in the properties
tex_property
: WORLDSIZE num num ';' { d.t->xscale = d.t->width/$2; d.t->yscale = d.t->height/$3; }
| SCALE num num ';'     { d.t->yscale = 1/$2; d.t->xscale = d.t->yscale/$3; } 
| SCALE num ';'         { d.t->xscale = d.t->yscale = 1/$2; }
| TEXELOFFSETS num ';'  { d.texeloffsets = $2; }
| OFFSET num num ';'
    {
      if (d.texeloffsets)
	{
	  d.t->leftoffs = ($2 / d.t->xscale).floor();
	  d.t->topoffs = ($3 / d.t->yscale).floor();
	}
      else
	{
	  d.t->leftoffs = $2;
	  d.t->topoffs = $3;
	}
    }
;

// any real number
num
: FLOAT
| INT    { $$ = $1; }
;


/*
// error locations
{
  $$ = 1;
  fprintf (stderr, "%d.%d-%d.%d: division by zero",
	   @3.first_line, @3.first_column,
	   @3.last_line, @3.last_column);
}
*/


//============================================================================
%% // epilogue
//============================================================================


void yy::ntexture_parser::error(const yy::ntexture_parser::location_type& l, const std::string& m)
{
  d.error(l, m);
}
