SUBDIRS=html

## Building the manpage requires:
##    asciidoc:	http://www.methods.co.nz/asciidoc/
##    xmlto:	http://cyberelk.net/tim/xmlto/
##

## Man Page
dist_man_MANS = twolame.1

if MAINTAINER_MODE

twolame.1.xml: twolame.1.txt
	@PATH_ASCIIDOC@ -d manpage -b docbook -o twolame.1.xml twolame.1.txt

twolame.1: twolame.1.xml
	@PATH_XMLTO@ --skip-validation man twolame.1.xml

MAINTAINERCLEANFILES = twolame.1 twolame.1.xml

endif

pkgdocdir = @docdir@
pkgdoc_DATA = \
	api.txt \
	psycho.txt \
	vbr.txt

EXTRA_DIST = \
	$(pkgdoc_DATA) \
	dist10-text/common.txt \
	dist10-text/commonh.txt \
	dist10-text/encode.txt \
	dist10-text/encoderh.txt \
	dist10-text/musicin.txt \
	dist10-text/psy.txt \
	dist10-text/tonal.txt \
	index.txt
