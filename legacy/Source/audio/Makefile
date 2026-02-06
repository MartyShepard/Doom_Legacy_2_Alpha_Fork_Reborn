# Auxiliary Makefile for Doom Legacy audio
# Ville Bergholm 2002

i = ../include

CFLAGS = $(CF) -I$(i)

audio_objects := $(subst $(objdir),../$(objdir),$(audio_objects))

objdir := ../$(objdir)


all	: $(audio_objects)

.PHONY : depend

depend : audio.dep

audio.dep :: *.cpp

%.dep :
	$(CC) -MM $(CFLAGS) $^ > $(@F)


# implied rules

$(objdir)/%.o	: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@


-include audio.dep
