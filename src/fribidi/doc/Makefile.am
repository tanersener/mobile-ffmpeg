dist_man_MANS =
dist_noinst_MANS =
EXTRA_DIST =
MOSTLYCLEANFILES =
MAINTAINERCLEANFILES =

# man3

includepath =
includevpath =
headers =
inst_symbols =
noinst_symbols =

# The lib/Headers.mk stuff should come after all optional stuff.
include $(top_srcdir)/lib/Headers.mk
headers += $(libfribidi_la_headers)
includepath += -I$(top_builddir)/lib -I$(top_srcdir)/lib
includevpath += :$(top_builddir)/lib:$(top_srcdir)/lib
# The las two lines are not functions, we don't want them here.
inst_symbols += $(shell sed '$$d' $(top_srcdir)/lib/fribidi.def | sed '$$d')

dist_man_MANS += $(inst_symbols:=.3)
dist_noinst_MANS += $(noinst_symbols:=.3)
EXTRA_DIST += c2man.stamp
MOSTLYCLEANFILES += c2man.stamp.tmp
MAINTAINERCLEANFILES += $(man3) c2man.stamp

man3 = $(dist_man_MANS) $(dist_noinst_MANS)

C2MANFLAGS = $(includepath) \
		-D__FRIBIDI_DOC \
		-DDONT_HAVE_FRIBIDI_CONFIG_H \
		-M "Programmer's Manual" 

VPATH += $(includevpath)

c2man.stamp: $(headers)
	@$(RM) $@.tmp
	@touch $@.tmp
	@($(LN_S) $^ . || cp $^ .)
	@echo Running c2man
	@$(top_srcdir)/missing --run c2man -T n $(C2MANFLAGS) $(headers) 2>&1 \
	|| true
	@$(RM) $(headers)
	@mv $@.tmp $@

$(man3): c2man.stamp
	@if ! test -f $@; then \
		$(RM) $<; \
		$(MAKE) $(AM_MAKEFLAGS) $<;\
	fi
	@touch $@

-include $(top_srcdir)/git.mk
