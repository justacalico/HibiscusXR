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
  String get projectName => 'PN2Lineage';

  @override
  String get navFeatures => '功能';

  @override
  String get navScreenshots => '截图';

  @override
  String get navDownload => '下载';

  @override
  String get navAbout => '关于';

  @override
  String get navDocs => '文档';

  @override
  String get navMenu => '菜单';

  @override
  String get navClose => '关闭';

  @override
  String get themeSystem => '跟随系统';

  @override
  String get themeLight => '浅色';

  @override
  String get themeDark => '深色';

  @override
  String get themeLabel => '外观';

  @override
  String get languageLabel => '语言';

  @override
  String get heroEyebrow => 'Pico Neo 2 LineageOS 移植版';

  @override
  String get heroTitle => '应用库';

  @override
  String get heroSubtitle => '头显里的所有应用,收在一个安静的网格里。搜索、置顶、分组、启动。';

  @override
  String get heroPrimary => '查看功能';

  @override
  String get heroSecondary => '阅读文档';

  @override
  String get homeShowcaseEyebrow => '应用网格';

  @override
  String get homeShowcaseTitle => '装过的应用,一眼看全。';

  @override
  String get homeShowcaseBody =>
      '每个磁贴都按应用图标的颜色衬底,带名称、角标和菜单按钮。它住在 vrhome 外壳里,充当应用窗口。';

  @override
  String get homeFindTitle => '找起来快';

  @override
  String get homeFindBody => '搜索不区分大小写和重音,集合把普通应用和系统应用分开,四种排序让网格保持你离开时的样子。';

  @override
  String get homeArrangeTitle => '顺序由你';

  @override
  String get homeArrangeBody => '把常用应用钉在顶部,长按拖动磁贴排位,再把应用收进自己命名的分组。';

  @override
  String get homeControlTitle => '手柄优先';

  @override
  String get homeControlBody => '方向键移动焦点,确认键启动,菜单键打开磁贴菜单。为手柄设计,用什么都顺手。';

  @override
  String get homeOpenTitle => '开源到底。';

  @override
  String get homeOpenBody =>
      '应用库是 AGPL-3.0 下的自由软件。每个界面、每条字符串、每次提交都公开在 GitLab 上。';

  @override
  String get homeOpenSource => '浏览源码';

  @override
  String get homeDownloadTitle => '还没发货。';

  @override
  String get homeDownloadBody => '现在还没有可以安装的发布版本。移植还在一块块拼,想提前用就盯仓库或者自己从源码构建。';

  @override
  String get homeDownloadCta => '下载状态';

  @override
  String get featuresTitle => '功能';

  @override
  String get featuresSubtitle => '一个启动器形态的工具,为头显调过。';

  @override
  String get featGridTitle => '网格像货架一样好认';

  @override
  String get featGridBody => '每个能启动的应用都有磁贴:图标、名称,还有按图标主色衬出来的底色。新装和卸载实时出现,不用刷新。';

  @override
  String get featFindTitle => '查找';

  @override
  String get featSearchTitle => '搜索不挑输入法';

  @override
  String get featSearchBody => '匹配忽略大小写和重音,凭印象敲出来的名字就够找到它。';

  @override
  String get featCollectionsTitle => '集合带计数';

  @override
  String get featCollectionsBody => '全部、置顶、应用、系统一直在。自己建的分组也排在同一行,各自带实时计数。';

  @override
  String get featSortTitle => '四种排序';

  @override
  String get featSortBody => 'A-Z、Z-A、最近安装、最近更新——拖动磁贴之后就是自定义顺序。';

  @override
  String get featArrangeTitle => '整理';

  @override
  String get featPinTitle => '钉在顶部';

  @override
  String get featPinBody => '置顶的应用在每个集合里都排头。长按拖动任意磁贴重排,顺序重启后还在。';

  @override
  String get featGroupsTitle => '分组';

  @override
  String get featGroupsBody => '建、改名、删分组,再从磁贴菜单把应用收进去。分组就是另一个集合。';

  @override
  String get featControlTitle => '控制';

  @override
  String get featMenuTitle => '每个磁贴都有菜单';

  @override
  String get featMenuBody => '打开、置顶、加进分组、应用详情,用户应用还能卸载——长按或者菜单键就够。';

  @override
  String get featInstallTitle => '原地装 APK';

  @override
  String get featInstallBody => '系统文件选择器处理安装,侧载不用离开窗口。';

  @override
  String get featNavigateTitle => '导航';

  @override
  String get featDpadTitle => '原生方向键';

  @override
  String get featDpadBody => '方向键移动焦点,确认键启动焦点应用,菜单键开它的磁贴菜单。焦点回绕方式和电视启动器一样。';

  @override
  String get featLiveTitle => '包更新实时刷新';

  @override
  String get featLiveBody => '在系统任何地方装或删应用,网格当着你的面更新。';

  @override
  String get featI18nTitle => '字符串全部外置';

  @override
  String get featI18nBody => '所有界面文字都在 ARB 文件里,翻译这个应用就是改一个文件,不用翻代码。';

  @override
  String get screenshotsTitle => '截图';

  @override
  String get screenshotsSubtitle => '真机应用,跑在头显上。';

  @override
  String get shotGridCaption => 'vrhome 里的应用网格。';

  @override
  String get shotCollectionCaption => '集合和分组,带计数。';

  @override
  String get shotMenuCaption => '长按出来的磁贴菜单。';

  @override
  String get downloadTitle => '下载';

  @override
  String get downloadSubtitle => '还没有可装的东西。';

  @override
  String get downloadStatusTitle => '尚未发布';

  @override
  String get downloadStatusBody =>
      '应用库随 PN2Lineage 系统镜像一起发货,镜像还在构建中。等有了可刷写版本,第一时间放这里。';

  @override
  String get downloadStepsTitle => '想提前用?';

  @override
  String get downloadStepSource => '克隆仓库,用 Flutter 工具链自己构建应用。';

  @override
  String get downloadStepDocs => '看文档里移植的构建流水线和当前进度。';

  @override
  String get downloadStepWatch => '盯仓库,发布消息会在那里出现。';

  @override
  String get downloadSourceCta => '在 GitLab 上看源码';

  @override
  String get downloadDocsCta => '打开文档';

  @override
  String get downloadNote => '构建版本是给头显系统镜像签名的。侧载能跑,但应用库本来就打算住在 vrhome 里。';

  @override
  String get aboutTitle => '关于';

  @override
  String get aboutSubtitle => '大移植里的一小块。';

  @override
  String get aboutWhatTitle => '这是什么';

  @override
  String get aboutWhatBody =>
      '应用库是 vrhome 的应用窗口——vrhome 是 Pico Neo 2 跑 LineageOS 17.1 时的 VR 主界面。它列出头显上每个能启动的应用,然后让开路。';

  @override
  String get aboutProjectTitle => '项目';

  @override
  String get aboutProjectBody =>
      'PN2Lineage 把 LineageOS 17.1——通过 phh GSI 的 Android 10——移植到 Pico Neo 2。移植的每一块,从 GSI 覆盖层到 VR 栈,都在 GitLab 的 neosalsa 下各占一个仓库。';

  @override
  String get aboutTechTitle => '怎么搭的';

  @override
  String get aboutTechBody =>
      'Flutter,所有决定都在纯 Dart 模块里,平台胶水压到最薄。Kotlin 那边只问 PackageManager、栅格化图标、发 intent。';

  @override
  String get aboutLicenseTitle => '许可证';

  @override
  String get aboutLicenseBody =>
      '应用库是 GNU Affero 通用公共许可证 v3 下的自由软件。本站捆绑的 Inter 字体走 SIL 开放字体许可证 1.1。';

  @override
  String get aboutRepoCta => '应用库仓库';

  @override
  String get aboutDocsCta => '项目文档';

  @override
  String get aboutGroupCta => 'neosalsa 全部仓库';

  @override
  String get footerProduct => '应用库';

  @override
  String get footerProject => '项目';

  @override
  String footerCopyright(int year) {
    return '版权所有 $year neosalsa';
  }

  @override
  String get footerLicense => 'AGPL-3.0 许可证';

  @override
  String get footerBuiltWith => '用 Flutter 构建';

  @override
  String get languageName => '中文';
}
