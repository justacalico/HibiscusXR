import '../l10n/app_localizations.dart';
import 'models.dart';

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
    case SectionId.sound:
      return l10n.sectionSound;
    case SectionId.camera:
      return l10n.sectionCamera;
    case SectionId.language:
      return l10n.sectionLanguage;
    case SectionId.time:
      return l10n.sectionTime;
    case SectionId.keyboard:
      return l10n.sectionKeyboard;
    case SectionId.headsetTracking:
      return l10n.sectionHeadsetTracking;
    case SectionId.developer:
      return l10n.sectionDeveloper;
    case SectionId.softwareUpdate:
      return l10n.sectionSoftwareUpdate;
    case SectionId.about:
      return l10n.sectionAbout;
    case SectionId.tips:
      return l10n.sectionTips;
  }
}

String itemTitle(AppLocalizations l10n, ItemId id) {
  switch (id) {
    case ItemId.wifiToggle:
      return l10n.itemWifiToggle;
    case ItemId.wifiSsid:
      return l10n.itemWifiSsid;
    case ItemId.wifiSettings:
      return l10n.itemWifiSettings;
    case ItemId.bluetoothToggle:
      return l10n.itemBluetoothToggle;
    case ItemId.bluetoothSettings:
      return l10n.itemBluetoothSettings;
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
    case ItemId.nightMode:
      return l10n.itemNightMode;
    case ItemId.volume:
      return l10n.itemVolume;
    case ItemId.micMute:
      return l10n.itemMicMute;
    case ItemId.seethrough:
      return l10n.itemSeethrough;
    case ItemId.languagePicker:
      return l10n.itemLanguagePicker;
    case ItemId.timeZone:
      return l10n.itemTimeZone;
    case ItemId.keyboardPicker:
      return l10n.itemKeyboardPicker;
    case ItemId.trackingToggle:
      return l10n.itemTrackingToggle;
    case ItemId.trackingFrequency:
      return l10n.itemTrackingFrequency;
    case ItemId.boundary:
      return l10n.itemBoundary;
    case ItemId.resetView:
      return l10n.itemResetView;
    case ItemId.devOptions:
      return l10n.itemDevOptions;
    case ItemId.updateCheck:
      return l10n.itemUpdateCheck;
    case ItemId.buildNumber:
      return l10n.itemBuildNumber;
    case ItemId.modelName:
      return l10n.itemModelName;
    case ItemId.androidVersion:
      return l10n.itemAndroidVersion;
    case ItemId.aboutBrand:
      return l10n.itemAboutBrand;
    case ItemId.tipsBody:
      return l10n.itemTipsBody;
  }
}

String itemDescription(AppLocalizations l10n, ItemId id) {
  switch (id) {
    case ItemId.wifiToggle:
      return l10n.itemWifiToggleDesc;
    case ItemId.wifiSsid:
      return l10n.itemWifiSsidDesc;
    case ItemId.wifiSettings:
      return l10n.itemWifiSettingsDesc;
    case ItemId.bluetoothToggle:
      return l10n.itemBluetoothToggleDesc;
    case ItemId.bluetoothSettings:
      return l10n.itemBluetoothSettingsDesc;
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
    case ItemId.nightMode:
      return l10n.itemNightModeDesc;
    case ItemId.volume:
      return l10n.itemVolumeDesc;
    case ItemId.micMute:
      return l10n.itemMicMuteDesc;
    case ItemId.seethrough:
      return l10n.itemSeethroughDesc;
    case ItemId.languagePicker:
      return l10n.itemLanguagePickerDesc;
    case ItemId.timeZone:
      return l10n.itemTimeZoneDesc;
    case ItemId.keyboardPicker:
      return l10n.itemKeyboardPickerDesc;
    case ItemId.trackingToggle:
      return l10n.itemTrackingToggleDesc;
    case ItemId.trackingFrequency:
      return l10n.itemTrackingFrequencyDesc;
    case ItemId.boundary:
      return l10n.itemBoundaryDesc;
    case ItemId.resetView:
      return l10n.itemResetViewDesc;
    case ItemId.devOptions:
      return l10n.itemDevOptionsDesc;
    case ItemId.updateCheck:
      return l10n.itemUpdateCheckDesc;
    case ItemId.buildNumber:
      return l10n.itemBuildNumberDesc;
    case ItemId.modelName:
      return l10n.itemModelNameDesc;
    case ItemId.androidVersion:
      return l10n.itemAndroidVersionDesc;
    case ItemId.aboutBrand:
      return l10n.itemAboutBrandDesc;
    case ItemId.tipsBody:
      return l10n.itemTipsBodyDesc;
  }
}

/// Label for a dropdown option value.
String choiceLabel(AppLocalizations l10n, String value) {
  switch (value) {
    case '60hz':
      return l10n.value60hz;
    case '50hz':
      return l10n.value50hz;
    default:
      return l10n.valueAuto;
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

/// Status line under the scan card title.
String scanStatusLabel(AppLocalizations l10n, bool scanning) =>
    scanning ? l10n.controllerScanning : l10n.controllerScanIdle;
