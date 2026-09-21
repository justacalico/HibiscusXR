# 375 - Pico Neo 2 controller 6DOF tracking: root cause map and blocker

Deep dive into why the controllers connect but report only sentinel poses
(+/-100,-300,-300, quat -1,0,0,0). Settings pairing/battery/keys work; the
fused 6DOF path does not.

## Verified + deployed fixes (libPvr_UnitySDKCV.so, /system/priv-app/CVService/lib/arm/)

Two real bugs found by intercepting dlopen with a shim:

1. `InitServiceClient` exported symbol (0x787b0) was stubbed to
   `movs r0,0; bx lr` (bytes `00 20 70 47`). The real body sits at 0x787b4
   orphaned. Restored the `push {r3-r7,lr}` prologue. This alone made the
   client path reachable.
2. `GetPvrServiceClient` dlopens `libpvrserviceclient.so` via a literal that
   lands +2 bytes into the string (requests `bpvrserviceclient.so`). The
   `add r0,pc` uses pc=0x78722 not the aligned 0x78720. Shifted the literal
   pool entry by -2. After both fixes `InitServiceClient` returns 1 and
   `GetPvrServiceClient` returns a real client pointer.

## Head sensor dispatch (config 0xe in psmvr_GetIntConfig)

`StartSensor` picks the head-reference class from config index 0xe:
- 0 FalconServiceSensor -> ASensorManager_getDefaultSensor(0x25=37). No type-37
  sensor exists on this device -> "Failed to Start Head Sensor".
- 1 UsbSensor, 2/3 PhoneSensor (accel+gyro+mag, all stream), 4
  PicoNeoControllerSensor, 6 BnoSensor, 7 QvrServiceSensor (svrInitialize/
  svrBeginVr).
- Forced `movs r4,#2` (PhoneSensor) at 0x3bc4c: head IMU thread now runs,
  accel/gyro/mag stream to it. Stops the null head-sensor crash.
- Stock actually uses headSensorState=1 and logs "Stopped Head Sensor" - stock
  uses the PvrService head pose (getTrackingDataExt works, live SLAM pose), not
  a local sensor. PhoneSensor is a workaround, not the stock path.

## The actual blocker: EM/NDI fusion threads never spawn

Controllers stream raw IMU over the Nordic radio (txIMURead reads spidev1.0
~257 reads/s; setEmData runs). But the decode/fusion thread group never spawns:
- em-decode-imu (logs setconnectState on stock) - absent
- emReadNDIDATAThreadMain (~0x3b5cc, waits on spiReadThreadStatus, produces
  fused position/quat/status) - absent
- emReadEMDataThreadMain (~0x3a58e) - absent

The EM data reader opens /dev/spidev2.0 and a GPIO sysfs
(89c000.i2c/i2c-5/5-0057/temp_gpio129); neither exists. Kernel SPI tree has
spi0/spi1/spi3, no spi2. Need to confirm from stock captures whether
spidev2.0 ever existed or EM data arrives another way (radio?).

Pose getter `getControllerSensorStateWithHeadDataAndPreTime` ->
EmImuDataProcess::GetIMUData crashes (null) because the fusion object is never
built -> RemoteService restart loop.

## Open questions

- Where are the EM/decode threads spawned and what gates them (tracking mode?
  EM source present? calibration?).
- Does 6DOF position need spidev2.0/FPGA EM hardware that the kernel DT lacks,
  or is EM data sent back over the radio (spidev1.0) and purely software-gated?
- product is FalconCV2 / ro.pvr.controller.service=cv - is "CV" camera-tracked
  (QVRServiceCamDeviceHAL3 runs) rather than EM? If camera-tracked, position may
  come from the SLAM camera path, not EM at all.
