find_package(FFmpeg REQUIRED)

include_directories(
	${FFMPEG_LIBAVFORMAT_INCLUDE_DIRS}
	${FFMPEG_LIBAVCODEC_INCLUDE_DIRS}
	${FFMPEG_LIBAVUTIL_INCLUDE_DIRS}
	${AUDIO_PROCESSOR_INCLUDE_DIRS}
)

set(fpcalc_LIBS
	chromaprint
	${FFMPEG_LIBAVFORMAT_LIBRARIES}
	${FFMPEG_LIBAVCODEC_LIBRARIES}
	${FFMPEG_LIBAVUTIL_LIBRARIES}
	${AUDIO_PROCESSOR_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
)

set(fpcalc_LINK_FLAGS)

if(APPLE)
	set(fpcalc_LIBS ${fpcalc_LIBS} -lz)
	set(fpcalc_LINK_FLAGS "${fpcalc_LINK_FLAGS} -sectcreate __TEXT __info_plist ${CMAKE_CURRENT_BINARY_DIR}/fpcalc.Info.plist")
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fpcalc.Info.plist.in ${CMAKE_CURRENT_BINARY_DIR}/fpcalc.Info.plist)
endif()

if(WIN32)
	set(fpcalc_LIBS ${fpcalc_LIBS} bcrypt)
endif()

add_library(fpcalc_libs INTERFACE)
target_link_libraries(fpcalc_libs INTERFACE ${fpcalc_LIBS})

add_executable(fpcalc fpcalc.cpp)
target_link_libraries(fpcalc PRIVATE fpcalc_libs)
if(fpcalc_LINK_FLAGS)
	set_target_properties(fpcalc PROPERTIES LINK_FLAGS ${fpcalc_LINK_FLAGS})
endif()

install(TARGETS fpcalc RUNTIME DESTINATION ${BIN_INSTALL_DIR})
