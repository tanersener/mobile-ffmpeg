bin_PROGRAMS = kvazaar
lib_LTLIBRARIES = libkvazaar.la


EXTRA_DIST = \
	extras/getopt.c \
	extras/getopt.h


pkgconfigdir = $(libdir)/pkgconfig
nodist_pkgconfig_DATA = kvazaar.pc


include_HEADERS = \
	kvazaar.h

noinst_HEADERS = \
	extras/x86inc.asm

noinst_LTLIBRARIES = \
	libaltivec.la \
	libavx2.la \
	libsse2.la \
	libsse41.la

kvazaar_SOURCES = \
	encmain.c \
	cli.h \
	cli.c \
	yuv_io.c \
	yuv_io.h

kvazaar_LDADD = libkvazaar.la $(LIBS)

kvazaar_CPPFLAGS = -DKVZ_VERSION="`$(srcdir)/../tools/version.sh`"

if USE_CRYPTOPP
kvazaar_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CXX $(AM_LIBTOOLFLAGS) \
	$(LIBTOOLFLAGS) --mode=link $(CXXLD) $(AM_CXXFLAGS) $(CXXFLAGS) \
	$(AM_LDFLAGS) $(LDFLAGS) -o $@
else
kvazaar_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CC $(AM_LIBTOOLFLAGS) \
	$(LIBTOOLFLAGS) --mode=link $(CCLD) $(AM_CFLAGS) $(CFLAGS) \
	$(AM_LDFLAGS) $(LDFLAGS) -o $@
endif

libkvazaar_la_SOURCES = \
	bitstream.c \
	bitstream.h \
	cabac.c \
	cabac.h \
	checkpoint.c \
	checkpoint.h \
	cfg.c \
	cfg.h \
	context.c \
	context.h \
	cu.c \
	cu.h \
	encoder.c \
	encoder.h \
	encoderstate.c \
	encoderstate.h \
	encoder_state-bitstream.c \
	encoder_state-bitstream.h \
	encoder_state-ctors_dtors.c \
	encoder_state-ctors_dtors.h \
	encoder_state-geometry.c \
	encoder_state-geometry.h \
	encode_coding_tree.c \
	encode_coding_tree.h \
	filter.c \
	filter.h \
	global.h \
	image.c \
	image.h \
	imagelist.c \
	imagelist.h \
	input_frame_buffer.c \
	input_frame_buffer.h \
	inter.c \
	inter.h \
	intra.c \
	intra.h \
	kvazaar.c \
	kvazaar_internal.h \
	kvz_math.h \
	nal.c \
	nal.h \
	rate_control.c \
	rate_control.h \
	rdo.c \
	rdo.h \
	sao.c \
	sao.h \
	scalinglist.c \
	scalinglist.h \
	search.c \
	search.h \
	search_inter.c \
	search_inter.h \
	search_intra.c \
	search_intra.h \
	tables.c \
	tables.h \
	threadqueue.c \
	threadqueue.h \
	threads.h \
	transform.c \
	transform.h \
	videoframe.c \
	videoframe.h \
	strategies/generic/dct-generic.c \
	strategies/generic/dct-generic.h \
	strategies/generic/intra-generic.c \
	strategies/generic/intra-generic.h \
	strategies/generic/ipol-generic.c \
	strategies/generic/ipol-generic.h \
	strategies/generic/nal-generic.c \
	strategies/generic/nal-generic.h \
	strategies/generic/picture-generic.c \
	strategies/generic/picture-generic.h \
	strategies/generic/quant-generic.c \
	strategies/generic/quant-generic.h \
	strategies/generic/sao-generic.c \
	strategies/generic/sao-generic.h \
	strategies/generic/encode_coding_tree-generic.c \
	strategies/generic/encode_coding_tree-generic.h \
	strategies/strategies-common.h \
	strategies/strategies-dct.c \
	strategies/strategies-dct.h \
	strategies/strategies-intra.c \
	strategies/strategies-intra.h \
	strategies/strategies-ipol.c \
	strategies/strategies-ipol.h \
	strategies/strategies-nal.c \
	strategies/strategies-nal.h \
	strategies/strategies-picture.c \
	strategies/strategies-picture.h \
	strategies/strategies-quant.c \
	strategies/strategies-quant.h \
	strategies/strategies-sao.c \
	strategies/strategies-sao.h \
	strategies/strategies-encode.c \
	strategies/strategies-encode.h \
	strategies/x86_asm/picture-x86-asm.c \
	strategies/x86_asm/picture-x86-asm.h \
	strategyselector.c \
	strategyselector.h \
	extras/libmd5.c \
	extras/libmd5.h \
	extras/crypto.h

