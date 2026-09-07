# gsi

English | [中文](#中文) | [Русский](#русский)

## What this is

The base system image for the port: the phh LineageOS 17.1 (Android 10) GSI for treble_arm64_avS (arm64, A-only), plus gsi_raw.img (the simg2img'd raw ext4 copy) and boot_adb.img (a boot image with adb enabled).

## How to remake this dump

Download the phh LineageOS 17.1 `treble_arm64_avS` .xz build, `unxz` it, and run `simg2img` on it to get gsi_raw.img. boot_adb.img is the stock boot image repacked with adb/debuggable patches.

## License

These files come from a third-party project and remain under that project's own license - see the source for terms. The AGPL v3 in this repository applies only to this README and our own original files.

## 中文

### 这是什么

本移植使用的基础系统镜像：phh 的 LineageOS 17.1（Android 10）GSI，型号 treble_arm64_avS（arm64，A-only），外加 gsi_raw.img（simg2img 转换后的原始 ext4 镜像）和 boot_adb.img（开启了 adb 的 boot 镜像）。

### 如何重新制作这些转储

重新制作：下载 phh 的 LineageOS 17.1 `treble_arm64_avS` .xz 镜像，`unxz` 解压后用 `simg2img` 转换得到 gsi_raw.img。boot_adb.img 是将官方 boot 镜像重打包、加入 adb/debuggable 补丁得到的。

### 许可证说明

这些文件来自第三方项目，仍受其原项目许可证约束，具体条款见原始来源。本仓库的 AGPL v3 仅适用于本 README 以及我们原创的文件。

## Русский

### Что это

Базовый системный образ для порта: phh GSI LineageOS 17.1 (Android 10) для treble_arm64_avS (arm64, A-only), плюс gsi_raw.img (сырая ext4-копия после simg2img) и boot_adb.img (boot-образ с включённым adb).

### Как воспроизвести дамп

Воспроизведение: скачать phh-сборку LineageOS 17.1 `treble_arm64_avS` в .xz, распаковать `unxz` и прогнать через `simg2img` для получения gsi_raw.img. boot_adb.img — стоковый boot-образ, перепакованный с патчами adb/debuggable.

### Лицензия

Эти файлы взяты из стороннего проекта и остаются под его собственной лицензией - условия смотрите в источнике. AGPL v3 в этом репозитории распространяется только на этот README и наши собственные файлы.

## Files in this folder / 本目录文件 / Файлы в этой папке

4 files / 共 4 个文件 / всего файлов: 4

```
  15.0 MiB  boot_adb.img
   1.9 GiB  gsi_raw.img
   1.8 GiB  lineage-17.1-20210808-UNOFFICIAL-treble_arm64_avS.img
 567.5 MiB  lineage-17.1-20210808-UNOFFICIAL-treble_arm64_avS.img.xz
```
