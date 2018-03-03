INCLUDES = -I$(top_srcdir)/include
AM_CFLAGS = $(OGG_CFLAGS) $(CAIRO_CFLAGS)

EXTRA_DIST = \
	cpu.c \
	encoder_disabled.c \
	x86/mmxencfrag.c \
	x86/mmxfdct.c \
	x86/sse2fdct.c \
	x86/x86enc.c \
	x86/x86enc.h \
	x86/mmxfrag.c \
	x86/mmxfrag.h \
	x86/mmxidct.c \
	x86/mmxloop.h \
	x86/mmxstate.c \
	x86/x86int.h \
	x86/x86state.c \
	x86_vc

lib_LTLIBRARIES = libtheoradec.la libtheoraenc.la libtheora.la

if THEORA_DISABLE_ENCODE
encoder_uniq_sources = \
	encoder_disabled.c

encoder_sources = \
	$(encoder_uniq_sources)
else
encoder_uniq_x86_sources = \
	x86/mmxencfrag.c \
	x86/mmxfdct.c \
	x86/x86enc.c

encoder_uniq_x86_64_sources = \
	x86/sse2fdct.c

encoder_shared_x86_sources = \
	x86/mmxfrag.c \
	x86/mmxidct.c \
	x86/mmxstate.c \
	x86/x86state.c

encoder_shared_x86_64_sources =

if CPU_x86_64
encoder_uniq_arch_sources = \
 $(encoder_uniq_x86_sources) \
 $(encoder_uniq_x86_64_sources)
encoder_shared_arch_sources = \
 $(encoder_shared_x86_sources) \
 $(encoder_shared_x86_64_sources)
else
if CPU_x86_32
encoder_uniq_arch_sources = $(encoder_uniq_x86_sources)
encoder_shared_arch_sources = $(encoder_shared_x86_sources)
else
encoder_uniq_arch_sources =
encoder_shared_arch_sources =
endif
endif

encoder_uniq_sources = \
	analyze.c \
	fdct.c \
	encfrag.c \
	encapiwrapper.c \
	encinfo.c \
	encode.c \
	enquant.c \
	huffenc.c \
	mathops.c \
	mcenc.c \
	rate.c \
	tokenize.c \
	$(encoder_uniq_arch_sources)

encoder_sources = \
	apiwrapper.c \
	fragment.c \
	idct.c \
	internal.c \
	state.c \
	quant.c \
	$(encoder_shared_arch_sources) \
	$(encoder_uniq_sources)

endif

decoder_x86_sources = \
	x86/mmxidct.c \
	x86/mmxfrag.c \
	x86/mmxstate.c \
	x86/x86state.c
if CPU_x86_64
decoder_arch_sources = $(decoder_x86_sources)
else
if CPU_x86_32
decoder_arch_sources = $(decoder_x86_sources)
else
decoder_arch_sources =
endif
endif

decoder_sources = \
	apiwrapper.c \
	bitpack.c \
	decapiwrapper.c \
	decinfo.c \
	decode.c \
	dequant.c \
	fragment.c \
	huffdec.c \
	idct.c \
	info.c \
	internal.c \
	quant.c \
	state.c \
	$(decoder_arch_sources)

noinst_HEADERS = \
	cpu.h \
	internal.h \
	encint.h \
	enquant.h \
	huffenc.h \
	mathops.h \
	modedec.h \
	x86/x86enc.h \
	apiwrapper.h \
	bitpack.h \
	dct.h \
	decint.h \
	dequant.h \
	huffdec.h \
	huffman.h \
	ocintrin.h \
	quant.h \
	x86/mmxfrag.h \
	x86/mmxloop.h \
	x86/x86int.h

libtheoradec_la_SOURCES = \
	$(decoder_sources) \
	Version_script-dec theoradec.exp
libtheoradec_la_LDFLAGS = \
  -version-info @THDEC_LIB_CURRENT@:@THDEC_LIB_REVISION@:@THDEC_LIB_AGE@ \
  @THEORADEC_LDFLAGS@ @CAIRO_LIBS@

libtheoraenc_la_SOURCES = \
	$(encoder_sources) \
	Version_script-enc theoraenc.exp
libtheoraenc_la_LDFLAGS = \
  -version-info @THENC_LIB_CURRENT@:@THENC_LIB_REVISION@:@THENC_LIB_AGE@ \
  @THEORAENC_LDFLAGS@ $(OGG_LIBS)

libtheora_la_SOURCES = \
	$(decoder_sources) \
	$(encoder_uniq_sources) \
	Version_script theora.exp
libtheora_la_LDFLAGS = \
  -version-info @TH_LIB_CURRENT@:@TH_LIB_REVISION@:@TH_LIB_AGE@ \
  @THEORA_LDFLAGS@ @CAIRO_LIBS@ $(OGG_LIBS)

debug:
	$(MAKE) all CFLAGS="@DEBUG@" 

profile:
	$(MAKE) all CFLAGS="@PROFILE@"

# contstruct various symbol export list files
.def.exp : defexp.awk
	awk -f defexp.awk $< > $@
