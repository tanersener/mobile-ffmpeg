AUTOMAKE_OPTIONS = parallel-tests
AM_CFLAGS = $(DEBUG_FLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src
LDADD = $(top_builddir)/src/liblept.la $(LIBM)
 
INSTALL_PROGS = convertfilestopdf convertfilestops \
	convertformat convertsegfilestopdf convertsegfilestops \
	converttopdf converttops fileinfo xtractprotos

AUTO_REG_PROGS = adaptmap_reg adaptnorm_reg affine_reg alphaops_reg \
	alphaxform_reg baseline_reg bilateral2_reg \
	bilinear_reg binarize_reg \
	binmorph1_reg binmorph3_reg blackwhite_reg \
	blend1_reg blend2_reg blend3_reg blend4_reg blend5_reg \
	boxa1_reg boxa2_reg boxa3_reg bytea_reg \
	ccthin1_reg ccthin2_reg cmapquant_reg \
	colorcontent_reg coloring_reg colorize_reg \
	colormask_reg colormorph_reg colorquant_reg \
	colorseg_reg colorspace_reg compare_reg \
	compfilter_reg conncomp_reg conversion_reg \
	convolve_reg dewarp_reg distance_reg \
	dither_reg dna_reg dwamorph1_reg edge_reg enhance_reg \
	equal_reg expand_reg extrema_reg \
	falsecolor_reg fhmtauto_reg \
	findcorners_reg findpattern_reg \
	fpix1_reg fpix2_reg genfonts_reg \
	grayfill_reg graymorph1_reg graymorph2_reg \
	grayquant_reg hardlight_reg \
	insert_reg ioformats_reg iomisc_reg italic_reg \
	jbclass_reg jpegio_reg \
	kernel_reg label_reg lineremoval_reg \
	locminmax_reg logicops_reg lowaccess_reg \
	maze_reg mtiff_reg multitype_reg \
	nearline_reg newspaper_reg numa1_reg numa2_reg \
	overlap_reg pageseg_reg paint_reg paintmask_reg \
	pdfseg_reg pixa2_reg pixadisp_reg pixcomp_reg \
	pixmem_reg pixserial_reg pngio_reg pnmio_reg \
	projection_reg projective_reg \
	psio_reg psioseg_reg pta_reg \
	ptra1_reg ptra2_reg \
	quadtree_reg rankbin_reg rankhisto_reg \
	rank_reg rasteropip_reg rasterop_reg \
	rotate1_reg rotate2_reg rotateorth_reg \
	scale_reg seedspread_reg \
	selio_reg shear1_reg shear2_reg \
	skew_reg speckle_reg splitcomp_reg subpixel_reg \
	texturefill_reg threshnorm_reg translate_reg \
	warper_reg watershed_reg wordboxes_reg \
	writetext_reg xformbox_reg

if HAVE_LIBGIF
AUTO_REG_PROGS += gifio_reg
endif

if HAVE_LIBWEBP
AUTO_REG_PROGS += webpio_reg
endif

if HAVE_LIBJP2K
AUTO_REG_PROGS += jp2kio_reg
endif

MANUAL_REG_PROGS = alltests_reg bilateral1_reg \
	binmorph2_reg binmorph4_reg binmorph5_reg \
	dwamorph2_reg \
	files_reg flipdetect_reg fmorphauto_reg \
	heap_reg morphseq_reg pixa1_reg \
	pixalloc_reg pixtile_reg \
	smallpix_reg smoothedge_reg string_reg

OTHER_PROGS = adaptmap_dark \
	arabic_lines arithtest \
	autogentest1 autogentest2 barcodetest \
	binarizefiles binarize_set bincompare \
	blendcmaptest buffertest \
	ccbordtest cctest1 cleanpdf \
	colorsegtest comparepages comparepixa \
	comparetest concatpdf \
	contrasttest converttogray \
	cornertest corrupttest croptest croptext \
	dewarprules dewarptest1 dewarptest2 \
	dewarptest3 dewarptest4 dewarptest5 \
	digitprep1 displayboxa displayboxes_on_pixa \
	displaypix displaypixa dwalineargen \
	fcombautogen fhmtautogen findbinding find_colorregions \
	findpattern1 findpattern2 findpattern3 \
	flipselgen fmorphautogen \
	fpixcontours gammatest graphicstest graymorphtest \
	hashtest histotest histoduptest htmlviewer \
	jbcorrelation jbrankhaus jbwords \
	listtest livre_adapt livre_hmt \
	livre_makefigs livre_orient \
	livre_pageseg livre_seedgen livre_tophat \
	maketile maptest misctest1 \
	modifyhuesat morphtest1 \
	numaranktest otsutest1 otsutest2 \
	pagesegtest1 pagesegtest2 \
	partitiontest pdfiotest percolatetest \
	pixaatest pixafileinfo plottest \
	printimage printsplitimage printtiff rbtreetest \
	recog_bootnum1 recog_bootnum2 recog_bootnum3 \
	recogsort recogtest1 recogtest2 recogtest3 \
	recogtest4 recogtest5 recogtest6 recogtest7 \
	reducetest removecmap \
	renderfonts rotatefastalt \
	rotateorthtest1 rotatetest1 \
	runlengthtest scaleandtile \
	scaletest1 scaletest2 \
	seedfilltest settest sharptest \
	sheartest showedges skewtest \
	sorttest splitimage2pdf sudokutest textorient trctest \
	warpertest wordsinorder \
	writemtiff yuvtest

if ENABLE_PROGRAMS
bin_PROGRAMS = $(INSTALL_PROGS)
noinst_PROGRAMS = $(AUTO_REG_PROGS) $(MANUAL_REG_PROGS) $(OTHER_PROGS)
endif

check_PROGRAMS = $(AUTO_REG_PROGS)
TESTS = $(AUTO_REG_PROGS)
TESTS_ENVIRONMENT = $(srcdir)/reg_wrapper.sh
EXTRA_DIST = reg_wrapper.sh

dwamorph1_reg_SOURCES = dwamorph1_reg.c dwalinear.3.c dwalinearlow.3.c
dwamorph2_reg_SOURCES = dwamorph2_reg.c dwalinear.3.c dwalinearlow.3.c

autogentest2_SOURCES = autogentest2.c autogen.137.c
