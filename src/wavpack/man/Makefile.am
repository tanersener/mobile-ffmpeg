man_MANS = wavpack.1 wvgain.1 wvunpack.1 wvtag.1

if ENABLE_MAN
%.1: %.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<
endif

EXTRA_DIST = $(man_MANS) $(patsubst %.1,%.xml,$(man_MANS))

MAINTAINERCLEANFILES = \
	$(man_MANS) \
	Makefile.in

