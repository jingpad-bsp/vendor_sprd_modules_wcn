ifeq ($(strip $(SUPPORT_GNSS_HARDWARE)), true)
TARGET_VARIANTS := x86 x86_64

PRODUCT_COPY_FILES += \
	vendor/sprd/modules/wcn/gnss/misc/spirentroot.cer:/vendor/etc/spirentroot.cer \
	vendor/sprd/modules/wcn/gnss/misc/supl.xml:/vendor/etc/supl.xml \
	vendor/sprd/modules/wcn/gnss/misc/config.xml:/vendor/etc/config.xml

ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH),$(TARGET_VARIANTS)))
ifneq ($(strip $(SPRD_MODULES_GNSS3)),true)
PRODUCT_COPY_FILES += \
	vendor/sprd/modules/wcn/gnss/lte/x86/32bit/liblte.so:/vendor/lib/liblte.so
endif
ifeq ($(TARGET_ARCH), x86_64)
ifneq ($(strip $(SPRD_MODULES_GNSS3)),true)
PRODUCT_COPY_FILES += \
	vendor/sprd/modules/wcn/gnss/lte/x86/64bit/liblte.so:/vendor/lib64/liblte.so
endif
endif
else #arm arch
ifneq ($(strip $(SPRD_MODULES_GNSS3)),true)
PRODUCT_COPY_FILES += \
	vendor/sprd/modules/wcn/gnss/lte/arm/32bit/liblte.so:/vendor/lib/liblte.so
endif

# Sharkl5Pro
ifeq ($(strip $(TARGET_BOARD_PLATFORM)),ums512)
PRODUCT_COPY_FILES += \
	vendor/sprd/modules/wcn/gnss/lte/arm/32bit/liblte.so:/vendor/lib/liblte.so
endif

# Sharkl3
ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sp9863a)
PRODUCT_COPY_FILES += \
	vendor/sprd/modules/wcn/gnss/lte/arm/32bit/liblte.so:/vendor/lib/liblte.so
endif

ifeq ($(strip $(SPRD_MODULES_GNSS3)),true)
ifeq ($(strip $(SPRD_MODULES_NAVC_PATH)),lite)
PRODUCT_COPY_FILES += \
	vendor/sprd/modules/wcn/gnss/navcore/marlin3lite/lib32bit/libnavcore.so:/vendor/lib/libnavcore.so
else
PRODUCT_COPY_FILES += \
	vendor/sprd/modules/wcn/gnss/navcore/marlin3/lib32bit/libnavcore.so:/vendor/lib/libnavcore.so
endif
endif
ifeq ($(TARGET_ARCH), arm64)
ifneq ($(strip $(SPRD_MODULES_GNSS3)),true)
PRODUCT_COPY_FILES += \
	vendor/sprd/modules/wcn/gnss/lte/arm/64bit/liblte.so:/vendor/lib64/liblte.so
endif

# Sharkl5Pro
ifeq ($(strip $(TARGET_BOARD_PLATFORM)),ums512)
PRODUCT_COPY_FILES += \
	vendor/sprd/modules/wcn/gnss/lte/arm/64bit/liblte.so:/vendor/lib64/liblte.so
endif

# Sharkl3
ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sp9863a)
PRODUCT_COPY_FILES += \
	vendor/sprd/modules/wcn/gnss/lte/arm/64bit/liblte.so:/vendor/lib64/liblte.so
endif

ifeq ($(strip $(SPRD_MODULES_GNSS3)),true)
ifeq ($(strip $(SPRD_MODULES_NAVC_PATH)),lite)
PRODUCT_COPY_FILES += \
	vendor/sprd/modules/wcn/gnss/navcore/marlin3lite/lib64bit/libnavcore.so:/vendor/lib64/libnavcore.so
else
PRODUCT_COPY_FILES += \
	vendor/sprd/modules/wcn/gnss/navcore/marlin3/lib64bit/libnavcore.so:/vendor/lib64/libnavcore.so
endif
endif
endif
endif

PRODUCT_PACKAGES += \
        gpsd
endif


