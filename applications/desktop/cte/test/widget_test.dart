import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:hibiscus_cte/ui/app.dart';

import 'fakes.dart';

void main() {
  testWidgets('connect page renders brand and sections', (tester) async {
    await tester.pumpWidget(CteApp(state: fakeState()));
    await tester.pumpAndSettle();
    expect(find.text('HCTE'), findsOneWidget);
    expect(find.text('Connect to a headset'), findsOneWidget);
    expect(find.text('Wireless'), findsOneWidget);
  });

  testWidgets('connected state shows the nav shell and overview',
      (tester) async {
    final state = fakeState();
    await tester.pumpWidget(CteApp(state: state));
    await tester.pumpAndSettle();
    await state.connectAdb('TEST');
    await tester.pumpAndSettle();
    expect(find.byType(NavigationRail), findsOneWidget);
    expect(find.text('Headset'), findsOneWidget);
    expect(find.text('Pico Neo 2'), findsWidgets);
    expect(find.text('Controllers'), findsOneWidget);
    // hop to the debug page through the rail
    await tester.tap(find.text('Debug'));
    await tester.pumpAndSettle();
    expect(find.text('Properties'), findsOneWidget);
  });
}
