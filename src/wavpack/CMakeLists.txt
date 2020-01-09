cmake_minimum_required(VERSION 3.2)

project(WavPack VERSION 5.2.0)

# Languages

include(CheckLanguage)

check_language(ASM)
if(CMAKE_ASM_COMPILER)
  enable_language(ASM)
endif()

if(MSVC)
  enable_language(ASM_MASM)
else()
  check_language(ASM-ATT)
  if(CMAKE_ASM-ATT_COMPILER)
    enable_language(ASM-ATT)
  endif()
endif()


# Policies

# CMake 3.11 warning: Include file check macros honor CMAKE_REQUIRED_LIBRARIES.
if(POLICY CMP0075)
  cmake_policy(SET CMP0075 NEW)
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
if(CMAKE_VERSION VERSION_LESS 3.11)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
endif()

if(CMAKE_VERSION VERSION_LESS 3.12)
	set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
	set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
	set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
	set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
endif()

include(CheckCCompilerFlag)
include(CheckCLinkerFlag)
include(TestLargeFiles)
include(CheckLanguage)
include(CheckLibraryExists)
include(CheckFunctionExists)
include(CMakeDependentOption)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(FeatureSummary)
include(CPack)
include(FindOpenSSL)

# Options. See also dependent options below

option(WAVPACK_ENABLE_LEGACY "Decode legacy (< 4.0) WavPack files" OFF)
option(WAVPACK_ENABLE_DSD "Enable support for WavPack DSD files" ON)
option(WAVPACK_INSTALL_CMAKE_MODULE "Generate and install CMake package configuration module" ON)
option(WAVPACK_INSTALL_DOCS "Install documentation" ON)
option(WAVPACK_INSTALL_PKGCONFIG_MODULE "Generate and install wavpack.pc" ON)

# Configuration tests

check_library_exists(m cos "" HAVE_LIBM)
if(HAVE_LIBM)
    list(APPEND CMAKE_REQUIRED_LIBRARIES m)
endif()

find_package(Threads)
find_package(Iconv)
find_package(LibXslt)
find_package(OpenSSL)

set(DOCBOOK_PREREQS_FOUND FALSE)
if(LIBXSLT_XSLTPROC_EXECUTABLE)
    if(NOT $ENV{XML_CATALOG_FILES} STREQUAL "")
        set(XML_CATALOG_FILEPATH $ENV{XML_CATALOG_FILES})
    else()
        set(XML_CATALOG_FILEPATH "/etc/xml/catalog")
    endif()

    find_program(XMLCATALOG_EXECUTABLE xmlcatalog)

    if(XMLCATALOG_EXECUTABLE)
        execute_process(
            COMMAND ${XMLCATALOG_EXECUTABLE} --noout ${XML_CATALOG_FILEPATH} "-//OASIS//DTD DocBook XML V4.1.2//EN"
            RESULT_VARIABLE DOCBOOK_XML_RSLT
            OUTPUT_QUIET
            ERROR_QUIET
        )
        if(DOCBOOK_XML_RSLT EQUAL 0)
            execute_process(
                COMMAND ${XMLCATALOG_EXECUTABLE} --noout ${XML_CATALOG_FILEPATH} "http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl"
                RESULT_VARIABLE DOCBOOK_XSL_RSLT
                OUTPUT_QUIET
                ERROR_QUIET
            )
            if(DOCBOOK_XML_RSLT EQUAL 0)
                set(DOCBOOK_PREREQS_FOUND TRUE)
            endif()
        endif()
    endif()
endif()

check_c_source_compiles(
        "int main()
        {
            return __builtin_clz(1);
        }"
        HAVE___BUILTIN_CLZ
)

test_large_files(LARGE_FILES_SUPPORTED)
if(LARGE_FILES_SUPPORTED)
    add_definitions(${LARGE_FILES_DEFINITIONS})
