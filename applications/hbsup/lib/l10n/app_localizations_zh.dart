// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for Chinese (`zh`).
class AppLocalizationsZh extends AppLocalizations {
  AppLocalizationsZh([String locale = 'zh']) : super(locale);

  @override
  String get appTitle => 'HBSUP';

  @override
  String get appSubtitle => 'Hibiscus 备份';

  @override
  String get unsupportedHostTitle => '不受支持的主机系统';

  @override
  String get unsupportedHostBody =>
      'Windows 不是 HBSUP 支持的主机系统。功能可能不正常,遇到问题请换 Linux 或 macOS。';

  @override
  String get unsupportedHostDismiss => '知道了';

  @override
  String get connectTitle => '连接头显';

  @override
  String get connectHint => 'HBSUP 需要 root 过的 adb。先启动原厂系统,执行 adb root,再插上头显。';

  @override
  String get connectRefresh => '刷新';

  @override
  String get connectNoDevices => '没有 adb 设备。插上头显,或者先把无线 adb 开起来。';

  @override
  String get connectUnknown => '未知型号';

  @override
  String get connectButton => '连接';

  @override
  String get disconnect => '断开';

  @override
  String get backupDestTitle => '备份目录';

  @override
  String get backupDestPick => '选择目录';

  @override
  String get backupDestNone => '还没选目录';

  @override
  String backupFreeSpace(String free) {
    return '剩余 $free';
  }

  @override
  String get backupFreeUnknown => '剩余空间未知';

  @override
  String get backupPartsTitle => '分区';

  @override
  String get backupPartsLoading => '正在读取分区表...';

  @override
  String get backupPartsEmpty => '没找到分区。头显上的 adb root 了吗?';

  @override
  String backupTotalSize(String size) {
    return '完整转储:$size';
  }

  @override
  String get backupSpaceOk => '剩余空间够放完整转储';

  @override
  String backupSpaceShort(String needed, String free) {
    return '空间不足 - 转储需要 $needed,只剩 $free';
  }

  @override
  String get backupStart => '开始备份';

  @override
  String get backupRunning => '正在备份...';

  @override
  String get backupCancel => '取消';

  @override
  String get backupDone => '备份完成。把这个目录收好 - 这是你回原厂的唯一退路。';

  @override
  String backupFailed(String error) {
    return '备份失败:$error';
  }

  @override
  String get backupCancelled => '备份已取消';

  @override
  String backupProgress(int done, int total) {
    return '$done / $total 个分区';
  }

  @override
  String get backupLogTitle => '日志';
}
