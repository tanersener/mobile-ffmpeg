# 
# Try to find Kiss FFT library  
#
# Defines the following variables:
# 
#   KISSFFT_FOUND - Found the Kiss FFT library
#   KISSFFT_INCLUDE_DIRS - Include directories
#   KISSFFT_SOURCES - Source code to include in your project
#
# Accepts the following variables as input:
#
#   KISSFFT_ROOT - (as CMake or environment variable)
#                  The root directory of Kiss FFT sources
#
# =========================================================
#
# Copyright (C) 2014 Lukas Lalinsky <lalinsky@gmail.com>
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

find_path(KISSFFT_SOURCE_DIR
    NAMES
        tools/kiss_fftr.h
        tools/kiss_fftr.c
    PATHS
        $ENV{KISSFFT_ROOT}
        ${KISSFFT_ROOT}
        ${CMAKE_SOURCE_DIR}/vendor/kissfft
    NO_DEFAULT_PATH
    DOC "Kiss FFT tools headers"
)
mark_as_advanced(KISSFFT_SOURCE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(KissFFT DEFAULT_MSG KISSFFT_SOURCE_DIR)

if(KISSFFT_FOUND)
    SET(KISSFFT_INCLUDE_DIRS
        ${KISSFFT_SOURCE_DIR}
        ${KISSFFT_SOURCE_DIR}/tools
    )
    SET(KISSFFT_SOURCES
        ${KISSFFT_SOURCE_DIR}/kiss_fft.c
        ${KISSFFT_SOURCE_DIR}/tools/kiss_fftr.c
    )
endif()
