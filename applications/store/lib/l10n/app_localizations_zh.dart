// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for Chinese (`zh`).
class AppLocalizationsZh extends AppLocalizations {
  AppLocalizationsZh([String locale = 'zh']) : super(locale);

  @override
  String get appTitle => '商店';

  @override
  String get searchHint => '搜索应用';

  @override
  String get categoryAll => '全部';

  @override
  String get sortUpdated => '最近更新';

  @override
  String get sortName => '名称 A-Z';

  @override
  String appsCount(int count) {
    String _temp0 = intl.Intl.pluralLogic(
      count,
      locale: localeName,
      other: '$count 个应用',
      one: '1 个应用',
    );
    return '$_temp0';
  }

  @override
  String get install => '安装';

  @override
  String get installing => '安装中';

  @override
  String get downloading => '下载中';

  @override
  String get open => '打开';

  @override
  String get retry => '重试';

  @override
  String get installedBadge => '已安装';

  @override
  String get installFailed => '安装失败';

  @override
  String get installPrompted => '在系统对话框里确认安装';

  @override
  String versionRow(String version, int code) {
    return '$version ($code)';
  }

  @override
  String get loadErrorTitle => '无法加载目录';

  @override
  String get loadErrorBody => '检查仓库地址和网络连接。';

  @override
  String get emptyResults => '没有匹配搜索的应用';

  @override
  String get selectAppPrompt => '选一个应用查看详情';

  @override
  String get repoMenu => '仓库';

  @override
  String get repoDialogTitle => '仓库';

  @override
  String get repoDialogBody => '目录和安装包来自这个兼容 F-Droid 的仓库。';

  @override
  String get repoUrlLabel => '仓库地址';

  @override
  String get repoUrlInvalid => '输入 http 或 https 地址';

  @override
  String get repoReset => '重置为默认';

  @override
  String get cancel => '取消';

  @override
  String get save => '保存';

  @override
  String get sectionDescription => '关于';

  @override
  String get sectionScreenshots => '截图';

  @override
  String get sectionVersions => '版本';

  @override
  String get metaVersion => '版本';

  @override
  String get metaSize => '大小';

  @override
  String get metaLicense => '许可证';

  @override
  String get metaUpdated => '更新';

  @override
  String get metaMinSdk => '最低 SDK';

  @override
  String byAuthor(String author) {
    return '作者 $author';
  }
}
