# A simple test for the minimal standard C++ library
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_STATIC_LIBRARIES += avcodec
LOCAL_STATIC_LIBRARIES += yuv_static
#LOCAL_STATIC_LIBRARIES += avformat
LOCAL_LDLIBS := -lz -llog
LOCAL_ARM_MODE := arm
ifeq ($(BUILD_WITH_NEON),1)
LOCAL_ARM_NEON := true
endif
LOCAL_MODULE := core


LOCAL_SRC_FILES := thecore.cpp \
		   decoding.cpp


LOCAL_SRC_FILES +=$(FF_COMMON_SRC)
LOCAL_SRC_FILES +=$(FREETYPE_FILES)

LOCAL_C_INCLUDES +=  $(CODECROOT) \
		     $(FORMATROOT) \
		     $(YUVROOT)

LOCAL_CFLAGS += -DHAVE_ARMV6=1
LOCAL_CFLAGS += -DLIBYUV_DISABLE_X86

ifeq ($(BUILD_WITH_NEON),1)
LOCAL_CFLAGS += -DHAVE_NEON=1
LOCAL_SRC_FILES += $(FF_ARM_NEON_SRC)
else
LOCAL_CFLAGS += -DHAVE_NEON=0
endif

ifeq ($(BUILD_WITH_NEON),1)
LOCAL_STATIC_LIBRARIES += arm_neon
endif

include $(BUILD_SHARED_LIBRARY)
