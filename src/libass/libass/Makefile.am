AM_CFLAGS = -std=gnu99 -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter \
            -Werror-implicit-function-declaration -Wstrict-prototypes        \
            -Wpointer-arith -Wredundant-decls -Wno-missing-field-initializers\
            -D_GNU_SOURCE

LIBASS_LT_CURRENT = 9
LIBASS_LT_REVISION = 2
LIBASS_LT_AGE = 0

nasm_verbose = $(nasm_verbose_$(V))
nasm_verbose_ = $(nasm_verbose_$(AM_DEFAULT_VERBOSITY))
nasm_verbose_0 = @echo "  NASM    " $@;

.asm.lo:
	$(nasm_verbose)$(LIBTOOL) $(AM_V_lt) --tag=CC --mode=compile $(AS) $(ASFLAGS) -I$(srcdir)/ -o $@ $< -prefer-non-pic

SRC_INTEL = x86/rasterizer.asm x86/blend_bitmaps.asm x86/blur.asm x86/cpuid.asm \
            x86/cpuid.h
SRC_INTEL64 = x86/be_blur.asm

SRC_FONTCONFIG = ass_fontconfig.c ass_fontconfig.h
SRC_DIRECTWRITE = ass_directwrite.c ass_directwrite.h dwrite_c.h
SRC_CORETEXT = ass_coretext.c ass_coretext.h

lib_LTLIBRARIES = libass.la
libass_la_SOURCES = ass.h ass.c ass_types.h ass_utils.h ass_utils.c \
                    ass_compat.h ass_string.h ass_string.c ass_strtod.c \
                    ass_library.h ass_library.c ass_cache.h ass_cache.c ass_cache_template.h \
                    ass_font.h ass_font.c ass_fontselect.h ass_fontselect.c \
                    ass_render.h ass_render.c ass_render_api.c \
                    ass_parse.h ass_parse.c ass_shaper.h ass_shaper.c \
                    ass_outline.h ass_outline.c ass_drawing.h ass_drawing.c \
                    ass_rasterizer.h ass_rasterizer.c ass_rasterizer_c.c \
                    ass_bitmap.h ass_bitmap.c ass_blur.c ass_func_template.h

libass_la_LDFLAGS = -no-undefined -version-info $(LIBASS_LT_CURRENT):$(LIBASS_LT_REVISION):$(LIBASS_LT_AGE)
libass_la_LDFLAGS += -export-symbols $(srcdir)/libass.sym

if FONTCONFIG
libass_la_SOURCES += $(SRC_FONTCONFIG)
endif

if DIRECTWRITE
libass_la_SOURCES += $(SRC_DIRECTWRITE)
endif

if CORETEXT
libass_la_SOURCES += $(SRC_CORETEXT)
endif

if ASM
if INTEL
libass_la_SOURCES += $(SRC_INTEL)
if X64
libass_la_SOURCES += $(SRC_INTEL64)
endif
endif
endif

assheadersdir = $(includedir)/ass
dist_assheaders_HEADERS = ass.h ass_types.h

EXTRA_DIST = libass.sym x86/x86inc.asm x86/utils.asm
