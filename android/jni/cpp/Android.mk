LOCAL_PATH := $(call my-dir)

MY_ARM_MODE := arm

include $(CLEAR_VARS)
LOCAL_ARM_MODE := $(MY_ARM_MODE)
LOCAL_MODULE := libcpp_shared
LOCAL_SRC_FILES := ../../../prebuilt/android-cpp-shared/${TARGET_ARCH}/libc++_shared.so

include $(PREBUILT_SHARED_LIBRARY)
