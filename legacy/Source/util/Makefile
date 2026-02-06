# Auxiliary Makefile for Doom Legacy utils
# Ville Bergholm 2002

i = ../include

CFLAGS = $(CF) -I$(i)

util_objects := $(subst $(objdir),../$(objdir),$(util_objects))

objdir := ../$(objdir)


all	: $(util_objects)

.PHONY : depend

depend : util.dep

util.dep :: *.cpp

%.dep :
	$(CC) -MM $(CFLAGS) $^ > $(@F)


# implied rules

$(objdir)/%.o	: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@


-include util.dep
