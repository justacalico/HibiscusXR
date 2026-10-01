import '../l10n/app_localizations.dart';
import 'envs/env_info.dart';
import 'models.dart';
import 'package:panel_theme/panel_theme.dart';
import 'units.dart';

/// id -> localized string resolution. Kept out of the widgets so the
/// mapping is one table and unit tests can hit every id.
String sectionTitle(AppLocalizations l10n, SectionId id) {
  switch (id) {
    case SectionId.wifi:
      return l10n.sectionWifi;
    case SectionId.bluetooth:
      return l10n.sectionBluetooth;
    case SectionId.controllers:
      return l10n.sectionControllers;
    case SectionId.display:
      return l10n.sectionDisplay;
    case SectionId.environment:
      return l10n.sectionEnvironment;
    case SectionId.sound:
      return l10n.sectionSound;
    case SectionId.language:
      return l10n.sectionLanguage;
    case SectionId.time:
      return l10n.sectionTime;
    case SectionId.keyboard:
      return l10n.sectionKeyboard;
    case SectionId.developer:
      return l10n.sectionDeveloper;
    case SectionId.about:
      return l10n.sectionAbout;
  }
}

String itemTitle(AppLocalizations l10n, ItemId id) {
  switch (id) {
    case ItemId.wifiToggle:
      return l10n.itemWifiToggle;
    case ItemId.wifiSsid:
      return l10n.itemWifiSsid;
    case ItemId.wifiList:
      return l10n.itemWifiList;
    case ItemId.bluetoothToggle:
      return l10n.itemBluetoothToggle;
    case ItemId.btList:
      return l10n.itemBtList;
    case ItemId.controllerPair:
      return l10n.itemControllerPair;
    case ItemId.controllerLeft:
      return l10n.itemControllerLeft;
    case ItemId.controllerRight:
      return l10n.itemControllerRight;
    case ItemId.controllerUnbind:
      return l10n.itemControllerUnbind;
    case ItemId.brightness:
      return l10n.itemBrightness;
    case ItemId.themeMode:
      return l10n.itemTheme;
    case ItemId.ipd:
      return l10n.itemIpd;
    case ItemId.deviceMode:
      return l10n.itemDeviceMode;
    case ItemId.nightMode:
      return l10n.itemNightMode;
    case ItemId.envList:
      return l10n.itemEnvList;
    case ItemId.homeEnv:
      return l10n.itemHomeEnv;
    case ItemId.volume:
      return l10n.itemVolume;
    case ItemId.micMute:
      return l10n.itemMicMute;
    case ItemId.languagePicker:
      return l10n.itemLanguagePicker;
    case ItemId.timeZone:
      return l10n.itemTimeZone;
    case ItemId.imeList:
      return l10n.itemImeList;
    case ItemId.adbToggle:
      return l10n.itemAdbToggle;
    case ItemId.stayAwake:
      return l10n.itemStayAwake;
    case ItemId.showTouches:
      return l10n.itemShowTouches;
    case ItemId.debugHud:
      return l10n.itemDebugHud;
    case ItemId.modelName:
      return l10n.itemModelName;
    case ItemId.androidVersion:
      return l10n.itemAndroidVersion;
    case ItemId.hibiscusVersion:
      return l10n.itemHibiscusVersion;
    case ItemId.aboutBrand:
      return l10n.itemAboutBrand;
  }
}

