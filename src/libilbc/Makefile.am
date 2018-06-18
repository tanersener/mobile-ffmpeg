ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/ilbc/interface -I$(top_srcdir)/signal_processing/include \
    -Wall -Wextra -fvisibility=hidden -Wno-unused-parameter -fno-strict-aliasing

libilbcincludedir = $(includedir)
libilbcinclude_HEADERS = $(top_srcdir)/ilbc/interface/ilbc.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libilbc.pc

lib_LTLIBRARIES = libilbc.la

libilbc_la_LDFLAGS = -version-info @LIBILBC_VERSION@ -no-undefined

ilbc_headers = \
    ilbc/abs_quant.h \
    ilbc/abs_quant_loop.h \
    ilbc/augmented_cb_corr.h \
    ilbc/bw_expand.h \
    ilbc/cb_construct.h \
    ilbc/cb_mem_energy_augmentation.h \
    ilbc/cb_mem_energy_calc.h \
    ilbc/cb_mem_energy.h \
    ilbc/cb_search_core.h \
    ilbc/cb_search.h \
    ilbc/cb_update_best_index.h \
    ilbc/chebyshev.h \
    ilbc/comp_corr.h \
    ilbc/constants.h \
    ilbc/create_augmented_vec.h \
    ilbc/decode.h \
    ilbc/decode_residual.h \
    ilbc/decoder_interpolate_lsf.h \
    ilbc/defines.h \
    ilbc/do_plc.h \
    ilbc/encode.h \
    ilbc/energy_inverse.h \
    ilbc/enhancer.h \
    ilbc/enhancer_interface.h \
    ilbc/enh_upsample.h \
    ilbc/filtered_cb_vecs.h \
    ilbc/frame_classify.h \
    ilbc/gain_dequant.h \
    ilbc/gain_quant.h \
    ilbc/get_cd_vec.h \
    ilbc/get_lsp_poly.h \
    ilbc/get_sync_seq.h \
    ilbc/hp_input.h \
    ilbc/hp_output.h \
    ilbc/index_conv_dec.h \
    ilbc/index_conv_enc.h \
    ilbc/init_decode.h \
    ilbc/init_encode.h \
    ilbc/interpolate.h \
    ilbc/interpolate_samples.h \
    ilbc/lpc_encode.h \
    ilbc/lsf_check.h \
    ilbc/lsf_interpolate_to_poly_dec.h \
    ilbc/lsf_interpolate_to_poly_enc.h \
    ilbc/lsf_to_lsp.h \
    ilbc/lsf_to_poly.h \
    ilbc/lsp_to_lsf.h \
    ilbc/my_corr.h \
    ilbc/nearest_neighbor.h \
    ilbc/pack_bits.h \
    ilbc/poly_to_lsf.h \
    ilbc/poly_to_lsp.h \
    ilbc/refiner.h \
    ilbc/simple_interpolate_lsf.h \
    ilbc/simple_lpc_analysis.h \
    ilbc/simple_lsf_dequant.h \
    ilbc/simple_lsf_quant.h \
    ilbc/smooth.h \
    ilbc/smooth_out_data.h \
    ilbc/sort_sq.h \
    ilbc/split_vq.h \
    ilbc/state_construct.h \
    ilbc/state_search.h \
    ilbc/swap_bytes.h \
    ilbc/unpack_bits.h \
    ilbc/vq3.h \
    ilbc/vq4.h \
    ilbc/window32_w32.h \
    ilbc/xcorr_coef.h

