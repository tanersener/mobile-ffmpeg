# -*- encoding: utf-8 -*-
#
# Copyright © 2003  Keith Packard
# Copyright © 2013  Google, Inc.
#
# Permission to use, copy, modify, distribute, and sell this software and its
# documentation for any purpose is hereby granted without fee, provided that
# the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation, and that the name of the author(s) not be used in
# advertising or publicity pertaining to distribution of the software without
# specific, written prior permission.  The authors make no
# representations about the suitability of this software for any purpose.  It
# is provided "as is" without express or implied warranty.
#
# THE AUTHOR(S) DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
# INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
# EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY SPECIAL, INDIRECT OR
# CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
# DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
# TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.
#
# Google Author(s): Behdad Esfahbod

DIR=fc-$(TAG)
OUT=fc$(TAG)
TMPL=$(OUT).tmpl.h
TARG=$(OUT).h
TSRC=$(DIR).c
TOOL=./$(DIR)$(EXEEXT_FOR_BUILD)

EXTRA_DIST = $(TARG) $(TMPL) $(TSRC) $(DIST)

AM_CPPFLAGS = \
	   -I$(builddir) \
	   -I$(srcdir) \
	   -I$(top_builddir)/src \
	   -I$(top_srcdir)/src \
	   -I$(top_builddir) \
	   -I$(top_srcdir) \
	   -DHAVE_CONFIG_H \
	   $(WARN_CFLAGS)

$(TOOL): $(TSRC) $(ALIAS_FILES)
	$(AM_V_GEN) $(CC_FOR_BUILD) -o $(TOOL) $< $(AM_CPPFLAGS)

$(TARG): $(TMPL) $(TSRC) $(DEPS)
	$(AM_V_GEN) $(MAKE) $(TOOL) && \
	$(RM) $(TARG) && \
	$(TOOL) $(ARGS) < $< > $(TARG).tmp && \
	mv $(TARG).tmp $(TARG) || ( $(RM) $(TARG).tmp && false )
noinst_HEADERS=$(TARG)

ALIAS_FILES = fcalias.h fcaliastail.h

BUILT_SOURCES = $(ALIAS_FILES)

$(ALIAS_FILES):
	$(AM_V_GEN) touch $@

CLEANFILES = $(ALIAS_FILES) $(TOOL)

MAINTAINERCLEANFILES = $(TARG)
