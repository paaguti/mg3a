#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([mg3a], [191206], [paaguti@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([display.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

# Checks for libraries.
AC_CHECK_LIB([ncurses], [baudrate])
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
                   [Enable dired @<:@default=yes@:>@])],
    [enable_dired="${enableval:-yes}"],
    [enable_dired=yes])
AS_IF([ test "x$enable_dired" == "xyes" ],
		[AM_CPPFLAGS="$AM_CPPFLAGS -DDIRED"],[true])

enable_pipein=yes
AC_ARG_ENABLE([_pipein=yes],
   AS_HELP_STRING([--enable-_pipein=yes],
                  [Enable read from pipe at startup @<:@default=yes@:>@])],
   [enable_pipein=yes="${enableval:-yes}"],
   [enable_pipein=yes=])
AS_IF([ test "x$enable_pipein" == "xyes" ],
		[AM_CPPFLAGS="$AM_CPPFLAGS -DPIPEIN"],[true])

enable_prefixregion=yes
AC_ARG_ENABLE([prefixregion],
   AS_HELP_STRING([--enable-prefixregion],
                  [Enable prefix region @<:@default=yes@:>@])],
   [enable_prefixregion="${enableval:-yes}"],
   [enable_prefixregion=yes])
AS_IF([ test "x$enable_prefixregion" == "xyes" ],
		[AM_CPPFLAGS="$AM_CPPFLAGS -DPREFIXREGION"],[true])

enable_slow=no
AC_ARG_ENABLE([slow],
   AS_HELP_STRING([--enable-slow],
                  [Enable slow terminal emulation@<:@default=no@:>@])],
   [enable_slow="${enableval:-yes}"],
   [enable_slow=no])
AS_IF([ test "x$enable_slow" == "xyes" ],
		[AM_CPPFLAGS="$AM_CPPFLAGS -DSLOW"],[true])

enable_searchall=yes
AC_ARG_ENABLE([searchall],
   AS_HELP_STRING([--enable-searchall],
                  [Enable search-all commands @<:@default=yes@:>@])],
   [enable_searchall="${enableval:-yes}"],
   [enable_searchall=yes])
AS_IF([ test "x$enable_searchall" == "xyes" ],
		[AM_CPPFLAGS="$AM_CPPFLAGS -DSEARCHALL"],[true])

enable_pythonmode=yes
AC_ARG_ENABLE([pythonmode],
   AS_HELP_STRING([--enable-pythonmode],
                  [Enable Python mode @<:@default=yes@:>@])],
   [enable_pythonmode="${enableval:-yes}"],
   [enable_pythonmode=yes])
AS_IF([ test "x$enable_pythonmode" == "xyes"],
		[AM_CPPFLAGS="$AM_CPPFLAGS -DLANGMODE_PYTHON"],[true])

enable_cmode=no
AC_ARG_ENABLE([cmode],
    AS_HELP_STRING([--enable-cmode],
                   [Enable old C mode @<:@default=no@:>@])],
    [enable_cmode="${enableval:-yes}"],
    [enable_cmode=no])
AS_IF([ test "x$enable_cmode" == "xyes"],
		[AM_CPPFLAGS="$AM_CPPFLAGS -DLANGMODE_C"],[true])

enable_clikemode=yes
AC_ARG_ENABLE([clikemode],
    AS_HELP_STRING([--enable-clikemode],
                   [Enable C-like modes @<:@default=yes@:>@])],
    [enable_clikemode="${enableval:-yes}"],
    [enable_clikemode=yes])
AS_IF([ test "x$enable_clikemode" == "xyes"],
		[AM_CPPFLAGS="$AM_CPPFLAGS -DLANGMODE_CLIKE"],[true])

enable_usermodes=yes
AC_ARG_ENABLE([usermodes],
    AS_HELP_STRING([--enable-usermodes],
                   [Enable user modes @<:@default=yes@:>@])],
    [enable_usermodes="${enableval:-yes}"],
    [enable_usermodes=yes])
AS_IF([ test "x$enable_usermodes" == "xyes"],
		[AM_CPPFLAGS="$AM_CPPFLAGS -DUSER_MODES"],[true])

enable_usermacros=yes
AC_ARG_ENABLE([usermacros],
    AS_HELP_STRING([--enable-usermacros],
                   [Enable user macros @<:@default=yes@:>@])],
    [enable_usermacros="${enableval:-yes}"],
    [enable_usermacros=yes])
AS_IF([ test "x$enable_usermacros" == "xyes"],
		[AM_CPPFLAGS="$AM_CPPFLAGS -DUSER_MACROS"],[true])

enable_ucsnames=no
AC_ARG_ENABLE([ucsnames],
    AS_HELP_STRING([--enable-ucsnames],
                   [Enable Unicode character names @<:@default=no@:>@])],
    [enable_ucsnames="${enableval:-yes}"],
    [enable_ucsnames=no])
AS_IF([ test "x$enable_ucsnames" == "xyes"],
		[AM_CPPFLAGS="$AM_CPPFLAGS -DUCSNAMES"],[true])

AC_ARG_ENABLE([all],
            AS_HELP_STRING([--enable-all],
              [Enable all features @<:@default=no@:>@])],
			[enable_all="${enableval:-yes}"],
            [enable_all=no])
AS_IF(
	[test "x$enable_all" == "xyes"],[AM_CPPFLAGS="-DALL"],[true]
)



AC_SUBST([AM_CPPFLAGS])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
