#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([libhalog], [1.0], [likai.root@gmail.com])
AM_INIT_AUTOMAKE
#AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
LT_INIT
AC_LANG([C++])

CXXFLAGS="-g -std=c++0x -fPIC -finline-functions -O2 \
					-Wall -Werror -Wextra -Wunused-parameter -Wformat -Wconversion \
					-fno-strict-aliasing \
					-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS"

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lgtest':
#AC_CHECK_LIB([gtest], [main])
# FIXME: Replace `main' with a function in `-lpthread':
#AC_CHECK_LIB([pthread], [main])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([dup2 gettimeofday localtime_r memset mkdir strdup strerror strrchr])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/clib/Makefile
                 test/Makefile
                 test/clib/Makefile])
AC_OUTPUT
