import 'models.dart';

/// Parse `getprop` output - lines look like `[ro.product.model]: [Pico Neo 2]`.
Map<String, String> parseGetprop(String text) {
  final out = <String, String>{};
  final re = RegExp(r'^\[([^\]]+)\]:\s*\[(.*)\]\s*$');
  for (final line in text.split('\n')) {
    final m = re.firstMatch(line.trimRight());
    if (m != null) out[m.group(1)!] = m.group(2)!;
  }
  return out;
}

/// `dumpsys battery` lines like `  level: 84`.
int parseBatteryLevel(String dumpsys) {
  final m = RegExp(r'level:\s*(\d+)').firstMatch(dumpsys);
  return m == null ? -1 : int.parse(m.group(1)!);
}

TrackingMode parseDofMode(Map<String, String> props) {
  final raw = (props['persist.pn2.dof'] ?? props['persist.hibiscus.dof'] ?? '')
      .toLowerCase();
  if (raw.contains('6')) return TrackingMode.dof6;
  if (raw.contains('3')) return TrackingMode.dof3;
  return TrackingMode.unknown;
}

DeviceInfo deviceInfoFromProps(
  Map<String, String> props, {
  String serial = '',
  String address = '',
  int batteryLevel = -1,
}) {
  return DeviceInfo(
    model: props['ro.product.model'] ?? '',
    device: props['ro.product.device'] ?? '',
    brand: props['ro.product.brand'] ?? '',
    manufacturer: props['ro.product.manufacturer'] ?? '',
    androidRelease: props['ro.build.version.release'] ?? '',
    sdkInt: int.tryParse(props['ro.build.version.sdk'] ?? '') ?? 0,
    buildId: props['ro.build.display.id'] ?? props['ro.build.id'] ?? '',
    hibiscusVersion: props['ro.hibiscus.version'] ?? '',
    trackingMode: parseDofMode(props),
    serial: serial,
    address: address,
    batteryLevel: batteryLevel,
  );
}
