# NOTE: This Makefile requires GNU make
# Location to put the targets.
TARGETBINDIR = .
TARGETLIBDIR = .
# DLL version information. Currently this must be updated manually.
# Fields are: major, minor, build number, QFE version
VERSION_FIELD = 1,0,0,0
VERSION_STRING = \\\"1.0\\\"
# Name of the targets
# Hooray for Windows DLL hell.
LIBTHEORAENC_TARGET = libtheoraenc.dll
LIBTHEORAENCD_TARGET = libtheoraencd.dll
LIBTHEORAENC70_TARGET = libtheoraenc70.dll
LIBTHEORAENC70D_TARGET = libtheoraenc70d.dll
LIBTHEORAENC71_TARGET = libtheoraenc71.dll
LIBTHEORAENC71D_TARGET = libtheoraenc71d.dll
LIBTHEORAENC80_TARGET = libtheoraenc80.dll
LIBTHEORAENC80D_TARGET = libtheoraenc80d.dll

LIBTHEORADEC_TARGET = libtheoradec.dll
LIBTHEORADECD_TARGET = libtheoradecd.dll
LIBTHEORADEC70_TARGET = libtheoradec70.dll
LIBTHEORADEC70D_TARGET = libtheoradec70d.dll
LIBTHEORADEC71_TARGET = libtheoradec71.dll
LIBTHEORADEC71D_TARGET = libtheoradec71d.dll
LIBTHEORADEC80_TARGET = libtheoradec80.dll
LIBTHEORADEC80D_TARGET = libtheoradec80d.dll

DUMP_VIDEO_TARGET = dump_video.exe
PLAYER_EXAMPLE_TARGET = player_example.exe
ENCODER_EXAMPLE_TARGET = encoder_example.exe

# The compiler tools to use
# The is no standard mingw prefix, so try to guess
MINGW_PREFIX := $(or $(strip $(foreach exeprefix, \
	i686-mingw32 i686-pc-mingw32 i586-mingw32msvc i386-mingw32 \
	no-mingw32, \
	$(if $(shell which $(exeprefix)-gcc 2>/dev/null), $(exeprefix) ))))
CC = $(MINGW_PREFIX)-gcc
RC = $(MINGW_PREFIX)-windres
DLLTOOL = $(MINGW_PREFIX)-dlltool
LD = $(MINGW_PREFIX)-ld
SDLCONFIG = $(MINGW_PREFIX)-sdl-config
# The command to use to generate dependency information
MAKEDEPEND = ${CC} -MM
#MAKEDEPEND = makedepend -f- -Y --

# The location of include files.
# Modify these to point to your Ogg and Vorbis include directories if they are
#  not installed in a standard location.
CINCLUDE = -D_REENTRANT
# Extra compilation flags.
# You may get speed increases by including flags such as -O2 or -O3 or
#  -ffast-math, or additional flags, depending on your system and compiler.
# The correct -march=<architecture> flag will also generate much better code
#  on newer architectures.
CFLAGS = -Wall -Wno-parentheses -DOC_X86_ASM
RELEASE_CFLAGS = ${CFLAGS} -mtune=native -O3 -fomit-frame-pointer -fforce-addr \
 -finline-functions
# The -g flag will generally include debugging information.
DEBUG_CFLAGS = ${CFLAGS} -g
# Libraries to link with, and the location of library files.
LIBS = -logg -lvorbis -lvorbisenc

# ANYTHING BELOW THIS LINE PROBABLY DOES NOT NEED EDITING
CINCLUDE := -I../../include ${CINCLUDE}
LIBSRCDIR = ../../lib
BINSRCDIR = ../../examples
WORKDIR = objs

# C source file lists

LIBTHEORADEC_CSOURCES = \
apiwrapper.c \
bitpack.c \
decapiwrapper.c \
decinfo.c \
decode.c \
dequant.c \
fragment.c \
huffdec.c \
idct.c \
info.c \
internal.c \
quant.c \
state.c \
$(if $(findstring -DOC_X86_ASM,${CFLAGS}), \
x86/mmxidct.c \
x86/mmxfrag.c \
x86/mmxstate.c \
x86/x86state.c \
)

