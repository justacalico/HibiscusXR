// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for Chinese (`zh`).
class AppLocalizationsZh extends AppLocalizations {
  AppLocalizationsZh([String locale = 'zh']) : super(locale);

  @override
  String get appTitle => '设置';

  @override
  String get sectionWifi => 'Wi-Fi';

  @override
  String get sectionBluetooth => '蓝牙';

  @override
  String get sectionControllers => '手柄';

  @override
  String get sectionDisplay => '显示';

  @override
  String get sectionEnvironment => '主界面环境';

  @override
  String get sectionSound => '声音';

  @override
  String get sectionLanguage => '语言和地区';

  @override
  String get sectionTime => '时间';

  @override
  String get sectionKeyboard => '键盘';

  @override
  String get sectionDeveloper => '开发者';

  @override
  String get sectionAbout => '关于';

  @override
  String get itemWifiToggle => 'Wi-Fi';

  @override
  String get itemWifiToggleDesc => '连接无线网络';

  @override
  String get itemWifiSsid => '网络';

  @override
  String get itemWifiSsidDesc => '头显当前连接的网络';

  @override
  String get itemWifiList => '可用网络';

  @override
  String get itemWifiListDesc => '点按网络进行连接或管理';

  @override
  String get itemBluetoothToggle => '蓝牙';

  @override
  String get itemBluetoothToggleDesc => '配对手柄和配件';

  @override
  String get itemBtList => '设备';

  @override
  String get itemBtListDesc => '已配对和附近的蓝牙设备';

  @override
  String get itemControllerPair => '扫描手柄';

  @override
  String get itemControllerPairDesc => '开始连接扫描,然后按住每个手柄上的配对按钮直到震动';

  @override
  String get itemControllerLeft => '左手柄';

  @override
  String get itemControllerLeftDesc => '连接状态和电量';

  @override
  String get itemControllerRight => '右手柄';

  @override
  String get itemControllerRightDesc => '连接状态和电量';

  @override
  String get itemControllerUnbind => '全部解除配对';

  @override
  String get itemControllerUnbindDesc => '忘记两个已配对的手柄';

  @override
  String get itemBrightness => '亮度';

  @override
  String get itemBrightnessDesc => '调整屏幕亮度';

  @override
  String get itemTheme => '主题';

  @override
  String get itemThemeDesc => '面板和头显 HUD 的颜色';

  @override
  String get themeDark => '深色';

  @override
  String get themeLight => '浅色';

  @override
  String get themeOled => 'OLED';

  @override
  String get itemIpd => '瞳距 (IPD)';

  @override
  String get itemIpdDesc => '将双眼间距调整到与你瞳孔的距离一致。Neo 2 的镜片是固定的,所以这里移动的是渲染画面';

  @override
  String get itemDeviceMode => '设备模式';

  @override
  String get itemDeviceModeDesc => '3DoF 只追踪头部转动。6DoF 还通过追踪摄像头追踪位置';

  @override
  String get itemNightMode => '夜间模式';

  @override
  String get itemNightModeDesc => '天黑后让屏幕颜色变暖';

  @override
  String get itemEnvList => '主界面环境';

  @override
  String get itemEnvListDesc =>
      '选择主界面的环境。通过 adb 把 .zip 文件推到 /data/local/tmp/hibiscus/envs';

  @override
  String get itemHomeEnv => '当前环境';

  @override
  String get itemHomeEnvDesc => '主界面加载的环境';

  @override
  String get envPassthrough => '透视';

  @override
  String get envPassthroughDesc => '追踪摄像头的实时画面';

  @override
  String get envBuiltin => '内置';

  @override
  String get envBuiltinDesc => '默认的天空穹顶和地面网格';

  @override
  String get envEmpty => '没有安装环境';

  @override
  String get envMissingMap => '里面没有 map.obj';

  @override
  String get envBadName => '文件名不是合法的环境 id';

  @override
  String get envUnreadable => 'zip 无法读取';

  @override
  String envRemoveTitle(String name) {
    return '移除 $name?';
  }

  @override
  String get envRemoveBody => 'zip 文件会从头显上删除。';

  @override
  String get envRemove => '移除';

  @override
  String get itemVolume => '音量';

