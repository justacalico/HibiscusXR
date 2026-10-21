import 'package:common/common.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  test('push keeps the newest entries once capacity is exceeded', () {
    final q = BoundedQueue<int>(3);
    for (var i = 0; i < 5; i++) {
      q.push(i);
    }
    expect(q.toList(), [2, 3, 4]);
    expect(q.length, 3);
  });

  test('push under capacity drops nothing', () {
    final q = BoundedQueue<String>(400)..push('a')..push('b');
    expect(q.toList(), ['a', 'b']);
  });

  test('behaves like a queue: iterate, join, clear', () {
    final q = BoundedQueue<String>(4)
      ..push('x')
      ..push('y');
    expect(q.join('\n'), 'x\ny');
    expect(q.isNotEmpty, isTrue);
    q.clear();
    expect(q.isEmpty, isTrue);
  });
}