ilbc_sources = \
    ilbc/abs_quant.c \
    ilbc/abs_quant_loop.c \
    ilbc/augmented_cb_corr.c \
    ilbc/bw_expand.c \
    ilbc/cb_construct.c \
    ilbc/cb_mem_energy.c \
    ilbc/cb_mem_energy_augmentation.c \
    ilbc/cb_mem_energy_calc.c \
    ilbc/cb_search.c \
    ilbc/cb_search_core.c \
    ilbc/cb_update_best_index.c \
    ilbc/chebyshev.c \
    ilbc/comp_corr.c \
    ilbc/constants.c \
    ilbc/create_augmented_vec.c \
    ilbc/decode.c \
    ilbc/decode_residual.c \
    ilbc/decoder_interpolate_lsf.c \
    ilbc/do_plc.c \
    ilbc/encode.c \
    ilbc/energy_inverse.c \
    ilbc/enh_upsample.c \
    ilbc/enhancer.c \
    ilbc/enhancer_interface.c \
    ilbc/filtered_cb_vecs.c \
    ilbc/frame_classify.c \
    ilbc/gain_dequant.c \
    ilbc/gain_quant.c \
    ilbc/get_cd_vec.c \
    ilbc/get_lsp_poly.c \
    ilbc/get_sync_seq.c \
    ilbc/hp_input.c \
    ilbc/hp_output.c \
    ilbc/ilbc.c \
    ilbc/index_conv_dec.c \
    ilbc/index_conv_enc.c \
    ilbc/init_decode.c \
    ilbc/init_encode.c \
    ilbc/interpolate.c \
    ilbc/interpolate_samples.c \
    ilbc/lpc_encode.c \
    ilbc/lsf_check.c \
    ilbc/lsf_interpolate_to_poly_dec.c \
    ilbc/lsf_interpolate_to_poly_enc.c \
    ilbc/lsf_to_lsp.c \
    ilbc/lsf_to_poly.c \
    ilbc/lsp_to_lsf.c \
    ilbc/my_corr.c \
    ilbc/nearest_neighbor.c \
    ilbc/pack_bits.c \
    ilbc/poly_to_lsf.c \
    ilbc/poly_to_lsp.c \
    ilbc/refiner.c \
    ilbc/simple_interpolate_lsf.c \
    ilbc/simple_lpc_analysis.c \
    ilbc/simple_lsf_dequant.c \
    ilbc/simple_lsf_quant.c \
    ilbc/smooth.c \
    ilbc/smooth_out_data.c \
    ilbc/sort_sq.c \
    ilbc/split_vq.c \
    ilbc/state_construct.c \
    ilbc/state_search.c \
    ilbc/swap_bytes.c \
    ilbc/unpack_bits.c \
    ilbc/vq3.c \
    ilbc/vq4.c \
    ilbc/window32_w32.c \
    ilbc/xcorr_coef.c

sigproc_sources = \
    signal_processing/auto_corr_to_refl_coef.c \
    signal_processing/auto_correlation.c \
    signal_processing/complex_bit_reverse.c \
    signal_processing/complex_fft.c \
    signal_processing/copy_set_operations.c \
    signal_processing/cross_correlation.c \
    signal_processing/division_operations.c \
    signal_processing/dot_product_with_scale.c \
    signal_processing/downsample_fast.c \
    signal_processing/energy.c \
    signal_processing/filter_ar.c \
    signal_processing/filter_ar_fast_q12.c \
    signal_processing/filter_ma_fast_q12.c \
    signal_processing/get_hanning_window.c \
    signal_processing/get_scaling_square.c \
    signal_processing/ilbc_specific_functions.c \
    signal_processing/levinson_durbin.c \
    signal_processing/lpc_to_refl_coef.c \
    signal_processing/min_max_operations.c \
    signal_processing/randomization_functions.c \
    signal_processing/real_fft.c \
    signal_processing/refl_coef_to_lpc.c \
    signal_processing/resample.c \
    signal_processing/resample_48khz.c \
    signal_processing/resample_by_2.c \
    signal_processing/resample_by_2_internal.c \
    signal_processing/resample_fractional.c \
    signal_processing/spl_init.c \
    signal_processing/spl_sqrt.c \
    signal_processing/spl_sqrt_floor.c \
    signal_processing/splitting_filter.c \
    signal_processing/sqrt_of_one_minus_x_squared.c \
    signal_processing/vector_scaling_operations.c

sigproc_headers = \
    signal_processing/complex_fft_tables.h \
    signal_processing/include/real_fft.h \
    signal_processing/include/signal_processing_library.h \
    signal_processing/include/spl_inl_armv7.h \
    signal_processing/include/spl_inl.h \
    signal_processing/include/spl_inl_mips.h \
    signal_processing/resample_by_2_internal.h

libilbc_la_SOURCES = \
    typedefs.h \
    $(ilbc_sources) \
    $(ilbc_headers) \
    $(sigproc_sources) \
    $(sigproc_headers)

dist_doc_DATA = \
    $(top_srcdir)/NEWS.md \
    $(top_srcdir)/README.md

DISTCLEANFILES = \
    doltcompile \
    doltlibtool

EXTRA_DIST = \
    $(top_srcdir)/CMakeLists.txt \
    $(top_srcdir)/config.h.cmakein \
    $(top_srcdir)/ilbc/OWNERS \
    $(top_srcdir)/ilbc/*.gypi \
    $(top_srcdir)/ilbc/*.m \
    $(top_srcdir)/ilbc/test/* \
    $(top_srcdir)/signal_processing/*_mips.c \
    $(top_srcdir)/signal_processing/*.cc \
    $(top_srcdir)/signal_processing/*.S
