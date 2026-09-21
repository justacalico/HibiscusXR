#!/bin/sh
# regenerate res/mipmap-*/ic_launcher.png from ic_launcher.svg
set -e
cd "$(dirname "$0")/.."
for d in mdpi:48 hdpi:72 xhdpi:96 xxhdpi:144 xxxhdpi:192; do
    dir=${d%%:*}; px=${d##*:}
    mkdir -p "res/mipmap-$dir"
    rsvg-convert -w "$px" -h "$px" art/ic_launcher.svg \
        -o "res/mipmap-$dir/ic_launcher.png"
done
