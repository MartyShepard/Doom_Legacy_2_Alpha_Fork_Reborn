# Gnu Make makefile for Doom Legacy
# Copyright (C) 2002-2014 by Doom Legacy Team.
#
# $Id: Makefile 1109 2014-05-17 17:38:05Z smite-meister $
#
# This primary Makefile calls auxiliary Makefiles in subdirectories.
# Use 'make OPT=1' to make optimized version, else you'll get debug info.

# Platform autodetect.

#UNAME = $(shell uname)

#ifeq ($(UNAME),Linux)
#export LINUX=1
#else
export WIN=1
#endif

# Debugging and optimization
ifdef OPT
CF = -O
else
CF = -g -O0
endif

# Dynamic or static linkage? I like static.
ifdef DYNAMIC
linkage = -DDYNAMIC_LINKAGE
else
linkage = 
endif

# Freeform defines
ifdef DEF
CF += -D$(DEF)
endif


# ----------- platform specific part begins
ifdef LINUX

# file removal utility
 RM = rm
# compiler
 platform  = -DLINUX
 interface = -DSDL $(shell sdl-config --cflags)
# linker
 LIBS	= $(shell sdl-config --libs) -lSDL_mixer -lpng -ljpeg -lz -ldl -L. -ltnl -ltomcrypt  # -lSDL_ttf
 OPENGLLIBS = -lGL -lGLU
 LDFLAGS = -Wall
# executable
 exename = doomlegacy2

else # assume WIN32 is defined

 exedir  = ../Build/
# file removal utility
 RM = rm
# compiler
 platform  = -D__WIN32__
 interface = -DSDL -I/mingw32/include/SDL -D_GNU_SOURCE=1  
#$(shell sdl-config --cflags)
 CF +=  -Wl,--subsystem,console

# linker
 #LIBS	= -lmingw32 -lSDL -lSDL_mixer -lmad -lSDLmain -lSDL  -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lversion -luuid -static-libgcc -lpng -ljpeg -lz -L. -ltnl -ltomcrypt -lwsock32 
 #OPENGLLIBS =-static -s  -lopengl32 -lglew32 -lglu32 -static-libgcc -lpthread -lzstd -lbcrypt -lz -lssp
 LDFLAGS = -Wall  -Wl,--subsystem,console
 LIB_DIR = ../LIBS/lib
 
 LIBS =   -lmingw32\
          $(LIB_DIR)SDL.a\
          $(LIB_DIR)SDL_mixer.a\
          $(LIB_DIR)mad.a\
          $(LIB_DIR)SDLmain.a\
          $(LIB_DIR)SDL.a\
          -lm\
          -ldinput8\
          -ldxguid\
          -ldxerr8\
          -luser32\
          -lgdi32\
          -lwinmm\
          -limm32\
          -lole32\
          -loleaut32\
          -lshell32\
          -lversion\
          -luuid\
          -static-libgcc\
          -lpng\
          -ljpeg\
          -lz\
          -L.\
          $(LIB_DIR)tnl.a\
          $(LIB_DIR)tomcrypt.a\
          -lwsock32\
          -static -s\
          -lopengl32\
          -lglew32\
          -lglu32\
          -static-libgcc\
          -lpthread\
          -lzstd\
          -lbcrypt\
          -lz\
          -lssp


# executable
 exename = $(exedir)doomlegacy2.exe

endif
# ----------- platform specific part ends

export RM
export LDFLAGS

# C++ compiler (usually g++)
export CC = gcc
export CXX = g++

# Defines.
#
# Automatic defines: Automatically defined by the compiler in the corresponding environment.
# __WIN32__, __WIN32, _WIN32, WIN32 : defined in Win32 environment
# __DJGPP__ : defined by DJGPP
# _MACOS_ : ???
# __OS2__ : ???
#
# Platform: use _exactly_ one of the following:
# LINUX : Linux platform
# __WIN32__ : Win32 platform (automatic, no need to define)
#
# Multimedia interface: use ONLY _one_ of the following:
# SDL : compile the SDL version of Legacy (use SDL for multimedia interface, SDL_mixer for music)
#
# Miscellaneous options: use as many as you like
# NO_SHADERS : do not include support for GLSL shaders in the build
# NO_MIXER   : do not include SDL_mixer in the build


