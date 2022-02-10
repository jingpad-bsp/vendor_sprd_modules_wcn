LOCAL_PATH:= $(call my-dir)
ifeq ($(BOARD_WLAN_DEVICE), $(filter $(BOARD_WLAN_DEVICE),sc2332 sp9832e))
include $(CLEAR_VARS)

LOCAL_SRC_FILES := wifi_mac_generate.c

LOCAL_MODULE := wifi_mac_gen
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_32_BIT_ONLY := true

LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_EXECUTABLES)

LOCAL_VENDOR_MODULE := true

LOCAL_SHARED_LIBRARIES:= liblog libc libcutils
include $(BUILD_EXECUTABLE)
endif
