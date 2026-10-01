// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for Chinese (`zh`).
class AppLocalizationsZh extends AppLocalizations {
  AppLocalizationsZh([String locale = 'zh']) : super(locale);

  @override
  String get appTitle => 'HibiscusXR';

  @override
  String get navRepos => '仓库';

  @override
  String get navGuide => '刷机指南';

  @override
  String get navAbout => '关于';

  @override
  String get navFaq => '常见问题';

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
  String get heroEyebrow => 'VR 一体机 · LineageOS 17.1';

  @override
  String get heroTitle => 'Android 回来了,在 VR 里。';

  @override
  String get heroSubtitle => '为 VR 一体头显打造的操作系统,基于 LineageOS 17.1。';

  @override
  String get heroSecondary => '阅读文档';

  @override
  String get heroShotCaption => 'vrhome 里悬浮的 library 窗口。';

  @override
  String get shotMockTime => '3:52';

  @override
  String get shotMockSearch => '搜索';

  @override
  String shotMockFilter(int count) {
    return '全部 ($count)';
  }

  @override
  String get shotMockSort => 'A-Z';

  @override
  String get shotMockSystem => '系统';

  @override
  String get shotAppCalendar => '日历';

  @override
  String get shotAppCamera => '相机';

  @override
  String get shotAppChrome => 'Chrome';

  @override
  String get shotAppClock => '时钟';

  @override
  String get shotAppContacts => '联系人';

  @override
  String get shotAppDrive => '云端硬盘';

  @override
  String get shotAppFiles => '文件';

  @override
  String get shotAppGemini => 'Gemini';

  @override
  String get shotAppGlasses => '眼镜';

  @override
  String get statBase => 'Android 10';

  @override
  String get statBaseLabel => 'LineageOS 17.1 GSI';

  @override
  String get statVendor => 'vendor 原样';

  @override
  String get statVendorLabel => '修复都在 overlay 里';

  @override
  String get statLicense => 'AGPL-3.0';

  @override
  String get statLicenseLabel => '所有原创代码';

  @override
  String get homeWayOverlayTitle => 'GSI 加 overlay';

  @override
  String get homeWayOverlayBody =>
      '系统主体是 LineageOS 17.1 GSI,我们的修复全部放在一个小的 overlay 里。vendor 分区保持和原厂逐字节一致,问题都在 system 侧解决。';

  @override
  String get homeWayDeviceTitle => '一个系统,所有头显';

  @override
  String get homeWayDeviceBody =>
      '设备相关的驱动和配置各自独立成目录。移植到新头显只需要写这一层,不用 fork 整个系统。';

  @override
  String get homeWayNotesTitle => '研究过程全部公开';

  @override
  String get homeWayNotesBody =>
      '每条死路都记了档。notes 仓库里约有 300 篇编号笔记,从第一次开机到最后一个黑屏帧,全是原始记录。';

  @override
  String get homeWayCta => '浏览所有仓库';

  @override
  String get homeOpenTitle => '彻底开源。';

  @override
  String get homeOpenBody =>
      '我们写的所有东西都是 AGPL-3.0。厂商的二进制归厂商所有——从你自己的设备提取,写进清单,绝不二次分发。';

  @override
  String get homeOpenSource => '浏览仓库';

  @override
  String get homeDownloadTitle => '自己动手刷。';

  @override
  String get homeDownloadBody =>
      '完整系统镜像——GSI、我们的修复和头显自己的整套栈——就在 out 仓库里。一条 fastboot 命令就能刷上去。';

  @override
  String get homeDownloadCta => '获取镜像';

  @override
  String get homeDevicesEyebrow => '设备兼容性';

  @override
  String get homeDevicesTitle => '能跑在哪些头显上。';

  @override
  String get homeDevicesBody => '每个移植都带自己的驱动和配置。新头显做完移植就会加进这个列表。';

  @override
  String get deviceStateSupported => '已支持,开发中';

  @override
  String get deviceStatePlanned => '计划中';

  @override
  String get deviceNeo2Name => 'Pico Neo 2';

