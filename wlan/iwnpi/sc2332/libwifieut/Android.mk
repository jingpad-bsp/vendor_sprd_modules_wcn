LOCAL_PATH:= $(call my-dir)
ifeq ($(BOARD_WLAN_DEVICE), $(filter $(BOARD_WLAN_DEVICE),sc2332 sp9832e))
include $(CLEAR_VARS)

LOCAL_32_BIT_ONLY := true

LOCAL_SRC_FILES := wifi_eut_hdlr.c

LOCAL_MODULE := libwifieut
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES:= \
	$(TOP)/vendor/sprd/proprietories-source/engpc/sprd_fts_inc/

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := npidevice

LOCAL_SHARED_LIBRARIES:= liblog libc libcutils

include $(BUILD_SHARED_LIBRARY)
endif
