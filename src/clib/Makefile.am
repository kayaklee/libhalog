noinst_LTLIBRARIES = libclib.a

AM_CPPFLAGS = \
	-I${top_srcdir}/src

clib_sources = \
	hal_error.h hal_error.cpp \
	hal_spin_rwlock.h hal_spin_rwlock.cpp \
	hal_util.h

libclib_a_SOURCES = ${clib_sources}