LIBTHEORAENC_CSOURCES = \
apiwrapper.c \
fragment.c \
idct.c \
internal.c \
state.c \
quant.c \
analyze.c \
fdct.c \
encfrag.c \
encapiwrapper.c \
encinfo.c \
encode.c \
enquant.c \
huffenc.c \
mathops.c \
mcenc.c \
rate.c \
tokenize.c \
$(if $(findstring -DOC_X86_ASM,${CFLAGS}), \
x86/mmxfrag.c \
x86/mmxidct.c \
x86/mmxstate.c \
x86/x86state.c \
x86/mmxencfrag.c \
x86/mmxfdct.c \
x86/x86enc.c \
)


DUMP_VIDEO_CSOURCES = dump_video.c
ENCODER_EXAMPLE_CSOURCES = encoder_example.c
PLAYER_EXAMPLE_CSOURCES = player_example.c

# Create object file list.
LIBTHEORADEC_OBJS:= ${LIBTHEORADEC_CSOURCES:%.c=${WORKDIR}/%.o}
LIBTHEORADECD_OBJS:= ${LIBTHEORADEC_CSOURCES:%.c=${WORKDIR}/%.do}
LIBTHEORAENC_OBJS:=  ${LIBTHEORAENC_CSOURCES:%.c=${WORKDIR}/%.o}
LIBTHEORAENCD_OBJS:= ${LIBTHEORAENC_CSOURCES:%.c=${WORKDIR}/%.do}
DUMP_VIDEO_OBJS:= ${DUMP_VIDEO_CSOURCES:%.c=${WORKDIR}/%.o}
ENCODER_EXAMPLE_OBJS:= ${ENCODER_EXAMPLE_CSOURCES:%.c=${WORKDIR}/%.o}
PLAYER_EXAMPLE_OBJS:= ${PLAYER_EXAMPLE_CSOURCES:%.c=${WORKDIR}/%.o}
RC_OBJS:= ${LIBTHEORADEC_TARGET} ${LIBTHEORAENC_TARGET} \
 ${LIBTHEORADECD_TARGET} ${LIBTHEORAENCD_TARGET} \
 ${LIBTHEORADEC70_TARGET} ${LIBTHEORAENC70_TARGET} \
 ${LIBTHEORADEC70D_TARGET} ${LIBTHEORAENC70D_TARGET} \
 ${LIBTHEORADEC71_TARGET} ${LIBTHEORAENC71_TARGET} \
 ${LIBTHEORADEC71D_TARGET} ${LIBTHEORAENC71D_TARGET} \
 ${LIBTHEORADEC80_TARGET} ${LIBTHEORAENC80_TARGET} \
 ${LIBTHEORADEC80D_TARGET} ${LIBTHEORAENC80D_TARGET}
RC_OBJS:= ${RC_OBJS:%.dll=${WORKDIR}/%.rco}
ALL_OBJS:= ${LIBTHEORADEC_OBJS} ${LIBTHEORAENC_OBJS} \
 ${LIBTHEORADECD_OBJS} ${LIBTHEORAENCD_OBJS} ${RC_OBJS} \
 ${DUMP_VIDEO_OBJS} ${ENCODER_EXAMPLE_OBJS} #${PLAYER_EXAMPLE_OBJS}
