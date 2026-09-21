#!/system/bin/sh
# The launcher now skips Provision but stops on LightActivity, because
# show_light_tip defaults to true when absent. Stock has it false. Write the same
# preference (app stopped first, or it would rewrite the file on exit).
am force-stop com.pvr.launcher
sleep 1
F=/data/data/com.pvr.launcher/shared_prefs/sp_file.xml
cp "$F" "$F.bak" 2>/dev/null
cat > "$F" <<'EOF'
<?xml version='1.0' encoding='utf-8' standalone='yes' ?>
<map>
    <boolean name="show_light_tip" value="false" />
    <boolean name="clarity_disable" value="false" />
</map>
EOF
chown system:system "$F"
chmod 660 "$F"
restorecon "$F" 2>/dev/null
sync
echo "--- prefs now ---"
cat "$F"
echo
am start -n com.pvr.launcher/.MainActivity
