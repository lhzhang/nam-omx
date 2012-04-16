LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := NAM_OMX_Component_Register.c \
	NAM_OMX_Core.c


LOCAL_MODULE := libNAM_OMX_Core

LOCAL_PRELINK_MODULE := false

LOCAL_CFLAGS :=

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := libnamosal libnambasecomponent
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libutils

LOCAL_C_INCLUDES := $(NAM_OMX_INC)/khronos \
	$(NAM_OMX_INC)/nam \
	$(NAM_OMX_TOP)/nam_osal \
	$(NAM_OMX_TOP)/nam_omx_component/common

include $(BUILD_SHARED_LIBRARY)

