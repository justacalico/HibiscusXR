import 'package:flutter/foundation.dart';

import 'fdroid_index.dart';

enum LoadStatus { idle, loading, ready, error }

enum SortMode { updated, name }

enum InstallPhase { none, downloading, installing, prompted, installed, failed }

/// Per-app install bookkeeping: which step the pipeline is on and how
/// far the download got.
class InstallProgress {
  const InstallProgress(this.phase, {this.progress = 0, this.error});

  final InstallPhase phase;
  final double progress;
  final String? error;
}

/// All app state. Widgets only read this and notify through the
/// controller; it holds no futures and no platform types.
class StoreState extends ChangeNotifier {
  LoadStatus status = LoadStatus.idle;
  String? error;
  RepoIndex? index;
  Uri repoUrl;
  String query = '';
  String? category;
  SortMode sort = SortMode.updated;
  String? selectedPackage;
  final Map<String, InstallProgress> installs = {};

  StoreState({required this.repoUrl});

  List<RepoApp>? _visible;

  /// The catalog filtered by search and category, then sorted. The list
  /// is memoized - download progress notifies every chunk and re-sorting
  /// a repo-sized catalog each time would show up in profiles.
  List<RepoApp> get apps => _visible ??= _computeApps();

  List<RepoApp> _computeApps() {
    final all = index?.apps ?? const <RepoApp>[];
    final q = query.trim().toLowerCase();
    final filtered = [
      for (final app in all)
        if ((category == null || app.categories.contains(category)) &&
            (q.isEmpty ||
                app.name.toLowerCase().contains(q) ||
                app.packageName.toLowerCase().contains(q) ||
                app.summary.toLowerCase().contains(q)))
          app,
    ];
    switch (sort) {
      case SortMode.name:
        filtered.sort(
          (a, b) => a.name.toLowerCase().compareTo(b.name.toLowerCase()),
        );
      case SortMode.updated:
        filtered.sort((a, b) => b.lastUpdated.compareTo(a.lastUpdated));
    }
    return filtered;
  }

  /// Distinct categories across the index, alphabetical.
  List<String> get categories {
    final seen = <String>{};
    for (final app in index?.apps ?? const <RepoApp>[]) {
      seen.addAll(app.categories);
    }
    return seen.toList()..sort();
  }

  RepoApp? appByPackage(String packageName) {
    for (final app in index?.apps ?? const <RepoApp>[]) {
      if (app.packageName == packageName) return app;
    }
    return null;
  }

  RepoApp? get selected =>
      selectedPackage == null ? null : appByPackage(selectedPackage!);

  InstallProgress progressOf(String packageName) =>
      installs[packageName] ?? const InstallProgress(InstallPhase.none);

  void setLoading() {
    status = LoadStatus.loading;
    error = null;
    notifyListeners();
  }

  void setReady(RepoIndex newIndex) {
    index = newIndex;
    status = LoadStatus.ready;
    error = null;
    // The new catalog may have dropped the selected app or the active
    // category chip - drop both rather than show a dead selection.
    if (selectedPackage != null && selected == null) {
      selectedPackage = null;
    }
    if (category != null && !categories.contains(category)) {
      category = null;
    }
    _visible = null;
    notifyListeners();
  }

  void setError(String message) {
    status = LoadStatus.error;
    error = message;
    notifyListeners();
  }

  void setQuery(String value) {
    if (query == value) return;
    query = value;
    _visible = null;
    notifyListeners();
  }

  void setCategory(String? value) {
    if (category == value) return;
    category = value;
    _visible = null;
    notifyListeners();
  }

  void setSort(SortMode value) {
    if (sort == value) return;
    sort = value;
    _visible = null;
    notifyListeners();
  }

  void select(String? packageName) {
    if (selectedPackage == packageName) return;
    selectedPackage = packageName;
    notifyListeners();
  }

  void setRepoUrl(Uri url) {
    if (repoUrl == url) return;
    repoUrl = url;
    notifyListeners();
  }

  void setInstall(String packageName, InstallProgress progress) {
    installs[packageName] = progress;
    notifyListeners();
  }
}
