#
# Pico Neo 2 (A7B10) product configuration
#

LOCAL_PATH := device/pico/A7B10

# ---------------------------------------------------------------------------
# Treble / VNDK 27
# ---------------------------------------------------------------------------
PRODUCT_COMPATIBLE_PROPERTY_OVERRIDE := true
PRODUCT_SHIPPING_API_LEVEL := 27
PRODUCT_ENFORCE_RRO_TARGETS := *

PRODUCT_PACKAGES += \
    vndk_package

# ---------------------------------------------------------------------------
# Boot / rootdir
# ---------------------------------------------------------------------------
PRODUCT_PACKAGES += \
    fstab.qcom \
    init.qcom.rc \
    init.recovery.qcom.rc \
    ueventd.qcom.rc

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/rootdir/etc/fstab.qcom:$(TARGET_COPY_OUT_VENDOR)/etc/fstab.qcom

# ---------------------------------------------------------------------------
# Display
#   Panel is 3840x2160 landscape (dual DSI, 1080x3840 per link, presented
#   rotated). Density is deliberately low: this is a headset panel viewed
#   through lenses, not a phone screen held at arm's length.
# ---------------------------------------------------------------------------
PRODUCT_AAPT_CONFIG := normal
PRODUCT_AAPT_PREF_CONFIG := xxhdpi
TARGET_SCREEN_DENSITY := 320

PRODUCT_PACKAGES += \
    android.hardware.graphics.allocator@2.0-impl \
    android.hardware.graphics.allocator@2.0-service \
    android.hardware.graphics.composer@2.1-impl \
    android.hardware.graphics.composer@2.1-service \
    android.hardware.graphics.mapper@2.0-impl \
    android.hardware.memtrack@1.0-impl \
    android.hardware.memtrack@1.0-service

PRODUCT_PROPERTY_OVERRIDES += \
    ro.sf.lcd_density=320 \
    debug.sf.latch_unsignaled=1 \
    sdm.debug.prefersplit=1 \
    sdm.debug.disable_inline_rotator=1 \
    sdm.debug.disable_inline_rotator_secure=1 \
    debug.gralloc.gfx_ubwc_disable=0

# ---------------------------------------------------------------------------
# VR
#   vrflinger is AOSP (frameworks/native/services/vr) and is already what stock
#   uses - surfaceflinger.rc opens the pdx/system/vr/display sockets. Enabling
#   it here keeps the direct-mode display path available.
# ---------------------------------------------------------------------------
PRODUCT_PACKAGES += \
    android.hardware.vr@1.0-impl \
    android.hardware.vr@1.0-service

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.vr.high_performance.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.vr.high_performance.xml

PRODUCT_PROPERTY_OVERRIDES += \
    ro.vr.avoid_gfx_limits=true

# ---------------------------------------------------------------------------
# Sensors
#   IMU is icm206xx over the Qualcomm sensor core (SLPI). The NDI EM controller
#   tracking arrives through the same sensors HAL as sensor "ndi_fpga".
# ---------------------------------------------------------------------------
PRODUCT_PACKAGES += \
    android.hardware.sensors@1.0-impl \
    android.hardware.sensors@1.0-service

# ---------------------------------------------------------------------------
# Media / codecs
# ---------------------------------------------------------------------------
PRODUCT_PACKAGES += \
    android.hardware.media.omx@1.0-service \
    libavservices_minijail_vendor

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/media/media_codecs.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs.xml

# ---------------------------------------------------------------------------
# Wi-Fi / Bluetooth
# ---------------------------------------------------------------------------
PRODUCT_PACKAGES += \
    android.hardware.wifi@1.0-service \
    android.hardware.bluetooth@1.0-impl \
    android.hardware.bluetooth@1.0-service \
    libwifi-hal-qcom \
    wificond \
    wpa_supplicant \
    wpa_supplicant.conf

# ---------------------------------------------------------------------------
# Features
#   No telephony, no camera app (the cameras are tracking sensors reached
#   through QVR, not through the Android camera HAL), no fingerprint.
# ---------------------------------------------------------------------------
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
    frameworks/native/data/etc/android.hardware.bluetooth.xml:system/etc/permissions/android.hardware.bluetooth.xml \
    frameworks/native/data/etc/android.hardware.bluetooth_le.xml:system/etc/permissions/android.hardware.bluetooth_le.xml \
    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/native/data/etc/android.software.vr.mode.xml:system/etc/permissions/android.software.vr.mode.xml \
    frameworks/native/data/etc/android.hardware.vr.high_performance.xml:system/etc/permissions/android.hardware.vr.high_performance.xml \
    frameworks/native/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml

# ---------------------------------------------------------------------------
# Properties carried over from stock system.prop that the vendor blobs read
# ---------------------------------------------------------------------------
PRODUCT_PROPERTY_OVERRIDES += \
    persist.sys.vr_mode_enabled=1 \
    ro.hardware=qcom \
    ro.opengles.version=196610 \
    ro.vendor.at_library=libqti-at.so \
    ro.vendor.qti.core_ctl_min_cpu=2 \
    ro.vendor.qti.core_ctl_max_cpu=4

# ---------------------------------------------------------------------------
# Blobs (populated by extract-files.sh into vendor/pico/A7B10)
# ---------------------------------------------------------------------------
$(call inherit-product-if-exists, vendor/pico/A7B10/A7B10-vendor.mk)
