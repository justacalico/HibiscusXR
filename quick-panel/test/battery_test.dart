import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_quicksettings/src/battery.dart';

void main() {
  test('battery tint bands', () {
    expect(batteryTintFor(100), BatteryTint.good);
    expect(batteryTintFor(80), BatteryTint.good);
    expect(batteryTintFor(79), BatteryTint.normal);
    expect(batteryTintFor(60), BatteryTint.normal);
    expect(batteryTintFor(59), BatteryTint.warn);
    expect(batteryTintFor(20), BatteryTint.warn);
    expect(batteryTintFor(19), BatteryTint.critical);
    expect(batteryTintFor(0), BatteryTint.critical);
  });
}
