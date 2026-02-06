# Gnu Make makefile for Doom Legacy
# Copyright (C) 2003-2004 by DooM Legacy Team.
#
# Auxiliary Makefile for Doom Legacy tools
#

i = ../include

CFLAGS = $(CF) -I$(i)

objdir := ../$(objdir)

all	: ../wad/d2h.exe ../wad/wadtool.exe ../wad/convert_deh.exe


../wad/d2h.exe: $(objdir)/d2h.o
	$(LD) $(LDFLAGS) $^ -o $@

../wad/wadtool.exe: $(objdir)/wadtool.o $(objdir)/md5.o
	$(LD) $(LDFLAGS) $^ -o $@

../wad/convert_deh.exe: $(objdir)/convert_deh.o $(objdir)/mnemonics.o
	$(LD) $(LDFLAGS) $^ -o $@


depend	:
	$(CC) -MM $(CFLAGS) *.cpp | sed -e 's/^\([^ ]\)/$$(objdir)\/\1/' > tools.dep


# implied rules

$(objdir)/%.o	: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@


include tools.dep
