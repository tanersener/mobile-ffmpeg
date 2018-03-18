LOCAL_PATH := $(call my-dir)
$(call import-add-path, $(LOCAL_PATH))

MY_ARM_MODE := arm
MY_PATH := ../app/src/main/cpp

include $(CLEAR_VARS)
LOCAL_ARM_MODE := $(MY_ARM_MODE)
LOCAL_MODULE := cpufeatures
LOCAL_SRC_FILES := $(NDK_ROOT)/sources/android/cpufeatures/cpu-features.c
LOCAL_CFLAGS := -Wall -Wextra -Werror
LOCAL_EXPORT_C_INCLUDES := $(NDK_ROOT)/sources/android/cpufeatures
LOCAL_EXPORT_LDLIBS := -ldl
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_ARM_MODE := $(MY_ARM_MODE)
LOCAL_MODULE := mobileffmpeg
LOCAL_SRC_FILES := $(MY_PATH)/mobileffmpeg.cpp $(MY_PATH)/cmdutils.c $(MY_PATH)/ffmpeg.c $(MY_PATH)/ffmpeg_opt.c $(MY_PATH)/ffmpeg_hw.c $(MY_PATH)/ffmpeg_filter.c
LOCAL_CFLAGS := -I$(NDK_ROOT)/prebuilt/android-$(TARGET_ARCH)/ffmpeg/include
LOCAL_LDLIBS := -llog -lz -landroid
LOCAL_SHARED_LIBRARIES := cpufeatures libavfilter libavformat libavcodec libavutil libswresample libavdevice
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
    LOCAL_SHARED_LIBRARIES += libavfilter-neon libavformat-neon libavcodec-neon libavutil-neon libswresample-neon libavdevice-neon
    LOCAL_ARM_NEON := true
endif
include $(BUILD_SHARED_LIBRARY)

$(call import-module, ffmpeg)
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
    $(call import-module, ffmpeg/neon)
endif