# Create the dependency file list
ALL_DEPS:= ${ALL_OBJS:%.o=%.d}
ALL_DEPS:= ${ALL_DEPS:%.do=%.dd}
ALL_DEPS:= ${ALL_DEPS:%.rco=%.d}
# Prepend source path to file names.
LIBTHEORADEC_CSOURCES:= ${LIBTHEORADEC_CSOURCES:%=${LIBSRCDIR}/%}
LIBTHEORAENC_CSOURCES:= ${LIBTHEORAENC_CSOURCES:%=${LIBSRCDIR}/%}
DUMP_VIDEO_CSOURCES:= ${DUMP_VIDEO_CSOURCES:%=${BINSRCDIR}/%}
ENCODER_EXAMPLE_CSOURCES:= ${ENCODER_EXAMPLE_CSOURCES:%=${BINSRCDIR}/%}
PLAYER_EXAMPLE_CSOURCES:= ${PLAYER_EXAMPLE_CSOURCES:%=${BINSRCDIR}/%}
ALL_CSOURCES:= ${LIBTHEORADEC_CSOURCES} ${LIBTHEORAENC_CSOURCES} \
 ${DUMP_VIDEO_CSOURCES} ${PLAYER_EXAMPLE_CSOURCES} \
 ${ENCODER_EXAMPLE_CSOURCES}
LIBTHEORAENC_RCO:= ${WORKDIR}/${LIBTHEORAENC_TARGET:%.dll=%.rco}
LIBTHEORAENCD_RCO:= ${WORKDIR}/${LIBTHEORAENCD_TARGET:%.dll=%.rco}
LIBTHEORAENC70_RCO:= ${WORKDIR}/${LIBTHEORAENC70_TARGET:%.dll=%.rco}
LIBTHEORAENC70D_RCO:= ${WORKDIR}/${LIBTHEORAENC70D_TARGET:%.dll=%.rco}
LIBTHEORAENC71_RCO:= ${WORKDIR}/${LIBTHEORAENC71_TARGET:%.dll=%.rco}
LIBTHEORAENC71D_RCO:= ${WORKDIR}/${LIBTHEORAENC71D_TARGET:%.dll=%.rco}
LIBTHEORAENC80_RCO:= ${WORKDIR}/${LIBTHEORAENC80_TARGET:%.dll=%.rco}
LIBTHEORAENC80D_RCO:= ${WORKDIR}/${LIBTHEORAENC80D_TARGET:%.dll=%.rco}
LIBTHEORADEC_RCO:= ${WORKDIR}/${LIBTHEORADEC_TARGET:%.dll=%.rco}
LIBTHEORADECD_RCO:= ${WORKDIR}/${LIBTHEORADECD_TARGET:%.dll=%.rco}
LIBTHEORADEC70_RCO:= ${WORKDIR}/${LIBTHEORADEC70_TARGET:%.dll=%.rco}
LIBTHEORADEC70D_RCO:= ${WORKDIR}/${LIBTHEORADEC70D_TARGET:%.dll=%.rco}
LIBTHEORADEC71_RCO:= ${WORKDIR}/${LIBTHEORADEC71_TARGET:%.dll=%.rco}
LIBTHEORADEC71D_RCO:= ${WORKDIR}/${LIBTHEORADEC71D_TARGET:%.dll=%.rco}
LIBTHEORADEC80_RCO:= ${WORKDIR}/${LIBTHEORADEC80_TARGET:%.dll=%.rco}
LIBTHEORADEC80D_RCO:= ${WORKDIR}/${LIBTHEORADEC80D_TARGET:%.dll=%.rco}
# Prepand target path to file names.
LIBTHEORAENC_TARGET:= ${TARGETLIBDIR}/${LIBTHEORAENC_TARGET}
LIBTHEORAENCD_TARGET:= ${TARGETLIBDIR}/${LIBTHEORAENCD_TARGET}
LIBTHEORAENC70_TARGET:= ${TARGETLIBDIR}/${LIBTHEORAENC70_TARGET}
LIBTHEORAENC70D_TARGET:= ${TARGETLIBDIR}/${LIBTHEORAENC70D_TARGET}
LIBTHEORAENC71_TARGET:= ${TARGETLIBDIR}/${LIBTHEORAENC71_TARGET}
LIBTHEORAENC71D_TARGET:= ${TARGETLIBDIR}/${LIBTHEORAENC71D_TARGET}
LIBTHEORAENC80_TARGET:= ${TARGETLIBDIR}/${LIBTHEORAENC80_TARGET}
LIBTHEORAENC80D_TARGET:= ${TARGETLIBDIR}/${LIBTHEORAENC80D_TARGET}
LIBTHEORADEC_TARGET:= ${TARGETLIBDIR}/${LIBTHEORADEC_TARGET}
LIBTHEORADECD_TARGET:= ${TARGETLIBDIR}/${LIBTHEORADECD_TARGET}
LIBTHEORADEC70_TARGET:= ${TARGETLIBDIR}/${LIBTHEORADEC70_TARGET}
LIBTHEORADEC70D_TARGET:= ${TARGETLIBDIR}/${LIBTHEORADEC70D_TARGET}
LIBTHEORADEC71_TARGET:= ${TARGETLIBDIR}/${LIBTHEORADEC71_TARGET}
LIBTHEORADEC71D_TARGET:= ${TARGETLIBDIR}/${LIBTHEORADEC71D_TARGET}
LIBTHEORADEC80_TARGET:= ${TARGETLIBDIR}/${LIBTHEORADEC80_TARGET}
LIBTHEORADEC80D_TARGET:= ${TARGETLIBDIR}/${LIBTHEORADEC80D_TARGET}
DUMP_VIDEO_TARGET:= ${TARGETBINDIR}/${DUMP_VIDEO_TARGET}
ENCODER_EXAMPLE_TARGET:= ${TARGETBINDIR}/${ENCODER_EXAMPLE_TARGET}
PLAYER_EXAMPLE_TARGET:= ${TARGETBINDIR}/${PLAYER_EXAMPLE_TARGET}
DLL_TARGETS:= ${LIBTHEORADEC_TARGET} ${LIBTHEORAENC_TARGET} \
 ${LIBTHEORADECD_TARGET} ${LIBTHEORAENCD_TARGET} \
 ${LIBTHEORADEC70_TARGET} ${LIBTHEORAENC70_TARGET} \
 ${LIBTHEORADEC70D_TARGET} ${LIBTHEORAENC70D_TARGET} \
 ${LIBTHEORADEC71_TARGET} ${LIBTHEORAENC71_TARGET} \
 ${LIBTHEORADEC71D_TARGET} ${LIBTHEORAENC71D_TARGET} \
 ${LIBTHEORADEC80_TARGET} ${LIBTHEORAENC80_TARGET} \
 ${LIBTHEORADEC80D_TARGET} ${LIBTHEORAENC80D_TARGET}
