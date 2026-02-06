# Auxiliary Makefile for Doom Legacy Windows interface (win/)

i = ../../include

CFLAGS = $(CF) -I$(i) -Iwin   # win/ für rc und icon

# Deine bestehenden Objekte (falls welche da sind)
win_objects := $(subst $(objdir),../../$(objdir),$(win_objects))

objdir := ../../$(objdir)

# Neu: Resource-Objekt
RES_OBJ = $(objdir)/DL2_Icon.o

all: $(win_objects) $(RES_OBJ)

# Resource kompilieren (windres)
$(RES_OBJ): DL2_Icon.rc DL2_Icon.ico
	windres -Iwin -i DL2_Icon.rc -o $@

# Implizite Regel für normale .cpp (falls du später welche hinzufügst)
$(objdir)/%.o : %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

# Depend (falls du es nutzt)
depend: win.dep

win.dep: *.cpp
	$(CC) -MM $(CFLAGS) $^ > $@

-include win.dep

clean:
	rm -f $(win_objects) $(RES_OBJ) win.dep
