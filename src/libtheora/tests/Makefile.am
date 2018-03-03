INCLUDES = -I$(top_srcdir)/include

noinst_HEADERS = tests.h

AM_CFLAGS = $(OGG_CFLAGS)

THEORADIR = ../lib
THEORA_LIBS = $(THEORADIR)/libtheora.la $(OGG_LIBS)
THEORADEC_LIBS = $(THEORADIR)/libtheoradec.la $(OGG_LIBS)
THEORAENC_LIBS = $(THEORADIR)/libtheoraenc.la \
		 $(THEORADIR)/libtheoradec.la $(OGG_LIBS)
test: check

TESTS_ENVIRONMENT = $(VALGRIND_ENVIRONMENT)

TESTS_DEC = noop_theora \
	comment comment_theoradec comment_theora

TESTS_ENC = noop noop_theoraenc \
	granulepos granulepos_theoraenc granulepos_theora

if THEORA_DISABLE_ENCODE
TESTS = $(TESTS_DEC)
else
TESTS = $(TESTS_DEC) $(TESTS_ENC)
endif

check_PROGRAMS = $(TESTS)
# dummy call tests for the current api
noop_SOURCES = noop.c
noop_LDADD = $(THEORAENC_LIBS)
noop_CFLAGS = $(OGG_CFLAGS)

# dummy call tests for the pre-1.0 legacy api with current link line
noop_theoraenc_SOURCES = noop_theora.c
noop_theoraenc_LDADD = $(THEORAENC_LIBS)
noop_theoraenc_CFLAGS = $(OGG_CFLAGS)

# dummy call tests for the pre-1.0 legacy api with legacy link line
noop_theora_SOURCES = noop_theora.c
noop_theora_LDADD = $(THEORA_LIBS)
noop_theora_CFLAGS = $(OGG_CFLAGS)

# comment utilities for the current api
comment_SOURCES = comment.c
comment_LDADD = $(THEORADEC_LIBS)
comment_CFLAGS = $(OGG_CFLAGS)

# comment utilities for the legacy api and current lib
comment_theoradec_SOURCES = comment.c
comment_theoradec_LDADD = $(THEORADEC_LIBS)
comment_theoradec_CFLAGS = $(OGG_CFLAGS)

# comment utilities for the legacy api and legacy lib
comment_theora_SOURCES = comment_theora.c
comment_theora_LDADD = $(THEORA_LIBS)
comment_theora_CFLAGS = $(OGG_CFLAGS)

granulepos_SOURCES = granulepos.c
granulepos_LDADD = $(THEORAENC_LIBS) -lm
granulepos_CFLAGS = $(OGG_CFLAGS)

granulepos_theoraenc_SOURCES = granulepos_theora.c
granulepos_theoraenc_LDADD = $(THEORAENC_LIBS) -lm
granulepos_theoraenc_CFLAGS = $(OGG_CFLAGS)

granulepos_theora_SOURCES = granulepos_theora.c
granulepos_theora_LDADD = $(THEORA_LIBS) -lm
granulepos_theora_CFLAGS = $(OGG_CFLAGS)
