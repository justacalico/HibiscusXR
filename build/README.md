# build

English | [中文](#中文) | [Русский](#русский)

## What this is

Android signing material generated locally for this project: the platform test key pair (platform.pk8 private key, platform.x509.pem certificate) used to sign patched system apps, and debug.keystore for regular apks.

## How to remake this dump

Regenerate the platform key with AOSP `development/tools/make_key platform <subject>` and the debug keystore with `keytool -genkeypair -v -keystore debug.keystore -alias androiddebugkey -storepass android -keypass android`.

## License

Everything in this folder was generated locally by us. The private keys are secrets and are never committed. The AGPL v3 in this repository applies to this README and our own original files.

## 中文

### 这是什么

本项目在本地生成的 Android 签名材料：用于给修改过的系统应用签名的 platform 测试密钥对（platform.pk8 私钥、platform.x509.pem 证书），以及用于普通 apk 的 debug.keystore。

### 如何重新制作这些转储

重新生成：platform 密钥用 AOSP 的 `development/tools/make_key platform <subject>`；debug keystore 用 `keytool -genkeypair -v -keystore debug.keystore -alias androiddebugkey -storepass android -keypass android`。

### 许可证说明

本目录中的所有文件都是我们在本地生成的。私钥属于保密信息，绝不会被提交。本仓库的 AGPL v3 适用于本 README 以及我们原创的文件。

## Русский

### Что это

Локально сгенерированный материал для подписи Android: тестовая пара ключей platform (platform.pk8 — приватный ключ, platform.x509.pem — сертификат) для подписи пропатченных системных приложений, и debug.keystore для обычных apk.

### Как воспроизвести дамп

Пересоздать: ключ platform — через `development/tools/make_key platform <subject>` из AOSP; debug keystore — `keytool -genkeypair -v -keystore debug.keystore -alias androiddebugkey -storepass android -keypass android`.

### Лицензия

Все файлы в этой папке сгенерированы нами локально. Приватные ключи являются секретами и никогда не коммитятся. AGPL v3 в этом репозитории распространяется на этот README и наши собственные файлы.

## Files in this folder / 本目录文件 / Файлы в этой папке

3 files / 共 3 个文件 / всего файлов: 3

```
   2.6 KiB  debug.keystore
   1.2 KiB  keys/platform.pk8
   1.6 KiB  keys/platform.x509.pem
```
