LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	NAM_OMX_Venc.c

LOCAL_MODULE := libNAM_OMX_Venc
LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := $(NAM_OMX_INC)/khronos \
	$(NAM_OMX_INC)/nam \
	$(NAM_OMX_TOP)/nam_osal \
	$(NAM_OMX_TOP)/nam_omx_core \
	$(NAM_OMX_COMPONENT)/common \
	$(NAM_OMX_COMPONENT)/video/dec

LOCAL_C_INCLUDES += $(NAM_OMX_TOP)/nam_codecs/video/mfc_c110/include

include $(BUILD_STATIC_LIBRARY)
