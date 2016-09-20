AM_CPPFLAGS = \
	-I${top_srcdir}/src

#AM_CPPFLAGS += -DDO_NOT_CHECK

LDADD = ${top_builddir}/src/clib/libclib.a

AM_LDFLAGS = -lpthread -lgtest -L/usr/local/lib

bin_PROGRAMS = \
	test_cas.bin \
	hv_sample_fifo.bin \
	hv_sample_lifo.bin \
	test_hazard_version.bin \
	test_btree.bin \
	test_spin_rwlock.bin \
	test_page_arena.bin \
	test_base_log.bin

test_cas = test_cas.cpp
hv_sample_fifo_bin_SOURCES = hv_sample_fifo.cpp
hv_sample_lifo_bin_SOURCES = hv_sample_lifo.cpp
test_hazard_version_bin_SOURCES = test_hazard_version.cpp
test_btree_bin_SOURCES = test_btree.cpp
test_spin_rwlock_bin_SOURCES = test_spin_rwlock.cpp
test_page_arena_bin_SOURCES = test_page_arena.cpp
test_base_log_bin_SOURCES = test_base_log.cpp
