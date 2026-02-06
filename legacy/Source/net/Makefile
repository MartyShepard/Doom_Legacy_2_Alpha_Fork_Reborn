# Auxiliary Makefile for Doom Legacy net
# Ville Bergholm 2002

i = ../include

CFLAGS = $(CF) -I$(i)

net_objects := $(subst $(objdir),../$(objdir),$(net_objects))

objdir := ../$(objdir)


all	: $(net_objects)

.PHONY : depend

depend : net.dep

net.dep :: *.cpp

%.dep :
	$(CC) -MM $(CFLAGS) $^ > $(@F)


# implied rules

$(objdir)/%.o	: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@


-include net.dep
