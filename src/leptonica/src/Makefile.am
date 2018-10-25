AM_CFLAGS = $(DEBUG_FLAGS)
AM_CPPFLAGS = $(ZLIB_CFLAGS) $(LIBPNG_CFLAGS) $(JPEG_CFLAGS) $(LIBTIFF_CFLAGS) $(LIBWEBP_CFLAGS) $(LIBJP2K_CFLAGS)

lib_LTLIBRARIES = liblept.la
liblept_la_LIBADD = $(LIBM) $(ZLIB_LIBS) $(LIBPNG_LIBS) $(JPEG_LIBS) $(GIFLIB_LIBS) $(LIBTIFF_LIBS) $(LIBWEBP_LIBS) $(LIBJP2K_LIBS) $(GDI_LIBS)

liblept_la_LDFLAGS = -no-undefined -version-info 5:3:0

liblept_la_SOURCES = adaptmap.c affine.c                        \
 affinecompose.c arrayaccess.c                                  \
 bardecode.c baseline.c bbuffer.c                               \
 bilateral.c bilinear.c binarize.c                              \
 binexpand.c binreduce.c                                        \
 blend.c bmf.c bmpio.c bmpiostub.c                              \
 bootnumgen1.c bootnumgen2.c bootnumgen3.c                      \
 boxbasic.c boxfunc1.c boxfunc2.c boxfunc3.c boxfunc4.c         \
 bytearray.c ccbord.c ccthin.c classapp.c                       \
 colorcontent.c coloring.c                                      \
 colormap.c colormorph.c	                                \
 colorquant1.c colorquant2.c                                    \
 colorseg.c colorspace.c                                        \
 compare.c conncomp.c convertfiles.c                            \
 convolve.c correlscore.c                                       \
 dewarp1.c dewarp2.c dewarp3.c dewarp4.c                        \
 dnabasic.c dnafunc1.c dnahash.c                                \
 dwacomb.2.c dwacomblow.2.c                                     \
 edge.c encoding.c enhance.c                                    \
 fhmtauto.c fhmtgen.1.c fhmtgenlow.1.c			        \
 finditalic.c flipdetect.c fliphmtgen.c                         \
 fmorphauto.c fmorphgen.1.c fmorphgenlow.1.c                    \
 fpix1.c fpix2.c gifio.c gifiostub.c                            \
 gplot.c graphics.c graymorph.c                                 \
 grayquant.c heap.c jbclass.c                                   \
 jp2kheader.c jp2kheaderstub.c                                  \
 jp2kio.c jp2kiostub.c jpegio.c jpegiostub.c                    \
 kernel.c leptwin.c libversions.c list.c map.c maze.c           \
 morph.c morphapp.c morphdwa.c morphseq.c                       \
 numabasic.c numafunc1.c numafunc2.c                            \
 pageseg.c paintcmap.c                                          \
 parseprotos.c partition.c                                      \
 pdfio1.c pdfio1stub.c pdfio2.c pdfio2stub.c                    \
 pix1.c pix2.c pix3.c pix4.c pix5.c                             \
 pixabasic.c pixacc.c pixafunc1.c pixafunc2.c                   \
 pixalloc.c pixarith.c pixcomp.c pixconv.c	                \
 pixlabel.c pixtiling.c pngio.c pngiostub.c                     \
 pnmio.c pnmiostub.c projective.c	                        \
 psio1.c psio1stub.c psio2.c psio2stub.c                        \
 ptabasic.c ptafunc1.c ptafunc2.c ptra.c	                \
 quadtree.c queue.c rank.c rbtree.c                             \
 readbarcode.c readfile.c                                       \
 recogbasic.c recogdid.c recogident.c                           \
 recogtrain.c regutils.c                                        \
 rop.c roplow.c                                                 \
 rotate.c rotateam.c rotateorth.c rotateshear.c                 \
 runlength.c sarray1.c sarray2.c                                \
 scale1.c scale2.c seedfill.c                                   \
 sel1.c sel2.c selgen.c                                         \
 shear.c skew.c	spixio.c                                        \
 stack.c stringcode.c                                           \
 strokes.c sudoku.c textops.c                                   \
 tiffio.c tiffiostub.c 		                                \
 utils1.c utils2.c warper.c watershed.c                         \
 webpio.c webpiostub.c		                                \
 writefile.c zlibmem.c zlibmemstub.c

pkginclude_HEADERS = allheaders.h alltypes.h                    \
 array.h arrayaccess.h bbuffer.h bilateral.h                    \
 bmf.h bmfdata.h bmp.h ccbord.h                                 \
 dewarp.h endianness.h environ.h		                \
 gplot.h heap.h imageio.h jbclass.h                             \
 leptwin.h list.h	                                        \
 morph.h pix.h ptra.h queue.h rbtree.h                          \
 readbarcode.h recog.h regutils.h stack.h                       \
 stringcode.h sudoku.h watershed.h

LDADD = liblept.la

EXTRA_DIST = hmttemplate1.txt hmttemplate2.txt                  \
 leptonica-license.txt		                                \
 morphtemplate1.txt morphtemplate2.txt                          \
 stringtemplate1.txt stringtemplate2.txt

$(top_builddir)/prog/xtractprotos$(EXEEXT): liblept.la
	$(MAKE) -C $(top_builddir)/prog xtractprotos$(EXEEXT)

allheaders: $(top_builddir)/prog/xtractprotos$(EXEEXT) $(liblept_la_SOURCES)
	cd $(srcdir) && $(abs_top_builddir)/prog/xtractprotos$(EXEEXT) -prestring=LEPT_DLL -protos=inline $(liblept_la_SOURCES)
