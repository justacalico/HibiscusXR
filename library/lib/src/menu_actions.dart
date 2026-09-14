import 'library_store.dart';
import 'models.dart';
import 'text_norm.dart';

/// Context-menu actions for a tile. Ordered as shown.
enum AppAction {
  open,
  pin,
  unpin,
  addToGroup,
  removeFromGroup,
  details,
  uninstall,
}

/// Which actions a tile's menu offers. Pure: the UI only maps entries to
/// labels and icons.
List<AppAction> menuActionsFor(
  AppEntry app, {
  required bool pinned,
  required bool inGroup,
}) {
  return [
    AppAction.open,
    pinned ? AppAction.unpin : AppAction.pin,
    AppAction.addToGroup,
    if (inGroup) AppAction.removeFromGroup,
    AppAction.details,
    if (!app.isSystem) AppAction.uninstall,
  ];
}

/// Entries in the collection dropdown, in display order: flat filters,
/// groups sorted by name, then the create action.
sealed class CollectionItem {
  const CollectionItem();
}

class FilterItem extends CollectionItem {
  const FilterItem(this.filter);
  final LibraryFilter filter;
}

class GroupItem extends CollectionItem {
  const GroupItem(this.group);
  final AppGroup group;
}

class NewGroupItem extends CollectionItem {
  const NewGroupItem();
}

List<CollectionItem> collectionItems(LibraryStore store) {
  final groups = List.of(store.groups)
    ..sort(
        (a, b) => normalizeForSearch(a.name).compareTo(normalizeForSearch(b.name)));
  return [
    const FilterItem(FilterAll()),
    const FilterItem(FilterPinned()),
    const FilterItem(FilterUserApps()),
    const FilterItem(FilterSystemApps()),
    for (final g in groups) GroupItem(g),
    const NewGroupItem(),
  ];
}
