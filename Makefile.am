AM_CPPFLAGS += -DDIST_VERSION=\"191206\"
AM_CFLAGS = 
AM_LDFLAGS =


OSRCS	= cinfo.c fileio.c spawn.c ttyio.c tty.c ttykbd.c
SRCS	= basic.c dir.c dired.c file.c line.c match.c paragraph.c \
	  random.c region.c search.c version.c window.c word.c \
	  buffer.c display.c echo.c extend.c help.c kbd.c keymap.c \
	  macro.c main.c modes.c ucs.c util.c variables.c width.c \
	  langmode_c.c langmode_python.c langmode_clike.c \
	  ucsnames.c undo.c

OINCS =	ttydef.h sysdef.h chrdef.h cpextern.h
INCS =	def.h

mg_SOURCES = $(OSRCS) $(SRCS) $(OINCS) $(INCS)
bin_PROGRAMS = mg

docdir = $(datadir)/doc/@PACKAGE@
doc_DATA = README README.debian README.programmer README.ucsnames \
	README.auto-execute README.dired README.python-mode README.undo \
	README.bindings README.fill-paragraph README.reference README.user_macros \
	README.bsd README.md README.shell-command README.user_modes \
	README.clike-mode README.misc README.todo
