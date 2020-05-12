LOCAL_PATH := $(call my-dir)/../../../prebuilt/android-$(TARGET_ARCH)/cpu_features/lib

include $(CLEAR_VARS)
LOCAL_ARM_MODE := $(MY_ARM_MODE)
LOCAL_MODULE := cpu_features
LOCAL_SRC_FILES := libndk_compat.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../include/ndk_compat
include $(PREBUILT_SHARED_LIBRARY)
