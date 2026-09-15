import '../l10n/app_localizations.dart';
import 'catalog.dart';
import 'models.dart';

/// Tile and subtitle text resolution. Pure mapping so tests can assert
/// every string the panel can show without pumping widgets.
String tileLabel(TileSpec spec, AppLocalizations l10n) {
  switch (spec.toggleId) {
    case ToggleId.wifi:
      return l10n.tileWifi;
    case ToggleId.bluetooth:
      return l10n.tileBluetooth;
    case ToggleId.seethrough:
      return l10n.tileSeethrough;
    case ToggleId.boundary:
      return l10n.tileBoundary;
    case ToggleId.microphone:
      return l10n.tileMicrophone;
    case ToggleId.nightMode:
      return l10n.tileNightMode;
    case ToggleId.doNotDisturb:
      return l10n.tileDoNotDisturb;
    case ToggleId.airplaneMode:
      return l10n.tileAirplaneMode;
    case ToggleId.batterySaver:
      return l10n.tileBatterySaver;
    case null:
      break;
  }
  switch (spec.actionId) {
    case ActionId.resetView:
      return l10n.actionResetView;
    case ActionId.reportProblem:
      return l10n.actionReportProblem;
    case ActionId.aboutDevice:
      return l10n.actionAboutDevice;
    case ActionId.openSettings:
      return l10n.settings;
    case ActionId.close:
      return l10n.close;
    case ActionId.minimize:
      return l10n.minimize;
    case null:
      return '';
  }
}

String subtitleText(TileSubtitle subtitle, AppLocalizations l10n) {
  switch (subtitle.kind) {
    case SubtitleKind.none:
      return '';
    case SubtitleKind.on:
      return l10n.stateOn;
    case SubtitleKind.off:
      return l10n.stateOff;
    case SubtitleKind.notConnected:
      return l10n.stateNotConnected;
    case SubtitleKind.custom:
      return subtitle.text ?? '';
  }
}
