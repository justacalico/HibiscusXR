#
# Pico Neo 2 (A7B10 / PICOA7B10)
#
# Every value here was read out of the stock PUI 4.1.3 (b346) images rather than
# copied from a similar sdm845 device. Sources are noted per block so they can be
# re-derived if a different firmware is used as the base.
#

DEVICE_PATH := device/pico/A7B10

# ---------------------------------------------------------------------------
# Platform
#   ro.board.platform=sdm845, ro.product.cpu.abilist from /system/build.prop
#   dalvik.vm.isa.arm64.variant=kryo300
# ---------------------------------------------------------------------------
TARGET_BOARD_PLATFORM := sdm845
TARGET_BOARD_PLATFORM_GPU := qcom-adreno630
TARGET_BOOTLOADER_BOARD_NAME := sdm845
TARGET_NO_BOOTLOADER := true

TARGET_ARCH := arm64
TARGET_ARCH_VARIANT := armv8-a
TARGET_CPU_ABI := arm64-v8a
TARGET_CPU_ABI2 :=
TARGET_CPU_VARIANT := kryo300

TARGET_2ND_ARCH := arm
TARGET_2ND_ARCH_VARIANT := armv8-a
TARGET_2ND_CPU_ABI := armeabi-v7a
TARGET_2ND_CPU_ABI2 := armeabi
TARGET_2ND_CPU_VARIANT := cortex-a75
TARGET_USES_64_BIT_BINDER := true

# ---------------------------------------------------------------------------
# Kernel
#   boot.img header v0, page_size 4096.
#   Addresses come straight from the header: kernel 0x00008000,
#   ramdisk 0x01000000, second 0x00f00000, tags 0x00000100.
#   Base is therefore 0x00000000 and the rest are expressed as offsets.
#   Version string: "Linux version 4.9.65-perf+ ... #1 SMP PREEMPT Fri Apr 9 2021"
#
#   NOTE: Pico published no kernel source. Until a matching CAF tree
#   (msm-4.9, LA.UM.7.1.r1-*-sdm845.0) is reconstructed, we ship the stock
#   prebuilt kernel. See prebuilt/README for provenance.
# ---------------------------------------------------------------------------
BOARD_KERNEL_BASE        := 0x00000000
BOARD_KERNEL_PAGESIZE    := 4096
BOARD_KERNEL_OFFSET      := 0x00008000
BOARD_RAMDISK_OFFSET     := 0x01000000
BOARD_KERNEL_SECOND_OFFSET := 0x00f00000
BOARD_KERNEL_TAGS_OFFSET := 0x00000100

# Verbatim from the stock boot.img. Note androidboot.selinux=permissive is
# FACTORY - it ships that way on retail user builds, we are not adding it.
BOARD_KERNEL_CMDLINE := console=ttyMSM0,115200n8
BOARD_KERNEL_CMDLINE += earlycon=msm_geni_serial,0xA84000
BOARD_KERNEL_CMDLINE += androidboot.hardware=qcom
BOARD_KERNEL_CMDLINE += androidboot.console=ttyMSM0
BOARD_KERNEL_CMDLINE += video=vfb:640x400,bpp=32,memsize=3072000
BOARD_KERNEL_CMDLINE += msm_rtb.filter=0x237
BOARD_KERNEL_CMDLINE += ehci-hcd.park=3
BOARD_KERNEL_CMDLINE += lpm_levels.sleep_disabled=1
BOARD_KERNEL_CMDLINE += service_locator.enable=1
BOARD_KERNEL_CMDLINE += swiotlb=2048
BOARD_KERNEL_CMDLINE += androidboot.configfs=true
BOARD_KERNEL_CMDLINE += androidboot.usbcontroller=a600000.dwc3
BOARD_KERNEL_CMDLINE += androidboot.selinux=permissive
BOARD_KERNEL_CMDLINE += buildvariant=user

BOARD_MKBOOTIMG_ARGS := --header_version 0
TARGET_PREBUILT_KERNEL := $(DEVICE_PATH)/prebuilt/kernel
BOARD_PREBUILT_DTBOIMAGE := $(DEVICE_PATH)/prebuilt/dtbo.img

# VERIFIED ON DEVICE: of the 40 DTBO entries, the board selects
#   dtbo_05 -> "Qualcomm Technologies, Inc. sda845 v2.1 MTP"
# NOT the QVR or SVR reference entry. androidboot.baseband=sda (no modem SKU).
# Keep the stock dtbo.img intact - the selection is made by the bootloader from
# the board ID, so shipping the full 40-entry image preserves that behaviour.

# ---------------------------------------------------------------------------
# Partitions
#   Sizes taken from the by-name block devices on the live unit and confirmed
#   against the firehose dump. Layout is NON-A/B with a discrete recovery
#   partition; the *bak entries are backup copies, NOT slots.
#   There is no super partition - system/vendor/oem are real partitions.
# ---------------------------------------------------------------------------
BOARD_BOOTIMAGE_PARTITION_SIZE     := 67108864     # 64M
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 67108864     # 64M
BOARD_DTBOIMG_PARTITION_SIZE       := 8388608      # 8M
BOARD_SYSTEMIMAGE_PARTITION_SIZE   := 3943694336   # ~3761M
BOARD_VENDORIMAGE_PARTITION_SIZE   := 1073741824   # 1024M
BOARD_CACHEIMAGE_PARTITION_SIZE    := 2684354560   # 2560M
BOARD_FLASH_BLOCK_SIZE             := 262144