  @override
  String get deviceNeo2Specs => 'A7B10 · 骁龙 845 · 3840×2160';

  @override
  String get deviceNeo2Body => '目前的移植和开发目标机。';

  @override
  String get deviceQuest1Name => 'Oculus Quest 1';

  @override
  String get deviceQuest1Specs => '骁龙 835 · 2880×1600 OLED';

  @override
  String get deviceNeo3Name => 'Pico Neo 3';

  @override
  String get deviceNeo3Specs => '骁龙 XR2 · 3664×1920';

  @override
  String get devicePlannedBody => '等驱动层拆成独立目录后列入计划。';

  @override
  String get reposTitle => '仓库';

  @override
  String get reposSubtitle => '都在 HibiscusXR 单仓库里,一个部件一个目录。';

  @override
  String get reposSoftwareTitle => '软件';

  @override
  String get reposSoftwareBody => '给人用、给人读的东西。';

  @override
  String get reposSourceTitle => '移植源码';

  @override
  String get reposSourceBody => '设备树、修复、工具链和研究笔记——全是我们自己的工作。';

  @override
  String get reposDumpsTitle => '转储与暂存';

  @override
  String get reposDumpsBody => '从硬件里拉出的二进制和流水线中间产物,仅供研究。厂商专有文件归各自所有者所有,绝不二次分发。';

  @override
  String reposCount(int count) {
    return '$count 个目录';
  }

  @override
  String get repoVrhome => '开源 VR 桌面:2D 应用悬浮成窗,原厂 VR 应用全屏运行。';

  @override
  String get repoLibrary => 'vrhome 里的 Flutter 应用网格——搜索、置顶、分组、启动。';

  @override
  String get repoVrdemo => '最小原生 VR 测试应用(pn2vr),用于合成器通路调试。';

  @override
  String get repoWebsite => '就是本站。';

  @override
  String get repoTools => '移植用的全部脚本,按用途分类:侦查、构建、刷机。';

  @override
  String get repoAndroid => 'LineageOS 设备树 device/pico/A7B10,数值全部读自原厂固件。';

  @override
  String get repoOverlay => '铺在 GSI 上的文件:init rc 修复、补丁库、专有文件清单。';

  @override
  String get repoVendorPatch => 'vendor 侧的 init 和 vintf 补丁文件。';

  @override
  String get repoShim => 'ABI shim 库源码,把 8.1 的二进制桥接到 Android 10。';

  @override
  String get repoKeylayout => '头显按键的 input keylayout 文件。';

  @override
  String get repoLens => '来自 /vendor/etc/qvr 的镜片、畸变和 svrapi 配置。';

  @override
  String get repoPersistCalib => '/persist 上的校准文件:相机、镜片。';

  @override
  String get repoNotes => '研究日志——约 300 篇编号文件,全是原始发现。';

  @override
  String get repoExtracted => '反编译的 boot 镜像、设备树、prop 和 VR 二进制。';

  @override
  String get repoPvrDex => 'PVR 系统应用 dex 反编译产物。';

  @override
  String get repoPvrStack => '从原厂拉出的 PVR 服务二进制、库和配置。';

  @override
  String get repoFullstage => '镜像 /system 目录结构的完整镜像构建暂存区。';

  @override
  String get repoImages => '原厂 PUI 4.1.3 OTA、重建镜像和 LUN0 快照。';

  @override
  String get repoBackupNonEye => '非 Eye 机型的全分区备份——回滚的来源。';

  @override
  String get repoGsi => 'LineageOS 17.1 GSI 基底及其 raw ext4 转换产物。';

  @override
  String get repoPvrApps => '从原厂拉出的全部 PVR 系统应用,apk + oat。';

  @override
  String get repoPvrApplibs => '每个系统 apk 旁边的应用私有 lib/ 目录。';

  @override
  String get repoPvrAppsDexed => 'PVR 重打包流水线的 dex 反编译阶段。';

  @override
  String get repoPvrAppsSigned => '重打包流水线的重新签名阶段。';

  @override
  String get repoPvrAppsInjected => '重打包流水线的 native 库注入阶段。';