endif()
check_function_exists(fseeko HAVE_FSEEKO)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "amd64.*|x86_64.*|AMD64.*")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CPU_ASM_X64 1)
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(CPU_ASM_X86 1)
    endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386.*|i486.*|i586.*|i686.*|i786.*")
    set(CPU_ASM_X86 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm*")
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(CPU_ASM_ARM32 1)
    endif()
endif()

if(WIN32 AND (NOT MINGW))
    if(CMAKE_ASM_MASM_COMPILER AND (CPU_ASM_X86 OR CPU_ASM_X64))
        set(HAVE_MASM 1)
    endif()
else()
    if(CMAKE_ASM-ATT_COMPILER)
        if(CPU_ASM_X86 OR CPU_ASM_X64 OR CPU_ASM_ARM32)
            set(HAVE_ASM 1)
        endif()
    endif()
endif()

# Dependent options

cmake_dependent_option(WAVPACK_ENABLE_ASM "Enable assembly optimizations" ON "HAVE_ASM OR HAVE_MASM" OFF)
cmake_dependent_option(WAVPACK_ENABLE_LIBCRYPTO "Use OpenSSL::Crypto library" ON "OPENSSL_FOUND" OFF)
cmake_dependent_option(WAVPACK_BUILD_PROGRAMS "Build programs" ON "Iconv_FOUND" OFF)
cmake_dependent_option(WAVPACK_BUILD_COOLEDIT_PLUGIN "Build CoolEdit plugin" ON "WIN32" OFF)
cmake_dependent_option(WAVPACK_BUILD_WINAMP_PLUGIN "Build WinAmp plugin" ON "WIN32" OFF)
cmake_dependent_option(BUILD_TESTING "Build tests" ON "NOT WIN32" OFF)
cmake_dependent_option(WAVPACK_BUILD_DOCS "Build documentation" ON "DOCBOOK_PREREQS_FOUND" OFF)

# Targets

add_library(wavpack
    src/common_utils.c
    src/decorr_utils.c
    src/entropy_utils.c
    src/extra1.c
    src/extra2.c
    src/open_utils.c
    src/open_filename.c
    src/open_legacy.c
    src/open_raw.c
    src/pack.c
    src/pack_dns.c
    src/pack_floats.c
    src/pack_utils.c
    src/read_words.c
    src/tags.c
    src/tag_utils.c
    src/unpack.c
    src/unpack_floats.c
    src/unpack_seek.c
    src/unpack_utils.c
    src/write_words.c
    src/decorr_tables.h
    src/unpack3.h
    src/wavpack_local.h
    src/wavpack_version.h
    $<$<BOOL:${WAVPACK_ENABLE_LEGACY}>:src/unpack3.c>
    $<$<BOOL:${WAVPACK_ENABLE_LEGACY}>:src/unpack3_open.c>
    $<$<BOOL:${WAVPACK_ENABLE_LEGACY}>:src/unpack3_seek.c>
    $<$<BOOL:${WAVPACK_ENABLE_DSD}>:src/pack_dsd.c>
    $<$<BOOL:${WAVPACK_ENABLE_DSD}>:src/unpack_dsd.c>
    $<$<AND:$<BOOL:${WAVPACK_ENABLE_ASM}>,$<BOOL:${HAVE_MASM}>,$<BOOL:${CPU_ASM_X86}>>:src/pack_x86.asm>
    $<$<AND:$<BOOL:${WAVPACK_ENABLE_ASM}>,$<BOOL:${HAVE_MASM}>,$<BOOL:${CPU_ASM_X86}>>:src/unpack_x86.asm>
    $<$<AND:$<BOOL:${WAVPACK_ENABLE_ASM}>,$<BOOL:${HAVE_MASM}>,$<BOOL:${CPU_ASM_X64}>>:src/pack_x64.asm>
    $<$<AND:$<BOOL:${WAVPACK_ENABLE_ASM}>,$<BOOL:${HAVE_MASM}>,$<BOOL:${CPU_ASM_X64}>>:src/unpack_x64.asm>
    $<$<AND:$<BOOL:${WAVPACK_ENABLE_ASM}>,$<BOOL:${HAVE_ASM}>,$<BOOL:${CPU_ASM_X86}>>:src/pack_x86.S>
    $<$<AND:$<BOOL:${WAVPACK_ENABLE_ASM}>,$<BOOL:${HAVE_ASM}>,$<BOOL:${CPU_ASM_X86}>>:src/unpack_x86.S>
    $<$<AND:$<BOOL:${WAVPACK_ENABLE_ASM}>,$<BOOL:${HAVE_ASM}>,$<BOOL:${CPU_ASM_X64}>>:src/pack_x64.S>
    $<$<AND:$<BOOL:${WAVPACK_ENABLE_ASM}>,$<BOOL:${HAVE_ASM}>,$<BOOL:${CPU_ASM_X64}>>:src/unpack_x64.S>
    $<$<AND:$<BOOL:${WAVPACK_ENABLE_ASM}>,$<BOOL:${HAVE_ASM}>,$<BOOL:${CPU_ASM_ARM32}>>:src/unpack_armv7.S>
    $<$<AND:$<BOOL:${WIN32}>,$<BOOL:${BUILD_SHARED_LIBS}>>:wavpackdll/wavpackdll.rc>
)
target_include_directories(wavpack
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        $<$<BOOL:${WAVPACK_ENABLE_LIBCRYPTO}>:${OPENSSL_INCLUDE_DIR}>
)
target_link_libraries(wavpack
    PRIVATE
        $<$<BOOL:${HAVE_LIBM}>:m>
        $<$<BOOL:${WAVPACK_ENABLE_LIBCRYPTO}>:${OPENSSL_CRYPTO_LIBRARY}>
)
target_compile_definitions(wavpack
    PRIVATE
        $<$<BOOL:${WAVPACK_ENABLE_LEGACY}>:ENABLE_LEGACY>
        $<$<BOOL:${WAVPACK_ENABLE_DSD}>:ENABLE_DSD>
        $<$<BOOL:${MSVC}>:_CRT_SECURE_NO_WARNINGS>
        $<$<BOOL:${HAVE___BUILTIN_CLZ}>:HAVE___BUILTIN_CLZ>
        $<$<BOOL:${HAVE_FSEEKO}>:HAVE_FSEEKO>
        $<$<BOOL:${WAVPACK_ENABLE_LIBCRYPTO}>:HAVE_LIBCRYPTO>
        $<$<AND:$<BOOL:${WAVPACK_ENABLE_ASM}>,$<BOOL:${CPU_ASM_X86}>>:OPT_ASM_X86>
        $<$<AND:$<BOOL:${WAVPACK_ENABLE_ASM}>,$<BOOL:${CPU_ASM_X64}>>:OPT_ASM_X64>
        $<$<AND:$<BOOL:${WAVPACK_ENABLE_ASM}>,$<BOOL:${CPU_ASM_ARM32}>>:OPT_ASM_ARM32>
)

if(WAVPACK_ENABLE_ASM AND HAVE_MASM AND CPU_ASM_X86)
	set_source_files_properties(src/pack_x86.asm src/unpack_x86.asm PROPERTIES COMPILE_FLAGS "/safeseh")
endif()

if(WIN32 AND (NOT MINGW))
    if(BUILD_SHARED_LIBS)
        set_target_properties(wavpack PROPERTIES OUTPUT_NAME wavpackdll)
    else()
        set_target_properties(wavpack PROPERTIES OUTPUT_NAME libwavpack)
    endif()
endif()

# Symbols files

set(WAVPACK_EXPORT_SYMBOLS
    WavpackAddWrapper
    WavpackAppendBinaryTagItem
    WavpackAppendTagItem
    WavpackBigEndianToNative
    WavpackCloseFile
    WavpackDeleteTagItem
    WavpackFloatNormalize
    WavpackFlushSamples
    WavpackFreeWrapper
    WavpackGetAverageBitrate
    WavpackGetBinaryTagItem
    WavpackGetBinaryTagItemIndexed
    WavpackGetBitsPerSample
    WavpackGetBytesPerSample
    WavpackGetChannelIdentities
    WavpackGetChannelLayout
    WavpackGetChannelMask
    WavpackGetEncodedNoise
    WavpackGetErrorMessage
    WavpackGetFileExtension
    WavpackGetFileFormat
    WavpackGetFileSize
    WavpackGetFileSize64
    WavpackGetFloatNormExp
    WavpackGetInstantBitrate
    WavpackGetLibraryVersion
    WavpackGetLibraryVersionString
    WavpackGetMD5Sum
    WavpackGetMode
    WavpackGetNativeSampleRate
    WavpackGetNumBinaryTagItems
    WavpackGetNumChannels
    WavpackGetNumErrors
    WavpackGetNumSamples
    WavpackGetNumSamples64
    WavpackGetNumSamplesInFrame
    WavpackGetNumTagItems
    WavpackGetProgress
    WavpackGetQualifyMode
    WavpackGetRatio
    WavpackGetReducedChannels
    WavpackGetSampleIndex
    WavpackGetSampleIndex64
    WavpackGetSampleRate
    WavpackGetTagItem
    WavpackGetTagItemIndexed
    WavpackGetVersion
    WavpackGetWrapperBytes
    WavpackGetWrapperData
    WavpackGetWrapperLocation
    WavpackLittleEndianToNative
    WavpackLossyBlocks
    WavpackNativeToBigEndian
    WavpackNativeToLittleEndian
    WavpackOpenFileInput
    WavpackOpenFileInputEx
    WavpackOpenFileInputEx64
    WavpackOpenFileOutput
    WavpackOpenRawDecoder
    WavpackPackInit
    WavpackPackSamples
    WavpackSeekSample
    WavpackSeekSample64
    WavpackSeekTrailingWrapper
    WavpackSetChannelLayout
    WavpackSetConfiguration
    WavpackSetConfiguration64
    WavpackSetFileInformation
    WavpackStoreMD5Sum
    WavpackUnpackSamples
    WavpackUpdateNumSamples
    WavpackVerifySingleBlock
    WavpackWriteTag
)

if(BUILD_SHARED_LIBS AND (WIN32 OR CYGWIN))
    set(FILE_CONTENTS "EXPORTS\n")
    foreach(EXPORT_SYMBOL ${WAVPACK_EXPORT_SYMBOLS})
        list(APPEND FILE_CONTENTS "    ${EXPORT_SYMBOL}\n")
    endforeach()

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/libwavpack.def ${FILE_CONTENTS})
    target_sources(wavpack PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/libwavpack.def)

elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(FILE_CONTENTS "")
    foreach(EXPORT_SYMBOL ${WAVPACK_EXPORT_SYMBOLS})
        list(APPEND FILE_CONTENTS "_${EXPORT_SYMBOL}\n")
    endforeach()
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/libwavpack.sym ${FILE_CONTENTS})
    target_link_libraries(wavpack PRIVATE "-Wl,-exported_symbols_list,'${CMAKE_CURRENT_BINARY_DIR}/libwavpack.sym'")
else()
    set(CONFTTEST_CONTENTS "VERS_1 {\n    global: sym\;\n\n};\n\nVERS_2 {\n    global: sym;\n} VERS_1\;")
    file(WRITE ${PROJECT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/conftest.map "${CONFTTEST_CONTENTS}")
    check_c_linker_flag("-Wl,--version-script='${PROJECT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/conftest.map'" COMPILER_SUPPORTS_SYMBOL_MAPS)

    if(COMPILER_SUPPORTS_SYMBOL_MAPS)
        set(FILE_CONTENTS "{ global:\n")
        foreach(EXPORT_SYMBOL ${WAVPACK_EXPORT_SYMBOLS})
            list(APPEND FILE_CONTENTS "${EXPORT_SYMBOL}\;\n")
        endforeach()
        list(APPEND FILE_CONTENTS "local: *\; }\;")
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/libwavpack.map ${FILE_CONTENTS})
        target_link_libraries(wavpack PRIVATE "-Wl,--version-script='${CMAKE_CURRENT_BINARY_DIR}/libwavpack.map';-Wl,-no-undefined")
    endif()
