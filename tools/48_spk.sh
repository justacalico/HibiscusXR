#!/system/bin/sh
# Do the WSA881x smart speaker amps probe correctly now that the modules load at
# apexd-ready instead of being insmod'd by hand hundreds of seconds in? When I
# loaded them manually at t=372s the SoundWire devices reported "not ready" and
# the SpkrLeft/SpkrRight DAPM routes failed to bind.
echo "=== speaker amp / codec probe ==="
dmesg | grep -iE 'wsa881x|SpkrLeft|SpkrRight|wsa-max-devs|Sound card|swrm_get_logical' | tail -25
echo
echo "=== card + pcm ==="
cat /proc/asound/cards
echo
echo "=== when did the modules land this boot ==="
dmesg | grep -iE 'module_load|snd_soc_sdm845|Sound card .* registered' | tail -8
echo
echo "=== any audio errors since boot ==="
logcat -d -b main | grep -iE 'audio_hw|audioserver|AudioFlinger.*error|tinyalsa|pcm_open|E audio' | tail -15
echo DONE
