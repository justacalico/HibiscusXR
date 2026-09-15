// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for Chinese (`zh`).
class AppLocalizationsZh extends AppLocalizations {
  AppLocalizationsZh([String locale = 'zh']) : super(locale);

  @override
  String get appTitle => 'PN2Lineage';

  @override
  String get navStatus => '现状';

  @override
  String get navRepos => '仓库';

  @override
  String get navScreenshots => '截图';

  @override
  String get navDownload => '下载';

  @override
  String get navAbout => '关于';

  @override
  String get navFaq => '常见问题';

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
  String get heroEyebrow => 'Pico Neo 2 · LineageOS 17.1';

  @override
  String get heroTitle => '在 VR 里跑 Android 10。';

  @override
  String get heroSubtitle =>
      '把 LineageOS 17.1 完整移植到 Pico Neo 2 头显。原厂 vendor 分区一字节不动,Pico 的 VR 栈从你自已的设备里恢复回来,每一步研究都公开。';

  @override
  String get heroPrimary => '查看现状';

  @override
  String get heroSecondary => '阅读文档';

  @override
  String get heroShotCaption => 'vrhome 里的 library 应用窗口。';

  @override
  String get statSoc => '骁龙 845';

  @override
  String get statSocLabel => 'Adreno 630 GPU';

  @override
  String get statPanel => '3840×2160 @ 72 Hz';

  @override
  String get statPanelLabel => 'JDI 4K 屏幕';

  @override
  String get statRepos => '46';

  @override
  String get statReposLabel => '个仓库';

  @override
  String get statNotes => '约 300';

  @override
  String get statNotesLabel => '篇研究笔记';

  @override
  String get homeShellEyebrow => '桌面环境';

  @override
  String get homeShellTitle => '开源的 VR 桌面,已经能跑了。';

  @override
  String get homeShellBody =>
      'vrhome 用我们自己的 NativeActivity 替换了原厂 Pico 桌面:2D 应用变成悬浮窗口,VR 应用照常全屏启动,library 网格也只是一个普通应用。';

  @override
  String get homeShellCta => '看截图';

  @override
  String get shotCollectionCaption => '集合与分组,带数量。';

  @override
  String get shotMenuCaption => '长按弹出的磁贴菜单。';

  @override
  String get homeWayOverlayTitle => 'GSI 加 overlay';

  @override
  String get homeWayOverlayBody =>
      '系统主体是 LineageOS 17.1 GSI,我们的修复全部放在一个小的 overlay 里。vendor 分区保持和原厂逐字节一致,问题都在 system 侧解决。';

  @override
  String get homeWayReposTitle => '一个仓库一个部件';

  @override
  String get homeWayReposBody =>
      '设备树、shim 源码、转储仓库、暂存目录——46 个仓库挂在同一个组下,每个都小到能一次读完。';

  @override
  String get homeWayNotesTitle => '研究过程全部公开';

  @override
  String get homeWayNotesBody =>
      '每条死路都记了档。notes 仓库里约有 300 篇编号笔记,从第一次开机到最后一个黑屏帧,全是原始记录。';

  @override
  String get homeWayCta => '浏览所有仓库';

  @override
  String get homeStatusEyebrow => '进展';

  @override
  String get homeStatusTitle => '能开机,桌面能跑,还剩一个 bug。';

  @override
  String get homeStatusBody =>
      '移植版可以启动,有声音,头部旋转实时生效,VRShell 驱动着真正的 Pico 合成器。VR 画面仍然是黑的——离显示出画面只差一个 tracking-state bug。';

  @override
  String get homeStatusCta => '完整现状';

  @override
  String get homeOpenTitle => '彻底开源。';

  @override
  String get homeOpenBody =>
      '我们写的所有东西都是 AGPL-3.0。Pico 的二进制归 Pico 所有——从你自己的设备提取,写进清单,绝不二次分发。';

  @override
  String get homeOpenSource => '浏览仓库组';

  @override
  String get homeDownloadTitle => '自己动手刷。';

  @override
  String get homeDownloadBody =>
      '完整的系统镜像——GSI、我们的修复和整套 Pico 栈——就在 out 仓库里。一条 fastboot 命令就能刷上头显。';

  @override
  String get homeDownloadCta => '获取镜像';

  @override
  String get statusTitle => '现状';

