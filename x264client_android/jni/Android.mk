LOCAL_PATH := $(call my-dir)
BUILD_WITH_NEON := 1

ifeq ($(APP_ABI),armeabi)
BUILD_WITH_NEON := 0
endif

include $(CLEAR_VARS)
CODECROOT := $(LOCAL_PATH)/codecformat
FORMATROOT := $(LOCAL_PATH)/codecformat
YUVROOT := $(LOCAL_PATH)/libyuv/include
EXTROOT := $(LOCAL_PATH)/
include $(call all-makefiles-under,$(LOCAL_PATH))
