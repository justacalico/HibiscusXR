#!/system/bin/sh
echo "################ /persist/pvr/lens/axisOffset.txt ################"
cat /persist/pvr/lens/axisOffset.txt 2>&1
ls -la /persist/pvr/lens/ 2>&1
echo
echo "################ /vendor/etc/qvr/config_default.txt ################"
cat /vendor/etc/qvr/config_default.txt 2>&1
echo
echo "################ svrapi_config.txt (lens / distortion / fov lines) ################"
grep -inE 'lens|distort|warp|fov|eye|ipd|mesh|chroma|projection|frustum' /vendor/etc/qvr/svrapi_config.txt 2>/dev/null
echo
echo "################ svrapi_config_default.txt (same filter) ################"
grep -inE 'lens|distort|warp|fov|eye|ipd|mesh|chroma|projection|frustum' /vendor/etc/qvr/svrapi_config_default.txt 2>/dev/null
echo
echo "################ qvrservice_config_default.txt (same filter) ################"
grep -inE 'lens|distort|warp|fov|eye|ipd|mesh|chroma|projection|frustum|panel|resolution' /vendor/etc/qvr/qvrservice_config_default.txt 2>/dev/null
echo
echo "################ 6dof_config.xml ################"
cat /vendor/etc/qvr/6dof_config.xml 2>&1
echo
echo "################ live qvrservice config (persist) ################"
cat /persist/vendorhw/pxr/qvrservice_config.txt 2>/dev/null | grep -inE 'lens|distort|warp|fov|eye|ipd' | head -30
echo
echo "################ does the dangling psmvrapi symlink matter? ################"
ls -la /persist/pvr/psmvrapi_config.txt 2>&1
ls -la /system/etc/pvr/ 2>&1
echo DONE
