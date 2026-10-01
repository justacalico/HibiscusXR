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
    ItemId.wifiList,
  ]),
  SectionDef(SectionId.bluetooth, [
    ItemId.bluetoothToggle,
    ItemId.btList,
  ]),
  SectionDef(SectionId.controllers, [
    ItemId.controllerPair,
    ItemId.controllerLeft,
    ItemId.controllerRight,
    ItemId.controllerUnbind,
  ]),
  SectionDef(SectionId.display, [
    ItemId.brightness,
    ItemId.themeMode,
    ItemId.ipd,
    ItemId.deviceMode,
    ItemId.nightMode,
  ]),
  SectionDef(SectionId.environment, [
    ItemId.envList,
    ItemId.homeEnv,
  ]),
  SectionDef(SectionId.sound, [ItemId.volume, ItemId.micMute]),
  SectionDef(SectionId.language, [ItemId.languagePicker]),
  SectionDef(SectionId.time, [ItemId.timeZone]),
  SectionDef(SectionId.keyboard, [ItemId.imeList]),
  SectionDef(SectionId.developer, [
    ItemId.adbToggle,
    ItemId.stayAwake,
    ItemId.showTouches,
    ItemId.debugHud,
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
  ItemId.wifiList: ItemKind.wifiList,
  ItemId.bluetoothToggle: ItemKind.toggle,
  ItemId.btList: ItemKind.btList,
  ItemId.controllerPair: ItemKind.scanCard,
  ItemId.controllerLeft: ItemKind.controller,
  ItemId.controllerRight: ItemKind.controller,
  ItemId.controllerUnbind: ItemKind.action,
  ItemId.brightness: ItemKind.slider,
  // three options, one string on the wire: the hibiscus_theme global key
  ItemId.themeMode: ItemKind.theme,
  ItemId.ipd: ItemKind.slider,
  // two modes, one bool on the wire: off is 3DoF, on is 6DoF
  ItemId.deviceMode: ItemKind.choice,
  ItemId.nightMode: ItemKind.toggle,
  ItemId.envList: ItemKind.envList,
  // the selection itself is a text row: the card edits it, the row shows it
  ItemId.homeEnv: ItemKind.info,
  ItemId.volume: ItemKind.slider,
  ItemId.micMute: ItemKind.toggle,
  ItemId.languagePicker: ItemKind.language,
  ItemId.timeZone: ItemKind.action,
  ItemId.imeList: ItemKind.imeList,
  ItemId.adbToggle: ItemKind.toggle,
  ItemId.stayAwake: ItemKind.toggle,
  ItemId.showTouches: ItemKind.toggle,
  ItemId.debugHud: ItemKind.toggle,
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

/// Rows whose change only counts after a reboot. Picking a new value
/// pops the reboot-confirm dialog instead of applying right away; the
/// write still goes down the toggle channel once the user confirms.
const kRequiresReboot = <ItemId>{
  ItemId.deviceMode,
};

/// Locale tags the language dropdown offers, in display order. The row
/// rides the text channel: the wire value is the BCP 47 tag the platform
/// writes through LocalePicker.
const kLanguageOptions = <String>['en', 'zh-CN'];

/// Map a platform-reported locale tag onto a dropdown option. English
/// dialects land on English; only unambiguous Simplified tags land on
/// zh-CN, so a Traditional locale (zh-TW, zh-Hant) shows its raw tag
/// instead of a wrong pick.
String? languageOptionFor(String? tag) {
  if (tag == null || tag.isEmpty) return null;
  final t = tag.toLowerCase();
  if (t.startsWith('en')) return 'en';
  if (t == 'zh' ||
      t.startsWith('zh-hans') ||
      t.startsWith('zh-cn') ||
      t.startsWith('zh-sg') ||
      t.startsWith('zh-my')) {
    return 'zh-CN';
  }
  return null;
}

SectionDef sectionDef(SectionId id) => kSections.firstWhere((s) => s.id == id);

ItemKind kindOf(ItemId id) => kItemKinds[id] ?? ItemKind.info;

bool implementedOf(ItemId id) => !kUnimplemented.contains(id);

bool requiresRebootOf(ItemId id) => kRequiresReboot.contains(id);
