LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

include $(NAM_OMX_TOP)/Config.mk

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	NAM_OMX_H264dec.c \
	library_register.c


LOCAL_MODULE := libOMX.NAM.AVC.Decoder

LOCAL_PRELINK_MODULE := false

LOCAL_CFLAGS := -Dxdc_target_types__=gnu/targets/arm/std.h

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := libNAM_OMX_Vdec libnamosal libnambasecomponent libnamcsc
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libutils libui libhardware libticodec libtidmaiiface

LOCAL_C_INCLUDES := $(NAM_OMX_INC)/khronos \
	$(NAM_OMX_INC)/nam \
	$(NAM_OMX_TOP)/nam_osal \
	$(NAM_OMX_TOP)/nam_omx_core \
	$(NAM_OMX_COMPONENT)/common \
	$(NAM_OMX_COMPONENT)/video/dec \
	$(OMX_XDCPATH) \
	$(NAM_OMX_TOP)/nam_codecs/ti \
	$(NAM_OMX_TOP)/nam_codecs/common \

LOCAL_C_INCLUDES += $(NAM_OMX_TOP)/nam_codecs/video/dm3730/include

include $(BUILD_SHARED_LIBRARY)
