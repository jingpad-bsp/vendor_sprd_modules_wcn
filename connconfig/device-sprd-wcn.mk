#
# Spreadtrum WCN configuration
#

# Check envionment variables
ifeq ($(TARGET_BOARD),)
$(error WCN connectivity.mk should include after TARGET_BOARD)
endif

ifeq ($(BOARD_HAVE_SPRD_WCN_COMBO),)
$(error PRODUCT MK should have BOARD_HAVE_SPRD_WCN_COMBO config)
endif

# Config
# Help fix the board conflict without TARGET_BOARD
ifeq ($(SPRD_WCN_HW_CONFIG),)
SPRD_WCN_HW_CONFIG := $(TARGET_BOARD)
endif

# Legacy
# Help compatible with legacy project
ifeq ($(PLATFORM_VERSION),6.0)
SPRD_WCN_ETC_PATH := system/etc
endif

ifeq ($(PLATFORM_VERSION),4.4.4)
SPRD_WCN_ETC_PATH := system/etc
endif

# check lazy hidl in use
ifeq ($(strip $(PRODUCT_GO_DEVICE)),true)
SPRD_WCN_LAZY_HIDL_USE := false
else
SPRD_WCN_LAZY_HIDL_USE := false
endif

ifeq ($(SPRD_WCN_LAZY_HIDL_USE),true)
    PRODUCT_PACKAGES_WCN_HIDL += android.hardware.bluetooth@1.0-service.unisoc-lazy \
                                 android.hardware.bluetooth.audio@2.0-service.unisoc
else
    PRODUCT_PACKAGES_WCN_HIDL += android.hardware.bluetooth@1.0-service.unisoc \
                                 android.hardware.bluetooth.audio@2.0-service.unisoc
endif

SPRD_WCNBT_CHISET := $(BOARD_HAVE_SPRD_WCN_COMBO)
SPRD_WCN_HW_MODEL := $(BOARD_HAVE_SPRD_WCN_COMBO)
UNISOC_WCN_KERNEL_PATH := $(KERNEL_PATH)

ifeq ($(wildcard $(LOCAL_PATH)/$(SPRD_WCN_HW_MODEL)/$(SPRD_WCN_HW_CONFIG)),)
$(error wcn chip ini configuration miss. please fix it, and don't take a random one)
# If you don't know how to choose, please contact project PM!
#
# steven.chen
endif

ifeq ($(wildcard $(LOCAL_PATH)/$(SPRD_WCN_HW_MODEL)/$(SPRD_WCN_HW_CONFIG)),)
$(error wcn chip ini configuration miss. please fix it, and don't take a random one)
# If you don't know how to choose, please contact project PM!
#
# steven.chen
endif

$(call inherit-product, vendor/sprd/modules/wcn/connconfig/$(SPRD_WCN_HW_MODEL)/connectivity.mk)
ifeq ($(BOARD_SPRD_WCNBT_CARKIT_SET),true)
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := vendor/sprd/modules/wcn/bt/libbt/conf/sprd/$(SPRD_WCNBT_CHISET)/include_carkit \
                                               vendor/sprd/modules/wcn/bt/libbt/include
else
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := vendor/sprd/modules/wcn/bt/libbt/conf/sprd/$(SPRD_WCNBT_CHISET)/include \
                                               vendor/sprd/modules/wcn/bt/libbt/include
endif

PRODUCT_PACKAGES += \
    hcidump \
    libbqbbt \
    libbt-vendor \
    btools \
    android.hardware.bluetooth@1.0-impl-unisoc \
    libbluetooth_audio_session_unisoc \
    android.hardware.bluetooth.audio@2.0-impl-unisoc \
    audio.bluetooth.default \
    libbt-sprd_suite \
    libbt-sprd_eut \
    libfm-sprd_eut \
    autotestfm \
    fm_tools \
    libatfm \
    $(PRODUCT_PACKAGES_WCN_HIDL)

ifeq ($(strip $(SPRD_WCN_HW_MODEL)),marlin2)
PRODUCT_PACKAGES += libwifieut
PRODUCT_PACKAGES += wifi_mac_gen
else ifeq ($(strip $(SPRD_WCN_HW_MODEL)),sharklep)
PRODUCT_PACKAGES += libwifieut
PRODUCT_PACKAGES += wifi_mac_gen
else ifeq ($(strip $(SPRD_WCN_HW_MODEL)),marlin3e)
PRODUCT_PACKAGES += libwifieut
PRODUCT_PACKAGES += wifi_mac_gen
endif
