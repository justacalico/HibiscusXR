# HBSUP - Hibiscus Backup

Desktop tool that takes a full-disk backup of a Hibiscus headset over
rooted adb. It reads the by-name partition table, dumps every partition
with `dd` into a folder you pick, checks the destination has room before
it starts, and leaves a `manifest.txt` next to the images so the dump
stays restorable.

Runs on Linux and macOS. Windows builds exist but are not a supported
host OS - the app says so on open and you are on your own there.

## Usage

1. Boot the headset into stock, get rooted adb (`adb root`).
2. Plug in over USB, or bring wireless adb up first.
3. Pick the headset, pick a folder, hit Start backup.

The dump is one `<name>.img` per partition plus `manifest.txt` - that
folder is your way back to stock, keep it safe.
