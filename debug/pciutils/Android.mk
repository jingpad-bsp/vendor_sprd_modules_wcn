LOCAL_PATH:= $(call my-dir)

ifeq ($(BOARD_HAVE_PCIE_WCN), true)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	ioctl_app.c

#LOCAL_SHARED_LIBRARIES := \
#		libutils \
#		libcutils

LOCAL_MODULE := PCIE_APP
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
endif
