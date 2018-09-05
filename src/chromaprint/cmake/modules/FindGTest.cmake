# Locate the Google C++ Testing Framework source directory.
#
# Defines the following variables:
#
#   GTEST_FOUND - Found the Google Testing framework sources
#   GTEST_INCLUDE_DIRS - Include directories
#   GTEST_SOURCE_DIR - Source code directory
#   GTEST_LIBRARIES - libgtest
#   GTEST_MAIN_LIBRARIES - libgtest-main
#   GTEST_BOTH_LIBRARIES - libgtest & libgtest-main
#
# Accepts the following variables as input:
#
#   GTEST_ROOT - (as CMake or environment variable)
#                The root directory of the gtest install prefix
#
# Example usage:
#
#    find_package(GTest REQUIRED)
#    include_directories(${GTEST_INCLUDE_DIRS})
#    add_subdirectory(${GTEST_SOURCE_DIR}
#        ${CMAKE_CURRENT_BINARY_DIR}/gtest_build)
#
#    add_executable(foo foo.cc)
#    target_link_libraries(foo ${GTEST_BOTH_LIBRARIES})
#
#    enable_testing(true)
#    add_test(AllTestsInFoo foo)
#
# =========================================================
#
# Copyright (C) 2012 Lukas Lalinsky <lalinsky@gmail.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#  * Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
#  * Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
#  * The names of Kitware, Inc., the Insight Consortium, or the names of
#    any consortium members, or of any contributors, may not be used to
#    endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


find_path(GTEST_SOURCE_DIR
	NAMES src/gtest-all.cc CMakeLists.txt
	HINTS $ENV{GTEST_ROOT} ${GTEST_ROOT} /usr/src/gtest
)
mark_as_advanced(GTEST_SOURCE_DIR)

find_path(GTEST_INCLUDE_DIR
	NAMES gtest/gtest.h
	HINTS $ENV{GTEST_ROOT}/include ${GTEST_ROOT}/include
)
mark_as_advanced(GTEST_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GTest DEFAULT_MSG GTEST_SOURCE_DIR GTEST_INCLUDE_DIR)

if(GTEST_FOUND)
	set(GTEST_INCLUDE_DIRS ${GTEST_INCLUDE_DIR})
	set(GTEST_LIBRARIES gtest)
	set(GTEST_MAIN_LIBRARIES gtest_main)
    set(GTEST_BOTH_LIBRARIES ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES})
endif()
