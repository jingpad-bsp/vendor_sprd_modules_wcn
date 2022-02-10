LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_32_BIT_ONLY := true

LOCAL_SRC_FILES := fmat.c

LOCAL_MODULE := libatfm
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_RELATIVE_PATH := npidevice

LOCAL_C_INCLUDES:= \
	$(TOP)/vendor/sprd/proprietories-source/engpc/sprd_fts_inc \
	$(LOCAL_PATH)/include \

LOCAL_SHARED_LIBRARIES:= liblog libc libcutils libtracedump libutils
#ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
#endif
include $(BUILD_SHARED_LIBRARY)
