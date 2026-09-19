# build

English | [中文](#中文) | [Русский](#русский)

## What this is

Android signing material generated locally for this project: the real PN2 platform key pair (platform.pk8 private key, platform.x509.pem certificate) used to sign everything in the system image - framework, stock apps, the Pico stack and our shell apps - plus pn2-platform-release.jks, the same keypair as a password-protected keystore, and debug.keystore for regular apks.

## How to remake this dump

Generate the keypair and self-signed cert, then convert to both formats:

```
openssl req -newkey rsa:4096 -nodes -keyout key.pem -x509 -days 10950 -sha256 \
    -subj "/C=US/ST=California/L=San Francisco/O=Neosalsa/OU=PN2/CN=PN2/emailAddress=pn2@neosalsa.local" \
    -out keys/platform.x509.pem
openssl pkcs8 -topk8 -nocrypt -in key.pem -outform DER -out keys/platform.pk8
openssl pkcs12 -export -inkey key.pem -in keys/platform.x509.pem -name pn2-platform \
    -passout pass:<storepass> -out tmp.p12
keytool -importkeystore -srckeystore tmp.p12 -srcstoretype PKCS12 -srcalias pn2-platform \
    -destkeystore keys/pn2-platform-release.jks -deststoretype JKS \
    -deststorepass <storepass> -destkeypass <keypass> -destalias pn2-platform
```

The keystore passwords live in keys/credentials.txt (never committed) and the
backup copy on the desktop at pn2-platform-signing/.

## License

Everything in this folder was generated locally by us. The private keys are secrets and are never committed. The AGPL v3 in this repository applies to this README and our own original files.

## 中文

### 这是什么

本项目在本地生成的 Android 签名材料：真正的 PN2 platform 密钥对（platform.pk8 私钥、platform.x509.pem 证书），用于给系统镜像里的所有东西签名——framework、系统应用、Pico 栈和我们的 shell 应用；另有 pn2-platform-release.jks（同一密钥对的带密码 keystore），以及给普通 apk 用的 debug.keystore。

### 如何重新制作这些转储

生成密钥对和自签名证书，再转成两种格式（见上方英文命令）。

keystore 密码保存在 keys/credentials.txt（绝不提交），备份在桌面的 pn2-platform-signing/ 目录。

### 许可证说明

本目录中的所有文件都是我们在本地生成的。私钥属于保密信息，绝不会被提交。本仓库的 AGPL v3 适用于本 README 以及我们原创的文件。

## Русский

### Что это

Локально сгенерированный материал для подписи Android: настоящая пара ключей PN2 platform (platform.pk8 — приватный ключ, platform.x509.pem — сертификат) для подписи всего в образе системы - framework, системных приложений, стека Pico и наших shell-приложений, плюс pn2-platform-release.jks (та же пара ключей в виде keystore с паролем) и debug.keystore для обычных apk.

### Как воспроизвести дамп

Сгенерировать пару ключей и самоподписанный сертификат, затем конвертировать в оба формата (команды выше на английском).

Пароли keystore хранятся в keys/credentials.txt (никогда не коммитится), резервная копия на рабочем столе в pn2-platform-signing/.

### Лицензия

Все файлы в этой папке сгенерированы нами локально. Приватные ключи являются секретами и никогда не коммитятся. AGPL v3 в этом репозитории распространяется на этот README и наши собственные файлы.

## Files in this folder / 本目录文件 / Файлы в этой папке

5 files / 共 5 个文件 / всего файлов: 5

```
   2.6 KiB  debug.keystore
   0.8 KiB  keys/credentials.txt
   4.0 KiB  keys/pn2-platform-release.jks
   2.3 KiB  keys/platform.pk8
   2.1 KiB  keys/platform.x509.pem
```