#export CF += -std=c++0x -Wall $(platform) $(interface) $(linkage)
export CF += -std=gnu++11 -Wall $(platform) $(interface) $(linkage)



INCLUDES = -Iinclude
CFLAGS = $(CF) $(INCLUDES)

# linker
export LD = $(CXX)

export objdir = objs


export engine_objects = \
	$(objdir)/g_game.o \
	$(objdir)/g_state.o \
	$(objdir)/g_demo.o \
	$(objdir)/g_input.o \
	$(objdir)/g_type.o \
	$(objdir)/g_level.o \
	$(objdir)/g_mapinfo.o \
	$(objdir)/g_map.o \
	$(objdir)/g_player.o \
	$(objdir)/g_team.o \
	$(objdir)/g_think.o \
	$(objdir)/g_actor.o \
	$(objdir)/g_pawn.o \
	$(objdir)/g_decorate.o \
	$(objdir)/p_tick.o \
	$(objdir)/p_setup.o \
	$(objdir)/p_saveg.o \
	$(objdir)/p_effects.o \
	$(objdir)/p_spec.o \
	$(objdir)/p_events.o \
	$(objdir)/p_floor.o \
	$(objdir)/p_plats.o \
	$(objdir)/p_ceilng.o \
	$(objdir)/p_doors.o \
	$(objdir)/p_genlin.o \
	$(objdir)/p_things.o \
	$(objdir)/p_lights.o \
	$(objdir)/p_switch.o \
	$(objdir)/p_polyobj.o \
	$(objdir)/a_action.o \
	$(objdir)/p_pspr.o \
	$(objdir)/p_hpspr.o \
	$(objdir)/p_xpspr.o \
	$(objdir)/p_enemy.o \
	$(objdir)/p_henemy.o \
	$(objdir)/p_xenemy.o \
	$(objdir)/ai_mobjinfo.o \
	$(objdir)/ai_states.o \
	$(objdir)/acs.o \
	$(objdir)/t_oper.o \
	$(objdir)/t_parse.o \
	$(objdir)/t_prepro.o \
	$(objdir)/t_spec.o \
	$(objdir)/t_vari.o \
	$(objdir)/t_script.o \
	$(objdir)/t_func.o \
	$(objdir)/p_map.o \
	$(objdir)/p_maputl.o \
	$(objdir)/p_sight.o \
	$(objdir)/p_telept.o \
	$(objdir)/p_camera.o \
	$(objdir)/p_user.o \
	$(objdir)/p_inter.o \
	$(objdir)/am_map.o \
	$(objdir)/menu.o \
	$(objdir)/f_finale.o \
	$(objdir)/wi_stuff.o \
	$(objdir)/st_lib.o \
	$(objdir)/st_stuff.o \
	$(objdir)/hu_stuff.o \
	$(objdir)/m_cheat.o \
	$(objdir)/p_fab.o \
	$(objdir)/p_hacks.o \
	$(objdir)/d_items.o \
	$(objdir)/d_main.o \
	$(objdir)/dstrings.o \
	$(objdir)/acbot.o \
	$(objdir)/b_bot.o \
	$(objdir)/b_path.o

export util_objects = \
	$(objdir)/command.o \
	$(objdir)/console.o \
	$(objdir)/dehacked.o \
	$(objdir)/mnemonics.o \
	$(objdir)/m_argv.o \
	$(objdir)/m_archive.o \
	$(objdir)/m_bbox.o \
	$(objdir)/m_dll.o \
	$(objdir)/m_fixed.o \
	$(objdir)/m_misc.o \
	$(objdir)/m_random.o \
	$(objdir)/m_swap.o \
	$(objdir)/md5.o \
	$(objdir)/parser.o \
	$(objdir)/tables.o \
	$(objdir)/vfile.o \
	$(objdir)/wad.o \
	$(objdir)/w_wad.o \
	$(objdir)/z_cache.o \
	$(objdir)/z_zone.o

export audio_objects = \
	$(objdir)/qmus2mid.o \
	$(objdir)/s_sound.o \
	$(objdir)/s_sndseq.o \
	$(objdir)/sounds.o

