###############################################################################
#
# Installation DIRECTORIES
#
# Change these if you want to install somewhere else.

INSTALL_PLUGINS_DIR	=	${PACKAGE_DIR}/lib/ladspa/
INSTALL_INCLUDE_DIR	=	${PACKAGE_DIR}/include/
INSTALL_BINARY_DIR	=	${PACKAGE_DIR}/bin/

###############################################################################
#
# GENERAL
#

INCLUDES	=	-I.
LIBRARIES	=	-ldl -lm
C_FLAGS		=	$(INCLUDES) ${CFLAGS} -fPIC 		\
			-DDEFAULT_LADSPA_PATH=$(INSTALL_PLUGINS_DIR)
BINFLAGS	=	-fPIE
CXX_FLAGS	=	$(INCLUDES) ${CXXFLAGS} -fPIC 		\
			-DDEFAULT_LADSPA_PATH=$(INSTALL_PLUGINS_DIR)
PLUGINS		=	../plugins/amp.so				\
			../plugins/delay.so				\
			../plugins/filter.so				\
			../plugins/noise.so				\
			../plugins/sine.so
PROGRAMS	=	../bin/analyseplugin				\
			../bin/applyplugin 				\
			../bin/listplugins

###############################################################################
#
# RULES TO BUILD PLUGINS FROM C OR C++ CODE
#

../plugins/%.so:	plugins/%.c ladspa.h gcc_exports.map
	${CC} $(C_FLAGS) -o plugins/$*.o -c plugins/$*.c
	${CC}	-o ../plugins/$*.so					\
		plugins/$*.o						\
		-shared							\
		$(C_FLAGS)						\
		-fvisibility=hidden					\
		-fvisibility-inlines-hidden				\
		-Wl

../plugins/%.so:	plugins/%.cpp ladspa.h gcc_exports.map
	${CXX} $(CXX_FLAGS) -o plugins/$*.o -c plugins/$*.cpp
	${CXX}	-o ../plugins/$*.so					\
		plugins/$*.o						\
		-shared							\
		$(CXX_FLAGS)						\
		-fvisibility=hidden					\
		-fvisibility-inlines-hidden				\
		-Wl

###############################################################################
#
# TARGETS
#

install:	targets
	-mkdir -p $(INSTALL_PLUGINS_DIR)
	-mkdir -p $(INSTALL_INCLUDE_DIR)
	-mkdir -p $(INSTALL_BINARY_DIR)
	cp ../plugins/* $(INSTALL_PLUGINS_DIR)
	cp ladspa.h $(INSTALL_INCLUDE_DIR)
	cp ../bin/* $(INSTALL_BINARY_DIR)

/tmp/test.wav:	targets ../snd/noise.wav
	../bin/listplugins
	../bin/analyseplugin ../plugins/filter.so
	../bin/analyseplugin ../plugins/delay.so
	../bin/analyseplugin ../plugins/sine.so
	echo ; ../bin/analyseplugin -l ../plugins/sine.so ; echo
	../bin/analyseplugin ../plugins/amp.so
	../bin/analyseplugin ../plugins/noise.so
	../bin/applyplugin	-s 1					\
				../snd/noise.wav /tmp/test.wav		\
				../plugins/filter.so lpf 500		\
				../plugins/filter.so lpf 500		\
				../plugins/sine.so sine_fcaa 6000	\
				../plugins/delay.so delay_5s 1 0.1	\
				../plugins/amp.so amp_mono 4		\

targets:	$(PLUGINS) $(PROGRAMS)

###############################################################################
#
# PROGRAMS
#

../bin/applyplugin:	applyplugin.o load.o default.o
	${CC} $(C_FLAGS) $(BINFLAGS)					\
		-o ../bin/applyplugin					\
		applyplugin.o load.o default.o				\
		$(LIBRARIES)

../bin/analyseplugin:	analyseplugin.o load.o default.o
	${CC} $(C_FLAGS) $(BINFLAGS)					\
		-o ../bin/analyseplugin 				\
		analyseplugin.o load.o default.o			\
		$(LIBRARIES)

../bin/listplugins:	listplugins.o search.o
	${CC} $(C_FLAGS) $(BINFLAGS)					\
		-o ../bin/listplugins	 				\
		listplugins.o search.o					\
		$(LIBRARIES)

###############################################################################
#
# UTILITIES
#

always:

clean:
	-rm -f `find . -name "*.o"` ../bin/* ../plugins/*
	-rm -f `find .. -name "*~"`
	-rm -f *.bak core score.srt
	-rm -f *.bb *.bbg *.da *-ann gmon.out bb.out
	-rm -f `find .. -name "*.class"`

backup:		clean
	(cd ../../;							\
	tar czf `date '+../backup/ladspa_sdk.%Y%m%d%H%M.tgz'` ladspa_sdk/)

###############################################################################
