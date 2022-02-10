SUFFIX_NAME := .ini
MARLINBA_SUFFIX_NAME := .ba.ini
CALIBRATION_NAME := connectivity_calibration
CONFIGURE_NAME := connectivity_configure

CALIBRATION_SRC := $(LOCAL_PATH)/$(SPRD_WCN_HW_CONFIG)/$(addsuffix $(SUFFIX_NAME),$(basename $(CALIBRATION_NAME)))
CALIBRATION_BA_SRC := $(LOCAL_PATH)/$(SPRD_WCN_HW_CONFIG)/$(addsuffix $(MARLINBA_SUFFIX_NAME), $(basename $(CALIBRATION_NAME)))

ifeq ($(strip $(WCN_EXTENSION)),)
    WCN_EXTENSION := true
endif

ifeq (,$(wildcard $(CALIBRATION_SRC)))
# configuration file does not exist. report error
$(error wcn chip ini CALIBRATION_SRC miss. please fix it, and don't take a random one)
endif
ifeq (,$(wildcard $(CALIBRATION_BA_SRC)))
# configuration file does not exist. report error
$(error wcn chip ini CALIBRATION_BA_SRC miss. please fix it, and don't take a random one)
endif

CONFIGURE_SRC := $(LOCAL_PATH)/$(SPRD_WCN_HW_CONFIG)/$(addsuffix $(SUFFIX_NAME),$(basename $(CONFIGURE_NAME)))
CONFIGURE_BA_SRC := $(LOCAL_PATH)/$(SPRD_WCN_HW_CONFIG)/$(addsuffix $(MARLINBA_SUFFIX_NAME), $(basename $(CONFIGURE_NAME)))

ifeq (,$(wildcard $(CONFIGURE_SRC)))
# configuration file does not exist. report error
$(error wcn chip ini CONFIGURE_SRC miss. please fix it, and don't take a random one)
endif
ifeq (,$(wildcard $(CONFIGURE_BA_SRC)))
# configuration file does not exist. report error
$(error wcn chip ini CONFIGURE_BA_SRC miss. please fix it, and don't take a random one)
endif

PRODUCT_COPY_FILES += \
    $(CONFIGURE_SRC):system/etc/connectivity_configure.ini                  \
    $(CALIBRATION_SRC):system/etc/connectivity_calibration.ini              \
    $(CONFIGURE_BA_SRC):system/etc/marlinba/connectivity_configure.ini      \
    $(CALIBRATION_BA_SRC):system/etc/marlinba/connectivity_calibration.ini  \
    frameworks/native/data/etc/android.hardware.bluetooth_le.xml:vendor/etc/permissions/android.hardware.bluetooth_le.xml

PRODUCT_PROPERTY_OVERRIDES += \
    ro.vendor.wcn.hardware.product=$(SPRD_WCN_HW_MODEL) \
    ro.bt.bdaddr_path="/data/vendor/bluetooth/btmac.txt"
