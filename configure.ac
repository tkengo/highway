#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([highway], [1.0.0], [embrace.ddd.flake.peace+highway@gmail.com])
AC_CONFIG_SRCDIR([src/highway.c])
AC_CONFIG_HEADERS([include/config.h])
AM_INIT_AUTOMAKE([no-define foreign subdir-objects])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
# FIXME: Replace `main' with a function in `-liconv':
AC_CHECK_LIB([iconv], [main])
# FIXME: Replace `main' with a function in `-ltcmalloc':
AC_CHECK_LIB([tcmalloc], [main])
CFLAGS="-std=c99 -O2"

# Checks for header files.
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([fcntl.h inttypes.h locale.h malloc.h stddef.h stdint.h stdlib.h string.h strings.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_REALLOC
AC_CHECK_FUNCS([atexit memchr memmove memset regcomp strstr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
