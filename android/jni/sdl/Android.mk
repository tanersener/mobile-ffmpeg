LOCAL_PATH := $(call my-dir)

MY_ARM_MODE := arm
MY_SDL_LIB := ../../../prebuilt/android-$(TARGET_ARCH)/sdl/lib

include $(CLEAR_VARS)
LOCAL_ARM_MODE := $(MY_ARM_MODE)
LOCAL_MODULE := sdl
LOCAL_SRC_FILES := $(MY_SDL_LIB)/libSDL2.a
include $(PREBUILT_STATIC_LIBRARY)
