// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for Chinese (`zh`).
class AppLocalizationsZh extends AppLocalizations {
  AppLocalizationsZh([String locale = 'zh']) : super(locale);

  @override
  String get appTitle => '应用库';

  @override
  String get searchHint => '搜索';

  @override
  String get clearSearch => '清除搜索';

  @override
  String get collectionAll => '全部';

  @override
  String collectionAllCount(int count) {
    return '全部 ($count)';
  }

  @override
  String collectionCount(String label, int count) {
    return '$label ($count)';
  }

  @override
  String get collectionPinned => '已置顶';

  @override
  String get collectionUserApps => '应用';

  @override
  String get collectionSystemApps => '系统';

  @override
  String get collectionGroups => '分组';

  @override
  String get sortCustom => '自定义排序';

  @override
  String get sortNameAsc => 'A-Z';

  @override
  String get sortNameDesc => 'Z-A';

  @override
  String get sortNewest => '最近安装';

  @override
  String get sortUpdated => '最近更新';

  @override
  String get openApp => '打开';

  @override
  String get pinApp => '置顶';

  @override
  String get unpinApp => '取消置顶';

  @override
  String get uninstallApp => '卸载';

  @override
  String get appInfo => '应用信息';

  @override
  String get addToGroup => '加入分组';

  @override
  String get removeFromGroup => '移出分组';

  @override
  String get newGroup => '新建分组';

  @override
  String get renameGroup => '重命名分组';

  @override
  String get deleteGroup => '删除分组';

  @override
  String deleteGroupConfirm(String name) {
    return '删除 \"$name\"? 应用会保持安装。';
  }

  @override
  String get groupNameHint => '分组名称';

  @override
  String get noGroupsYet => '还没有分组';

  @override
  String get installApp => '安装应用';

  @override
  String get emptyLibrary => '没有安装应用';

  @override
  String emptySearch(String query) {
    return '没有匹配 \"$query\" 的结果';
  }

  @override
  String get emptyGroup => '这个分组里没有应用';

  @override
  String get loadingApps => '正在加载应用...';

  @override
  String get loadFailed => '无法加载应用';

  @override
  String launchFailed(String app) {
    return '无法打开 $app';
  }

  @override
  String uninstallFailed(String app) {
    return '无法卸载 $app';
  }

  @override
  String get retry => '重试';

  @override
  String get cancel => '取消';

  @override
  String get create => '创建';

  @override
  String get save => '保存';

  @override
  String get delete => '删除';

  @override
  String appCount(int count) {
    String _temp0 = intl.Intl.pluralLogic(
      count,
      locale: localeName,
      other: '$count 个应用',
      one: '1 个应用',
      zero: '没有应用',
    );
    return '$_temp0';
  }

  @override
  String get systemBadge => '系统';

  @override
  String get details => '详情';

  @override
  String detailsTitle(String app) {
    return '关于 $app';
  }

  @override
  String get detailsPackage => '包名';

  @override
  String get detailsVersion => '版本';

  @override
  String get detailsInstalled => '安装时间';

  @override
  String get detailsUpdated => '更新时间';

  @override
  String get notAvailable => '—';
}
