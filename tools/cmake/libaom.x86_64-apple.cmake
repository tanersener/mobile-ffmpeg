#
# Copyright (c) 2017, Alliance for Open Media. All rights reserved
#
# This source code is subject to the terms of the BSD 2 Clause License and the
# Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License was
# not distributed with this source code in the LICENSE file, you can obtain it
# at www.aomedia.org/license/software. If the Alliance for Open Media Patent
# License 1.0 was not distributed with this source code in the PATENTS file, you
# can obtain it at www.aomedia.org/license/patent.
#
if(AOM_BUILD_CMAKE_TOOLCHAINS_X86_64_MAC_CATALYST_CMAKE_)
  return()
endif() # AOM_BUILD_CMAKE_TOOLCHAINS_X86_64_MAC_CATALYST_CMAKE_
set(AOM_BUILD_CMAKE_TOOLCHAINS_X86_64_MAC_CATALYST_CMAKE_ 1)

if(XCODE)
  message(FATAL_ERROR "This toolchain does not support Xcode.")
endif()

set(CMAKE_SYSTEM_PROCESSOR "x86_64")
set(CMAKE_OSX_ARCHITECTURES "x86_64")
set(CMAKE_SYSTEM_NAME "Darwin")
set(CMAKE_OSX_SYSROOT ${SDK_PATH})
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_ARG1 "-arch ${CMAKE_SYSTEM_PROCESSOR}")
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_COMPILER_ARG1 "-arch ${CMAKE_SYSTEM_PROCESSOR}")
