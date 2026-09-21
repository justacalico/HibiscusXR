#!/bin/bash
#
# Pull the proprietary blobs this device needs.
#
# Nothing proprietary ships in this tree. Blobs come from ONE of:
#   1. the user's own device over adb (default), or
#   2. an official Pico OTA zip the user downloaded themselves:
#        ./extract-files.sh /path/to/update_PicoNeo2_pui4.1.3_*.zip
#
# Requires root on the device for the adb path, since /vendor is not readable
# by shell for every file.
#
set -e

DEVICE=A7B10
VENDOR=pico

export INITIAL_COPYRIGHT_YEAR=2026

MY_DIR="${BASH_SOURCE%/*}"
[[ ! -d "$MY_DIR" ]] && MY_DIR="$PWD"

ANDROID_ROOT="$MY_DIR"/../../..

HELPER="$ANDROID_ROOT"/tools/extract-utils/extract_utils.sh
if [ ! -f "$HELPER" ]; then
    echo "Unable to find helper script at $HELPER"
    exit 1
fi
. "$HELPER"

SRC=adb
SECTION=
KANG=

while [ "$#" -gt 0 ]; do
    case "$1" in
        -n | --no-cleanup) CLEAN_VENDOR=false ;;
        -k | --kang)       KANG="--kang" ;;
        -s | --section)    SECTION="$2"; shift; CLEAN_VENDOR=false ;;
        * )                SRC="$1" ;;
    esac
    shift
done

if [ -z "$SRC" ]; then
    SRC=adb
fi

setup_vendor "$DEVICE" "$VENDOR" "$ANDROID_ROOT" false "$CLEAN_VENDOR"

extract "$MY_DIR"/proprietary-files.txt "$SRC" "$KANG" --section "$SECTION"

"$MY_DIR"/setup-makefiles.sh
