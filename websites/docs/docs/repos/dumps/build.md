# build

[HibiscusXR/system/build](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/system/build)

## What this is

Android signing material generated locally for this project: the platform test key pair (platform.pk8 private key, platform.x509.pem certificate) used to sign patched system apps, and debug.keystore for regular apks.

## How to remake this dump

Regenerate the platform key with AOSP `development/tools/make_key platform <subject>` and the debug keystore with `keytool -genkeypair -v -keystore debug.keystore -alias androiddebugkey -storepass android -keypass android`.

## Files

3 files / 共 3 个文件 / всего файлов: 3

```
   2.6 KiB  debug.keystore
   1.2 KiB  keys/platform.pk8
   1.6 KiB  keys/platform.x509.pem
```

## License

Everything in this folder was generated locally by us. The private keys are secrets and are never committed. The AGPL v3 in this repository applies to this README and our own original files.