ALL_TARGETS:= ${DLL_TARGETS} ${DLL_TARGETS:%.dll=%.dll.a} \
 ${DUMP_VIDEO_TARGET} ${ENCODER_EXAMPLE_TARGET} #${PLAYER_EXAMPLE_TARGET}
IMPLIB_TARGETS:= ${DLL_TARGETS:%.dll=%.def} ${DLL_TARGETS:%.dll=%.lib} \
 ${DLL_TARGETS:%.dll=%.exp}

# Targets:
# Everything (default)
all: ${ALL_TARGETS}

# These require Microsoft's lib.exe to build, and so are not made by default.
implibs: ${IMPLIB_TARGETS}

# libtheoradec
${LIBTHEORADEC_TARGET}: ${LIBTHEORADEC_OBJS} ${LIBTHEORADEC_RCO} \
 libtheoradec-all.def
	mkdir -p ${TARGETLIBDIR}
	${CC} -shared -o $@ ${LIBTHEORADEC_OBJS} -logg -lmsvcrt \
         ${LIBTHEORADEC_RCO} \
         -Wl,--output-def,${@:.dll=.def},--out-implib,$@.a,libtheoradec-all.def
${LIBTHEORADECD_TARGET}: ${LIBTHEORADECD_OBJS} ${LIBTHEORADECD_RCO} \
 libtheoradec-all.def
	mkdir -p ${TARGETLIBDIR}
	${CC} -shared -o $@ ${LIBTHEORADECD_OBJS} -logg -lmsvcrtd \
         ${LIBTHEORADECD_RCO} \
         -Wl,--output-def,${@:.dll=.def},--out-implib,$@.a,libtheoradec-all.def
