#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([mg3a],[20250111],[paaguti@gmail.com])
AM_INIT_AUTOMAKE
dnl AC_CONFIG_SRCDIR([display.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

# Checks for libraries.

AC_SEARCH_LIBS([beep],[ncursesw ncurses],[],
    [ AC_MSG_ERROR([unable to find beep in NCURSES])]
)

# FIXME: Replace `main' with a function in `-ltermcap':
# AC_CHECK_LIB([termcap], [main])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h langinfo.h limits.h locale.h sgtty.h stddef.h stdint.h stdlib.h string.h sys/file.h sys/ioctl.h sys/time.h termio.h termios.h unistd.h wchar.h wctype.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_FORK
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([dup2 getcwd gettimeofday memchr memmove mkdir nl_langinfo realpath rmdir select setlocale strchr strerror strrchr strstr strtol wcwidth])

AM_CPPFLAGS=""

AC_ARG_ENABLE([dired],
    AS_HELP_STRING([--enable-dired],
                   [ Enable dired @<:@default=yes@:>@ ])],
    [enable_dired="${enableval:-yes}"],
    [enable_dired=yes])
AS_IF([ test "x$enable_dired" == "xyes" ],
   [AC_DEFINE([DIRED], [1], [Enable dired.])]
)

AC_ARG_ENABLE([pipein],
   AS_HELP_STRING([--enable-pipein],
                  [ Enable read from pipe at startup @<:@default=yes@:>@ ])],
   [enable_pipein=yes"${enableval:-yes}"],
   [enable_pipein=yes])
AS_IF([ test "x$enable_pipein" == "xyes" ],
   [AC_DEFINE([PIPEIN], [1], [Enable input from pipe.])]
)

AC_ARG_ENABLE([prefixregion],
   AS_HELP_STRING([--enable-prefixregion],
      [ Enable prefix region @<:@default=yes@:>@ ])],
   [enable_prefixregion="${enableval:-yes}"],
   [enable_prefixregion=yes])
AS_IF([ test "x$enable_prefixregion" == "xyes" ],
   [AC_DEFINE([PREFIXREGION], [1], [Enable prefix region.])]
)

AC_ARG_ENABLE([slow],
   AS_HELP_STRING([--enable-slow],
      [ Enable slow terminal emulation@<:@default=no@:>@ ])],
   [enable_slow="${enableval:-yes}"],
   [enable_slow=no])
AS_IF([ test "x$enable_slow" == "xyes" ],
   [AC_DEFINE([SLOW], [1], [Enable slow terminal emulation.])]
)

AC_ARG_ENABLE([mouse],
   AS_HELP_STRING([--enable-mouse],
      [ Enable mouse@<:@default=no@:>@ ])],
   [enable_mouse="${enableval:-yes}"],
   [enable_mouse=yes])
AS_IF([ test "x$enable_mouse" == "xyes" ],
   [AC_DEFINE([MOUSE], [1], [Enable mouse.])]
)

AC_ARG_ENABLE([pythonmode],
   AS_HELP_STRING([--enable-pythonmode],
	    [ Enable Python mode @<:@default=yes@:>@ ])],
   [enable_pythonmode="${enableval:-yes}"],
   [enable_pythonmode=yes])
AS_IF([ test "x$enable_pythonmode" == "xyes"],
   [AC_DEFINE([LANGMODE_PYTHON], [1], [Enable Python mode.])]
)

AC_ARG_ENABLE([makemode],
   AS_HELP_STRING([--enable-makemode],
	    [ Enable Makefile mode @<:@default=no@:>@ ])],
   [enable_makemode="${enableval:-yes}"],
   [enable_makemode=no])
AS_IF([ test "x$enable_makemode" == "xyes"],
   [AC_DEFINE([LANGMODE_MAKE], [1], [Enable Makefile mode.])]
)

AC_ARG_ENABLE([cmode],
    AS_HELP_STRING([--enable-cmode],
		[ Enable old C mode @<:@default=no@:>@ ])],
    [enable_cmode="${enableval:-yes}"],
    [enable_cmode=no])
AS_IF([ test "x$enable_cmode" == "xyes"],
    [AC_DEFINE([LANGMODE_C], [1], [Enable C mode.])]
)

AC_ARG_ENABLE([clikemode],
    AS_HELP_STRING([--enable-clikemode],
		[ Enable C-like modes @<:@default=yes@:>@ ])],
    [enable_clikemode="${enableval:-yes}"],
    [enable_clikemode=yes])
AS_IF([ test "x$enable_clikemode" == "xyes"],
    [AC_DEFINE([LANGMODE_CLIKE], [1], [Enable C-like mode.])]
)

