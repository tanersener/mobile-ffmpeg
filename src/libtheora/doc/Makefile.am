## Process this file with automake to produce Makefile.in

SUBDIRS = spec

docdir = $(datadir)/doc/$(PACKAGE)-$(VERSION)

static_docs = vp3-format.txt color.html \
	draft-ietf-avt-rtp-theora-00.xml \
	draft-ietf-avt-rtp-theora-00.txt

doc_DATA = $(static_docs) doxygen-build.stamp

EXTRA_DIST = $(static_docs) Doxyfile.in

if HAVE_DOXYGEN
doxygen-build.stamp: Doxyfile $(top_srcdir)/include/theora/*.h
	doxygen
	touch doxygen-build.stamp
else
doxygen-build.stamp:
	echo "*** Warning: Doxygen not found; documentation will not be built."
	touch doxygen-build.stamp
endif

dist_docdir = $(distdir)/libtheora

dist-hook:
	if test -d libtheora; then \
	  mkdir $(dist_docdir); \
	  echo -n "copying built documenation..."; \
	  for dir in libtheora/*; do \
	    b=`basename $$dir`; \
	    if test $$b != ".svn"; then \
	      if test -d $$dir; then \
		mkdir $(dist_docdir)/$$b; \
		for f in $$dir/*; do \
		  cp -p $$f $(dist_docdir)/$$b; \
		done; \
	      fi; \
	    fi; \
	  done; \
	  echo "OK"; \
	fi
	for item in $(EXTRA_DIST); do \
	  if test -d $$item; then \
	    echo -n "cleaning $$item dir for distribution..."; \
	    rm -rf `find $(distdir)/$$item -name .svn`; \
	    echo "OK"; \
	  fi; \
	done



install-data-local: doxygen-build.stamp
	$(mkinstalldirs) $(DESTDIR)$(docdir)
	if test -d libtheora; then \
	  for dir in libtheora/*; do \
	    if test -d $$dir; then \
	      b=`basename $$dir`; \
	      $(mkinstalldirs) $(DESTDIR)$(docdir)/$$b; \
	      for f in $$dir/*; do \
		$(INSTALL_DATA) $$f $(DESTDIR)$(docdir)/$$b; \
	      done \
	    fi \
	  done \
	fi

uninstall-local:
	rm -rf $(DESTDIR)$(docdir)

clean-local:
	if test -d libtheora; then rm -rf libtheora; fi
	if test -f doxygen-build.stamp; then rm -f doxygen-build.stamp; fi

