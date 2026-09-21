#!/system/bin/sh
echo "=========== how much did Pico actually change in the QVR layer? ==========="
a=/vendor/etc/qvr/qvrservice_config_default.txt
b=/persist/vendorhw/pxr/qvrservice_config.txt
echo "  QC default : $(wc -c < $a) bytes  md5 $(md5sum $a | cut -d' ' -f1)"
echo "  Pico's     : $(wc -c < $b) bytes  md5 $(md5sum $b | cut -d' ' -f1)"
echo "--- diff ---"
diff "$a" "$b" && echo "  IDENTICAL - Pico did not modify qvrservice's config at all"

echo
echo "=========== and the svrapi config Pico ships vs QC reference ==========="
c=/vendor/etc/qvr/svrapi_config.txt
d=/vendor/etc/qvr/svrapi_config_default.txt
echo "  shipped: $(md5sum $c | cut -d' ' -f1)   default: $(md5sum $d | cut -d' ' -f1)"
diff "$d" "$c" | head -40

echo
echo "=========== config.txt redirect ==========="
grep -vE "^\s*#|^\s*$" /data/misc/user/pxrconfig/config_default.txt 2>/dev/null | head -30
