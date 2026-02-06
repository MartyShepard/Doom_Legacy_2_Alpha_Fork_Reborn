%{ // Emacs style mode select   -*- C++ -*-
// GNU Flex scanner for the JDS NTEXTURE lumps
// Ville Bergholm 2005

// prologue
#include <string>
#include "ntexture.h"     // parser driver
#include "ntexture.tab.h" // parser itself

static std::string temp;
%}

D    [0-9]
L    [a-zA-Z_]
H    [a-fA-F0-9]
E    [Ee][+-]?{D}+
WHITESP [ \t\v\n\r\f]


%option noyywrap nounput batch outfile="ntexture.yy.c"
%x comment
%x str
%%

%{ // C++ -style comments
%}
"//"     { BEGIN(comment); }
<comment>{
  [^\n]*
  \n     { BEGIN(INITIAL); }
}

%{
  // C string literal parsing
  // no octal escape sequences, no backslash-newline continuation :)
%}
\"      { temp.clear(); BEGIN(str); }
<str>{
  \"      { // closing quote
            BEGIN(INITIAL);
            yylval->stype = temp.c_str(); return STR; }

  \n       { /* error, unterminated string */ }
  \\[0-9]+ { /* error, unknown escape seq */ }

  \\n   { temp += '\n'; }
  \\t   { temp += '\t'; }
  \\r   { temp += '\r'; }
  \\b   { temp += '\b'; }
  \\f   { temp += '\f'; }

  \\.   { temp += yytext[1]; }

  [^\\\n\"]+  { temp += yytext; }
}

%{ // keywords
%}
texture      { return TEXTURE; }
sprite       { return SPRITE; }
data         { return DATA; }
worldsize    { return WORLDSIZE; }
scale        { return SCALE; }
texeloffsets { return TEXELOFFSETS; } 
offset       { return OFFSET; }


%{
// octals? can be confusing...
//0{D}+	     { yylval->itype = strtol(yytext, NULL, 8);  return INT; }
%}

0[xX]{H}+  { yylval->itype = strtol(yytext, NULL, 16); return INT; }
{D}+	   { yylval->itype = strtol(yytext, NULL, 10); return INT; }

{D}+{E}          |
{D}*"."{D}+{E}?	 |
{D}+"."{D}*{E}?	 { yylval->ftype = strtod(yytext, NULL); return FLOAT; }

%{ // unquoted string TODO should accept more chars
%}
{L}({L}|{D})*  { temp = yytext; yylval->stype = temp.c_str(); return STR; }

%{ // delimiters
%}
[;{}]  { return yytext[0]; }

{WHITESP}  { /* skip whitespace */ }

%%


static YY_BUFFER_STATE bufstate;

void ntexture_driver::scan_begin()
{
  // attach the scanner to the buffer
  bufstate = yy_scan_bytes(buffer, length);
  //yy_flex_debug = trace_scanning;
}
     
void ntexture_driver::scan_end()
{
  // free the scanner buffer
  yy_delete_buffer(bufstate);
}
