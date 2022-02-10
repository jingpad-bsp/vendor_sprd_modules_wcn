SUFFIX_NAME := .ini
CALIBRATION_NAME := connectivity_calibration
CONFIGURE_NAME := connectivity_configure

CALIBRATION_SRC := $(LOCAL_PATH)/$(CONNECTIVITY_HW_CONFIG)/$(addsuffix $(SUFFIX_NAME),$(basename $(CALIBRATION_NAME)))
ifeq (,$(wildcard $(CALIBRATION_SRC)))
# configuration file does not exist. report error
$(error wcn chip ini CALIBRATION_SRC miss. please fix it, and don't take a random one)
endif

CONFIGURE_SRC := $(LOCAL_PATH)/$(CONNECTIVITY_HW_CONFIG)/$(addsuffix $(SUFFIX_NAME),$(basename $(CONFIGURE_NAME)))
ifeq (,$(wildcard $(CONFIGURE_SRC)))
# configuration file does not exist. report error
$(error wcn chip ini CONFIGURE_SRC miss. please fix it, and don't take a random one)
endif

PRODUCT_COPY_FILES += \
    $(CONFIGURE_SRC):system/etc/connectivity_configure.ini      \
    $(CALIBRATION_SRC):system/etc/connectivity_calibration.ini

PRODUCT_PROPERTY_OVERRIDES += \
    ro.vendor.wcn.hardware.product=$(SPRD_WCNBT_CHISET)
