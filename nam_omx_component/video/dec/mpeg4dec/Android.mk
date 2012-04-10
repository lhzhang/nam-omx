LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	NAM_OMX_Mpeg4dec.c \
	library_register.c


LOCAL_MODULE := libOMX.NAM.M4V.Decoder

LOCAL_CFLAGS :=

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := libNAM_OMX_Vdec libnamosal libnambanamomponent \
						libnammfcdecapi libnamcsc
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libutils libui libhardware

LOCAL_C_INCLUDES := $(NAM_OMX_INC)/khronos \
	$(NAM_OMX_INC)/nam \
	$(NAM_OMX_TOP)/nam_osal \
	$(NAM_OMX_TOP)/nam_omx_core \
	$(NAM_OMX_COMPONENT)/common \
	$(NAM_OMX_COMPONENT)/video/dec

LOCAL_C_INCLUDES += $(NAM_OMX_TOP)/nam_codecs/video/mfc_c110/include

include $(BUILD_SHARED_LIBRARY)
