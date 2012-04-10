LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include   $(NAM_CODECS)/video/mfc_c110/dec/Android.mk
include   $(NAM_CODECS)/video/mfc_c110/enc/Android.mk
include   $(NAM_CODECS)/video/mfc_c110/csc/Android.mk
