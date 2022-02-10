CONNECTIVITY_OWN_FILES := \
    bt_configure_rf.ini \
    bt_configure_pskey.ini \
    fm_board_config.ini \
    wifi_board_config.ini

SPRD_WCN_ETC_PATH ?= vendor/etc

GENERATE_WCN_PRODUCT_COPY_FILES += $(foreach own, $(CONNECTIVITY_OWN_FILES), \
    $(if $(wildcard $(LOCAL_PATH)/$(SPRD_WCN_HW_CONFIG)/$(own)), \
        $(LOCAL_PATH)/$(SPRD_WCN_HW_CONFIG)/$(own):$(SPRD_WCN_ETC_PATH)/$(own), \
        $(LOCAL_PATH)/default/$(own):$(SPRD_WCN_ETC_PATH)/$(own)))

PRODUCT_COPY_FILES += \
    $(GENERATE_WCN_PRODUCT_COPY_FILES) \
        $(LOCAL_PATH)/wcn.rc:/vendor/etc/init/wcn.rc \
    frameworks/native/data/etc/android.hardware.bluetooth_le.xml:vendor/etc/permissions/android.hardware.bluetooth_le.xml

PRODUCT_PROPERTY_OVERRIDES += \
    ro.vendor.wcn.hardware.product=$(SPRD_WCN_HW_MODEL) \
    ro.vendor.wcn.hardware.etcpath=/$(SPRD_WCN_ETC_PATH) \
    ro.bt.bdaddr_path="/data/vendor/bluetooth/btmac.txt" \
    persist.bluetooth.a2dp_offload.cap = "sbc" \
    persist.bluetooth.a2dp_offload.switch = "false"

ifeq ($(BOARD_SPRD_WCNBT_CARKIT_SET),true)
PRODUCT_PROPERTY_OVERRIDES += \
    ro.bluetooth.a2dp_offload.supported="false" \
    persist.bluetooth.a2dp_offload.disabled = "true"
else
    ifeq ($(TARGET_BOARD_PLATFORM),sp9863a)
    PRODUCT_PROPERTY_OVERRIDES += \
        ro.bluetooth.a2dp_offload.supported="false" \
        persist.bluetooth.a2dp_offload.disabled = "true"
	else ifeq ($(TARGET_BOARD_PLATFORM),sp9832e)
    PRODUCT_PROPERTY_OVERRIDES += \
        ro.bluetooth.a2dp_offload.supported="false" \
        persist.bluetooth.a2dp_offload.disabled = "true"
    else
    PRODUCT_PROPERTY_OVERRIDES += \
        ro.bluetooth.a2dp_offload.supported="true" \
        persist.bluetooth.a2dp_offload.disabled = "false"
    endif
endif

PRODUCT_PACKAGES += \
    sprdbt_tty.ko


