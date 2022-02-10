LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(PLATFORM_VERSION),6.0)
    LOCAL_SRC_FILES := hcidump.c
else
    #LOCAL_SRC_FILES := hcidump_for_raw.c
    LOCAL_SRC_FILES := hcidump.c
endif



LOCAL_MODULE := hcidump
LOCAL_STATIC_LIBRARIES := libcutils
LOCAL_MODULE_TAGS := optional
#LOCAL_LDLIBS += -lpthread
LOCAL_SHARED_LIBRARIES := liblog libz
include $(BUILD_EXECUTABLE)

CUSTOM_MODULES += hcidump
