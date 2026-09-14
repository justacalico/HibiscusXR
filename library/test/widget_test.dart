import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_library/main.dart';
import 'package:pn2_library/src/library_controller.dart';
import 'package:pn2_library/src/persistence.dart';
import 'package:pn2_library/src/platform/fake_app_source.dart';

LibraryController fakeController(FakeAppSource source) =>
    LibraryController(source: source, persistence: MemoryPersistence());

void main() {
  testWidgets('renders the library grid with demo apps', (tester) async {
    final src = FakeAppSource();
    addTearDown(src.dispose);
    await tester.pumpWidget(LibraryApp(controller: fakeController(src)));
    await tester.pumpAndSettle();
    expect(find.text('App Library'), findsOneWidget);
    expect(find.text('VR Home'), findsOneWidget);
    expect(find.text('Settings'), findsOneWidget);
  });
}
