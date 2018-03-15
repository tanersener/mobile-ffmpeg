LOCAL_PATH := $(call my-dir)
MY_PATH := ../app/src/main/cpp

include $(CLEAR_VARS)
LOCAL_MODULE := libavcodec
LOCAL_SRC_FILES := $(NDK_ROOT)/prebuilt/android-arm/ffmpeg/lib/libavcodec.a
LOCAL_EXPORT_C_INCLUDES := $(NDK_ROOT)/prebuilt/android-arm/ffmpeg/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libavfilter
LOCAL_SRC_FILES := $(NDK_ROOT)/prebuilt/android-arm/ffmpeg/lib/libavfilter.a
LOCAL_EXPORT_C_INCLUDES := $(NDK_ROOT)/prebuilt/android-arm/ffmpeg/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libavdevice
LOCAL_SRC_FILES := $(NDK_ROOT)/prebuilt/android-arm/ffmpeg/lib/libavdevice.a
LOCAL_EXPORT_C_INCLUDES := $(NDK_ROOT)/prebuilt/android-arm/ffmpeg/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libavformat
LOCAL_SRC_FILES := $(NDK_ROOT)/prebuilt/android-arm/ffmpeg/lib/libavformat.a
LOCAL_EXPORT_C_INCLUDES := $(NDK_ROOT)/prebuilt/android-arm/ffmpeg/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libavutil
LOCAL_SRC_FILES := $(NDK_ROOT)/prebuilt/android-arm/ffmpeg/lib/libavutil.a
LOCAL_EXPORT_C_INCLUDES := $(NDK_ROOT)/prebuilt/android-arm/ffmpeg/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libswresample
LOCAL_SRC_FILES := $(NDK_ROOT)/prebuilt/android-arm/ffmpeg/lib/libswresample.a
LOCAL_EXPORT_C_INCLUDES := $(NDK_ROOT)/prebuilt/android-arm/ffmpeg/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := mobileffmpeg
LOCAL_SRC_FILES := $(MY_PATH)/mobileffmpeg.cpp $(MY_PATH)/cmdutils.c $(MY_PATH)/ffmpeg.c $(MY_PATH)/ffmpeg_opt.c $(MY_PATH)/ffmpeg_hw.c $(MY_PATH)/ffmpeg_filter.c
LOCAL_CFLAGS := -I$(NDK_ROOT)/prebuilt/android-$(TARGET_ARCH)/ffmpeg/include -Wno-deprecated-declarations -Wno-pointer-sign -Wno-switch
LOCAL_STATIC_LIBRARIES := cpufeatures libavfilter libavformat libavcodec libavutil libswresample libavdevice
include $(BUILD_STATIC_LIBRARY)

$(call import-module, android/cpufeatures)