${LIBTHEORADEC70_TARGET}: ${LIBTHEORADEC_OBJS} ${LIBTHEORADEC70_RCO} \
 libtheoradec-all.def
	mkdir -p ${TARGETLIBDIR}
	${CC} -shared -o $@ ${LIBTHEORADEC_OBJS} -logg -lmsvcr70 \
         ${LIBTHEORADEC70_RCO} \
         -Wl,--output-def,${@:.dll=.def},--out-implib,$@.a,libtheoradec-all.def
${LIBTHEORADEC70D_TARGET}: ${LIBTHEORADECD_OBJS} ${LIBTHEORADEC70D_RCO} \
 libtheoradec-all.def
	mkdir -p ${TARGETLIBDIR}
	${CC} -shared -o $@ ${LIBTHEORADECD_OBJS} -logg -lmsvcr70d \
         ${LIBTHEORADEC70D_RCO} \
         -Wl,--output-def,${@:.dll=.def},--out-implib,$@.a,libtheoradec-all.def
${LIBTHEORADEC71_TARGET}: ${LIBTHEORADEC_OBJS} ${LIBTHEORADEC71_RCO} \
 libtheoradec-all.def
	mkdir -p ${TARGETLIBDIR}
	${CC} -shared -o $@ ${LIBTHEORADEC_OBJS} -logg -lmsvcr71 \
         ${LIBTHEORADEC71_RCO} \
         -Wl,--output-def,${@:.dll=.def},--out-implib,$@.a,libtheoradec-all.def
${LIBTHEORADEC71D_TARGET}: ${LIBTHEORADECD_OBJS} ${LIBTHEORADEC71D_RCO} \
 libtheoradec-all.def
	mkdir -p ${TARGETLIBDIR}
	${CC} -shared -o $@ ${LIBTHEORADECD_OBJS} -logg -lmsvcr71d \
         ${LIBTHEORADEC71D_RCO} \
         -Wl,--output-def,${@:.dll=.def},--out-implib,$@.a,libtheoradec-all.def
${LIBTHEORADEC80_TARGET}: ${LIBTHEORADEC_OBJS} ${LIBTHEORADEC80_RCO} \
 libtheoradec-all.def
	mkdir -p ${TARGETLIBDIR}
	${CC} -shared -o $@ ${LIBTHEORADEC_OBJS} -logg -lmsvcr80 \
         ${LIBTHEORADEC80_RCO} \
         -Wl,--output-def,${@:.dll=.def},--out-implib,$@.a,libtheoradec-all.def
${LIBTHEORADEC80D_TARGET}: ${LIBTHEORADECD_OBJS} ${LIBTHEORADEC80D_RCO} \
 libtheoradec-all.def
	mkdir -p ${TARGETLIBDIR}
	${CC} -shared -o $@ ${LIBTHEORADECD_OBJS} -logg -lmsvcr80d \
         ${LIBTHEORADEC80D_RCO} \
         -Wl,--output-def,${@:.dll=.def},--out-implib,$@.a,libtheoradec-all.def

# libtheoraenc
${LIBTHEORAENC_TARGET}: ${LIBTHEORAENC_OBJS} ${LIBTHEORAENC_RCO} \
 libtheoraenc-all.def
	mkdir -p ${TARGETLIBDIR}
	${CC} -shared -o $@ \
         ${LIBTHEORAENC_OBJS} ${LIBTHEORADEC_TARGET} -logg -lmsvcrt \
         ${LIBTHEORAENC_RCO} \
         -Wl,--output-def,${@:.dll=.def},--out-implib,$@.a,libtheoraenc-all.def