libkvazaar_la_CFLAGS =

libkvazaar_la_LIBADD = \
	libaltivec.la \
	libavx2.la \
	libsse2.la \
	libsse41.la

libkvazaar_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined -version-number $(KVZ_API_VERSION)

if USE_CRYPTOPP
libkvazaar_la_SOURCES += \
	extras/crypto.h \
	extras/crypto.cpp
libkvazaar_la_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CXX $(AM_LIBTOOLFLAGS) \
	$(LIBTOOLFLAGS) --mode=link $(CXXLD) $(AM_CXXFLAGS) $(CXXFLAGS) \
	$(libkvazaar_la_LDFLAGS) $(LDFLAGS) -o $@
else
libkvazaar_la_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CC $(AM_LIBTOOLFLAGS) \
	$(LIBTOOLFLAGS) --mode=link $(CCLD) $(AM_CFLAGS) $(CFLAGS) \
	$(libkvazaar_la_LDFLAGS) $(LDFLAGS) -o $@
endif

libaltivec_la_SOURCES = \
	strategies/altivec/picture-altivec.c \
	strategies/altivec/picture-altivec.h

libavx2_la_SOURCES = \
	strategies/avx2/dct-avx2.c \
	strategies/avx2/dct-avx2.h \
	strategies/avx2/intra-avx2.c \
	strategies/avx2/intra-avx2.h \
	strategies/avx2/ipol-avx2.c \
	strategies/avx2/ipol-avx2.h \
	strategies/avx2/picture-avx2.c \
	strategies/avx2/picture-avx2.h \
	strategies/avx2/quant-avx2.c \
	strategies/avx2/quant-avx2.h \
	strategies/avx2/sao-avx2.c \
	strategies/avx2/sao-avx2.h \
	strategies/avx2/encode_coding_tree-avx2.c \
	strategies/avx2/encode_coding_tree-avx2.h

libsse2_la_SOURCES = \
	strategies/sse2/picture-sse2.c \
	strategies/sse2/picture-sse2.h

libsse41_la_SOURCES = \
	strategies/sse41/picture-sse41.c \
	strategies/sse41/picture-sse41.h

if HAVE_PPC

if HAVE_ALTIVEC
libaltivec_la_CFLAGS = -maltivec
endif

endif #HAVE_PPC

if HAVE_X86

if HAVE_AVX2
libavx2_la_CFLAGS = -mavx2 -mbmi -mabm -mbmi2
endif
if HAVE_SSE4_1
libsse41_la_CFLAGS = -msse4.1
endif
if HAVE_SSE2
libsse2_la_CFLAGS = -msse2
endif

if ENABLE_ASM
noinst_LTLIBRARIES += libasm.la
libkvazaar_la_LIBADD += libasm.la
libasm_la_SOURCES = \
	strategies/x86_asm/picture-x86-asm-sad.asm \
	strategies/x86_asm/picture-x86-asm-sad.h \
	strategies/x86_asm/picture-x86-asm-satd.asm \
	strategies/x86_asm/picture-x86-asm-satd.h
libkvazaar_la_CFLAGS += -DKVZ_COMPILE_ASM

strategies/x86_asm/picture-x86-asm-sad.lo: strategies/x86_asm/picture-x86-asm-sad.asm
strategies/x86_asm/picture-x86-asm-satd.lo: strategies/x86_asm/picture-x86-asm-satd.asm
endif #ENABLE_ASM
endif #HAVE_X86


yasm_verbose = $(yasm_verbose_@AM_V@)
yasm_verbose_ = $(yasm_verbose_@AM_DEFAULT_V@)
yasm_verbose_0 = @echo "  YASM    " $@;

.asm.lo:
	$(yasm_verbose)$(LIBTOOL) --mode=compile --tag=CC $(YASM) -I$(srcdir)/extras $(ASFLAGS) $< -o $@ -prefer-non-pic 1>/dev/null