  @override
  String get repoPvrAppsFinal => '最终重打包并签名的 PVR 应用。';

  @override
  String get repoOemApps => '原始 /oem 分区应用:PVRLauncher、PVRHome 等。';

  @override
  String get repoOemDex => '/oem 应用的 dex 反编译阶段。';

  @override
  String get repoOemInjected => '签名前注入 native 库的 /oem 应用。';

  @override
  String get repoOemFinal => '最终重打包并签名的 /oem 应用。';

  @override
  String get repoSeethrough => '透视校准应用及其 native 库。';

  @override
  String get repoSensorpatch => 'libsensorservice 的二进制补丁工作区。';

  @override
  String get repoAirsvc => '原厂 airservice 和 virtual_input 守护进程及 rc 文件。';

  @override
  String get repoFan => '原厂 fancontrol 和 thermalserviced 二进制。';

  @override
  String get repoOverlayPvr => 'Pico 的资源 overlay 和 public.libraries.txt。';

  @override
  String get repoCdsp => '来自原厂 vendor 的高通 CDSP RPC 库。';

  @override
  String get repoRfsa => 'Hexagon DSP skel 库和 rfsa 文件系统部件。';

  @override
  String get repoQvr => 'QVR 服务客户端库,两种 ABI。';

  @override
  String get repoQvrlibs => 'QVR vendor 库,含 Tobii 眼动核心桩代码。';

  @override
  String get repoNdiFirmware => 'NDI 眼动追踪固件和 w25q 烧写 ELF。';

  @override
  String get repoDeadunit => '一块死机眼动板的 SPI NOR 转储。';

  @override
  String get repoEyeunit => '一块正常眼动机型的 SPI flash 转储。';

  @override
  String get repoBuild => '本地生成的签名密钥——真实密钥绝不入库。';

  @override
  String get repoOut => '构建产物:system-pn2-full.img 和编译好的 shim。';

  @override
  String get repoRef => 'alvr-pico-legacy 的本地克隆,仅作参考。';

  @override
  String get downloadBackupCta => '获取 HBSUP';

  @override
  String get issuesButton => '提交 issue';

  @override
  String get issuesSubtitle => 'Bug、功能请求、求助,都欢迎。';

  @override
  String get buildsTitle => '可用构建';

  @override
  String get buildsSubtitle => '从 dist 流水线实时拉取。Alpha 和 Beta 是预发布版。';

  @override
  String get buildsChannelRelease => '正式版';

  @override
  String get buildsChannelBeta => '测试版';

  @override
  String get buildsChannelAlpha => '内测版';

  @override
  String get buildsEmpty => '此通道暂无构建。';

  @override
  String get buildsError => '无法加载构建,请稍后再试。';

  @override
  String get buildsRetry => '重试';

  @override
  String get buildsFullImage => '完整镜像';

  @override
  String get buildsCleanImage => '精简镜像';

  @override
  String get buildsLogs => '构建日志';

  @override
  String get buildsChecksums => '校验和';

  @override
  String get buildsViewRelease => '在 GitLab 查看';

  @override
  String get buildsLoading => '正在加载构建…';

  @override
  String get faqTitle => '常见问题';

  @override
  String get faqSubtitle => '短回答,不吹牛。';

  @override
  String get faqQ1 => '真的能用了吗?';

  @override
  String get faqA1 => '在支持的硬件上能开机,VRShell 能跑,头部追踪实时生效,VR 画面正常出图。';

  @override
  String get faqQ2 => '刷机安全吗?';

  @override
  String get faqA2 =>
      '有真实风险。只刷 system 分区——写入低于防回滚熔丝允许版本的 bootloader 镜像会让头显永久硬砖。先读刷机指南。';

  @override
  String get faqQ3 => '支持哪个头显?';

  @override
  String get faqA3 =>
      'Pico Neo 2(A7B10 / PICOA7B10)是目前唯一支持的设备,Eye 和非 Eye 两个版本都行,眼动追踪是额外的活。系统本身按跨厂商设计,驱动层拆分完成后会支持更多头显。';

