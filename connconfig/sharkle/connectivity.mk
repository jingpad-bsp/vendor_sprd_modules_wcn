CONNECTIVITY_OWN_FILES := \
    connectivity_calibration.ini \
    connectivity_configure.ini

SUFFIX_AD_NAME := .ad.ini
SPRD_WCN_ETC_PATH ?= vendor/etc

SPRD_WCN_ETC_AD_PATH := $(SPRD_WCN_ETC_PATH)/wcn

GENERATE_WCN_PRODUCT_COPY_FILES += $(foreach own, $(CONNECTIVITY_OWN_FILES), \
    $(if $(wildcard $(LOCAL_PATH)/$(SPRD_WCN_HW_CONFIG)/$(own)), \
        $(LOCAL_PATH)/$(SPRD_WCN_HW_CONFIG)/$(own):$(SPRD_WCN_ETC_PATH)/$(own), \
        $(error wcn chip ini $(SPRD_WCN_HW_CONFIG) $(own) miss. please fix it, and don't take a random one)))

GENERATE_WCN_PRODUCT_COPY_AD_FILE += $(foreach own, $(CONNECTIVITY_OWN_FILES), \
    $(if $(wildcard $(LOCAL_PATH)/$(SPRD_WCN_HW_CONFIG)/$(addsuffix $(SUFFIX_AD_NAME), $(basename $(own)))), \
        $(LOCAL_PATH)/$(SPRD_WCN_HW_CONFIG)/$(addsuffix $(SUFFIX_AD_NAME), $(basename $(own))):$(SPRD_WCN_ETC_AD_PATH)/$(own), \
        $(error wcn chip ini $(SPRD_WCN_HW_CONFIG) $(own) miss. please fix it, and don't take a random one)))

PRODUCT_COPY_FILES += \
    $(GENERATE_WCN_PRODUCT_COPY_FILES) \
    $(GENERATE_WCN_PRODUCT_COPY_AD_FILE) \
    $(LOCAL_PATH)/wcn.rc:/vendor/etc/init/wcn.rc \
    frameworks/native/data/etc/android.hardware.bluetooth_le.xml:vendor/etc/permissions/android.hardware.bluetooth_le.xml

PRODUCT_PROPERTY_OVERRIDES += \
    ro.vendor.wcn.hardware.product=$(SPRD_WCN_HW_MODEL) \
    ro.vendor.wcn.hardware.etcpath=/$(SPRD_WCN_ETC_PATH) \
    ro.bt.bdaddr_path="/data/vendor/bluetooth/btmac.txt"