endif()

set_target_properties(wavpack PROPERTIES PUBLIC_HEADER include/wavpack.h)

if(WAVPACK_BUILD_PROGRAMS)

	add_executable(wavpackapp
		cli/wavpack.c
		cli/riff.c
		cli/wave64.c
		cli/caff.c
		cli/dsdiff.c
		cli/dsf.c
		cli/utils.c
		cli/md5.c
		cli/import_id3.c
		$<$<BOOL:${WIN32}>:cli/win32_unicode_support.h>
		$<$<BOOL:${WIN32}>:cli/win32_unicode_support.c>
	)
	target_compile_definitions(wavpackapp
		PRIVATE
			$<$<BOOL:${MSVC}>:_CRT_SECURE_NO_WARNINGS>
			"PACKAGE_VERSION=\"${PROJECT_VERSION}\""
			"VERSION_OS=\"${CMAKE_SYSTEM_NAME}\""
	)
	set_target_properties(wavpackapp PROPERTIES OUTPUT_NAME wavpack)
	target_link_libraries(wavpackapp
		PRIVATE
			wavpack
			Iconv::Iconv
			$<$<BOOL:${HAVE_LIBM}>:m>
	)

	add_executable(wvunpack
		cli/wvunpack.c
		cli/riff_write.c
		cli/wave64_write.c
		cli/caff_write.c
		cli/dsdiff_write.c
		cli/dsf_write.c
		cli/utils.c
		cli/md5.c
		$<$<BOOL:${WIN32}>:cli/win32_unicode_support.h>
		$<$<BOOL:${WIN32}>:cli/win32_unicode_support.c>
	)
	target_compile_definitions(wvunpack
		PRIVATE
			$<$<BOOL:${MSVC}>:_CRT_SECURE_NO_WARNINGS>
			"PACKAGE_VERSION=\"${PROJECT_VERSION}\""
			"VERSION_OS=\"${CMAKE_SYSTEM_NAME}\""
	)
	set_target_properties(wvunpack PROPERTIES OUTPUT_NAME wvunpack)
	target_link_libraries(wvunpack
		PRIVATE
			wavpack
			Iconv::Iconv
			$<$<BOOL:${HAVE_LIBM}>:m>
	)

	add_executable(wvgain
		cli/wvgain.c
		cli/utils.h
		cli/utils.c
		$<$<BOOL:${WIN32}>:cli/win32_unicode_support.h>
		$<$<BOOL:${WIN32}>:cli/win32_unicode_support.c>
	)
	target_compile_definitions(wvgain
		PRIVATE
			$<$<BOOL:${MSVC}>:_CRT_SECURE_NO_WARNINGS>
			"PACKAGE_VERSION=\"${PROJECT_VERSION}\""
			"VERSION_OS=\"${CMAKE_SYSTEM_NAME}\""
	)
	set_target_properties(wvgain PROPERTIES OUTPUT_NAME wvgain)
	target_link_libraries(wvgain
		PRIVATE
			wavpack
			$<$<BOOL:${HAVE_LIBM}>:m>
	)

	add_executable(wvtag
		cli/wvtag.c
		cli/utils.c
		cli/import_id3.c
		$<$<BOOL:${WIN32}>:cli/win32_unicode_support.h>
		$<$<BOOL:${WIN32}>:cli/win32_unicode_support.c>
	)
	target_compile_definitions(wvtag
		PRIVATE
			$<$<BOOL:${MSVC}>:_CRT_SECURE_NO_WARNINGS>
			"PACKAGE_VERSION=\"${PROJECT_VERSION}\""
			"VERSION_OS=\"${CMAKE_SYSTEM_NAME}\""
	)
	target_link_libraries(wvtag
		PRIVATE
			wavpack
			Iconv::Iconv
			$<$<BOOL:${HAVE_LIBM}>:m>
	)

