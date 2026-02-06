# Auxiliary Makefile for Doom Legacy engine
# Ville Bergholm 2002

i = ../include

CFLAGS = $(CF) -I$(i)

engine_objects := $(subst $(objdir),../$(objdir),$(engine_objects))

objdir := ../$(objdir)


all	: $(engine_objects)


.PHONY : depend

depend	: engine.dep

engine.dep :: *.cpp automap/*.cpp bots/*.cpp finale/*.cpp hud/*.cpp menu/*.cpp scripting/*.cpp

%.dep :
	$(CC) -MM $(CFLAGS) $^ > $(@F)


# implied rules, one for each subdirectory. sad but true.

$(objdir)/%.o	: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

$(objdir)/%.o	: automap/%.cpp
	$(CC) -c $(CFLAGS) $< -o $@

$(objdir)/%.o	: bots/%.cpp
	$(CC) -c $(CFLAGS) $< -o $@

$(objdir)/%.o	: finale/%.cpp
	$(CC) -c $(CFLAGS) $< -o $@

$(objdir)/%.o	: hud/%.cpp
	$(CC) -c $(CFLAGS) $< -o $@

$(objdir)/%.o	: menu/%.cpp
	$(CC) -c $(CFLAGS) $< -o $@

$(objdir)/%.o	: scripting/%.cpp
	$(CC) -c $(CFLAGS) $< -o $@


-include engine.dep