  @override
  String get faqQ4 => '厂商专有文件从哪来?';

  @override
  String get faqA4 =>
      '来自你自己的设备或它的原厂 OTA。overlay 仓库里有一份清单,列出每个所需 blob 的路径、大小、sha256 前缀和用途,源码仓库里一个都不提交。';

  @override
  String get faqQ5 => 'vrhome 是什么?';

  @override
  String get faqA5 =>
      '我们自己写的 VR 桌面环境。原厂 VRShell 依赖闭源的厂商合成器;vrhome 是一个 NativeActivity,把 2D 应用放到悬浮面板上,真正的 VR 应用照常全屏启动。';

  @override
  String get faqQ6 => '用什么许可证?';

  @override
  String get faqA6 =>
      '我们写的所有东西都是 AGPL-3.0。转储出来的 vendor 二进制归各自所有者所有,放在转储仓库里仅供研究。';

  @override
  String get faqQ7 => '能换主环境吗?';

  @override
  String get faqA7 =>
      '能。设置里的「主环境」页有三个选项:摄像头透视、内置天空场景,或用 adb 推到 /data/local/tmp/hibiscus/envs 的自定义 zip 环境包。包里是一个 map.obj(里面的 SpawnUser 部件标记站立点),可再带 map.png 截图和 map.json 元数据。';

  @override
  String get aboutTitle => '关于';

  @override
  String get aboutSubtitle => '一个系统,多台头显,每一步都公开。';

  @override
  String get aboutWhatTitle => '这是什么';

  @override
  String get aboutWhatBody =>
      'Hibiscus 是为 VR 一体头显打造的操作系统,基于 LineageOS 17.1——通过 phh GSI 实现的 Android 10。目标是那些卡在旧版 Android、VR 栈高度专有的头显,一台一台移植。';

  @override
  String get aboutHowTitle => '怎么做到的';

  @override
  String get aboutHowBody =>
      'GSI 加 overlay 再加头显自己的栈。vendor 分区一概不动,所以每个兼容性问题——vold 死锁、声卡缺失、ABI 断裂——都在 system 侧用 init 规则和 shim 库解决。';

  @override
  String get aboutGroupTitle => '单仓库';

  @override
  String get aboutGroupBody =>
      '所有东西都在 GitLab 上一个仓库里,一个部件一个目录:设备树、shim、研究笔记、转储、VR 桌面,还有本站。';

  @override
  String get aboutLicenseTitle => '许可证';

  @override
  String get aboutLicenseBody =>
      '所有原创内容都是 AGPL-3.0。转储仓库里的 vendor 专有二进制归各自所有者所有,绝不二次分发。本站内嵌的 Inter 字体遵循 SIL OFL 1.1。';

  @override
  String get aboutRepoCta => 'GitLab 仓库';

  @override
  String get aboutDocsCta => '项目文档';

  @override
  String get aboutNotesCta => '研究笔记';

  @override
  String get footerSite => '站点';

  @override
  String get footerProject => '项目';

  @override
  String footerCopyright(int year) {
    return 'Copyright $year HibiscusXR';
  }

  @override
  String get footerLicense => '以 AGPL-3.0 许可发布';

  @override
  String get footerBuiltWith => '用 Flutter 构建';

  @override
  String get notFoundTitle => '页面不存在';

  @override
  String get notFoundBody => '你要找的页面不存在。';

  @override
  String get notFoundCta => '回到 HibiscusXR';

  @override
  String get navCte => 'HCTE';

  @override
  String get navHbsup => 'HBSUP';

  @override
  String get cteTitle => 'HCTE';

  @override
  String get cteSubtitle => 'Hibiscus 头显调试环境 - 运行 Hibiscus 头显的桌面伴侣工具。';

  @override
  String get cteWhatTitle => '这是什么';

  @override
  String get cteWhatBody =>
      'HCTE 是一个桌面应用(Linux、Windows、macOS),与运行 Hibiscus 的头显配对,把整个开发界面放进一个窗口。USB 直插或 Wi-Fi 连接都行 - 电脑上有 adb 就够,开启设备上的 CTE 服务后连 adb 都不用。';

