import 'package:flutter_test/flutter_test.dart';
import 'package:hibiscus_cte/src/models.dart';
import 'package:hibiscus_cte/src/props.dart';

const sampleProps = '''
[ro.product.brand]: [Pico]
[ro.product.device]: [PICOA7B10]
[ro.product.manufacturer]: [Pico]
[ro.product.model]: [Pico Neo 2]
[ro.product.name]: [A7B10]
[ro.build.display.id]: [lineage_A7B10-userdebug 10 QQ3A.200805.001 eng.dev]
[ro.build.version.release]: [10]
[ro.build.version.sdk]: [29]
[ro.hibiscus.version]: [v2026.09.01-r12]
[persist.pn2.dof]: [6dof]
[sys.init.svc.adbd]: [running]
''';

void main() {
  test('parses [key]: [value] lines', () {
    final props = parseGetprop(sampleProps);
    expect(props['ro.product.model'], 'Pico Neo 2');
    expect(props['persist.pn2.dof'], '6dof');
    expect(props.length, 11);
  });

  test('ignores malformed lines', () {
    final props = parseGetprop('noise\n[bad line\n[k]: [v]');
    expect(props.length, 1);
  });

  test('parses dumpsys battery level', () {
    expect(parseBatteryLevel('level: 84\nscale: 100\nstatus: 3'), 84);
    expect(parseBatteryLevel('no match'), -1);
  });

  test('dof mode from props', () {
    expect(parseDofMode({'persist.pn2.dof': '6dof'}), TrackingMode.dof6);
    expect(parseDofMode({'persist.pn2.dof': '3dof'}), TrackingMode.dof3);
    expect(parseDofMode({'persist.hibiscus.dof': '6dof'}), TrackingMode.dof6);
    expect(parseDofMode({}), TrackingMode.unknown);
  });

  test('builds DeviceInfo with known headset name', () {
    final info = deviceInfoFromProps(parseGetprop(sampleProps),
        serial: 'PABC123', batteryLevel: 88);
    expect(info.headsetName, 'Pico Neo 2');
    expect(info.trackingMode, TrackingMode.dof6);
    expect(info.sdkInt, 29);
    expect(info.batteryLevel, 88);
    expect(info.hibiscusVersion, 'v2026.09.01-r12');
  });

  test('unknown device falls back to raw model', () {
    final info = deviceInfoFromProps(
        {'ro.product.model': 'Quest 9', 'ro.product.device': 'whatever'});
    expect(info.headsetName, 'Quest 9');
  });
}