AC_ARG_ENABLE([usermodes],
    AS_HELP_STRING([--enable-usermodes],
	    [ Enable user modes @<:@default=yes@:>@ ])],
    [enable_usermodes="${enableval:-yes}"],
    [enable_usermodes=yes])
AS_IF([ test "x$enable_usermodes" == "xyes"],
    [AC_DEFINE([USER_MODES], [1], [Enable user modes.])]
)

AC_ARG_ENABLE([usermacros],
    AS_HELP_STRING([--enable-usermacros],
        [ Enable user macros @<:@default=yes@:>@ ])],
    [enable_usermacros="${enableval:-yes}"],
    [enable_usermacros=yes])
AS_IF([ test "x$enable_usermacros" == "xyes"],
    [AC_DEFINE([USER_MACROS], [1], [Enable user macros.])]
)
AC_ARG_ENABLE([ucsnames],
    AS_HELP_STRING([--enable-ucsnames],
        [ Enable Unicode character names @<:@default=no@:>@ ])],
    [enable_ucsnames="${enableval:-yes}"],
    [enable_ucsnames=no])
AS_IF([ test "x$enable_ucsnames" == "xyes"],
    [AC_DEFINE([UCSNAMES], [1], [Enable end search with Enter key.])]
)

AC_ARG_ENABLE([all],
    AS_HELP_STRING([--enable-all],
       [ Enable all features @<:@default=no@:>@ ])],
	[enable_all="${enableval:-yes}"],
    [enable_all=no])
AS_IF(
	[ test "x$enable_all" == "xyes" ],
	[AC_DEFINE([ALL], [1], [Enable all options.])]
)

AC_ARG_ENABLE([searchall],
   AS_HELP_STRING([--enable-searchall],
      [ Enable search-all commands @<:@default=yes@:>@ ])],
   [enable_searchall="${enableval:-yes}"],
   [enable_searchall=yes])
AS_IF(
	[ test "x$enable_searchall" == "xyes" ],
	[AC_DEFINE([SEARCHALL], [1], [Enable search all commands.])]
)

AC_ARG_ENABLE([searchsimple],
   AS_HELP_STRING([--enable-searchsimple],
      [ Enable search-all commands @<:@default=no@:>@ ])],
   [enable_searchsimple="${enableval:-yes}"],
   [enable_searchsimple=no])
AS_IF(
	[ test "x$enable_searchsimple" == "xyes" ],
	[AC_DEFINE([SEARCHSIMPLE], [1], [Enable simple search commands.])]
)

AC_ARG_ENABLE([searchenter],
   AS_HELP_STRING([--enable-searchenter],
      [ Enable end search with Enter key @<:@default=no@:>@ ])],
   [enable_searchenter="${enableval:-yes}"],
   [enable_searchenter=no])
AS_IF(
	[ test "x$enable_searchenter" == "xyes" ],
	[AC_DEFINE([SEARCHENTER], [1], [Enable end search with Enter key.])]
)

AC_ARG_ENABLE([noobjs],
	AS_HELP_STRING([--enable-no-objs],
		[Enable supressing .o files in dired @<:@default=no@:>@])],
	[enable_noobjs="${enableval:-yes}"],
	[enable_noobjs=no])
AS_IF(
	[test "x$enable_noobjs" == "xyes"],
	[AC_DEFINE([NO_O], [1], [Define to suppress .o files in dired])]
)

AC_ARG_ENABLE([backups],
	AS_HELP_STRING([--enable-backups],
		[Enable automatic backups @<:@default=no@:>@])],
	[enable_backups="${enableval:-yes}"],
	[enable_backups=no])
AS_IF(
	[test "x$enable_backups" == "xyes"],
	[AC_DEFINE([MAKEBACKUP], [1], [Enable automatic backups])]
)

AC_ARG_ENABLE([minidired],
    AS_HELP_STRING([--enable-minidired],
                   [Enable minidired @<:@default=no@:>@])],
    [enable_minidired="${enableval:-yes}"],
    [enable_minidired=no])
AS_IF(
    [test "x$minidired" == "xyes"],
        [AC_DEFINE([MINIDIRED], [1], [Enable minidired mode.])]
)

termh=no
AC_CHECK_HEADERS([term.h], [termh=yes])
AS_IF([test "x$termh" != "xyes"],
      [AC_DEFINE([NO_TERMH], [1], [Define if there is no term.h.])])

dnl Just to silence the debian/rules warning

AC_ARG_ENABLE([maintainer-mode],
    AS_HELP_STRING([--enable-maintainer-mode],
                   [Enable maintainer-mode @<:@default=no@:>@])],
    [enable_maintainer_mode="${enableval:-yes}"],
    [enable_maintainer_mode=no])

AC_SUBST([AM_CPPFLAGS])
AC_CONFIG_FILES([Makefile bl/Makefile orig/Makefile])
AC_OUTPUT
