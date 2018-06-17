noinst_LTLIBRARIES = libclib.la

AM_CPPFLAGS = \
	-I${top_srcdir}/src

clib_sources = \
	hal_error.h hal_error.cpp \
	hal_spin_rwlock.h hal_spin_rwlock.cpp \
	hal_base_log.h hal_base_log.cpp \
	hal_page_arena.h hal_page_arena.cpp \
	hal_malloc.h hal_malloc.cpp \
	hal_mod_define.h hal_mod_define.cpp \
	hal_util.h hal_util.cpp \
	hal_i_allocator.h \
	hal_define.h

libclib_la_SOURCES = ${clib_sources}
