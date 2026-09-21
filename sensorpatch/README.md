# sensorpatch

English | [中文](#中文) | [Русский](#русский)

## What this is

Working area for the two libsensorservice patches: the stock lib plus .orig/.patched/.p3 variants, the on-device test builds (ondevice.so, cur.so) and the libgui.so it links against. The patches stop Android 10's sensor asserts on Pico's event types 57/58/126/127 and the dynamic-sensor connect path.

## How to remake this dump

Binary-patch the stock libsensorservice.so as documented in the notes, and rebuild; the patch scripts live in the tools repo.

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.

## 中文

### 这是什么

两个 libsensorservice 补丁的工作目录：官方库及 .orig/.patched/.p3 各版本、设备上的测试构建（ondevice.so、cur.so）以及它链接的 libgui.so。补丁用于消除 Android 10 对 Pico 事件类型 57/58/126/127 的传感器断言以及动态传感器连接路径的问题。

### 如何重新制作这些转储

重新制作：按 notes 中的记录对官方 libsensorservice.so 做二进制补丁并重新构建；补丁脚本在 tools 仓库中。

### 许可证说明

本仓库中的 AGPL v3 许可证仅适用于本 README 以及我们原创的文件（脚本、配置、补丁、笔记）。下面列出的二进制文件属于 Pico（字节跳动）及/或相关厂商（高通、Tobii、NDI 等）的专有固件或软件，不是我们的作品，不受 AGPL 授权，不会在本仓库中分发（已被 gitignore 排除），也不得再分发。如需获取，请从你合法拥有的设备或固件中自行提取。

## Русский

### Что это

Рабочая область для двух патчей libsensorservice: стоковая библиотека и варианты .orig/.patched/.p3, тестовые сборки на устройстве (ondevice.so, cur.so) и libgui.so, с которой она линкуется. Патчи убирают падения сенсоров Android 10 на событиях Pico типов 57/58/126/127 и путь подключения динамических сенсоров.

### Как воспроизвести дамп

Воспроизведение: бинарно пропатчить стоковую libsensorservice.so согласно notes и пересобрать; скрипты патчей — в репозитории tools.

### Лицензия

Лицензия AGPL v3 в этом репозитории распространяется ТОЛЬКО на этот README и другие наши собственные работы (скрипты, конфиги, патчи, заметки). Перечисленные ниже бинарные файлы являются проприетарной прошивкой или ПО компании Pico (ByteDance) и/или соответствующих вендоров (Qualcomm, Tobii, NDI и др.). Они не являются нашей работой, не покрываются AGPL, не распространяются в этом репозитории (исключены через gitignore) и не подлежат повторному распространению. Чтобы получить их, извлеките их из оборудования или прошивки, которыми вы легально владеете.

## Files in this folder / 本目录文件 / Файлы в этой папке

9 files / 共 9 个文件 / всего файлов: 9

```
 221.9 KiB  cur.so
 942.0 KiB  libgui.so
 221.9 KiB  libsensorservice.so
 221.9 KiB  libsensorservice.so.orig
 221.9 KiB  libsensorservice.so.p3
 221.9 KiB  libsensorservice.so.patched
 221.9 KiB  libsensorservice.so.patched2
 221.9 KiB  ondev_now.so
 221.9 KiB  ondevice.so
```
