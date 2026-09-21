#!/system/bin/sh
# The launcher loops trying to start Provision, which crashes in its language
# picker. If the device can simply be marked provisioned, the launcher skips it
# and we get a working home shell without fixing that app at all.
echo "=== global provisioning flags ==="
for k in device_provisioned; do echo "  global $k = $(settings get global $k)"; done
for k in user_setup_complete tv_user_setup_complete; do echo "  secure $k = $(settings get secure $k)"; done
echo
echo "=== any pico-specific provision/setup flags ==="
for ns in system secure global; do
  settings list $ns 2>/dev/null | grep -iE 'provis|setup|first|guide|wizard|pvr|pico'
done
echo
echo "=== props ==="
getprop 2>/dev/null | grep -iE 'provis|setup|first_boot|pvr\.|pui'
