LOCAL_PATH := $(call my-dir)
BUILD_WITH_NEON := 1

ifeq ($(APP_ABI),armeabi)
BUILD_WITH_NEON := 0
endif

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_LDLIBS := -llog

LOCAL_MODULE := 3dvOpen3d
LOCAL_SRC_FILES :=open3d.cpp
include $(BUILD_SHARED_LIBRARY)