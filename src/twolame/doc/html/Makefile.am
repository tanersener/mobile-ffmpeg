
asciidoc=@PATH_ASCIIDOC@ -b xhtml11 \
	-a revision="@PACKAGE_VERSION@" \
	-a stylesheet=twolame.css \
	-a linkcss \
	-a stylesdir='.'

pkgdocdir = @docdir@
pkghtmldir = $(pkgdocdir)/html

doxygen_files = \
    doxygen.css \
    doxygen.png \
    nav_f.png \
    nav_h.png \
    tabs.css \
    twolame_8h_source.html \
    twolame_8h.html

asciidoc_files = \
    api.html \
    authors.html \
    news.html \
    index.html \
    psycho.html \
    readme.html \
    twolame.1.html \
    vbr.html

stylesheet_files = \
    twolame-manpage.css \
    twolame.css

EXTRA_DIST = Doxyfile.in

if MAINTAINER_MODE

pkghtml_DATA = $(doxygen_files) $(asciidoc_files) $(stylesheet_files)
EXTRA_DIST += Doxyfile.in $(pkghtml_DATA)

$(doxygen_files): Doxyfile $(top_srcdir)/libtwolame/twolame.h
	@PATH_DOXYGEN@
	rm -f index.html files.html globals*.html

index.html: $(top_srcdir)/doc/index.txt
	$(asciidoc) -o $@ $<

readme.html: $(top_srcdir)/README
	$(asciidoc) -o $@ $<

authors.html: $(top_srcdir)/AUTHORS
	$(asciidoc) -o $@ $<

news.html: $(top_srcdir)/NEWS
	$(asciidoc) -o $@ $<

api.html: $(top_srcdir)/doc/api.txt
	$(asciidoc) -o $@ $<

psycho.html: $(top_srcdir)/doc/psycho.txt
	$(asciidoc) -o $@ $<

vbr.html: $(top_srcdir)/doc/vbr.txt
	$(asciidoc) -o $@ $<

twolame.1.html: $(top_srcdir)/doc/twolame.1.txt
	$(asciidoc) -d manpage -o $@ $<

MAINTAINERCLEANFILES = $(doxygen_files) $(asciidoc_files)

endif

