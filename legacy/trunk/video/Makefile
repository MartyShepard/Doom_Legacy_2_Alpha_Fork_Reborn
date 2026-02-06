# Auxiliary Makefile for Doom Legacy video
# Ville Bergholm 2002

i = ../include

CFLAGS = $(CF) -I$(i)

video_objects := $(subst $(objdir),../$(objdir),$(video_objects))

objdir := ../$(objdir)


all	: $(video_objects)

.PHONY : depend

depend : video.dep

video.dep :: *.cpp hardware/*.cpp

%.dep :
	$(CC) -MM $(CFLAGS) $^ > $(@F)


# implied rules

$(objdir)/%.o	: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

$(objdir)/%.o	: hardware/%.cpp
	$(CC) -c $(CFLAGS) $< -o $@


-include video.dep
