import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_quicksettings/main.dart';

void main() {
  testWidgets('app boots', (tester) async {
    await tester.pumpWidget(const QuickSettingsApp());
    expect(find.text('Quick Settings'), findsOneWidget);
  });
}
