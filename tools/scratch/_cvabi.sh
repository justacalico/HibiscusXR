P=/data/system/packages.xml
echo "--- before ---"
grep -o "name=\"com.picovr.picovrlib.cvcontroller\"[^>]*" $P | head -1
grep -A2 "com.picovr.picovrlib.cvcontroller" $P | grep -o "primaryCpuAbi=\"[^\"]*\"" | head -1
cp $P $P.bak
# stop the framework so nothing rewrites packages.xml under us
stop
sleep 3
# only touch the cvcontroller package block
sed -i "/com.picovr.picovrlib.cvcontroller/,/<\/package>/ s/primaryCpuAbi=\"arm64-v8a\"/primaryCpuAbi=\"armeabi-v7a\"/" $P
sync
echo "--- after ---"
grep -A4 "com.picovr.picovrlib.cvcontroller" $P | grep -o "primaryCpuAbi=\"[^\"]*\"" | head -1
