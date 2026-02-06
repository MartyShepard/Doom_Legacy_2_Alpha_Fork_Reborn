# Gnu Make makefile for Doom Legacy
# Copyright (C) 2003-2004 by DooM Legacy Team.
#
# Auxiliary Makefile for Doom Legacy tools
#

i = ../include

CFLAGS = $(CF) -I$(i)

objdir := ../objs
subdir = ../objs

 LIBS =  -static-libgcc\
          -static -s\
          -static-libgcc\
          -lpthread
          
all	: ../wad/d2h.exe ../wad/wadtool.exe ../wad/convert_deh.exe


../wad/d2h.exe: $(objdir)/d2h.o
	$(LD) $(LDFLAGS) $(LIBS) $^ -o $@

../wad/wadtool.exe: $(subdir)/wadtool.o $(subdir)/md5.o
	$(LD) $(LDFLAGS) $(LIBS) $^ -o $@

../wad/convert_deh.exe: $(objdir)/convert_deh.o $(subdir)/mnemonics.o
	$(LD) $(LDFLAGS) $(LIBS) $^ -o $@


depend	:
	$(CC) -MM $(CFLAGS) *.cpp | sed -e 's/^\([^ ]\)/$$(objdir)\/\1/' > tools.dep


# implied rules

$(objdir)/%.o	: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@


#include tools.dep
