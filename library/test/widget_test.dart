import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_library/main.dart';

void main() {
  testWidgets('app shell renders the localized title', (tester) async {
    await tester.pumpWidget(const LibraryApp());
    expect(find.text('App Library'), findsOneWidget);
  });
}
