#add 2332 npi app
LOCAL_PATH := $(call my-dir)
ifeq ($(BOARD_WLAN_DEVICE), $(filter $(BOARD_WLAN_DEVICE),sc2332 sp9832e))
#source files
WLNPI_OBJS	  += wlnpi.c cmd2332.c
#cflags
IWNPI_CFLAGS ?= -O2 -g
IWNPI_CFLAGS += -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -DCONFIG_LIBNL20
#include dirs
INCLUDES += external/libnl/include

#Build wlnpi tool
include $(CLEAR_VARS)
LOCAL_MODULE := iwnpi
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SHARED_LIBRARIES += libcutils   \
                          libutils    \
			  liblog

LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS = $(IWNPI_CFLAGS)
LOCAL_SRC_FILES = $(WLNPI_OBJS)
LOCAL_C_INCLUDES = $(INCLUDES)
LOCAL_STATIC_LIBRARIES += libnl
include $(BUILD_EXECUTABLE)
include $(call all-makefiles-under,$(LOCAL_PATH))
endif
