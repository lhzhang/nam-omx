LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

include $(NAM_OMX_TOP)/Config.mk

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	NAM_OMX_Basecomponent.c \
	NAM_OMX_Baseport.c \
	NAM_OMX_Resourcemanager.c


LOCAL_MODULE := libnambasecomponent

LOCAL_CFLAGS := -Dxdc_target_types__=gnu/targets/arm/std.h

LOCAL_STATIC_LIBRARIES := libnamosal
LOCAL_SHARED_LIBRARIES := libcutils libutils libticodec

LOCAL_C_INCLUDES := $(NAM_OMX_INC)/khronos \
	$(NAM_OMX_INC)/nam \
	$(NAM_OMX_TOP)/nam_osal \
	$(OMX_XDCPATH) \
	$(NAM_OMX_TOP)/nam_codecs/ti

include $(BUILD_STATIC_LIBRARY)