endif()

if(WAVPACK_BUILD_COOLEDIT_PLUGIN)

    set(WAVPACK_COOLEDIT_PLUGIN_EXPORT_SYMBOLS
        QueryCoolFilter
        FilterUnderstandsFormat
        GetSuggestedSampleType
        OpenFilterInput
        FilterGetFileSize
        ReadFilterInput
        CloseFilterInput
        FilterOptions
        FilterOptionsString
        OpenFilterOutput
        CloseFilterOutput
        WriteFilterOutput
        FilterGetOptions
        FilterWriteSpecialData
        FilterGetFirstSpecialData
        FilterGetNextSpecialData
    )

    set(FILE_CONTENTS "EXPORTS\n")
    foreach(EXPORT_SYMBOL ${WAVPACK_COOLEDIT_PLUGIN_EXPORT_SYMBOLS})
        list(APPEND FILE_CONTENTS "    ${EXPORT_SYMBOL}\n")
    endforeach()

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/audition/cool_wv4.def ${FILE_CONTENTS})

    add_library(cool_wv4 SHARED
        audition/cool_wv4.c
        audition/filters.h
        audition/resource.h
        audition/wavpack.rc
        ${CMAKE_CURRENT_BINARY_DIR}/audition/cool_wv4.def
    )
    target_compile_definitions(cool_wv4 PRIVATE $<$<BOOL:${MSVC}>:_CRT_SECURE_NO_WARNINGS>)
    set_target_properties(cool_wv4 PROPERTIES
        DEFINE_SYMBOL AUDITION_EXPORTS
        SUFFIX ".flt"
        PREFIX ""
    )
    target_link_libraries(cool_wv4 PRIVATE wavpack)

endif()