${LIBTHEORAENCD_TARGET}: ${LIBTHEORAENCD_OBJS} ${LIBTHEORAENCD_RCO} \
 libtheoraenc-all.def
	mkdir -p ${TARGETLIBDIR}
	${CC} -shared -o $@ \
         ${LIBTHEORAENCD_OBJS} ${LIBTHEORADECD_TARGET} -logg -lmsvcrtd \
         ${LIBTHEORAENCD_RCO} \
         -Wl,--output-def,${@:.dll=.def},--out-implib,$@.a,libtheoraenc-all.def
${LIBTHEORAENC70_TARGET}: ${LIBTHEORAENC_OBJS} ${LIBTHEORAENC70_RCO} \
 libtheoraenc-all.def
	mkdir -p ${TARGETLIBDIR}
	${CC} -shared -o $@ \
         ${LIBTHEORAENC_OBJS} ${LIBTHEORADEC70_TARGET} -logg -lmsvcr70 \
         ${LIBTHEORAENC70_RCO} \
         -Wl,--output-def,${@:.dll=.def},--out-implib,$@.a,libtheoraenc-all.def
${LIBTHEORAENC70D_TARGET}: ${LIBTHEORAENCD_OBJS} ${LIBTHEORAENC70D_RCO} \
 libtheoraenc-all.def
	mkdir -p ${TARGETLIBDIR}
	${CC} -shared -o $@ \
         ${LIBTHEORAENCD_OBJS} ${LIBTHEORADEC70D_TARGET} -logg -lmsvcr70d \
         ${LIBTHEORAENC70D_RCO} \
         -Wl,--output-def,${@:.dll=.def},--out-implib,$@.a,libtheoraenc-all.def
${LIBTHEORAENC71_TARGET}: ${LIBTHEORAENC_OBJS} ${LIBTHEORAENC71_RCO} \
 libtheoraenc-all.def
	mkdir -p ${TARGETLIBDIR}
	${CC} -shared -o $@ \
         ${LIBTHEORAENC_OBJS} ${LIBTHEORADEC71_TARGET} -logg -lmsvcr71 \
         ${LIBTHEORAENC71_RCO} \
         -Wl,--output-def,${@:.dll=.def},--out-implib,$@.a,libtheoraenc-all.def
${LIBTHEORAENC71D_TARGET}: ${LIBTHEORAENCD_OBJS} ${LIBTHEORAENC71D_RCO} \
 libtheoraenc-all.def
	mkdir -p ${TARGETLIBDIR}
	${CC} -shared -o $@ \
         ${LIBTHEORAENCD_OBJS} ${LIBTHEORADEC71D_TARGET} -logg -lmsvcr71d \
         ${LIBTHEORAENC71D_RCO} \
         -Wl,--output-def,${@:.dll=.def},--out-implib,$@.a,libtheoraenc-all.def
${LIBTHEORAENC80_TARGET}: ${LIBTHEORAENC_OBJS} ${LIBTHEORAENC80_RCO} \
 libtheoraenc-all.def
	mkdir -p ${TARGETLIBDIR}
	${CC} -shared -o $@ \
         ${LIBTHEORAENC_OBJS} ${LIBTHEORADEC80_TARGET} -logg -lmsvcr80 \
         ${LIBTHEORAENC80_RCO} \
         -Wl,--output-def,${@:.dll=.def},--out-implib,$@.a,libtheoraenc-all.def
${LIBTHEORAENC80D_TARGET}: ${LIBTHEORAENCD_OBJS} ${LIBTHEORAENC80D_RCO} \
 libtheoraenc-all.def
	mkdir -p ${TARGETLIBDIR}
	${CC} -shared -o $@ \
         ${LIBTHEORAENCD_OBJS} ${LIBTHEORADEC80D_TARGET} -logg -lmsvcr80d \
         ${LIBTHEORAENC80D_RCO} \
         -Wl,--output-def,${@:.dll=.def},--out-implib,$@.a,libtheoraenc-all.def

