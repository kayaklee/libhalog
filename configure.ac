#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT(libhalog, 1.0, likai.root@gmail.com)
AM_INIT_AUTOMAKE(libhalog, 1.0)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LIBTOOL
AC_LANG([C++])

CXXFLAGS="-g -std=c++0x -fPIC -finline-functions -O2 \
    -Wall -Werror -Wextra -Wunused-parameter -Wformat -Wconversion \
    -fno-strict-aliasing \
    -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS"

# Checks for libraries.
AC_PROG_RANLIB

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_OUTPUT(Makefile
    src/Makefile
    src/clib/Makefile
    test/Makefile
    test/clib/Makefile
    )
