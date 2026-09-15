import 'package:flutter/material.dart';

void main() {
  runApp(const QuickSettingsApp());
}

class QuickSettingsApp extends StatelessWidget {
  const QuickSettingsApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Quick Settings',
      theme: ThemeData.dark(useMaterial3: true),
      debugShowCheckedModeBanner: false,
      home: const Scaffold(
        body: Center(child: Text('Quick Settings')),
      ),
    );
  }
}
