#!/system/bin/sh
# The stock vendor loads the audio drivers as modules at boot via an init exec.
# Under the GSI that exec ran but nothing landed (lsmod shows only wlan), so
# there is no sound card. Load them by hand and capture the real error.
M=/vendor/lib/modules
echo "=== before ==="
lsmod
cat /proc/asound/cards 2>&1
echo

echo "=== modprobe binary ==="
ls -l /vendor/bin/modprobe /system/bin/modprobe 2>&1
echo

echo "=== try the exact stock command ==="
/vendor/bin/modprobe -a -d $M pinctrl-wcd wcd-dsp-glink snd-soc-wcd-spi snd-soc-sdm845 2>&1
echo "rc=$?"
echo

echo "=== if that failed, insmod in dependency order ==="
for m in wcd-core pinctrl-wcd wcd-dsp-glink snd-soc-wcd-spi snd-soc-wcd9xxx \
         snd-soc-wcd-mbhc swr-wcd-ctrl snd-soc-wcd934x snd-soc-wsa881x snd-soc-sdm845; do
  if lsmod | grep -q "^$(echo $m | tr - _) "; then echo "already: $m"; continue; fi
  out=$(insmod $M/$m.ko 2>&1)
  echo "insmod $m -> rc=$? ${out}"
done
echo

echo "=== after ==="
lsmod
echo "--- cards ---"
cat /proc/asound/cards 2>&1
echo "--- /dev/snd ---"
ls /dev/snd/ 2>&1
echo

echo "=== dmesg since ==="
dmesg 2>/dev/null | grep -iE 'asoc|snd_soc|wcd|wsa|sdm845.*snd|soundcard|sound card' | tail -30
echo DONE
