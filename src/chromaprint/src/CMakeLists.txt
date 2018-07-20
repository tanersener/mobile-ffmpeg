include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(chromaprint_SOURCES
	audio_processor.cpp
	chroma.cpp
	chroma_resampler.cpp
	chroma_filter.cpp
	spectrum.cpp
	fft.cpp
	fingerprinter.cpp
	image_builder.cpp
	simhash.h
	simhash.cpp
	silence_remover.cpp
	fingerprint_calculator.cpp
	fingerprint_compressor.cpp
	fingerprint_decompressor.cpp
	fingerprinter_configuration.cpp
	fingerprint_matcher.h
	fingerprint_matcher.cpp
	utils/base64.h
	utils/base64.cpp
	utils/gradient.h
	utils/gaussian_filter.h
	utils/scope_exit.h
	utils/rolling_integral_image.h
	audio/audio_slicer.h
	avresample/resample2.c
)

set(chromaprint_PUBLIC_SOURCES chromaprint.cpp)
set(chromaprint_PUBLIC_HEADERS chromaprint.h)

if(USE_AVFFT)
	set(chromaprint_SOURCES fft_lib_avfft.cpp ${chromaprint_SOURCES})
	set(chromaprint_LINK_LIBS
		${FFMPEG_LIBAVCODEC_LIBRARIES}
		${FFMPEG_LIBAVUTIL_LIBRARIES}
	)
	include_directories(
		${FFMPEG_LIBAVCODEC_INCLUDE_DIRS}
		${FFMPEG_LIBAVUTIL_INCLUDE_DIRS}
	)
endif(USE_AVFFT)

if(USE_FFTW3)
	set(chromaprint_SOURCES fft_lib_fftw3.cpp ${chromaprint_SOURCES})
	set(chromaprint_LINK_LIBS ${FFTW3_LIBRARIES})
	include_directories(${FFTW3_INCLUDE_DIR})
endif(USE_FFTW3)

if(USE_FFTW3F)
	set(chromaprint_SOURCES fft_lib_fftw3.cpp ${chromaprint_SOURCES})
	set(chromaprint_LINK_LIBS ${FFTW3_FFTWF_LIBRARY})
	include_directories(${FFTW3_INCLUDE_DIR})
endif(USE_FFTW3F)

if(USE_VDSP)
	set(chromaprint_SOURCES fft_lib_vdsp.cpp ${chromaprint_SOURCES})
	set(chromaprint_LINK_LIBS ${ACCELERATE_LIBRARIES})
endif()

if(USE_KISSFFT)
	set(chromaprint_SOURCES fft_lib_kissfft.cpp ${chromaprint_SOURCES} ${KISSFFT_SOURCES})
	include_directories(${KISSFFT_INCLUDE_DIRS})
endif()

add_library(chromaprint_objs OBJECT ${chromaprint_SOURCES})
if(BUILD_SHARED_LIBS)
	set_target_properties(chromaprint_objs PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
endif()

add_library(chromaprint ${chromaprint_PUBLIC_SOURCES} ${chromaprint_PUBLIC_HEADERS} $<TARGET_OBJECTS:chromaprint_objs>)
set_target_properties(chromaprint PROPERTIES
	DEFINE_SYMBOL CHROMAPRINT_API_EXPORTS
	PUBLIC_HEADER ${chromaprint_PUBLIC_HEADERS}
    VERSION ${chromaprint_VERSION}
    SOVERSION ${chromaprint_SOVERSION}
)
if(BUILD_FRAMEWORK)
	set_target_properties(chromaprint PROPERTIES FRAMEWORK TRUE)
endif()
target_link_libraries(chromaprint ${chromaprint_LINK_LIBS})

install(TARGETS chromaprint
	FRAMEWORK DESTINATION ${FRAMEWORK_INSTALL_DIR}
	LIBRARY DESTINATION ${LIB_INSTALL_DIR}
	RUNTIME DESTINATION ${BIN_INSTALL_DIR}
	ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
	PUBLIC_HEADER DESTINATION ${INCLUDE_INSTALL_DIR}
)

if(BUILD_TOOLS)
	add_subdirectory(cmd)
endif()
