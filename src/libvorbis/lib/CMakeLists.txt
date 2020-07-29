set(VORBIS_PUBLIC_HEADERS
    ../include/vorbis/codec.h
    ../include/vorbis/vorbisenc.h
    ../include/vorbis/vorbisfile.h
)

set(VORBIS_HEADERS
    envelope.h
    lpc.h
    lsp.h
    codebook.h
    misc.h
    psy.h
    masking.h
    os.h
    mdct.h
    smallft.h
    highlevel.h
    registry.h
    scales.h
    window.h
    lookup.h
    lookup_data.h
    codec_internal.h
    backends.h
    bitrate.h
)

set(VORBIS_SOURCES
    mdct.c
    smallft.c
    block.c
    envelope.c
    window.c
    lsp.c
    lpc.c
    analysis.c
    synthesis.c
    psy.c
    info.c
    floor1.c
    floor0.c
    res0.c
    mapping0.c
    registry.c
    codebook.c
    sharedbook.c
    lookup.c
    bitrate.c
)

set(VORBISFILE_SOURCES
    vorbisfile.c
)

set(VORBISENC_SOURCES
    vorbisenc.c
)

if(WIN32)
    list(APPEND VORBIS_SOURCES vorbisenc.c)
endif()

if(WIN32)
    list(APPEND VORBIS_SOURCES ../win32/vorbis.def)
    list(APPEND VORBISENC_SOURCES ../win32/vorbisenc.def)
    list(APPEND VORBISFILE_SOURCES ../win32/vorbisfile.def)
endif()

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
endif()

if (NOT BUILD_FRAMEWORK)
    add_library(vorbis ${VORBIS_HEADERS} ${VORBIS_SOURCES})
    add_library(vorbisenc ${VORBISENC_SOURCES})
    add_library(vorbisfile ${VORBISFILE_SOURCES})

    get_version_info(VORBIS_VERSION_INFO "V_LIB_CURRENT" "V_LIB_AGE" "V_LIB_REVISION")
    set_target_properties(vorbis PROPERTIES SOVERSION ${VORBIS_VERSION_INFO})
    get_version_info(VORBISENC_VERSION_INFO "VE_LIB_CURRENT" "VE_LIB_AGE" "VE_LIB_REVISION")
    set_target_properties(vorbisenc PROPERTIES SOVERSION ${VORBISENC_VERSION_INFO})
    get_version_info(VORBISFILE_VERSION_INFO "VF_LIB_CURRENT" "VF_LIB_AGE" "VF_LIB_REVISION")
    set_target_properties(vorbisfile PROPERTIES SOVERSION ${VORBISFILE_VERSION_INFO})

    target_include_directories(vorbis
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
       PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_include_directories(vorbisenc
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_include_directories(vorbisfile
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

    target_link_libraries(vorbis
        PUBLIC Ogg::ogg
        PRIVATE $<$<BOOL:${HAVE_LIBM}>:m>
    )
    target_link_libraries(vorbisenc PUBLIC vorbis)
    target_link_libraries(vorbisfile PUBLIC vorbis)

    install(FILES ${VORBIS_PUBLIC_HEADERS} DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/vorbis)

    install(TARGETS vorbis vorbisenc vorbisfile
        EXPORT VorbisTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    if(INSTALL_CMAKE_PACKAGE_MODULE)

    set(CMAKE_INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/Vorbis)

        install(EXPORT VorbisTargets
            DESTINATION ${CMAKE_INSTALL_CONFIGDIR}
            NAMESPACE Vorbis::
        )


        include(CMakePackageConfigHelpers)

        configure_package_config_file(${PROJECT_SOURCE_DIR}/cmake/VorbisConfig.cmake.in ${PROJECT_BINARY_DIR}/VorbisConfig.cmake
            INSTALL_DESTINATION ${CMAKE_INSTALL_CONFIGDIR}
        )

        write_basic_package_version_file(${PROJECT_BINARY_DIR}/VorbisConfigVersion.cmake
            COMPATIBILITY SameMajorVersion
        )

        install(FILES ${PROJECT_BINARY_DIR}/VorbisConfig.cmake ${PROJECT_BINARY_DIR}/VorbisConfigVersion.cmake
            DESTINATION ${CMAKE_INSTALL_CONFIGDIR}
        )

    endif()
else()
    add_library(vorbis ${VORBIS_PUBLIC_HEADERS} ${VORBIS_HEADERS} ${VORBIS_SOURCES} ${VORBISFILE_SOURCES} ${VORBISENC_SOURCES})
    set_target_properties(vorbis PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION ${PROJECT_VERSION}
        MACOSX_FRAMEWORK_IDENTIFIER org.xiph.vorbis
        MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_FRAMEWORK_BUNDLE_VERSION ${PROJECT_VERSION}
        XCODE_ATTRIBUTE_INSTALL_PATH "@rpath"
        PUBLIC_HEADER "${VORBIS_PUBLIC_HEADERS}"
        OUTPUT_NAME Vorbis
    )
    target_link_libraries(vorbis ${OGG_LIBRARIES})
endif()
