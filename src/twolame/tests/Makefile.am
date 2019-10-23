dist_check_SCRIPTS = test.pl
dist_check_DATA = testcase-44100.wav testcase-22050.wav testcase-float32.wav

TESTS = $(dist_check_SCRIPTS)
LOG_COMPILER = $(PERL)
AM_LOG_FLAGS = -Mstrict -w

TESTS_ENVIRONMENT = \
	TWOLAME_CMD="$(top_builddir)/frontend/twolame" \
	STWOLAME_CMD="$(top_builddir)/simplefrontend/stwolame"

CLEANFILES = *.mp2 *.raw
