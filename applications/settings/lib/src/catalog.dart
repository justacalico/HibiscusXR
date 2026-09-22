import 'models.dart';

/// Which rows live in each section. Pure data - labels and icons are
/// resolved in the UI layer from the ids.
class SectionDef {
  const SectionDef(this.id, this.items);

  final SectionId id;
  final List<ItemId> items;
}

const kSections = <SectionDef>[
  SectionDef(SectionId.wifi, [
    ItemId.wifiToggle,
    ItemId.wifiSsid,
    ItemId.wifiSettings,
  ]),
  SectionDef(SectionId.bluetooth, [
    ItemId.bluetoothToggle,
    ItemId.bluetoothSettings,
  ]),
  SectionDef(SectionId.controllers, [
    ItemId.controllerPair,
    ItemId.controllerLeft,
    ItemId.controllerRight,
    ItemId.controllerUnbind,
  ]),
  SectionDef(SectionId.display, [ItemId.brightness, ItemId.nightMode]),
  SectionDef(SectionId.sound, [ItemId.volume, ItemId.micMute]),
  SectionDef(SectionId.camera, [ItemId.seethrough]),
  SectionDef(SectionId.language, [ItemId.languagePicker]),
  SectionDef(SectionId.time, [ItemId.timeZone]),
  SectionDef(SectionId.keyboard, [ItemId.keyboardPicker]),
  SectionDef(SectionId.headsetTracking, [
    ItemId.trackingToggle,
    ItemId.trackingFrequency,
    ItemId.boundary,
    ItemId.resetView,
  ]),
  SectionDef(SectionId.backup, [ItemId.backupNow]),
  SectionDef(SectionId.developer, [ItemId.devOptions]),
  SectionDef(SectionId.softwareUpdate, [
    ItemId.buildNumber,
    ItemId.updateCheck,
  ]),
  SectionDef(SectionId.about, [
    ItemId.modelName,
    ItemId.androidVersion,
    ItemId.aboutOpen,
  ]),
  SectionDef(SectionId.tips, [ItemId.tipsBody]),
];

const kItemKinds = <ItemId, ItemKind>{
  ItemId.wifiToggle: ItemKind.toggle,
  ItemId.wifiSsid: ItemKind.info,
  ItemId.wifiSettings: ItemKind.action,
  ItemId.bluetoothToggle: ItemKind.toggle,
  ItemId.bluetoothSettings: ItemKind.action,
  ItemId.controllerPair: ItemKind.scanCard,
  ItemId.controllerLeft: ItemKind.controller,
  ItemId.controllerRight: ItemKind.controller,
  ItemId.controllerUnbind: ItemKind.action,
  ItemId.brightness: ItemKind.slider,
  ItemId.nightMode: ItemKind.toggle,
  ItemId.volume: ItemKind.slider,
  ItemId.micMute: ItemKind.toggle,
  ItemId.seethrough: ItemKind.toggle,
  ItemId.languagePicker: ItemKind.action,
  ItemId.timeZone: ItemKind.action,
  ItemId.keyboardPicker: ItemKind.action,
  ItemId.trackingToggle: ItemKind.toggle,
  ItemId.trackingFrequency: ItemKind.choice,
  ItemId.boundary: ItemKind.toggle,
  ItemId.resetView: ItemKind.action,
  ItemId.backupNow: ItemKind.action,
  ItemId.devOptions: ItemKind.action,
  ItemId.updateCheck: ItemKind.action,
  ItemId.buildNumber: ItemKind.info,
  ItemId.modelName: ItemKind.info,
  ItemId.androidVersion: ItemKind.info,
  ItemId.aboutOpen: ItemKind.action,
  ItemId.tipsBody: ItemKind.info,
};

/// Valid values for each choice row. First entry is the default shown
/// when the platform reports nothing.
const kChoiceOptions = <ItemId, List<String>>{
  ItemId.trackingFrequency: ['auto', '60hz', '50hz'],
};

/// Rows whose platform side does not exist yet. They render greyed out
/// and inert until the OS layer catches up - same convention as the
/// quick panel's unimplemented tiles. The pn2_* seam writes land in a
/// global key nothing reads and its broadcasts have no receiver;
/// updateCheck and resetView are bare broadcasts with the same problem;
/// nightMode calls UiModeManager without MODIFY_DAY_NIGHT_MODE declared,
/// so the SecurityException it always gets is swallowed.
const kUnimplemented = <ItemId>{
  ItemId.seethrough,
  ItemId.trackingToggle,
  ItemId.trackingFrequency,
  ItemId.boundary,
  ItemId.resetView,
  ItemId.updateCheck,
  ItemId.nightMode,
};

SectionDef sectionDef(SectionId id) => kSections.firstWhere((s) => s.id == id);

ItemKind kindOf(ItemId id) => kItemKinds[id] ?? ItemKind.info;

List<String> optionsOf(ItemId id) => kChoiceOptions[id] ?? const [];

bool implementedOf(ItemId id) => !kUnimplemented.contains(id);
