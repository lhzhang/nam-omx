WITH_NAM_OMX := true

ifeq ($(WITH_NAM_OMX), true)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

NAM_OMX_TOP := $(LOCAL_PATH)
NAM_CODECS := $(NAM_OMX_TOP)/nam_codecs/

NAM_OMX_INC := $(NAM_OMX_TOP)/nam_omx_include/
NAM_OMX_COMPONENT := $(NAM_OMX_TOP)/nam_omx_component

include $(NAM_OMX_TOP)/nam_osal/Android.mk
include $(NAM_OMX_TOP)/nam_omx_core/Android.mk

include $(NAM_CODECS)/Android.mk
include $(NAM_OMX_COMPONENT)/common/Android.mk
include $(NAM_OMX_COMPONENT)/video/dec/Android.mk
include $(NAM_OMX_COMPONENT)/video/dec/h264dec/Android.mk
include $(NAM_OMX_COMPONENT)/video/dec/mpeg4dec/Android.mk
include $(NAM_OMX_COMPONENT)/video/enc/Android.mk
include $(NAM_OMX_COMPONENT)/video/enc/h264enc/Android.mk
include $(NAM_OMX_COMPONENT)/video/enc/mpeg4enc/Android.mk

endif