  @override
  String get cteFeatOverviewTitle => '设备与手柄概览';

  @override
  String get cteFeatOverviewBody =>
      '头显是什么 - 型号、驱动目标、安卓和 Hibiscus 版本、追踪模式 - 以及配对了哪些手柄、电量和追踪状态。';

  @override
  String get cteFeatDisplayTitle => '屏幕镜像';

  @override
  String get cteFeatDisplayBody => '实时查看头显画面,每秒几帧,不用戴头显就能看启动过程和调试面板。';

  @override
  String get cteFeatInstallTitle => '安装 APK';

  @override
  String get cteFeatInstallBody => '一键把 APK 推到设备上,走 adb 或 socket 通道都行。';

  @override
  String get cteFeatTrackingTitle => '实时 6DoF/3DoF 追踪';

  @override
  String get cteFeatTrackingBody => '直接从运行时拿到的头部位姿流:朝向、位置、采样率、俯视轨迹,还有每只手柄的位姿。';

  @override
  String get cteFeatDebugTitle => '调试面板';

  @override
  String get cteFeatDebugBody => '完整的 getprop 表和滚动的 logcat,排查问题的时候用。';

  @override
  String get cteConnectTitle => '怎么连接';

  @override
  String get cteConnectBody =>
      '两条通道:adb(USB 或无线 adb)提供完整功能;设备上的 cted 服务在 7340 端口监听,不想用 adb 就走它。cted 默认关闭,需要时才开 - setprop persist.hibiscus.cted 1 - 和无线 adb 一样是可选开启的。';

  @override
  String get cteGetTitle => '获取方式';

  @override
  String get cteGetBody =>
      '每个 cte-v* 版本都会在 monorepo 上发布桌面构建包 - Linux、Windows、macOS 的 tar 和 zip,并同步到 GitLab release 页面,链接不会过期。';

  @override
  String get cteReleasesCta => 'CTE 发布页';

  @override
  String get cteSourceCta => '源码';

  @override
  String get cteShotsTitle => '界面长这样';

  @override
  String get cteShotConnect => '连接页:上面是扫到的 USB/adb 设备,下面走无线连接。';

  @override
  String get cteShotOverview => '连上之后:头显信息、手柄电量与追踪状态、连接状态都在。';

  @override
  String get cteShotDisplay => '显示页从头显实时拉帧镜像画面。';

  @override
  String get cteShotInstall => '一次完成的 APK 推送,下面是安装日志。';

  @override
  String get cteShotTracking => '头部位姿、俯视轨迹和原始位姿日志。';

  @override
  String get cteShotDebug => '调试页:左边是可过滤的 getprop 表,右边是 logcat。';

  @override
  String appDownloadsTitle(String app) {
    return '$app 下载';
  }

  @override
  String appDownloadsSubtitle(String app) {
    return '$app 的桌面构建包,从发布流水线实时拉取。';
  }

  @override
  String get hbsupWindowsTitle => 'Windows 不受支持';

  @override
  String get hbsupWindowsBody =>
      'HBSUP 提供 Windows 构建,但 Windows 不是受支持的主机系统——应用打开时也会提示,在那上面出问题自己承担。';

  @override
  String get hbsupShotsTitle => '界面长这样';

  @override
  String get hbsupShotConnect => 'adb 认到头显之后的连接页。';

  @override
  String get hbsupShotConnectUnsupported => '在 Windows 上打开,同一个页面会带上「不受支持」横幅。';

  @override
  String get hbsupShotBackupReady => '头显已连接、目录已选好、分区大小已列出,可以开始转储。';

  @override
  String get hbsupShotBackupBlocked => '目标目录空间不够时,备份会被拦住。';

  @override
  String get hbsupShotBackupDone => '一次完成的转储,下面是日志。';

  @override
  String get navFlashdocs => '刷机指南';

  @override
  String get flashdocsTitle => '刷机文档';

  @override
  String get flashdocsSidebarHome => '入门';

  @override
  String get flashdocsDevicesTitle => '设备';

