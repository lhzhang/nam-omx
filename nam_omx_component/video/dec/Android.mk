LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	NAM_OMX_Vdec.c

LOCAL_MODULE := libNAM_OMX_Vdec
LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := $(NAM_OMX_INC)/khronos \
	$(NAM_OMX_INC)/nam \
	$(NAM_OMX_TOP)/nam_osal \
	$(NAM_OMX_TOP)/nam_omx_core \
	$(NAM_OMX_COMPONENT)/common \
	$(NAM_OMX_COMPONENT)/video/dec \
	$(OMX_XDCPATH) \
	$(NAM_OMX_TOP)/nam_codecs/ti 

LOCAL_C_INCLUDES += $(NAM_OMX_TOP)/nam_codecs/video/dm3730/include

LOCAL_CFLAGS := -Dxdc_target_types__=gnu/targets/arm/std.h

LOCAL_SHARED_LIBRARIES := libcutils libutils libticodec

include $(BUILD_STATIC_LIBRARY)
