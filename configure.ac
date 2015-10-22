#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([highway], [1.0.1], [embrace.ddd.flake.peace+highway@gmail.com])
AC_CONFIG_SRCDIR([src/highway.c])
AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_AUX_DIR([config])
AM_INIT_AUTOMAKE([no-define foreign subdir-objects])

# Checks for programs.
AC_PROG_CC

m4_include([ax_pthread.m4])
AX_PTHREAD

# Checks for libraries.
AC_CHECK_LIB([iconv], [iconv])
AC_CHECK_LIB([tcmalloc], [tc_malloc])
AC_CHECK_LIB([m], [log10])

LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS -Wall -std=gnu99"
CC="$PTHREAD_CC"

# Checks for header files.
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([fcntl.h inttypes.h locale.h malloc.h stddef.h stdint.h stdlib.h string.h strings.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T
AC_CHECK_TYPES([ptrdiff_t])
AC_CHECK_MEMBERS([struct dirent.d_namlen], , , [#include <dirent.h>])

# Checks for library functions.
AC_FUNC_REALLOC
AC_CHECK_FUNCS([atexit memchr memmove memset regcomp strstr])

if test "x$OS" = xWindows_NT; then
  LIBS="$LIBS -lshlwapi -liconv"
fi

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