  @override
  String get itemVolumeDesc => '调整媒体音量';

  @override
  String get itemMicMute => '麦克风';

  @override
  String get itemMicMuteDesc => '允许应用使用麦克风';

  @override
  String get itemLanguagePicker => '语言';

  @override
  String get itemLanguagePickerDesc => '更改系统语言';

  @override
  String get languageEnglish => 'English';

  @override
  String get languageChineseSimplified => '简体中文';

  @override
  String get itemTimeZone => '日期和时间';

  @override
  String get itemTimeZoneDesc => '设置时区和时钟格式';

  @override
  String get itemImeList => '屏幕键盘';

  @override
  String get itemImeListDesc => '选择当前输入法';

  @override
  String get itemAdbToggle => 'USB 调试';

  @override
  String get itemAdbToggleDesc => '允许电脑通过 USB 调试头显';

  @override
  String get itemStayAwake => '保持唤醒';

  @override
  String get itemStayAwakeDesc => '充电时永不休眠';

  @override
  String get itemShowTouches => '显示触摸';

  @override
  String get itemShowTouchesDesc => '在触摸位置显示圆点';

  @override
  String get itemDebugHud => '调试 HUD';

  @override
  String get itemDebugHudDesc => '在任何应用上保持显示调试状态行';

  @override
  String get itemModelName => '型号';

  @override
  String get itemModelNameDesc => '头显的硬件型号';

  @override
  String get itemAndroidVersion => '安卓版本';

  @override
  String get itemAndroidVersionDesc => '系统运行的安卓版本';

  @override
  String get itemHibiscusVersion => 'Hibiscus 版本';

  @override
  String get itemHibiscusVersionDesc => '头显上安装的 Hibiscus 构建';

  @override
  String get itemAboutBrand => 'HibiscusXR';

  @override
  String get itemAboutBrandDesc => '头显上的系统软件';

  @override
  String get valueNotConnected => '未连接';

  @override
  String get valueUnknown => '未知';

  @override
  String get value3dof => '3DoF';

  @override
  String get value6dof => '6DoF';

  @override
  String get controllerConnected => '已连接';

  @override
  String get controllerDisconnected => '未连接';

  @override
  String get controllerPairing => '配对中...';

  @override
  String get controllerScanning => '正在扫描手柄...';

  @override
  String get controllerScanIdle => '未在扫描';

  @override
  String get wifiScanning => '正在扫描网络...';

  @override
  String get wifiScanIdle => '点按刷新进行扫描';

  @override
  String get wifiEmpty => '没有找到网络';

  @override
  String get wifiSecured => '已加密';

  @override
  String get wifiOpen => '开放';

  @override
  String get wifiSaved => '已保存';

  @override
  String get wifiConnected => '已连接';

  @override
  String get wifiRescan => '刷新';

  @override
  String wifiJoinTitle(String ssid) {
    return '连接到 $ssid';
  }

  @override
  String get wifiPassword => '密码';

  @override
  String get wifiConnect => '连接';

  @override
  String get wifiForget => '忘记';

  @override
  String get btScanning => '正在搜索设备...';

  @override
  String get btScanIdle => '点按刷新进行搜索';

  @override
  String get btEmpty => '没有找到设备';

  @override
  String get btRescan => '刷新';

  @override
  String get btPair => '配对';

  @override
  String get btPaired => '已配对';

  @override
  String get btConnected => '已连接';

  @override
  String get btForget => '忘记';

  @override
  String get imeActive => '使用中';

  @override
  String controllerBattery(int level) {
    return '电量 $level/5';
  }

  @override
  String get controllerCharging => '充电中';

  @override
  String sliderPercent(int percent) {
    return '$percent%';
  }

  @override
  String ipdMm(String mm) {
    return '$mm mm';
  }

  @override
  String get uiOnlyModeTitle => '仅界面模式';

  @override
  String get uiOnlyModeBody => '这个构建没有系统后端,页面只是预览。这里的改动不会到达设备。';

  @override
  String get rebootRequiredTitle => '需要重启';

  @override
  String get rebootRequiredBody => '更改这个设置需要重启头显。';

  @override
  String get rebootConfirm => '重启';
}
