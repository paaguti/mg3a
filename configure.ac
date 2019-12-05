#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([mg3a], [191205], [paaguti@gmail.com])
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

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
