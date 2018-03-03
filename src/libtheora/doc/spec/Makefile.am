## Process this file with automake to produce Makefile.in

# makefile to generate the spec document from sources
# requires transfig and pdflatex

docdir = $(datadir)/doc/$(PACKAGE)-$(VERSION)

built_docs = Theora.pdf

if BUILD_SPEC
doc_DATA = $(built_docs)
endif

SPEC_SRCS = spec.tex spec.bib

FIG_SRCS = pic-frame.fig hilbert-mb.fig hilbert-block.fig xifish.fig \
           superblock.fig macroblock.fig raster-block.fig reference-frames.fig \
           pixel444.fig pixel422.fig pixel420.fig idct.fig fdct.fig \
           pic_even.fig pic_even_odd.fig pic_odd.fig pic_odd_even.fig \
           lflim.fig

FIG_TEXS = $(FIG_SRCS:.fig=.tex)
FIG_AUXS = $(FIG_SRCS:.fig=.aux)
FIG_PDFS = $(FIG_SRCS:.fig=.pdf)

# add any native-pdf figures here
FIG_OBJS = $(FIG_PDFS)

EXTRA_DIST = $(built_docs) $(SPEC_SRCS) $(FIG_SRCS) ltablex.sty

if BUILD_SPEC
# latex three times is the charm with references
# long tables require the .aux file to start from scratch
Theora.pdf : $(SPEC_SRCS) $(FIG_OBJS) vp3huff.tex spec.bib
	-$(RM) spec.aux
	pdflatex -interaction nonstopmode spec.tex
	bibtex spec.aux
	pdflatex -interaction nonstopmode spec.tex
	pdflatex -interaction nonstopmode spec.tex
	mv spec.pdf $@
else
Theora.pdf : 
        echo "*** Warning: Missing tools; $@ will not be built."
endif

vp3huff.tex : vp3huff
	./vp3huff > $@

noinst_PROGRAMS = vp3huff
vp3huff_SOURCES = vp3huff.c
 
figures : $(FIG_OBJS)

# rules to generate latex and pdf versions of the xfig figures
.fig.tex:
	fig2dev -L latex $< $@

.fig.pdf:
	fig2dev -L pdf -p 0 $< $@

SUFFIXES = .fig .tex .pdf

# clean targets
clean-local:
	-$(RM) $(FIG_TEXS)
	-$(RM) $(FIG_AUXS)
	-$(RM) $(FIG_PDFS)
	-$(RM) vp3huff
	-$(RM) vp3huff.tex
	-$(RM) vp3huff.aux
	-$(RM) spec.aux
	-$(RM) spec.log
	-$(RM) spec.lof
	-$(RM) spec.lot
	-$(RM) spec.out
	-$(RM) spec.bbl
	-$(RM) spec.blg
	-$(RM) spec.toc

maintainer-clean-local:
	-$(RM) $(built_docs)

maintainerclean: maintainer-clean
