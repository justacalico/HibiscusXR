import 'dart:math' as math;

/// Quaternion + position pair, in the OpenXR view frame the runtime serves.
class Pose {
  const Pose({
    this.qx = 0,
    this.qy = 0,
    this.qz = 0,
    this.qw = 1,
    this.x = 0,
    this.y = 0,
    this.z = 0,
  });

  final double qx, qy, qz, qw;
  final double x, y, z;

  /// yaw/pitch/roll in degrees, for the readout line.
  (double, double, double) toEulerDeg() {
    final sinr = 2 * (qw * qx + qy * qz);
    final cosr = 1 - 2 * (qx * qx + qy * qy);
    final roll = math.atan2(sinr, cosr);
    final sinp = (2 * (qw * qy - qz * qx)).clamp(-1.0, 1.0);
    final pitch = math.asin(sinp);
    final siny = 2 * (qw * qz + qx * qy);
    final cosy = 1 - 2 * (qy * qy + qz * qz);
    final yaw = math.atan2(siny, cosy);
    return (yaw * 180 / math.pi, pitch * 180 / math.pi, roll * 180 / math.pi);
  }
}

enum TrackingMode { unknown, dof3, dof6 }

/// One head-pose sample pulled out of the pose log stream.
class PoseSample {
  const PoseSample({
    required this.pose,
    required this.timestampNs,
    this.angularVelocity = const [0, 0, 0],
    this.trackingState = 0,
    this.hasPosition = false,
  });

  final Pose pose;
  final int timestampNs;
  final List<double> angularVelocity;
  final int trackingState;
  final bool hasPosition;

  TrackingMode get mode =>
      hasPosition ? TrackingMode.dof6 : TrackingMode.dof3;
}

/// Decoded state of one controller (left=0, right=1).
class CtrlState {
  const CtrlState({
    required this.index,
    this.connected = false,
    this.battery = -1,
    this.pose = const Pose(),
    this.tracked = false,
    this.stickX = 0,
    this.stickY = 0,
    this.buttonA = false,
    this.buttonB = false,
    this.menu = false,
    this.system = false,
    this.trigger = false,
    this.grip = false,
  });

  final int index;
  final bool connected;
  final int battery;
  final Pose pose;
  final bool tracked;
  final int stickX, stickY;
  final bool buttonA, buttonB, menu, system, trigger, grip;

  String get label => index == 0 ? 'L' : 'R';
}

/// What the headset reports about itself.
class DeviceInfo {
  const DeviceInfo({
    this.model = '',
    this.device = '',
    this.brand = '',
    this.manufacturer = '',
    this.androidRelease = '',
    this.sdkInt = 0,
    this.buildId = '',
    this.hibiscusVersion = '',
    this.trackingMode = TrackingMode.unknown,
    this.serial = '',
    this.address = '',
    this.batteryLevel = -1,
  });

  final String model, device, brand, manufacturer;
  final String androidRelease;
  final int sdkInt;
  final String buildId, hibiscusVersion;
  final TrackingMode trackingMode;
  final String serial, address;
  final int batteryLevel;

  /// Display name for the headset: driver label when the device is one of
  /// the supported targets, otherwise the raw model string.
  String get headsetName {
    const known = {
      'PICOA7B10': 'Pico Neo 2',
      'A7B10': 'Pico Neo 2',
    };
    return known[device] ?? (model.isNotEmpty ? model : 'Unknown headset');
  }
}
