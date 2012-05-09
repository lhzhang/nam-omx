
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../Config.mk

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := ti_dmai_iface.c

LOCAL_MODULE := libtidmaiiface

LOCAL_CFLAGS := -Dxdc_target_types__=gnu/targets/arm/std.h

LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES := libticodec

LOCAL_C_INCLUDES := \
	$(NAM_OMX_INC)/khronos	\
	$(NAM_OMX_TOP)/nam_osal	\
	$(OMX_XDCPATH)

include $(BUILD_STATIC_LIBRARY)