# dump_video
${DUMP_VIDEO_TARGET}: ${DUMP_VIDEO_OBJS} ${LIBTHEORADEC_TARGET}
	mkdir -p ${TARGETBINDIR}
	${CC} ${CFLAGS} -o $@ ${DUMP_VIDEO_OBJS} ${LIBS} \
         ${LIBTHEORADEC_TARGET}.a

# encoder_example
${ENCODER_EXAMPLE_TARGET}: ${ENCODER_EXAMPLE_OBJS} ${LIBTHEORADEC_TARGET} \
                            ${LIBTHEORAENC_TARGET}
	mkdir -p ${TARGETBINDIR}
	${CC} ${CFLAGS} -o $@ ${ENCODER_EXAMPLE_OBJS} ${LIBS} \
         ${LIBTHEORAENC_TARGET}.a ${LIBTHEORADEC_TARGET}.a

# player_example
${PLAYER_EXAMPLE_TARGET}: CINCLUDE += $(SDLCONFIG) --cflags
${PLAYER_EXAMPLE_TARGET}: ${PLAYER_EXAMPLE_OBJS} ${LIBTHEORADEC_TARGET}
	mkdir -p ${TARGETBINDIR}
	${CC} ${CFLAGS} -o $@ ${PLAYER_EXAMPLE_OBJS} ${LIBS} \
         ${LIBTHEORADEC_TARGET}.a `${SDLCONFIG} --libs`

# Remove all targets.
clean:
	-rm $(sort ${ALL_OBJS} ${ALL_DEPS} ${ALL_TARGETS} ${IMPLIB_TARGETS})
	-rmdir ${WORKDIR}/x86
	-rmdir ${WORKDIR}

# Make everything depend on changes in the Makefile
${ALL_OBJS} ${ALL_DEPS} ${ALL_TARGETS} : Makefile

# Specify which targets are phony for GNU make
.PHONY : all clean

# Rules
# Windows-specific rules
%.dll.a : %.dll
%.def : %.dll
%.exp : %.lib
%.lib : %.def
	wine lib /machine:i386 /def:$<
${WORKDIR}/%.d : %.rc
	mkdir -p ${dir $@}
	${MAKEDEPEND} -x c-header ${CINCLUDE} $< -MT ${@:%.d=%.rco} > $@
${WORKDIR}/%.rco : %.rc
	mkdir -p ${dir $@}
	${RC} ${CINCLUDE} -DTH_VERSION_FIELD=${VERSION_FIELD} \
         -DTH_VERSION_STRING=${VERSION_STRING} $< $@
# Normal compilation
${WORKDIR}/%.d : ${LIBSRCDIR}/%.c
	mkdir -p ${dir $@}
	${MAKEDEPEND} ${CINCLUDE} ${RELEASE_CFLAGS} $< -MT ${@:%.d=%.o} > $@
${WORKDIR}/%.d : ${BINSRCDIR}/%.c
	mkdir -p ${dir $@}
	${MAKEDEPEND} ${CINCLUDE} ${RELEASE_CFLAGS} $< -MT ${@:%.d=%.o} > $@
${WORKDIR}/%.o : ${LIBSRCDIR}/%.c
	mkdir -p ${dir $@}
	${CC} ${CINCLUDE} ${RELEASE_CFLAGS} -c -o $@ $<
${WORKDIR}/%.o : ${BINSRCDIR}/%.c
	mkdir -p ${dir $@}
	${CC} ${CINCLUDE} ${RELEASE_CFLAGS} -c -o $@ $<
# Debug versions
${WORKDIR}/%.dd : ${LIBSRCDIR}/%.c
	mkdir -p ${dir $@}
	${MAKEDEPEND} ${CINCLUDE} ${DEBUG_CFLAGS} $< -MT ${@:%.d=%.do} > $@
${WORKDIR}/%.do : ${LIBSRCDIR}/%.c
	mkdir -p ${dir $@}
	${CC} ${CINCLUDE} ${DEBUG_CFLAGS} -c -o $@ $<

# Include header file dependencies
-include ${ALL_DEPS}
