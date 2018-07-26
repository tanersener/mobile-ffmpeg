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
LOCAL_MODULE := abidetect
LOCAL_SRC_FILES := $(MY_PATH)/abidetect.c
LOCAL_CFLAGS := -Wall -Wextra -Werror -Wno-unused-parameter -I${LOCAL_PATH}/../../prebuilt/android-$(TARGET_ARCH)/ffmpeg/include -I$(NDK_ROOT)/sources/android/cpufeatures
LOCAL_LDLIBS := -llog -lz -landroid
LOCAL_SHARED_LIBRARIES := cpufeatures
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_ARM_MODE := $(MY_ARM_MODE)
LOCAL_MODULE := ffmpeglog
LOCAL_SRC_FILES := $(MY_PATH)/log.c
LOCAL_CFLAGS := -Wall -Wextra -Werror -Wno-unused-parameter -I${LOCAL_PATH}/../../prebuilt/android-$(TARGET_ARCH)/ffmpeg/include
LOCAL_LDLIBS := -llog -lz -landroid
LOCAL_SHARED_LIBRARIES := libavutil
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_ARM_MODE := $(MY_ARM_MODE)
LOCAL_MODULE := mobileffmpeg
LOCAL_SRC_FILES := $(MY_PATH)/mobileffmpeg.c $(MY_PATH)/cmdutils.c $(MY_PATH)/ffmpeg.c $(MY_PATH)/ffmpeg_opt.c $(MY_PATH)/ffmpeg_hw.c $(MY_PATH)/ffmpeg_filter.c
LOCAL_CFLAGS := -I${LOCAL_PATH}/../../prebuilt/android-$(TARGET_ARCH)/ffmpeg/include
LOCAL_LDLIBS := -llog -lz -landroid
LOCAL_SHARED_LIBRARIES := ffmpeglog libavfilter libavformat libavcodec libavutil libswresample libavdevice libswscale
include $(BUILD_SHARED_LIBRARY)

ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
    ifeq ("$(shell test -e $(LOCAL_PATH)/../build/.neon && echo neon)","neon")

        include $(CLEAR_VARS)
        LOCAL_ARM_MODE := $(MY_ARM_MODE)
        LOCAL_MODULE := mobileffmpeg-armv7a-neon
        LOCAL_SRC_FILES := $(MY_PATH)/mobileffmpeg.c $(MY_PATH)/cmdutils.c $(MY_PATH)/ffmpeg.c $(MY_PATH)/ffmpeg_opt.c $(MY_PATH)/ffmpeg_hw.c $(MY_PATH)/ffmpeg_filter.c
        LOCAL_CFLAGS := -I${LOCAL_PATH}/../../prebuilt/android-$(TARGET_ARCH)/ffmpeg/include
        LOCAL_LDLIBS := -llog -lz -landroid
        LOCAL_SHARED_LIBRARIES := libavcodec-neon libavfilter-neon libswscale-neon libavformat libavutil libswresample libavdevice
        LOCAL_ARM_NEON := true
        include $(BUILD_SHARED_LIBRARY)

        $(call import-module, ffmpeg/neon)

    endif
endif

$(call import-module, ffmpeg)