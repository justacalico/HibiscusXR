#!/system/bin/sh
# Deliberately set the rotation settings WRONG, so that if they are correct after
# a reboot we know pn2-settings.rc actually did it, rather than the old values
# simply having survived in /data.
settings put system accelerometer_rotation 1
settings put system user_rotation 3
echo "sabotaged: user_rotation=$(settings get system user_rotation) accelerometer_rotation=$(settings get system accelerometer_rotation)"
