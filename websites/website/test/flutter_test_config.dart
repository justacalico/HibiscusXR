import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter_test/flutter_test.dart';

// Rasterization drifts slightly between engine releases - CI pins one
// Flutter while dev machines run newer ones, so a golden rendered
// locally never matches CI bit-for-bit. Within this share of changed
// pixels the picture is still the same picture.
const double _kGoldenTolerance = 0.02;

class _TolerantGoldenFileComparator extends LocalFileComparator {
  _TolerantGoldenFileComparator(super.testFile);

  @override
  Future<bool> compare(Uint8List imageBytes, Uri golden) async {
    final ComparisonResult result = await GoldenFileComparator.compareLists(
      imageBytes,
      await getGoldenBytes(golden),
    );
    if (result.passed || result.diffPercent <= _kGoldenTolerance) {
      result.dispose();
      return true;
    }
    final String error = await generateFailureOutput(result, golden, basedir);
    result.dispose();
    throw FlutterError(error);
  }
}

Future<void> testExecutable(FutureOr<void> Function() testMain) async {
  if (goldenFileComparator is LocalFileComparator) {
    final Uri basedir = (goldenFileComparator as LocalFileComparator).basedir;
    goldenFileComparator = _TolerantGoldenFileComparator(
      basedir.resolve('flutter_test_config.dart'),
    );
  }
  await testMain();
}
