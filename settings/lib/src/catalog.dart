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
  SectionDef(SectionId.display, [
    ItemId.brightness,
    ItemId.nightMode,
  ]),
  SectionDef(SectionId.sound, [
    ItemId.volume,
    ItemId.micMute,
  ]),
  SectionDef(SectionId.camera, [
    ItemId.seethrough,
  ]),
  SectionDef(SectionId.language, [
    ItemId.languagePicker,
  ]),
  SectionDef(SectionId.time, [
    ItemId.timeZone,
  ]),
  SectionDef(SectionId.keyboard, [
    ItemId.keyboardPicker,
  ]),
  SectionDef(SectionId.headsetTracking, [
    ItemId.trackingToggle,
    ItemId.trackingFrequency,
    ItemId.boundary,
    ItemId.resetView,
  ]),
  SectionDef(SectionId.backup, [
    ItemId.backupNow,
  ]),
  SectionDef(SectionId.developer, [
    ItemId.devOptions,
  ]),
  SectionDef(SectionId.softwareUpdate, [
    ItemId.buildNumber,
    ItemId.updateCheck,
  ]),
  SectionDef(SectionId.power, [
    ItemId.batterySaver,
    ItemId.sleep,
    ItemId.restart,
  ]),
  SectionDef(SectionId.about, [
    ItemId.modelName,
    ItemId.androidVersion,
    ItemId.aboutOpen,
  ]),
  SectionDef(SectionId.tips, [
    ItemId.tipsBody,
  ]),
];

const kItemKinds = <ItemId, ItemKind>{
  ItemId.wifiToggle: ItemKind.toggle,
  ItemId.wifiSsid: ItemKind.info,
  ItemId.wifiSettings: ItemKind.action,
  ItemId.bluetoothToggle: ItemKind.toggle,
  ItemId.bluetoothSettings: ItemKind.action,
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
  ItemId.batterySaver: ItemKind.action,
  ItemId.sleep: ItemKind.action,
  ItemId.restart: ItemKind.action,
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

SectionDef sectionDef(SectionId id) =>
    kSections.firstWhere((s) => s.id == id);

ItemKind kindOf(ItemId id) => kItemKinds[id] ?? ItemKind.info;

List<String> optionsOf(ItemId id) => kChoiceOptions[id] ?? const [];
