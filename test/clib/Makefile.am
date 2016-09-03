AM_CPPFLAGS = \
	-I${top_srcdir}/src

LDADD = ${top_builddir}/src/clib/libclib.a

AM_LDFLAGS = -lpthread -lgtest -L/usr/local/lib

bin_PROGRAMS = \
	test_btree \
	test_spin_rwlock \
	test_page_arena \
	test_base_log

test_btree_SOURCES = test_btree.cpp
test_spin_rwlock_SOURCES = test_spin_rwlock.cpp
test_page_arena_SOURCES = test_page_arena.cpp
test_base_log_SOURCES = test_base_log.cpp