  @override
  String get statusSubtitle => '实话实说的清单,跟着移植进度更新。';

  @override
  String get statusWorksTitle => '已正常';

  @override
  String get statusWorks1 => '干净启动:音频、3840×2160 横屏、休眠表现与原厂一致';

  @override
  String get statusWorks2 => 'pvrservice 实时发布头部旋转,四元数合法';

  @override
  String get statusWorks3 => 'VRShell 稳定启动,驱动真正的 Pico 合成器';

  @override
  String get statusWorks4 => 'airservice、virtual_input、pn2_qvrd 全部正常启动';

  @override
  String get statusWorks5 => '透视校准应用已安装、平台签名、可启动';

  @override
  String get statusWorks6 => '2D Pico 应用可渲染:VRUserCenter、Pico Store';

  @override
  String get statusWorks7 => '可选无线 adb:persist.pn2.adbwifi';

  @override
  String get statusBrokenTitle => '还不行';

  @override
  String get statusBroken1 => 'VR 画面仍是黑的——唯一的拦路虎';

  @override
  String get statusBroken2 => '6DoF / SLAM——qvrservice 从不打开追踪摄像头';

  @override
  String get statusBroken3 => '透视画面——libgui 接口在 Android 10 里被删了';

  @override
  String get statusBroken4 => 'CVService 手柄——因 wifi 广播崩溃,目前已禁用';

  @override
  String get statusBroken5 => 'Provision 初始化向导——语言选择器里崩溃';

  @override
  String get statusBlockerTitle => '离出画面只差一个 bug。';

  @override
  String get statusBlockerBody =>
      'pvrservice 给出的旋转数据是好的,但应用内部 SDK 报回来的 trackingstate 是 0x0,0x0。提交的 pose 过不了合成器的单位四元数校验,每一帧都被丢弃。数据在服务端存在,到客户端就变成了「无追踪」。';

  @override
  String get statusBlockerCta => '看内部原理文档';

  @override
  String get reposTitle => '仓库';

  @override
  String get reposSubtitle => 'neosalsa 组下,一个仓库一个部件。';

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
  String get reposDumpsBody =>
      '从硬件里拉出的二进制和流水线中间产物,仅供研究。Pico 专有文件归 Pico 所有,绝不二次分发。';

  @override
  String reposCount(int count) {
    return '$count 个仓库';
  }

  @override
  String get repoVrhome => '开源 VR 桌面:2D 应用悬浮成窗,Pico VR 应用全屏运行。';

  @override
  String get repoLibrary => 'vrhome 里的 Flutter 应用网格——搜索、置顶、分组、启动。';

  @override
  String get repoVrdemo => '最小原生 VR 测试应用(pn2vr),用于合成器通路调试。';

  @override
  String get repoDocs => 'MkDocs 文档站——指南、内部原理、仓库地图。';

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
  String get screenshotsTitle => '截图';

  @override
  String get screenshotsSubtitle => 'vrhome 和它的 library 窗口,跑在真机上。';

  @override
  String get shotGridCaption => 'vrhome 里的应用网格。';

  @override
  String get downloadTitle => '下载';

  @override
  String get downloadSubtitle => '自己动手刷——但先读完警告。';

  @override
  String get downloadWarnTitle => '刷机风险';

  @override
  String get downloadWarnBody =>
      '只刷 system 分区。往 bootloader 链里写低于防回滚熔丝版本的任何镜像,都会在 sdm845 上造成永久硬砖。';

  @override
  String get downloadImageTitle => '完整系统镜像';

  @override
  String get downloadImageBody =>
      'system-pn2-full.img——3.6 GB,ext4,fsck 检查干净。LineageOS GSI 加我们的修复加整套 Pico 栈,刷上即用。';

  @override
  String get downloadImageCta => '打开 out 仓库';

  @override
  String get downloadStepsTitle => '刷机步骤';

  @override
  String get downloadStep1 => 'adb reboot bootloader';

  @override
  String get downloadStep2 => 'fastboot oem pico unlock';

  @override
  String get downloadStep3 =>
      'fastboot -S 128M flash system system-pn2-full.img';

  @override
  String get downloadStep4 => 'fastboot reboot';

  @override
  String get downloadStepsNote =>
      '-S 128M 分块是硬性要求:更大的块会在刷到一半时弄断 USB 连接。tools 仓库里有脚本帮你处理这两个坑。';

