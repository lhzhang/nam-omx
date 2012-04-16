LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	NAM_OMX_Mpeg4enc.c \
	library_register.c


LOCAL_MODULE := libOMX.NAM.M4V.Encoder

LOCAL_CFLAGS :=

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := libNAM_OMX_Venc libnamosal libnambasecomponent libnamcsc
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libutils libui libhardware

LOCAL_C_INCLUDES := $(NAM_OMX_INC)/khronos \
	$(NAM_OMX_INC)/nam \
	$(NAM_OMX_TOP)/nam_osal \
	$(NAM_OMX_TOP)/nam_omx_core \
	$(NAM_OMX_COMPONENT)/common \
	$(NAM_OMX_COMPONENT)/video/enc

LOCAL_C_INCLUDES += $(NAM_OMX_TOP)/nam_codecs/video/dm3730/include

include $(BUILD_SHARED_LIBRARY)
