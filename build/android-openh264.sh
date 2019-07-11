#!/bin/bash

if [[ -z ${ANDROID_NDK_ROOT} ]]; then
    echo -e "(*) ANDROID_NDK_ROOT not defined\n"
    exit 1
fi

if [[ -z ${ARCH} ]]; then
    echo -e "(*) ARCH not defined\n"
    exit 1
fi

if [[ -z ${API} ]]; then
    echo -e "(*) API not defined\n"
    exit 1
fi

if [[ -z ${BASEDIR} ]]; then
    echo -e "(*) BASEDIR not defined\n"
    exit 1
fi

# ENABLE COMMON FUNCTIONS
. ${BASEDIR}/build/android-common.sh

# PREPARING PATHS & DEFINING ${INSTALL_PKG_CONFIG_DIR}
LIB_NAME="openh264"
set_toolchain_clang_paths ${LIB_NAME}

# PREPARING FLAGS
TARGET_HOST=$(get_target_host)
CFLAGS=$(get_cflags ${LIB_NAME})
CXXFLAGS=$(get_cxxflags ${LIB_NAME})
LDFLAGS=$(get_ldflags ${LIB_NAME})

case ${ARCH} in
    arm-v7a-neon)
        # Enabling NEON causes undefined symbol error for WelsCopy8x8_neon
        # CFLAGS+=" -DHAVE_NEON"
    ;;
    arm64-v8a)
        CFLAGS+=" -DHAVE_NEON_AARCH64"
    ;;
    x86*)
        CFLAGS+=" -DHAVE_AVX2"
    ;;
esac

cd ${BASEDIR}/src/${LIB_NAME} || exit 1

# MAKE SURE THAT ASM IS ENABLED FOR ALL IOS ARCHITECTURES
${SED_INLINE} 's/arm64 aarch64/arm64% aarch64/g' ${BASEDIR}/src/${LIB_NAME}/build/arch.mk
${SED_INLINE} 's/%86 x86_64,/%86 x86_64 x86-64,/g' ${BASEDIR}/src/${LIB_NAME}/build/arch.mk
${SED_INLINE} 's/filter-out arm64,/filter-out arm64%,/g' ${BASEDIR}/src/${LIB_NAME}/build/arch.mk
${SED_INLINE} 's/CFLAGS += -DHAVE_NEON/#CFLAGS += -DHAVE_NEON/g' ${BASEDIR}/src/${LIB_NAME}/build/arch.mk
${SED_INLINE} 's/ifeq (\$(ASM_ARCH), arm64)/ifneq (\$(filter arm64%, \$(ASM_ARCH)),)/g' ${BASEDIR}/src/${LIB_NAME}/codec/common/targets.mk
${SED_INLINE} 's/ifeq (\$(ASM_ARCH), x86)/ifneq (\$(filter %86 x86-64, \$(ASM_ARCH)),)/g' ${BASEDIR}/src/${LIB_NAME}/codec/common/targets.mk
${SED_INLINE} 's/ifeq (\$(ASM_ARCH), arm)/ifneq (\$(filter armv%, \$(ASM_ARCH)),)/g' ${BASEDIR}/src/${LIB_NAME}/codec/common/targets.mk

# DO NOT USE DEFAULT ANDROID ARCH FLAGS
${SED_INLINE} 's/ CFLAGS +=/ #CFLAGS +=/g' ${BASEDIR}/src/${LIB_NAME}/build/platform-android.mk
${SED_INLINE} 's/ LDFLAGS +=/ #LDFLAGS +=/g' ${BASEDIR}/src/${LIB_NAME}/build/platform-android.mk

make clean 2>/dev/null 1>/dev/null

make -j$(get_cpu_count) \
ARCH="$(get_toolchain_arch)" \
CC="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/bin/${CC}" \
CFLAGS="$CFLAGS" \
CXX="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/bin/${CXX}" \
CXXFLAGS="${CXXFLAGS}" \
LDFLAGS="$LDFLAGS" \
OS=android \
PREFIX="${BASEDIR}/prebuilt/android-$(get_target_build)/${LIB_NAME}" \
NDKLEVEL="${API}" \
NDKROOT="${ANDROID_NDK_ROOT}" \
NDK_TOOLCHAIN_VERSION=clang \
TARGET="android-${API}" \
install-static || exit 1

# MANUALLY COPY PKG-CONFIG FILES
cp ${BASEDIR}/src/${LIB_NAME}/openh264-static.pc ${INSTALL_PKG_CONFIG_DIR}/openh264.pc || exit 1