#!/system/bin/sh
A=$(pidof audioserver); H=$(pidof android.hardware.audio@2.0-service)
echo "audioserver=$A  hal=$H  uptime=$(cut -d. -f1 /proc/uptime)s"
echo "audioserver alive for: $(ps -o ETIME= -p $A 2>/dev/null)"
echo

echo "=== is media.audio_flinger published yet? ==="
service check media.audio_flinger
service check media.audio_policy
echo

echo "=== audioserver stack NOW ==="
debuggerd -b $A 2>&1 | grep -E '^"|#0[0-9] pc' | head -40
echo
echo "=== audio HAL stack NOW ==="
debuggerd -b $H 2>&1 | grep -E '^"|#0[0-9] pc' | head -40
echo

echo "=== sound card present? ==="
cat /proc/asound/cards 2>&1
ls /dev/snd/ 2>&1 | head
echo

echo "=== ADSP / q6 state ==="
dmesg 2>/dev/null | grep -iE 'adsp|q6|apr|audio_notifer|snd_soc|sdm845.*snd|msm.*audio' | tail -25
echo

echo "=== a2dp offload props ==="
getprop | grep -iE 'a2dp|bluetooth.*offload|audio.offload'
echo

echo "=== vendor audio config files present ==="
ls -l /vendor/etc/audio_policy_configuration.xml /vendor/etc/mixer_paths*.xml 2>&1 | head
ls -l /vendor/lib/hw/audio.primary*.so /vendor/lib64/hw/audio.primary*.so 2>&1
echo

echo "=== /data/vendor/audio exists? ==="
ls -ld /data/vendor/audio 2>&1
echo DONE
