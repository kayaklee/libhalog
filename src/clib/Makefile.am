noinst_LTLIBRARIES = libclib.la

AM_CPPFLAGS = -fPIC -std=c++0x \
	-I${top_srcdir}/src

clib_sources = \
	hal_error.h hal_error.cpp \
	hal_spin_rwlock.h hal_spin_rwlock.cpp \
	hal_util.h

libclib_la_SOURCES = ${clib_sources}