export video_objects = \
	$(objdir)/md3.o \
	$(objdir)/png.o \
	$(objdir)/jpeg.o \
	$(objdir)/screen.o \
	$(objdir)/v_video.o \
	$(objdir)/r_font.o \
	$(objdir)/r_render.o \
	$(objdir)/r_bsp.o \
	$(objdir)/r_draw.o \
	$(objdir)/r_draw8.o \
	$(objdir)/r_draw16.o \
	$(objdir)/r_main.o \
	$(objdir)/r_plane.o \
	$(objdir)/r_segs.o \
	$(objdir)/r_sky.o \
	$(objdir)/r_splats.o \
	$(objdir)/r_sprite.o \
	$(objdir)/r_things.o \
	$(objdir)/r_anim.o \
	$(objdir)/oglrenderer.o \
	$(objdir)/oglshaders.o \
	$(objdir)/r_data.o \
	$(objdir)/oglhelpers.o

export net_objects = \
	$(objdir)/n_interface.o \
	$(objdir)/n_connection.o \
	$(objdir)/sv_main.o \
	$(objdir)/sv_cmds.o \
	$(objdir)/cl_main.o

export sdl_objects = \
	$(objdir)/endtxt.o \
	$(objdir)/i_cdmus.o \
	$(objdir)/i_main.o \
	$(objdir)/i_net.o \
	$(objdir)/i_sound.o \
	$(objdir)/i_system.o \
	$(objdir)/i_video.o

export win_objects = \
	$(objdir)/DL2_Icon.o \
  
export grammar_objects = \
	$(objdir)/ntexture.parser.o \
	$(objdir)/ntexture.lexer.o \
	$(objdir)/decorate.parser.o \
	$(objdir)/decorate.lexer.o \
	$(objdir)/parser_driver.o


objects = $(engine_objects) $(util_objects) $(audio_objects) $(grammar_objects) $(video_objects) \
	$(net_objects) $(sdl_objects) $(win_objects)

SUBDIRS = engine util audio video net grammars tools

RES_OBJ = win/$(objdir)/DL2_Icon.o   # Pfad anpassen

# explicit rules
.PHONY	: all mkdirobjs clean depend docs wad versionstring tnl sdl win $(SUBDIRS)


all	: mkdirobjs $(exename)

mkdirobjs:
	mkdir -p objs

clean	:
	$(RM) $(objects)

$(SUBDIRS):
	$(MAKE) -C $@

depend:
	$(MAKE) -C engine depend
	$(MAKE) -C video depend
	$(MAKE) -C audio depend
	$(MAKE) -C util depend
	$(MAKE) -C net depend
	$(MAKE) -C grammars depend
	$(MAKE) -C interface/sdl depend
	$(MAKE) -C interface/win depend  
	$(MAKE) -C tools depend

docs	: Doxyfile
	doxygen

wad	: tools
	$(MAKE) -C wad

sdl	:
	$(MAKE) -C interface/sdl

win	:
	$(MAKE) -C interface/win
  
versionstring:
	$(CC) -c $(CFLAGS) -DSVN_REV=\"`svn info | grep Revision | sed -e 's/Revision: //'`\" engine/d_main.cpp -o objs/d_main.o



# TNL
ifdef TNL
tnl	:
	@echo "Building TNL using source tree at $(TNL)..."
	ln -sf $(TNL)/tnl include
	patch -d $(TNL) -p0 < libtnl_patch.diff
	make -C $(TNL)/libtomcrypt
	make -C $(TNL)/tnl
	mv $(TNL)/tnl/libtnl.a .
	mv $(TNL)/libtomcrypt/libtomcrypt.a .
	@echo "Finished building TNL."
else
tnl	:
	@echo "Usage: make tnl TNL=path_to_tnl_source"
endif


ifdef DYNAMIC
# main program
$(exename) : engine util audio video net sdl win grammars versionstring
	$(LD) $(LDFLAGS) $(objects) $(LIBS) -o $@
else
# all in one
$(exename) : engine util audio video net sdl win grammars versionstring
	$(LD) $(LDFLAGS) $(objects) $(LIBS) $(OPENGLLIBS) -o $@
endif