if(WAVPACK_BUILD_WINAMP_PLUGIN)
    add_library(in_wv SHARED
        winamp/in_wv.c
        winamp/in2.h
        winamp/out.h
        winamp/resource.h
        winamp/wasabi/wasabi.cpp
        winamp/wasabi/wasabi.h
    )
    target_compile_definitions(in_wv PRIVATE $<$<BOOL:${MSVC}>:_CRT_SECURE_NO_DEPRECATE>)
    set_target_properties(in_wv PROPERTIES DEFINE_SYMBOL WINAMP_EXPORTS)
    target_link_libraries(in_wv PRIVATE wavpack)

    add_library(in_wv_lang SHARED
        winamp/wavpack.rc
    )
    target_link_libraries(in_wv_lang PRIVATE $<$<BOOL:${MSVC}>:-NOENTRY>)
    set_target_properties(in_wv_lang PROPERTIES
        RUNTIME_OUTPUT_NAME in_wv
        PREFIX ""
        SUFFIX ".lang"
    )

endif()

set(WAVPACK_DOC_NAMES
    wavpack
    wvgain
    wvunpack
    wvtag
)
if(WAVPACK_BUILD_DOCS)

    foreach(WAVPACK_DOC_NAME ${WAVPACK_DOC_NAMES})
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/man/${WAVPACK_DOC_NAME}.1
            MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/man/${WAVPACK_DOC_NAME}.xml
            COMMAND ${LIBXSLT_XSLTPROC_EXECUTABLE} -nonet --output ${CMAKE_CURRENT_BINARY_DIR}/man/${WAVPACK_DOC_NAME}.1 "http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl" ${CMAKE_CURRENT_SOURCE_DIR}/man/${WAVPACK_DOC_NAME}.xml
            COMMENT "Generating MAN page '${WAVPACK_DOC_NAME}.1' from DocBook XML file ${WAVPACK_DOC_NAME}.xml..."
        )
    endforeach()

    set(GENERATED_MANS_LIST "")
    foreach(WAVPACK_DOC_NAME ${WAVPACK_DOC_NAMES})
        list(APPEND GENERATED_MANS_LIST ${CMAKE_CURRENT_BINARY_DIR}/man/${WAVPACK_DOC_NAME}.1)
    endforeach()
    add_custom_target(GEN_DOCS ALL
        DEPENDS ${GENERATED_MANS_LIST}
    )

endif()

# Features

set_package_properties(OpenSSL PROPERTIES
	TYPE RECOMMENDED
	DESCRIPTION "TLS/SSL and crypto library"
    PURPOSE "Can be used to build wavpack library."
)

set_package_properties(Iconv PROPERTIES
	TYPE RECOMMENDED
	DESCRIPTION "converts from one character encoding to another through Unicode conversion "
    PURPOSE "Required to build programs."
)
set_package_properties(Threads PROPERTIES
	TYPE OPTIONAL
	DESCRIPTION "Threads library"
    PURPOSE "Required to build tests."
)
set_package_properties(LibXslt PROPERTIES
	TYPE OPTIONAL
	DESCRIPTION " XSLT C library developed for the GNOME project."
    PURPOSE "Required to generate documentation."
)

add_feature_info(BUILD_SHARED_LIBS BUILD_SHARED_LIBS "Build shared library.")
add_feature_info(BUILD_TESTING BUILD_TESTING "Build tests.")
add_feature_info(ENABLE_LEGACY WAVPACK_ENABLE_LEGACY "Decode legacy (< 4.0) WavPack files.")
add_feature_info(ENABLE_DSD WAVPACK_ENABLE_DSD "Enable support for WavPack DSD files.")
add_feature_info(INSTALL_CMAKE_MODULE WAVPACK_INSTALL_CMAKE_MODULE "Generate and install CMake package configuration module.")


add_feature_info(ENABLE_ASM WAVPACK_ENABLE_ASM "Enable assembly optimizations.")
add_feature_info(ENABLE_LIBCRYPTO WAVPACK_ENABLE_LIBCRYPTO "Use OpenSSL::Crypto library.")
add_feature_info(BUILD_PROGRAMS WAVPACK_BUILD_PROGRAMS "Build programs.")
add_feature_info(BUILD_COOLEDIT_PLUGIN WAVPACK_BUILD_COOLEDIT_PLUGIN "Build CoolEdit plugin.")
add_feature_info(BUILD_WINAMP_PLUGIN WAVPACK_BUILD_WINAMP_PLUGIN "Build WinAmp plugin.")
add_feature_info(BUILD_DOCS WAVPACK_BUILD_DOCS "Build documentation.")
add_feature_info(INSTALL_DOCS WAVPACK_INSTALL_DOCS "Install documentation.")
add_feature_info(INSTALL_PKGCONFIG_MODULE WAVPACK_INSTALL_PKGCONFIG_MODULE "Generate and install wavpack.pc.")

