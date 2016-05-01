noinst_LTLIBRARIES = libclib.a

AM_CPPFLAGS = \
	-I${top_srcdir}/src

clib_sources = \
	hal_error.h hal_error.cpp \
	hal_spin_rwlock.h hal_spin_rwlock.cpp \
	hal_base_log.h hal_base_log.cpp \
	hal_util.h \
	hal_define.h

libclib_a_SOURCES = ${clib_sources}
