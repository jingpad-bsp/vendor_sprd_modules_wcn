CONNECTIVITY_OWN_FILES := \
    bt_configure_rf.ini \
    bt_configure_pskey.ini \
    wifi_board_config.ini\
    wifi_board_config_ab.ini\
    fm_board_config.ini

SPRD_WCN_ETC_PATH ?= vendor/etc

GENERATE_WCN_PRODUCT_COPY_FILES += $(foreach own, $(CONNECTIVITY_OWN_FILES), \
    $(if $(wildcard $(LOCAL_PATH)/$(SPRD_WCN_HW_CONFIG)/$(own)), \
        $(LOCAL_PATH)/$(SPRD_WCN_HW_CONFIG)/$(own):$(SPRD_WCN_ETC_PATH)/$(own), \
        $(error wcn chip ini $(SPRD_WCN_HW_CONFIG) $(own) miss. please fix it, and don't take a random one)))

PRODUCT_COPY_FILES += \
    $(GENERATE_WCN_PRODUCT_COPY_FILES) \
        $(LOCAL_PATH)/wcn.rc:/vendor/etc/init/wcn.rc \
    frameworks/native/data/etc/android.hardware.bluetooth_le.xml:vendor/etc/permissions/android.hardware.bluetooth_le.xml

PRODUCT_PROPERTY_OVERRIDES += \
    ro.vendor.wcn.hardware.product=$(SPRD_WCN_HW_MODEL) \
    ro.vendor.wcn.hardware.etcpath=/$(SPRD_WCN_ETC_PATH) \
    ro.bt.bdaddr_path="/data/vendor/bluetooth/btmac.txt" \
    ro.bluetooth.a2dp_offload.supported="true" \
    persist.bluetooth.a2dp_offload.disabled = "false" \
    persist.bluetooth.a2dp_offload.cap = "sbc" \
    persist.bluetooth.a2dp_offload.switch = "false"