feature_summary(WHAT ALL)

# Installation

set(WAVPACK_INSTALL_TARGETS wavpack)
if(WAVPACK_BUILD_PROGRAMS)
	list(APPEND WAVPACK_INSTALL_TARGETS wavpackapp wvunpack wvgain wvtag)
endif()

install(TARGETS ${WAVPACK_INSTALL_TARGETS}
    EXPORT wavpack-targets
    RUNTIME DESTINATION         ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION         ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION         ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION   ${CMAKE_INSTALL_INCLUDEDIR}
)

if(WAVPACK_INSTALL_CMAKE_MODULE)

	if(WIN32)
		set(CMAKE_INSTALL_PACKAGEDIR cmake)
	else()
		set(CMAKE_INSTALL_PACKAGEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
	endif()

	if(CMAKE_VERSION VERSION_LESS 3.11)
		write_basic_package_version_file(wavpack-config-version.cmake COMPATIBILITY SameMajorVersion)
	else()
		write_basic_package_version_file(wavpack-config-version.cmake COMPATIBILITY SameMinorVersion)
	endif()

	install(EXPORT wavpack-targets
		FILE wavpack-config.cmake
		NAMESPACE WavPack::
		DESTINATION ${CMAKE_INSTALL_PACKAGEDIR}
	)
	install(
		FILES ${CMAKE_CURRENT_BINARY_DIR}/wavpack-config-version.cmake
		DESTINATION	${CMAKE_INSTALL_PACKAGEDIR}
	)
endif()

if(WAVPACK_INSTALL_PKGCONFIG_MODULE)
	set (prefix ${CMAKE_INSTALL_PREFIX})
	set (exec_prefix "\$\{prefix\}")
	set (libdir "\$\{prefix\}/${CMAKE_INSTALL_LIBDIR}")
	set (includedir "\$\{prefix\}/${CMAKE_INSTALL_INCLUDEDIR}")
	set (PACKAGE_VERSION ${PROJECT_VERSION})
	if (HAVE_LIBM)
		set (LIBM "-lm")
	endif ()
	configure_file (wavpack.pc.in wavpack.pc @ONLY)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/wavpack.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()

if(WAVPACK_INSTALL_DOCS)
    set(MAN_FILES_INSTALL_LIST "")
    if(WAVPACK_BUILD_DOCS)
        foreach(WAVPACK_DOC_NAME ${WAVPACK_DOC_NAMES})
            list(APPEND MAN_FILES_INSTALL_LIST ${CMAKE_CURRENT_BINARY_DIR}/man/${WAVPACK_DOC_NAME}.1)
        endforeach()
    else()
        foreach(WAVPACK_DOC_NAME ${WAVPACK_DOC_NAMES})
            list(APPEND MAN_FILES_INSTALL_LIST ${CMAKE_CURRENT_SOURCE_DIR}/man/${WAVPACK_DOC_NAME}.1)
        endforeach()
    endif()

    install(
        FILES ${MAN_FILES_INSTALL_LIST}
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
    )
endif()

# Tests

if(BUILD_TESTING AND (NOT WIN32))

	enable_testing()

    add_executable(wvtest
        cli/wvtest.c
        cli/md5.c
        $<$<BOOL:${WIN32}>:cli/win32_unicode_support.h>
        $<$<BOOL:${WIN32}>:cli/win32_unicode_support.c>
    )
    target_compile_definitions(wvtest
        PRIVATE
            "PACKAGE_VERSION=\"${PROJECT_VERSION}\""
            "VERSION_OS=\"${CMAKE_SYSTEM_NAME}\"")
    target_link_libraries(wvtest
        PRIVATE
            wavpack
            Threads::Threads
            $<$<BOOL:${HAVE_LIBM}>:m>
    )
    add_test(NAME wvtest COMMAND $<TARGET_FILE:wvtest> --exhaustive --short --no-extras)

endif()
