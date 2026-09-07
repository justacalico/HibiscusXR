#!/system/bin/sh
# The pose values are valid but poseStatus is 0, so everything downstream treats
# them as invalid. poseStatus is derived from the tracking mode / QVR state, so
# diff every pvr- and tracking-related property against stock.
getprop | grep -iE "pvr|pxr|6dof|tracking|vr\." | sed 's/^ *//' | sort
