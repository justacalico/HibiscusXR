import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';

import '../../l10n/app_localizations.dart';
import '../store_controller.dart';
import '../store_state.dart';
import 'app_detail.dart';
import 'repo_sheet.dart';
import 'theme.dart';
import 'widgets.dart';

/// Two-pane catalog: a filterable list on the left, the selected app's
/// detail on the right. Narrow windows collapse to the list and push
/// the detail as a route.
class StorePage extends StatefulWidget {
  const StorePage({super.key, required this.controller});

  final StoreController controller;

  @override
  State<StorePage> createState() => _StorePageState();
}

class _StorePageState extends State<StorePage> {
  final _search = TextEditingController();

  @override
  void initState() {
    super.initState();
    _search.text = widget.controller.store.query;
  }

  @override
  void dispose() {
    _search.dispose();
    super.dispose();
  }

  void _openApp(String packageName, bool compact) {
    widget.controller.store.select(packageName);
    if (compact) {
      Navigator.of(context).push(
        MaterialPageRoute<void>(
          builder: (_) =>
              AppDetailPage(controller: widget.controller),
        ),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    final controller = widget.controller;
    return Scaffold(
      body: AnimatedBuilder(
        animation: controller.store,
        builder: (context, _) => LayoutBuilder(
          builder: (context, constraints) {
            final compact = constraints.maxWidth < 860;
            final list = _CatalogPane(
              controller: controller,
              search: _search,
              onOpen: (pkg) => _openApp(pkg, compact),
            );
            if (compact) return list;
            return Row(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                SizedBox(width: 400, child: list),
                const VerticalDivider(
                  width: 1,
                  thickness: 1,
                  color: StoreTheme.surfaceHigh,
                ),
                Expanded(
                  child: AppDetailPane(controller: controller),
                ),
              ],
            );
          },
        ),
      ),
    );
  }
}

class _CatalogPane extends StatelessWidget {
  const _CatalogPane({
    required this.controller,
    required this.search,
    required this.onOpen,
  });

  final StoreController controller;
  final TextEditingController search;
  final ValueChanged<String> onOpen;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final store = controller.store;
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Padding(
          padding: const EdgeInsets.fromLTRB(24, 24, 20, 4),
          child: Row(
            children: [
              SvgPicture.asset('assets/store_icon.svg', width: 30, height: 30),
              const SizedBox(width: 12),
              Text(
                l10n.appTitle,
                style: const TextStyle(
                  fontSize: 22,
                  fontWeight: FontWeight.w700,
                  color: StoreTheme.textPrimary,
                ),
              ),
              if (store.status == LoadStatus.ready) ...[
                const SizedBox(width: 10),
                Padding(
                  padding: const EdgeInsets.only(top: 4),
                  child: Text(
                    l10n.appsCount(store.index!.apps.length),
                    style: const TextStyle(
                      fontSize: 13,
                      color: StoreTheme.textSecondary,
                    ),
                  ),
                ),
              ],
              const Spacer(),
              _SortMenu(controller: controller),
              IconButton(
                tooltip: l10n.repoMenu,
                icon: const Icon(Icons.dns_outlined, size: 20),
                color: StoreTheme.textSecondary,
                onPressed: () => showRepoSheet(context, controller),
              ),
            ],
          ),
        ),
        Padding(
          padding: const EdgeInsets.fromLTRB(20, 10, 20, 6),
          child: TextField(
            controller: search,
            onChanged: store.setQuery,
            style: const TextStyle(fontSize: 14, color: StoreTheme.textPrimary),
            decoration: InputDecoration(
              hintText: l10n.searchHint,
              hintStyle: const TextStyle(
                fontSize: 14,
                color: StoreTheme.textSecondary,
              ),
              prefixIcon: const Icon(
                Icons.search,
                size: 18,
                color: StoreTheme.textSecondary,
              ),
              isDense: true,
              filled: true,
              fillColor: StoreTheme.surface,
              contentPadding: const EdgeInsets.symmetric(vertical: 12),
              border: OutlineInputBorder(
                borderRadius: BorderRadius.circular(24),
                borderSide: BorderSide.none,
              ),
            ),
          ),
        ),
        SizedBox(
          height: 44,
          child: ListView(
            scrollDirection: Axis.horizontal,
            padding: const EdgeInsets.symmetric(horizontal: 20),
            children: [
              _CategoryChip(
                label: l10n.categoryAll,
                active: store.category == null,
                onTap: () => store.setCategory(null),
              ),
              for (final c in store.categories)
                _CategoryChip(
                  label: c,
                  active: store.category == c,
                  onTap: () => store.setCategory(c),
                ),
            ],
          ),
        ),
        const SizedBox(height: 6),
        Expanded(child: _CatalogBody(controller: controller, onOpen: onOpen)),
      ],
    );
  }
}

