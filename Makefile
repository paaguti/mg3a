# Makefile for Mg 3a

# These are defaults, change them in your external build tool
# * in Linux, you can use pkg-config to customize ncurses libs and includes
# * in Cygwin, set CC=gcc
# * in xBSD or Darwin, set CC=clang

CC=cc
COPT=-O3
LD=cc
LIBS=$(shell ncursesw6-config --libs)
ifeq ($(LIBS),)
	LIBS=-lcurses
	# Try if -lncurses doesn't work
else
	COPT += $(shell ncursesw6-config --cflags)
endif
# For install entries
PREFIX ?= /usr/local

# Current supported compile-time options:
#
#	NO_TERMH	-- If you don't have <term.h> (or on Cygwin <ncurses/term.h>) you
#			   can use this. You then don't get keypad keys defined.
#	DIRED		-- enable "dired".
#	PIPEIN		-- enable reading from pipe at startup
#	PREFIXREGION	-- enable function "prefix-region"
#	CHARSDEBUG	-- A debugging tool for input/output
#	SLOW		-- Implement "slow-mode" command for emulating a slow terminal
#			   Another debugging tool.
#	SEARCHALL	-- Enable "search-all-forward" and "search-all-backward" commands.
#	LANGMODE_C	-- Implements a "c-mode" like OpenBSD mg.
#	LANGMODE_PYTHON	-- Implements a "python-mode" written by Pedro A. Aranda.
#	LANGMODE_CLIKE  -- Include modes for C-like languages, C and Perl.
#	UCSNAMES	-- Include functions for dealing with Unicode character names.
#	USER_MODES	-- Include commands for creating and deleting user-defined modes.
#	USER_MACROS	-- Include commands for creating and listing named macros.
#	ALL		-- Include everything above except NO_TERMH.
#	NO_CMODE	-- With -DALL, disable LANGMODE_C
#	NO_UCSN		-- With -DALL, disable USCNAMES
#	NO_PIPEIN	-- With -DALL, disable PIPEIN

# Minimal, with debug
#CDEFS	= -DCHARSDEBUG -DSLOW

# Turn on "bsmap-mode" mode by default, make backups by default
#CDEFS	= -DBSMAP=1 -DMAKEBACKUP=1

# Everything GNU Emacs and mg2a-like by default
# CDEFS = -DDIRED -DPREFIXREGION

# Advanced mg3a with python and clike mode,
# no c-mode, nor UCS_NAMES, nor simple search
CDEFS = -DDIRED -DPREFIXREGION -DSEARCHALL \
    -DLANGMODE_PYTHON -DLANGMODE_CLIKE \
    -DUSER_MODES -DUSER_MACROS

# Everything.
#CDEFS	= -DALL

CFLAGS	= $(COPT) $(CDEFS) -DDIST_VERSION=\"191107\"

# Objects which only depend on the "standard" includes
OBJS	= basic.o dir.o dired.o file.o line.o match.o paragraph.o \
	  random.o region.o search.o ucs.o util.o variables.o version.o \
	  width.o window.o word.o langmode_c.o langmode_python.o \
	  langmode_clike.o ucsnames.o undo.o

# Those with unique requirements
IND	= buffer.o display.o echo.o extend.o help.o kbd.o keymap.o \
	  macro.o main.o modes.o

# System dependent objects
OOBJS = cinfo.o spawn.o ttyio.o tty.o ttykbd.o

OBJ = $(OBJS) $(IND) $(OOBJS) fileio.o

OSRCS	= cinfo.c fileio.c spawn.c ttyio.c tty.c ttykbd.c
SRCS	= basic.c dir.c dired.c file.c line.c match.c paragraph.c \
	  random.c region.c search.c version.c window.c word.c \
	  buffer.c display.c echo.c extend.c help.c kbd.c keymap.c \
	  macro.c main.c modes.c ucs.c util.c variables.c width.c \
	  langmode_c.c langmode_python.c langmode_clike.c \
	  ucsnames.c undo.c

OINCS =	ttydef.h sysdef.h chrdef.h cpextern.h
INCS =	def.h

# The 'all' target is needed for BSD ports and handy for the rest

all:
	$(MAKE) CDEFS="-DALL -DNO_CMODE -DNO_UCSN" mg strip

full:
	$(MAKE) CDEFS="-DALL" mg strip

mg:	$(OBJ)
	$(LD) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)


strip: mg
	strip mg

# Install entries for Linux/BSD/OSX by Pedro A. Aranda

install-bin: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install mg $(DESTDIR)$(PREFIX)/bin

install-doc:
	install -d $(DESTDIR)$(PREFIX)/share/doc/mg3a/orig
	install README* $(DESTDIR)$(PREFIX)/share/doc/mg3a
	install orig/README $(DESTDIR)$(PREFIX)/share/doc/mg3a/orig

install-data:
	install -d $(DESTDIR)$(PREFIX)/share/mg3a
	install bl/* $(DESTDIR)$(PREFIX)/share/mg3a

install: install-bin install-doc install-data

# end install entries

$(OBJ):		$(INCS) $(OINCS)


dir.o search.o:	$(INCS) $(OINCS)

kbd.o:	$(INCS) $(OINCS) macro.h kbd.h key.h

macro.o main.o:	$(INCS) $(OINCS) macro.h

buffer.o display.o keymap.o help.o modes.o dired.o fileio.o: \
	$(INCS) $(OINCS) kbd.h

extend.o:	$(INCS) $(OINCS) kbd.h macro.h key.h

help.o:	$(INCS) $(OINCS) kbd.h key.h macro.h

echo.o:	$(INCS) $(OINCS) key.h macro.h

$(OOBJS):	$(INCS) $(OINCS)


clean:
	rm -f mg *.o *.s *.stackdump core

obj: $(OBJ)

display.o: cpall.h

ttyio.o: slowputc.h

# compatible with old make syntax

.c.o:
	$(CC) $(CFLAGS) -c $<

.c.s:
	$(CC) $(CFLAGS) -S $<
