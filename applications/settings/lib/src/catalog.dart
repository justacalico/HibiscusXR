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
  SectionDef(SectionId.language, [ItemId.languagePicker]),
  SectionDef(SectionId.time, [ItemId.timeZone]),
  SectionDef(SectionId.keyboard, [ItemId.keyboardPicker]),
  SectionDef(SectionId.developer, [
    ItemId.adbToggle,
    ItemId.stayAwake,
    ItemId.showTouches,
  ]),
  SectionDef(SectionId.about, [
    ItemId.aboutBrand,
    ItemId.modelName,
    ItemId.androidVersion,
    ItemId.hibiscusVersion,
  ]),
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
  ItemId.languagePicker: ItemKind.action,
  ItemId.timeZone: ItemKind.action,
  ItemId.keyboardPicker: ItemKind.action,
  ItemId.adbToggle: ItemKind.toggle,
  ItemId.stayAwake: ItemKind.toggle,
  ItemId.showTouches: ItemKind.toggle,
  ItemId.modelName: ItemKind.info,
  ItemId.androidVersion: ItemKind.info,
  ItemId.hibiscusVersion: ItemKind.info,
  ItemId.aboutBrand: ItemKind.brand,
};

/// Rows whose platform side is not trusted yet. They render greyed out
/// and inert until they are proven on hardware - same convention as the
/// quick panel's unimplemented tiles. nightMode calls UiModeManager
/// without MODIFY_DAY_NIGHT_MODE declared, so the SecurityException it
/// always gets is swallowed; the developer toggles are wired but parked
/// until the secure-settings writes are verified on device.
const kUnimplemented = <ItemId>{
  ItemId.nightMode,
  ItemId.adbToggle,
  ItemId.stayAwake,
  ItemId.showTouches,
};

SectionDef sectionDef(SectionId id) => kSections.firstWhere((s) => s.id == id);

ItemKind kindOf(ItemId id) => kItemKinds[id] ?? ItemKind.info;

bool implementedOf(ItemId id) => !kUnimplemented.contains(id);
