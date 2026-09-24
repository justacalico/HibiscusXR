import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_settings/src/units.dart';

void main() {
  test('slider endpoints map to the mm range ends', () {
    expect(ipdFromSlider(0.0), kIpdMinMm);
    expect(ipdFromSlider(1.0), kIpdMaxMm);
  });

  test('the panel default lands mid-range', () {
    expect(ipdToSlider(kIpdDefaultMm), closeTo(0.4167, 0.001));
    expect(ipdFromSlider(ipdToSlider(kIpdDefaultMm)), closeTo(63.5, 0.001));
  });

  test('conversion clamps out-of-range values', () {
    expect(ipdFromSlider(-0.5), kIpdMinMm);
    expect(ipdFromSlider(1.5), kIpdMaxMm);
    expect(ipdToSlider(30), 0.0);
    expect(ipdToSlider(90), 1.0);
  });

  test('round-trips a real setting', () {
    expect(ipdToSlider(ipdFromSlider(0.62)), closeTo(0.62, 1e-9));
  });
}
