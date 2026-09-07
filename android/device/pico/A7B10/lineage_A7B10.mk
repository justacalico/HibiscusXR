# Pico Neo 2 (A7B10)

$(call inherit-product, device/pico/A7B10/device.mk)
$(call inherit-product, vendor/lineage/config/common_full_phone.mk)

PRODUCT_NAME    := lineage_A7B10
PRODUCT_DEVICE  := A7B10
PRODUCT_BRAND   := Pico
PRODUCT_MODEL   := Pico Neo 2
PRODUCT_MANUFACTURER := Pico

PRODUCT_GMS_CLIENTID_BASE := android-pico

# Match the stock fingerprint so the (VNDK 27) vendor blobs see what they expect.
# Stock: Pico/A7B10/PICOA7B10:8.1.0/OPM1.171019.026/eng.scmbui.20210409.201441:user/test-keys
PRODUCT_BUILD_PROP_OVERRIDES += \
    PRODUCT_NAME=A7B10 \
    PRIVATE_BUILD_DESC="sdm845-user 8.1.0 OPM1.171019.026 eng.scmbui.20210409.201441 test-keys"

BUILD_FINGERPRINT := Pico/A7B10/PICOA7B10:8.1.0/OPM1.171019.026/eng.scmbui.20210409.201441:user/test-keys
