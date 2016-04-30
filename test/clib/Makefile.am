AM_CPPFLAGS = \
	-I${top_srcdir}/src

LDADD = ${top_builddir}/src/clib/libclib.a

AM_LDFLAGS = -lpthread -lgtest

bin_PROGRAMS = test_spin_rwlock

test_spin_rwlock_SOURCES = test_spin_rwlock.cpp
