VO_COMMON = common
ENC_SRC = amrwbenc/src

ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -I$(top_srcdir)/amrwbenc/inc -I$(top_srcdir)/common/include

if ARMV7NEON
    AM_CFLAGS += -DARM -DARMV7 -DASM_OPT
else
if ARMV5E
    AM_CFLAGS += -DARM -DASM_OPT
endif
endif

amrwbencincludedir = $(includedir)/vo-amrwbenc
amrwbencinclude_HEADERS = enc_if.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = vo-amrwbenc.pc

lib_LTLIBRARIES = libvo-amrwbenc.la

libvo_amrwbenc_la_LDFLAGS = -version-info @VO_AMRWBENC_VERSION@ -no-undefined -export-symbols $(top_srcdir)/vo-amrwbenc.sym

libvo_amrwbenc_la_SOURCES = \
    wrapper.c \
    $(VO_COMMON)/cmnMemory.c \
    $(ENC_SRC)/autocorr.c \
    $(ENC_SRC)/az_isp.c \
    $(ENC_SRC)/bits.c \
    $(ENC_SRC)/c2t64fx.c \
    $(ENC_SRC)/c4t64fx.c \
    $(ENC_SRC)/convolve.c \
    $(ENC_SRC)/cor_h_x.c \
    $(ENC_SRC)/decim54.c \
    $(ENC_SRC)/deemph.c \
    $(ENC_SRC)/dtx.c \
    $(ENC_SRC)/g_pitch.c \
    $(ENC_SRC)/gpclip.c \
    $(ENC_SRC)/homing.c \
    $(ENC_SRC)/hp400.c \
    $(ENC_SRC)/hp50.c \
    $(ENC_SRC)/hp6k.c \
    $(ENC_SRC)/hp_wsp.c \
    $(ENC_SRC)/int_lpc.c \
    $(ENC_SRC)/isp_az.c \
    $(ENC_SRC)/isp_isf.c \
    $(ENC_SRC)/lag_wind.c \
    $(ENC_SRC)/levinson.c \
    $(ENC_SRC)/log2.c \
    $(ENC_SRC)/lp_dec2.c \
    $(ENC_SRC)/math_op.c \
    $(ENC_SRC)/mem_align.c \
    $(ENC_SRC)/oper_32b.c \
    $(ENC_SRC)/p_med_ol.c \
    $(ENC_SRC)/pit_shrp.c \
    $(ENC_SRC)/pitch_f4.c \
    $(ENC_SRC)/pred_lt4.c \
    $(ENC_SRC)/preemph.c \
    $(ENC_SRC)/q_gain2.c \
    $(ENC_SRC)/q_pulse.c \
    $(ENC_SRC)/qisf_ns.c \
    $(ENC_SRC)/qpisf_2s.c \
    $(ENC_SRC)/random.c \
    $(ENC_SRC)/residu.c \
    $(ENC_SRC)/scale.c \
    $(ENC_SRC)/stream.c \
    $(ENC_SRC)/syn_filt.c \
    $(ENC_SRC)/updt_tar.c \
    $(ENC_SRC)/util.c \
    $(ENC_SRC)/voAMRWBEnc.c \
    $(ENC_SRC)/voicefac.c \
    $(ENC_SRC)/wb_vad.c \
    $(ENC_SRC)/weight_a.c

if ARMV7NEON
    libvo_amrwbenc_la_SOURCES += \
        $(ENC_SRC)/asm/ARMV7/convolve_neon.s \
        $(ENC_SRC)/asm/ARMV7/cor_h_vec_neon.s \
        $(ENC_SRC)/asm/ARMV7/Deemph_32_neon.s \
        $(ENC_SRC)/asm/ARMV7/Dot_p_neon.s \
        $(ENC_SRC)/asm/ARMV7/Filt_6k_7k_neon.s \
        $(ENC_SRC)/asm/ARMV7/Norm_Corr_neon.s \
        $(ENC_SRC)/asm/ARMV7/pred_lt4_1_neon.s \
        $(ENC_SRC)/asm/ARMV7/residu_asm_neon.s \
        $(ENC_SRC)/asm/ARMV7/scale_sig_neon.s \
        $(ENC_SRC)/asm/ARMV7/Syn_filt_32_neon.s \
        $(ENC_SRC)/asm/ARMV7/syn_filt_neon.s
else
if ARMV5E
    libvo_amrwbenc_la_SOURCES += \
        $(ENC_SRC)/asm/ARMV5E/convolve_opt.s \
        $(ENC_SRC)/asm/ARMV5E/cor_h_vec_opt.s \
        $(ENC_SRC)/asm/ARMV5E/Deemph_32_opt.s \
        $(ENC_SRC)/asm/ARMV5E/Dot_p_opt.s \
        $(ENC_SRC)/asm/ARMV5E/Filt_6k_7k_opt.s \
        $(ENC_SRC)/asm/ARMV5E/Norm_Corr_opt.s \
        $(ENC_SRC)/asm/ARMV5E/pred_lt4_1_opt.s \
        $(ENC_SRC)/asm/ARMV5E/residu_asm_opt.s \
        $(ENC_SRC)/asm/ARMV5E/scale_sig_opt.s \
        $(ENC_SRC)/asm/ARMV5E/Syn_filt_32_opt.s \
        $(ENC_SRC)/asm/ARMV5E/syn_filt_opt.s
endif
endif

noinst_HEADERS = $(top_srcdir)/amrwbenc/inc/*.h \
                 $(top_srcdir)/amrwbenc/inc/*.tab \
                 $(top_srcdir)/common/include/*.h

if EXAMPLE
bin_PROGRAMS = amrwb-enc$(EXEEXT)

amrwb_enc_LDADD = libvo-amrwbenc.la
amrwb_enc_SOURCES = amrwb-enc.c wavreader.c

noinst_HEADERS += wavreader.h
endif


EXTRA_DIST = $(top_srcdir)/NOTICE \
             $(top_srcdir)/vo-amrwbenc.sym \
             $(top_srcdir)/amrwbenc/*.cpp $(top_srcdir)/amrwbenc/*.h \
             $(top_srcdir)/amrwbenc/*.mk \
             $(top_srcdir)/amrwbenc/NOTICE \
             $(top_srcdir)/amrwbenc/MODULE_LICENSE_APACHE2 \
             $(top_srcdir)/amrwbenc/SampleCode \
             $(top_srcdir)/amrwbenc/doc $(top_srcdir)/amrwbenc/*.txt \
             $(top_srcdir)/common/*.mk \
             $(top_srcdir)/common/NOTICE \
             $(top_srcdir)/common/MODULE_LICENSE_APACHE2

