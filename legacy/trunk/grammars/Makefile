# Auxiliary Makefile for Doom Legacy grammars
# Ville Bergholm 2005-2007

i = ../include

CFLAGS = $(CF) -I$(i) -I.

grammar_objects := $(subst $(objdir),../$(objdir),$(grammar_objects))

objdir := ../$(objdir)


all	: $(grammar_objects)

.PHONY : depend

depend : grammars.dep

grammars.dep :: *.cpp

%.dep :
	$(CC) -MM $(CFLAGS) $^ > $(@F)

lemon	: lemon.c
	gcc $< -o $@

# lexer rules

$(objdir)/%.lexer.o: %.lexer.c parser_driver.h %.parser.h
	$(CXX) -c $(CFLAGS) $< -o $@

ntexture.lexer.c: ntexture.lexer.flex
	flex -PNTEXTURE_ $<

decorate.lexer.c: decorate.lexer.flex
	flex -PDECORATE_ $<


# rule for parsers and token definition headers

$(objdir)/%.parser.o: %.parser.c parser_driver.h %.parser.h
	$(CXX) -c $(CFLAGS) $< -o $@

%.parser.c %.parser.h: %.parser.y lemon
	./lemon $<

$(objdir)/%.o: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

-include grammars.dep
