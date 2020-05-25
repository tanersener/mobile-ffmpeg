LOCAL_PATH := $(call my-dir)
$(call import-add-path, $(LOCAL_PATH))
FFMPEG_INCLUDES := $(LOCAL_PATH)/../../prebuilt/android-$(TARGET_ARCH)/ffmpeg/include

MY_ARM_MODE := arm
MY_ARM_NEON := false
LOCAL_PATH := app/src/main/cpp

# DEFINE ARCH FLAGS
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
    MY_ARCH_FLAGS := ARM_V7A
    MY_ARM_NEON := true
endif
ifeq ($(TARGET_ARCH_ABI), arm64-v8a)
    MY_ARCH_FLAGS := ARM64_V8A
    MY_ARM_NEON := true
endif
ifeq ($(TARGET_ARCH_ABI), x86)
    MY_ARCH_FLAGS := X86
endif
ifeq ($(TARGET_ARCH_ABI), x86_64)
    MY_ARCH_FLAGS := X86_64
endif

include $(CLEAR_VARS)
LOCAL_ARM_MODE := $(MY_ARM_MODE)
LOCAL_MODULE := mobileffmpeg_abidetect
LOCAL_SRC_FILES := mobileffmpeg_abidetect.c
LOCAL_CFLAGS := -Wall -Wextra -Werror -Wno-unused-parameter -DMOBILE_FFMPEG_${MY_ARCH_FLAGS}
LOCAL_C_INCLUDES += $(FFMPEG_INCLUDES)
LOCAL_LDLIBS := -llog -lz -landroid
LOCAL_SHARED_LIBRARIES := cpu-features
LOCAL_ARM_NEON := ${MY_ARM_NEON}
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_ARM_MODE := $(MY_ARM_MODE)
LOCAL_MODULE := mobileffmpeg
ifeq ($(TARGET_PLATFORM),android-16)
    LOCAL_SRC_FILES := mobileffmpeg.c mobileffprobe.c android_lts_support.c mobileffmpeg_exception.c fftools_cmdutils.c fftools_ffmpeg.c fftools_ffprobe.c fftools_ffmpeg_opt.c fftools_ffmpeg_hw.c fftools_ffmpeg_filter.c
else ifeq ($(TARGET_PLATFORM),android-17)
    LOCAL_SRC_FILES := mobileffmpeg.c mobileffprobe.c android_lts_support.c mobileffmpeg_exception.c fftools_cmdutils.c fftools_ffmpeg.c fftools_ffprobe.c fftools_ffmpeg_opt.c fftools_ffmpeg_hw.c fftools_ffmpeg_filter.c
else
    LOCAL_SRC_FILES := mobileffmpeg.c mobileffprobe.c mobileffmpeg_exception.c fftools_cmdutils.c fftools_ffmpeg.c fftools_ffprobe.c fftools_ffmpeg_opt.c fftools_ffmpeg_hw.c fftools_ffmpeg_filter.c
endif
LOCAL_CFLAGS := -Wall -Werror -Wno-unused-parameter -Wno-switch -Wno-sign-compare
LOCAL_LDLIBS := -llog -lz -landroid
LOCAL_SHARED_LIBRARIES := libavfilter libavformat libavcodec libavutil libswresample libavdevice libswscale
ifeq ($(APP_STL),c++_shared)
	LOCAL_SHARED_LIBRARIES += c++_shared # otherwise NDK will not add the library for packaging
endif
LOCAL_ARM_NEON := ${MY_ARM_NEON}
include $(BUILD_SHARED_LIBRARY)

$(call import-module, ffmpeg)
$(call import-module, cpu-features)
