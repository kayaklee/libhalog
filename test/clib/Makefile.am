AM_CPPFLAGS = \
	-I${top_srcdir}/src

LDADD = ${top_builddir}/src/clib/libclib.a

AM_LDFLAGS = -lpthread -lgtest

bin_PROGRAMS = \
	test_spin_rwlock \
	test_base_log

test_spin_rwlock_SOURCES = test_spin_rwlock.cpp
test_base_log_SOURCES = test_base_log.cpp
