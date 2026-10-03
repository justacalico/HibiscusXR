// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for Chinese (`zh`).
class AppLocalizationsZh extends AppLocalizations {
  AppLocalizationsZh([String locale = 'zh']) : super(locale);

  @override
  String get appTitle => '快捷设置';

  @override
  String get settings => '设置';

  @override
  String get tileWifi => 'Wi-Fi';

  @override
  String get tileBluetooth => '蓝牙';

  @override
  String get tileSeethrough => '透视';

  @override
  String get tileBoundary => '边界';

  @override
  String get tileMicrophone => '麦克风';

  @override
  String get tileNightMode => '夜间模式';

  @override
  String get tileDoNotDisturb => '勿扰';

  @override
  String get tileAirplaneMode => '飞行模式';

  @override
  String get tileBatterySaver => '省电模式';

  @override
  String get tileCenterLaunch => '新窗口居中';

  @override
  String get actionResetView => '重置视角';

  @override
  String get actionReportProblem => '报告问题';

  @override
  String get actionAboutDevice => '关于设备';

  @override
  String get sliderVolume => '音量';

  @override
  String get sliderBrightness => '亮度';

  @override
  String get stateOn => '开';

  @override
  String get stateOff => '关';

  @override
  String get stateNotConnected => '未连接';

  @override
  String get notifTitle => '通知';

  @override
  String get notifEmpty => '没有通知';

  @override
  String get notifDismiss => '关闭';

  @override
  String get notifClearAll => '全部清除';

  @override
  String batteryPercent(int level) {
    return '$level%';
  }
}
