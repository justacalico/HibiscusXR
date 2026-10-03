// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for Chinese (`zh`).
class AppLocalizationsZh extends AppLocalizations {
  AppLocalizationsZh([String locale = 'zh']) : super(locale);

  @override
  String get appTitle => 'HCTE';

  @override
  String get appSubtitle => 'Hibiscus 头显调试工具';

  @override
  String get navOverview => '概览';

  @override
  String get navDisplay => '画面';

  @override
  String get navInstall => '安装';

  @override
  String get navTracking => '追踪';

  @override
  String get navDebug => '调试';

  @override
  String get connectTitle => '连接头显';

  @override
  String get connectUsbSection => 'USB / adb 设备';

  @override
  String get connectWirelessSection => '无线连接';

  @override
  String get connectWirelessHint => '头显 IP 地址';

  @override
  String get connectWirelessButton => '连接';

  @override
  String get connectRefresh => '刷新';

  @override
  String get connectScanning => '正在扫描设备...';

  @override
  String get connectConnecting => '正在连接...';

  @override
  String get connectNoDevices => '没有 adb 设备。插入头显,或通过 Wi-Fi 连接。';

  @override
  String get connectWirelessNote => '先尝试设备上的 CTE 服务,再退回无线 adb。';

  @override
  String get disconnect => '断开';

  @override
  String get overviewHeadset => '头显';

  @override
  String get overviewControllers => '手柄';

  @override
  String get overviewLink => '连接方式';

  @override
  String get overviewBattery => '电量';

  @override
  String get overviewTrackingMode => '追踪';

  @override
  String get overviewAndroid => '安卓';

  @override
  String get overviewBuild => '构建';

  @override
  String get overviewSerial => '序列号';

  @override
  String get overviewAddress => '地址';

  @override
  String get overviewHibiscus => 'Hibiscus';

  @override
  String get ctrlLeft => '左手柄';

  @override
  String get ctrlRight => '右手柄';

  @override
  String get ctrlConnected => '已连接';

  @override
  String get ctrlAbsent => '未连接';

  @override
  String ctrlBattery(Object pct) {
    return '$pct%';
  }

  @override
  String get ctrlTracked => '追踪中';

  @override
  String get ctrlUntracked => '无追踪';

  @override
  String get displayMirror => '镜像';

  @override
  String get displayStart => '开始镜像';

  @override
  String get displayStop => '停止';

  @override
  String get displayHint => '画面按头显截图的速度持续推流。';

  @override
  String displayStats(Object count, Object rate) {
    return '$rate 帧/秒 · 共 $count 帧';
  }

  @override
  String get displayNoFrame => '还没有画面';

  @override
  String get installTitle => '安装 APK';

  @override
  String get installPick => '选择 APK';

  @override
  String get installRun => '安装';

  @override
  String get installRunning => '安装中...';

  @override
  String get installDone => '完成';

  @override
  String get installDrop => '选一个 .apk 推到头显上。';

  @override
  String get trackingTitle => '追踪';

  @override
  String get trackingHead => '头部位姿';

  @override
  String trackingRate(Object rate) {
    return '$rate 次/秒';
  }

  @override
  String trackingMode(String mode) {
    String _temp0 = intl.Intl.selectLogic(mode, {
      'dof6': '6DoF',
      'dof3': '3DoF',
      'other': '未知',
    });
    return '$_temp0';
  }

  @override
  String get trackingIdle => '没有位姿数据 - 头显上要有 VR 应用在跑才会有数据流。';

  @override
  String get trackingPosition => '位置';

  @override
  String get trackingOrientation => '姿态';

  @override
  String get trackingLog => '位姿日志';

  @override
  String get debugTitle => '调试';

  @override
  String get debugProps => '属性';

  @override
  String get debugFilter => '过滤';

  @override
  String get debugLogcat => '日志';

  @override
  String get debugEmpty => '连上头显才能读状态。';

  @override
  String get valueUnknown => '未知';
}
