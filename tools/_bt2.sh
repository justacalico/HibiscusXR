P=$(pidof pvrservice)
echo "pvrservice pid=$P threads=$(ls /proc/$P/task | wc -l)"
debuggerd -b $P 2>&1 | grep -E '^"|#0[0-9]' | head -70
