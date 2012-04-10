LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	NAM_OSAL_Event.c \
	NAM_OSAL_Queue.c \
	NAM_OSAL_ETC.c \
	NAM_OSAL_Mutex.c \
	NAM_OSAL_Thread.c \
	NAM_OSAL_Memory.c \
	NAM_OSAL_Semaphore.c \
	NAM_OSAL_Library.c \
	NAM_OSAL_Log.c \
	NAM_OSAL_Buffer.cpp


LOCAL_MODULE := libnamosal

LOCAL_CFLAGS :=

LOCAL_STATIC_LIBRARIES :=

LOCAL_SHARED_LIBRARIES := libcutils libutils \
                          libui \
                          libhardware \
                          libandroid_runtime \
                          libsurfaceflinger_client \
                          libbinder \
                          libmedia

LOCAL_C_INCLUDES := $(NAM_OMX_INC)/khronos \
	$(NAM_OMX_INC)/nam \
	$(NAM_OMX_TOP)/nam_osal \
	$(NAM_OMX_COMPONENT)/common \
	$(NAM_OMX_TOP)/../../include

include $(BUILD_STATIC_LIBRARY)