  @override
  String get flashdocsPickDeviceTitle => '你用的是哪个头显?';

  @override
  String get flashdocsPickDeviceBody => '选择你的头显查看对应的刷机指南。后续移植的设备也会出现在这里。';

  @override
  String get flashdocsPickOsTitle => '你的电脑跑的是什么系统?';

  @override
  String get flashdocsPickOsBody => '刷机指南按电脑的系统区分,目前只有 Linux 版。';

  @override
  String get flashdocsOsLinux => 'Linux';

  @override
  String get flashdocsOsCardBody => '用发行版仓库里的 adb 和 fastboot。';

  @override
  String get flashdocsNeo2Title => '在 Pico Neo 2 上刷入 Hibiscus';

  @override
  String get flashdocsNeo2Intro => '在 Linux 主机上的完整流程:root 原厂系统、备份、然后刷系统镜像。';

  @override
  String get flashdocsNeo2WarnTitle => '先读这个';

  @override
  String get flashdocsNeo2WarnBody =>
      '下面的 boot 和 system 镜像是唯一能安全刷写的分区。往 bootloader 链里写低于防回滚熔丝版本的任何镜像,都会在 sdm845 上造成永久硬砖。';

  @override
  String get flashdocsNeo2ReqTitle => '开始之前';

  @override
  String get flashdocsNeo2Req1 => '一台装了 adb 和 fastboot 的 Linux 电脑';

  @override
  String get flashdocsNeo2Req2 => '一台 Pico Neo 2(A7B10),Eye 和非 Eye 版本都可以';

  @override
  String get flashdocsNeo2Req3 => '一根能传数据的 USB 线,不是只能充电的那种';

  @override
  String get flashdocsNeo2RootTitle => '1. Root 头显';

  @override
  String get flashdocsNeo2RootBody =>
      '在原厂系统上 root,备份步骤才能拿到 adb root。下载 Magisk 修补过的 boot 镜像,然后执行:';

  @override
  String get flashdocsNeo2RootDownload => '下载修补过的 boot 镜像';

  @override
  String get flashdocsNeo2RootCmd1 => 'adb reboot bootloader';

  @override
  String get flashdocsNeo2RootCmd2 => 'fastboot oem pico unlock';

  @override
  String get flashdocsNeo2RootCmd3 =>
      'fastboot flash boot magisk_patched_pico_neo_2_boot.img';

  @override
  String get flashdocsNeo2RootCmd4 => 'fastboot reboot';

  @override
  String get flashdocsNeo2RootNote =>
      'oem pico unlock 每个 fastboot 会话只需要执行一次。如果卡住,把头显关机、重新进 fastboot 再跑一遍。';

  @override
  String get flashdocsNeo2BackupTitle => '2. 备份原厂系统';

  @override
  String get flashdocsNeo2BackupBody =>
      '拿到 root 的 adb 之后,刷任何东西之前先把每个分区转储出来——HBSUP 一条龙搞定,想手动的话 tools 仓库里有备份脚本。把转储存到安全的地方:出了问题它是回原厂的唯一退路。';

  @override
  String get flashdocsNeo2FlashTitle => '3. 刷入 Hibiscus';

  @override
  String get flashdocsNeo2FlashBody => '下载好完整系统镜像后,让头显重新进 fastboot,执行:';

  @override
  String get flashdocsNeo2FlashCmd1 => 'adb reboot bootloader';

  @override
  String get flashdocsNeo2FlashCmd2 => 'fastboot oem pico unlock';

  @override
  String get flashdocsNeo2FlashCmd3 =>
      'fastboot -S 128M flash system system-hibiscus-full.img';

  @override
  String get flashdocsNeo2FlashCmd4 => 'fastboot reboot';

  @override
  String get flashdocsNeo2FlashNote => '-S 128M 分块是硬性要求:更大的块会在刷到一半时弄断 USB 连接。';

  @override
  String get flashdocsNeo2DoneTitle => '4. 完成';

  @override
  String get flashdocsNeo2DoneBody => '头显重启后进入 Hibiscus。第一次开机会花几分钟,等 Pico 栈就位。';
}
