#!/system/bin/sh
# The GSI leaves ro.product.model and ro.product.device empty. Pico's pvrservice
# selects its platform profile from the model, so with them blank it fell back to
# pvrservice_platform_G2U3_PU.ini - the Pico G2 4K profile - on a Neo 2. Wrong
# lens/display geometry is what UpdateLensInfo is reading when VRShell dies.
#
# Stock reports: model="Pico Neo 2", device=PICOA7B10. The Neo 2 profile is N2A2.
exec 2>&1
echo "before: model='$(getprop ro.product.model)' device='$(getprop ro.product.device)'"
setprop ro.product.model "Pico Neo 2"
setprop ro.product.device "PICOA7B10"
setprop ro.product.name "PICOA7B10"
setprop ro.product.brand "Pico"
setprop ro.product.manufacturer "Pico"
echo "after : model='$(getprop ro.product.model)' device='$(getprop ro.product.device)'"
echo
echo "restarting pvrservice so it re-reads the platform profile"
setprop sys.pvr.vrservice.state 2
sleep 5
echo "pvrservice pid: $(pidof pvrservice)"
logcat -d | grep -i 'Platform config file path' | tail -3
echo DONE
