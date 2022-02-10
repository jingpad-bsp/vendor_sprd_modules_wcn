ifeq ($(BOARD_WLAN_DEVICE), sc2355)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= wcn_check.c

LOCAL_SHARED_LIBRARIES := libutils

LOCAL_MODULE := wcn_check
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_EXECUTABLE)

endif