class _SortMenu extends StatelessWidget {
  const _SortMenu({required this.controller});

  final StoreController controller;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return PopupMenuButton<SortMode>(
      tooltip: null,
      icon: const Icon(Icons.sort, size: 20, color: StoreTheme.textSecondary),
      color: StoreTheme.surfaceHigh,
      initialValue: controller.store.sort,
      onSelected: controller.store.setSort,
      itemBuilder: (context) => [
        for (final mode in SortMode.values)
          PopupMenuItem(
            value: mode,
            child: Row(
              children: [
                SizedBox(
                  width: 20,
                  child: mode == controller.store.sort
                      ? const Icon(
                          Icons.check,
                          size: 16,
                          color: StoreTheme.accent,
                        )
                      : null,
                ),
                const SizedBox(width: 8),
                Text(
                  switch (mode) {
                    SortMode.updated => l10n.sortUpdated,
                    SortMode.name => l10n.sortName,
                  },
                  style: const TextStyle(
                    fontSize: 13,
                    color: StoreTheme.textPrimary,
                  ),
                ),
              ],
            ),
          ),
      ],
    );
  }
}

class _CategoryChip extends StatelessWidget {
  const _CategoryChip({
    required this.label,
    required this.active,
    required this.onTap,
  });

  final String label;
  final bool active;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.only(right: 8),
      child: ChoiceChip(
        label: Text(label),
        selected: active,
        onSelected: (_) => onTap(),
        labelStyle: TextStyle(
          fontSize: 13,
          fontWeight: active ? FontWeight.w600 : FontWeight.w400,
          color: active ? Colors.white : StoreTheme.textSecondary,
        ),
        selectedColor: StoreTheme.accent,
        backgroundColor: StoreTheme.surface,
        showCheckmark: false,
        side: BorderSide.none,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(16),
        ),
      ),
    );
  }
}

class _CatalogBody extends StatelessWidget {
  const _CatalogBody({required this.controller, required this.onOpen});

  final StoreController controller;
  final ValueChanged<String> onOpen;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final store = controller.store;
    switch (store.status) {
      case LoadStatus.idle:
      case LoadStatus.loading:
        return const Center(
          child: SizedBox(
            width: 28,
            height: 28,
            child: CircularProgressIndicator(
              strokeWidth: 3,
              color: StoreTheme.accent,
            ),
          ),
        );
      case LoadStatus.error:
        return StatusPane(
          icon: Icons.cloud_off_outlined,
          title: l10n.loadErrorTitle,
          body: '${l10n.loadErrorBody}\n${store.error ?? ''}',
          onRetry: controller.refresh,
        );
      case LoadStatus.ready:
        final apps = store.apps;
        if (apps.isEmpty) {
          return StatusPane(
            icon: Icons.search_off,
            title: l10n.emptyResults,
          );
        }
        return ListView.builder(
          padding: const EdgeInsets.only(bottom: 20),
          itemCount: apps.length,
          itemBuilder: (context, i) {
            final app = apps[i];
            return AppCard(
              app: app,
              controller: controller,
              selected: store.selectedPackage == app.packageName,
              onTap: () => onOpen(app.packageName),
            );
          },
        );
    }
  }
}
