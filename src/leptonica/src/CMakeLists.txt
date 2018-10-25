#
# leptonica
#

################################################################################

########################################
# SHARED LIBRARY leptonica
########################################

file(GLOB hdr "*.h")
file(GLOB src "*.c")
if (MSVC)
    set_source_files_properties(${src} PROPERTIES LANGUAGE CXX)
endif()

string(REPLACE "-O3" "-O2" CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})

add_library                     (leptonica ${LIBRARY_TYPE} ${src} ${hdr})
set_target_properties           (leptonica PROPERTIES VERSION   ${VERSION_PLAIN})
set_target_properties           (leptonica PROPERTIES SOVERSION 5.3.0)
if (WIN32)
set_target_properties           (leptonica PROPERTIES OUTPUT_NAME       leptonica-${VERSION_PLAIN})
set_target_properties           (leptonica PROPERTIES DEBUG_OUTPUT_NAME leptonica-${VERSION_PLAIN}d)
else()
set_target_properties           (leptonica PROPERTIES OUTPUT_NAME       leptonica)
set_target_properties           (leptonica PROPERTIES DEBUG_OUTPUT_NAME leptonica)
endif()

if (NOT STATIC)
    target_compile_definitions  (leptonica PRIVATE -DLIBLEPT_EXPORTS)
endif()

if (GIF_LIBRARY)
    target_include_directories  (leptonica PUBLIC ${GIF_INCLUDE_DIR})
    target_link_libraries       (leptonica ${GIF_LIBRARY})
endif()
if (JPEG_LIBRARY)
    target_include_directories  (leptonica PUBLIC ${JPEG_INCLUDE_DIR})
    target_link_libraries       (leptonica ${JPEG_LIBRARY})
endif()
if (JP2K_FOUND)
    target_include_directories  (leptonica PUBLIC ${JP2K_INCLUDE_DIR})
    target_link_libraries       (leptonica ${JP2K_LIBRARIES})
endif()
if (PNG_LIBRARY)
    target_include_directories  (leptonica PUBLIC ${PNG_INCLUDE_DIRS})
    target_link_libraries       (leptonica ${PNG_LIBRARY})
endif()
if (TIFF_LIBRARY)
    target_include_directories  (leptonica PUBLIC ${TIFF_INCLUDE_DIR})
    target_link_libraries       (leptonica ${TIFF_LIBRARY})
endif()
if (WEBP_FOUND)
    target_include_directories  (leptonica PUBLIC ${WEBP_INCLUDE_DIR})
    target_link_libraries       (leptonica ${WEBP_LIBRARIES})
endif()
if (ZLIB_LIBRARY)
    target_include_directories  (leptonica PUBLIC ${ZLIB_INCLUDE_DIR})
    target_link_libraries       (leptonica ${ZLIB_LIBRARY})
endif()

if (UNIX)
    target_link_libraries       (leptonica m)
endif()

if (NOT CPPAN_BUILD)
    export(TARGETS leptonica FILE ${CMAKE_BINARY_DIR}/LeptonicaTargets.cmake)
else()
    target_link_libraries       (leptonica
        pvt.cppan.demo.gif
        pvt.cppan.demo.jpeg
        pvt.cppan.demo.png
        pvt.cppan.demo.tiff
        pvt.cppan.demo.webmproject.webp
        pvt.cppan.demo.openjpeg.openjp2
    )
    file(WRITE ${CMAKE_BINARY_DIR}/LeptonicaTargets.cmake "include(${CMAKE_BINARY_DIR}/cppan.cmake)\n")
    export(TARGETS leptonica APPEND FILE ${CMAKE_BINARY_DIR}/LeptonicaTargets.cmake)
endif()

install(TARGETS leptonica EXPORT LeptonicaTargets RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(EXPORT LeptonicaTargets DESTINATION cmake)
install(FILES
    ${hdr}
    ${CMAKE_BINARY_DIR}/src/endianness.h
    DESTINATION include/leptonica)

################################################################################