  @override
  String get downloadReqTitle => '你需要准备';

  @override
  String get downloadReq1 => '一台 Pico Neo 2(A7B10)——Eye 和非 Eye 版本都可以';

  @override
  String get downloadReq2 => '已 root 的原厂固件和可解锁的 bootloader';

  @override
  String get downloadReq3 => '一台装了 adb 和 fastboot 的 Linux 主机';

  @override
  String get downloadSoftwareTitle => '上面的软件';

  @override
  String get downloadSoftwareBody =>
      '镜像自带 vrhome 和 library 应用。想改它们就克隆对应仓库——vrhome 用一个普通 Makefile 构建,library 用 Flutter。';

  @override
  String get downloadVrhomeCta => 'vrhome 仓库';

  @override
  String get downloadLibraryCta => 'library 仓库';

  @override
  String get faqTitle => '常见问题';

  @override
  String get faqSubtitle => '短回答,不吹牛。';

  @override
  String get faqQ1 => '这个移植真的能用吗?';

  @override
  String get faqA1 =>
      '能开机,VRShell 能跑,头部追踪实时生效。唯一的拦路虎:VR 画面还是黑的,因为应用侧的 tracking state 传回来是零。现状页维护着最新清单。';

  @override
  String get faqQ2 => '刷机安全吗?';

  @override
  String get faqA2 =>
      '有真实风险。只刷 system 分区——写入低于防回滚熔丝允许版本的 bootloader 镜像会让 sdm845 永久硬砖。先读文档里的刷机指南。';

  @override
  String get faqQ3 => '支持哪个头显?';

  @override
  String get faqA3 =>
      'Pico Neo 2(A7B10 / PICOA7B10)。Eye 和非 Eye 两个版本都行,眼动追踪是额外的活。';

  @override
  String get faqQ4 => 'Pico 的专有文件从哪来?';

  @override
  String get faqA4 =>
      '来自你自己的设备或它的原厂 OTA。overlay 仓库里有一份清单,列出每个所需 blob 的路径、大小、sha256 前缀和用途,源码仓库里一个都不提交。';

  @override
  String get faqQ5 => 'vrhome 是什么?';

  @override
  String get faqA5 =>
      '我们自己写的 VR 桌面环境。原厂 VRShell 依赖闭源的 Pico 合成器;vrhome 是一个 NativeActivity,把 2D 应用放到悬浮面板上,真正的 VR 应用照常全屏启动。';

  @override
  String get faqQ6 => '用什么许可证?';

  @override
  String get faqA6 =>
      '我们写的所有东西都是 AGPL-3.0。转储出来的 Pico 和 vendor 二进制归各自所有者所有,放在转储仓库里仅供研究。';

  @override
  String get aboutTitle => '关于';

  @override
  String get aboutSubtitle => '一次移植,一台头显,每一步都公开。';

  @override
  String get aboutWhatTitle => '这是什么';

  @override
  String get aboutWhatBody =>
      'PN2Lineage 把 LineageOS 17.1——通过 phh GSI 实现的 Android 10——跑在了 Pico Neo 2 上,一台出厂只有 Android 8.1 和整套专有 VR 栈的头显。';

  @override
  String get aboutHowTitle => '怎么做到的';

  @override
  String get aboutHowBody =>
      'GSI 加 overlay 再加你自己的 Pico 栈。vendor 分区一概不动,所以每个兼容性问题——vold 死锁、声卡缺失、ABI 断裂——都在 system 侧用 init 规则和 shim 库解决。';

  @override
  String get aboutGroupTitle => '这个组';

  @override
  String get aboutGroupBody =>
      'neosalsa 下面 46 个仓库,一个部件一个:设备树、shim、研究笔记、转储仓库、VR 桌面,还有本站。';

  @override
  String get aboutLicenseTitle => '许可证';

  @override
  String get aboutLicenseBody =>
      '所有原创内容都是 AGPL-3.0。转储仓库里的 Pico 和 vendor 专有二进制归各自所有者所有,绝不二次分发。本站内嵌的 Inter 字体遵循 SIL OFL 1.1。';

  @override
  String get aboutRepoCta => 'GitLab 组';

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
    return 'Copyright $year neosalsa';
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
  String get notFoundCta => '回到 PN2Lineage';
}