TARGET_USERIMAGES_USE_EXT4 := true
TARGET_USERIMAGES_USE_F2FS := true
TARGET_COPY_OUT_VENDOR := vendor
BOARD_SYSTEMIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE  := ext4

AB_OTA_UPDATER := false
BOARD_BUILD_SYSTEM_ROOT_IMAGE := false

# ---------------------------------------------------------------------------
# Verified boot
#   A vbmeta partition exists (64K). Chained partitions are not used.
#   For bring-up, vbmeta is flashed with verification disabled:
#     fastboot --disable-verity --disable-verification flash vbmeta vbmeta.img
# ---------------------------------------------------------------------------
BOARD_AVB_ENABLE := true
BOARD_AVB_MAKE_VBMETA_IMAGE_ARGS += --flags 2

# ---------------------------------------------------------------------------
# Treble / VNDK
#   ro.treble.enabled=true, ro.build.version.sdk=27 -> VNDK 27.
#   This is the hard ceiling on how far the system image can be advanced while
#   reusing the stock vendor partition.
# ---------------------------------------------------------------------------
PRODUCT_FULL_TREBLE_OVERRIDE := true
BOARD_VNDK_VERSION := 27

# ---------------------------------------------------------------------------
# Display
#   VERIFIED ON DEVICE. The bootloader appends the display selection to the
#   kernel cmdline:
#     msm_drm.dsi_display0=dsi_jdi_uhd_55lcd_72_new_dual_video_display:
#   That is the dual-DSI wrapper around panel node
#     qcom,mdss_dsi_jdi_4k_55uhd_72_new_video
#   matching the jdi4k72 OTA build tag. Each DSI link is 1080x3840; combined
#   2160x3840, presented rotated as 3840x2160 (1920x2160 per eye).
#   72Hz, video mode, 4 lanes, 24bpp, DSC 540x8 slices at 8bpp.
#   Backlight is bl_ctrl_gpio, 3 bits across GPIO 49/50/51 (levels 1-7).
#
#   Sibling nodes exist for 70Hz and 75Hz and for 2k->4k scaling, which is what
#   /vendor/etc/pvr/pvr.display.fps.sh switches between at runtime:
#     dsi_jdi_uhd_55lcd_{70,72,75}_new_dual_video_display
#     dsi_jdi_uhd_lcd_dual_video_display_2kto4k
#
#   Because the bootloader supplies this, a replacement kernel inherits the
#   panel selection for free - it does not need to be encoded here.
# ---------------------------------------------------------------------------
TARGET_SCREEN_WIDTH  := 3840
TARGET_SCREEN_HEIGHT := 2160
TARGET_USES_HWC2 := true
TARGET_USES_ION := true
TARGET_USES_GRALLOC1 := true
MAX_EGL_CACHE_KEY_SIZE := 12*1024
MAX_EGL_CACHE_SIZE := 2048*1024
OVERRIDE_RS_DRIVER := libRSDriver_adreno.so

# ---------------------------------------------------------------------------
# SELinux
#   Stock ships permissive via the kernel cmdline. We still build real policy
#   so the tree stays honest and can be enforced later.
# ---------------------------------------------------------------------------
include device/qcom/sepolicy-legacy/sepolicy.mk
BOARD_SEPOLICY_DIRS += $(DEVICE_PATH)/sepolicy

# ---------------------------------------------------------------------------
# Recovery
# ---------------------------------------------------------------------------
TARGET_RECOVERY_PIXEL_FORMAT := RGBX_8888
TARGET_RECOVERY_FSTAB := $(DEVICE_PATH)/rootdir/etc/fstab.qcom
TARGET_USERIMAGES_SPARSE_EXT_DISABLED := false

# ---------------------------------------------------------------------------
# Hardware present on this board (from the DTBO overlay fragments)
#   picovr,spi-w25q   FPGA config flash  (fpga_rset GPIO 84, power-en GPIO 81)
#   picovr,nordic     Nordic BLE, controller link
#   icm@68            imu,icm206xx
#   eepromi2c@57      calibration EEPROM + tof-en
#   nq@28             NXP NFC
#   gpio_fan          active cooling, PWM tach IRQ
#   hw_version        3-bit board revision straps, GPIO 105/106/107
#   gpio_keys         app_key, confirm_key
# ---------------------------------------------------------------------------
BOARD_HAS_QCOM_WLAN := true
BOARD_HAS_QCOM_WLAN_SDK := true
WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_HOSTAPD_DRIVER := NL80211
BOARD_WLAN_DEVICE := qcwcn
WIFI_DRIVER_FW_PATH_STA := "sta"
WIFI_DRIVER_FW_PATH_AP := "ap"

BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_QCOM := true

TARGET_USES_QCOM_BSP_LEGACY := true
BOARD_USES_ADRENO := true

# no fingerprint, no NFC-as-user-feature, no telephony UI on this SKU
TARGET_HAS_NO_TELEPHONY := true
