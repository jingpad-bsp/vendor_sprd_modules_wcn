LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        main.c

LOCAL_STATIC_LIBRARIES += \
        libcutils \
        libstdc++ \
        libc \
        libfs_mgr \
        libcutils \
        libselinux \
        libz

LOCAL_MODULE := fm_tools
LOCAL_MODULE_TAGS := optional
LOCAL_FORCE_STATIC_EXECUTABLE := true
include $(BUILD_EXECUTABLE)

