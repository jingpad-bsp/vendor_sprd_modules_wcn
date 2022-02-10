LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_32_BIT_ONLY := true

LOCAL_SRC_FILES := fm.cpp \
		   audio.cpp \
		   fmtest.cpp

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := autotestfm
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_RELATIVE_PATH := npidevice
LOCAL_CFLAGS += -DGOOGLE_FM_INCLUDED

LOCAL_C_INCLUDES:= \
    $(TOP)/vendor/sprd/proprietories-source/engpc/sprd_fts_inc \
	    frameworks/av/include \
		system/core/base/include \
		vendor/sprd/modules/wcn/bt/libengbt \
		system/core/include
#		system/core/libutils/include/utils

LOCAL_SHARED_LIBRARIES:= \
             libcutils \
	     libutils \
             liblog  \
	     libhardware \
             libhardware_legacy

LOCAL_STATIC_LIBRARIES := \
             libcutils

include $(BUILD_SHARED_LIBRARY)