String itemDescription(AppLocalizations l10n, ItemId id) {
  switch (id) {
    case ItemId.wifiToggle:
      return l10n.itemWifiToggleDesc;
    case ItemId.wifiSsid:
      return l10n.itemWifiSsidDesc;
    case ItemId.wifiList:
      return l10n.itemWifiListDesc;
    case ItemId.bluetoothToggle:
      return l10n.itemBluetoothToggleDesc;
    case ItemId.btList:
      return l10n.itemBtListDesc;
    case ItemId.controllerPair:
      return l10n.itemControllerPairDesc;
    case ItemId.controllerLeft:
      return l10n.itemControllerLeftDesc;
    case ItemId.controllerRight:
      return l10n.itemControllerRightDesc;
    case ItemId.controllerUnbind:
      return l10n.itemControllerUnbindDesc;
    case ItemId.brightness:
      return l10n.itemBrightnessDesc;
    case ItemId.themeMode:
      return l10n.itemThemeDesc;
    case ItemId.ipd:
      return l10n.itemIpdDesc;
    case ItemId.deviceMode:
      return l10n.itemDeviceModeDesc;
    case ItemId.nightMode:
      return l10n.itemNightModeDesc;
    case ItemId.envList:
      return l10n.itemEnvListDesc;
    case ItemId.homeEnv:
      return l10n.itemHomeEnvDesc;
    case ItemId.volume:
      return l10n.itemVolumeDesc;
    case ItemId.micMute:
      return l10n.itemMicMuteDesc;
    case ItemId.languagePicker:
      return l10n.itemLanguagePickerDesc;
    case ItemId.timeZone:
      return l10n.itemTimeZoneDesc;
    case ItemId.imeList:
      return l10n.itemImeListDesc;
    case ItemId.adbToggle:
      return l10n.itemAdbToggleDesc;
    case ItemId.stayAwake:
      return l10n.itemStayAwakeDesc;
    case ItemId.showTouches:
      return l10n.itemShowTouchesDesc;
    case ItemId.debugHud:
      return l10n.itemDebugHudDesc;
    case ItemId.modelName:
      return l10n.itemModelNameDesc;
    case ItemId.androidVersion:
      return l10n.itemAndroidVersionDesc;
    case ItemId.hibiscusVersion:
      return l10n.itemHibiscusVersionDesc;
    case ItemId.aboutBrand:
      return l10n.itemAboutBrandDesc;
  }
}

/// Link state text for a controller row.
String controllerLinkLabel(AppLocalizations l10n, ControllerLink link) {
  switch (link) {
    case ControllerLink.connected:
      return l10n.controllerConnected;
    case ControllerLink.disconnected:
      return l10n.controllerDisconnected;
    case ControllerLink.pairing:
      return l10n.controllerPairing;
    case ControllerLink.unknown:
      return l10n.valueUnknown;
  }
}

/// What the "Active environment" row prints for a selection id. The
/// reserved ids get proper names; an environment id shows as-is.
String envSelectionLabel(AppLocalizations l10n, String? id) => switch (id) {
  null || '' || kEnvPassthrough => l10n.envPassthrough,
  kEnvBuiltin => l10n.envBuiltin,
  _ => id,
};

/// Label for one segment of the theme picker.
String themeChoiceLabel(AppLocalizations l10n, ThemeChoice choice) =>
    switch (choice) {
      ThemeChoice.dark => l10n.themeDark,
      ThemeChoice.light => l10n.themeLight,
      ThemeChoice.oled => l10n.themeOled,
    };

/// Status line under the scan card title.
String scanStatusLabel(AppLocalizations l10n, bool scanning) =>
    scanning ? l10n.controllerScanning : l10n.controllerScanIdle;

/// Security description for a wifi list row.
String wifiSecurityLabel(AppLocalizations l10n, WifiNetwork net) =>
    net.security == WifiSecurity.open ? l10n.wifiOpen : l10n.wifiSecured;

/// Subtitle under a wifi row's ssid: link state, then whether the
/// network is saved, then how it's secured.
String wifiNetworkLabel(AppLocalizations l10n, WifiNetwork net) {
  if (net.connected) return l10n.wifiConnected;
  if (net.saved) return '${l10n.wifiSaved} · ${wifiSecurityLabel(l10n, net)}';
  return wifiSecurityLabel(l10n, net);
}

/// Subtitle under a bluetooth row's name.
String btDeviceLabel(AppLocalizations l10n, BtDevice dev) => dev.connected
    ? l10n.btConnected
    : dev.bonded
    ? l10n.btPaired
    : dev.address;

/// Status line under the bluetooth card title.
String btScanLabel(AppLocalizations l10n, bool discovering) =>
    discovering ? l10n.btScanning : l10n.btScanIdle;

/// Text beside a slider showing its current value. Most sliders are a
/// plain percentage; the IPD row reports millimetres.
String sliderValueLabel(AppLocalizations l10n, ItemId id, double v) =>
    switch (id) {
      ItemId.ipd => l10n.ipdMm(ipdFromSlider(v).toStringAsFixed(1)),
      _ => l10n.sliderPercent((v * 100).round()),
    };

/// Text at a slider's right end marking the top of the range.
String sliderMaxLabel(AppLocalizations l10n, ItemId id) => switch (id) {
  ItemId.ipd => l10n.ipdMm(kIpdMaxMm.toStringAsFixed(0)),
  _ => l10n.sliderPercent(100),
};
