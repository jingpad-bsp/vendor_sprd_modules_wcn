CONNECTIVITY_OWN_FILES := \
    connectivity_calibration.ini \
    connectivity_configure.ini

SPRD_WCN_ETC_PATH ?= vendor/etc

GENERATE_WCN_PRODUCT_COPY_FILES += $(foreach own, $(CONNECTIVITY_OWN_FILES), \
    $(if $(wildcard $(LOCAL_PATH)/$(SPRD_WCN_HW_CONFIG)/$(own)), \
        $(LOCAL_PATH)/$(SPRD_WCN_HW_CONFIG)/$(own):$(SPRD_WCN_ETC_PATH)/$(own), \
        $(error wcn chip ini $(SPRD_WCN_HW_CONFIG) $(own) miss. please fix it, and don't take a random one)))

PRODUCT_COPY_FILES += \
    $(GENERATE_WCN_PRODUCT_COPY_FILES) \
    frameworks/native/data/etc/android.hardware.bluetooth_le.xml:vendor/etc/permissions/android.hardware.bluetooth_le.xml


PRODUCT_PROPERTY_OVERRIDES += \
    ro.vendor.wcn.hardware.product=$(SPRD_WCN_HW_MODEL) \
    ro.vendor.wcn.hardware.etcpath=/$(SPRD_WCN_ETC_PATH) \
    ro.bt.bdaddr_path="/data/vendor/bluetooth/btmac.txt"
