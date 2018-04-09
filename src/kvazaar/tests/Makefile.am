
TESTS = $(check_PROGRAMS) \
    test_external_symbols.sh \
    test_gop.sh \
    test_interlace.sh \
    test_intra.sh \
    test_invalid_input.sh \
    test_mv_constraint.sh \
    test_owf_wpp_tiles.sh \
    test_rate_control.sh \
    test_slices.sh \
    test_smp.sh \
    test_tools.sh \
    test_weird_shapes.sh

EXTRA_DIST = \
    test_external_symbols.sh \
    test_gop.sh \
    test_interlace.sh \
    test_intra.sh \
    test_invalid_input.sh \
    test_mv_constraint.sh \
    test_owf_wpp_tiles.sh \
    test_rate_control.sh \
    test_slices.sh \
    test_smp.sh \
    test_tools.sh \
    test_weird_shapes.sh \
    util.sh

check_PROGRAMS = kvazaar_tests

kvazaar_tests_SOURCES = \
	coeff_sum_tests.c \
	dct_tests.c \
	intra_sad_tests.c \
	mv_cand_tests.c \
	sad_tests.c \
	sad_tests.h \
	satd_tests.c \
	satd_tests.h \
	speed_tests.c \
	tests_main.c \
	test_strategies.c \
	test_strategies.h
kvazaar_tests_CFLAGS = -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src
kvazaar_tests_LDFLAGS = -static $(top_builddir)/src/libkvazaar.la $(LIBS)

# This makes sure that CXXLD gets defined.
nodist_EXTRA_kvazaar_tests_SOURCES = cpp.cpp

if USE_CRYPTOPP
kvazaar_tests_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CXX $(AM_LIBTOOLFLAGS) \
	$(LIBTOOLFLAGS) --mode=link $(CXXLD) $(kvazaar_tests_CFLAGS) $(CXXFLAGS) \
	$(kvazaar_tests_LDFLAGS) $(LDFLAGS) -o $@
else
kvazaar_tests_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CC $(AM_LIBTOOLFLAGS) \
	$(LIBTOOLFLAGS) --mode=link $(CCLD) $(kvazaar_tests_CFLAGS) $(CFLAGS) \
	$(kvazaar_tests_LDFLAGS) $(LDFLAGS) -o $@
endif
