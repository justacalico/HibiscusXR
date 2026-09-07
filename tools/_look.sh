echo "--- live trackingstate / pose ---"
logcat -d | grep -iE "trackingstate|Bad Pose|kLostDialog|CalculateDialogState" | tail -8
echo "  (empty above = no lost-tracking path taken)"
echo "--- is the compositor submitting frames? ---"
logcat -d | grep -iE "TimeWarp|submitFrame|BeginFrame|EndFrame|fps|Frame rate" | tail -10
echo "--- focused window ---"
dumpsys window | grep -E "mCurrentFocus|mFocusedApp"
echo "--- unity scene / shell activity ---"
logcat -d | grep -iE "Unity|VrShell|Home|Scene|Boundary|Seethrough|SafetyArea" | tail -14
echo "--- vr mode owner ---"
logcat -d | grep -iE "VR mode|EnterVrMode|LeaveVrMode" | tail -6
