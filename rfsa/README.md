# rfsa

English | [中文](#中文) | [Русский](#русский)

## What this is

Hexagon DSP skel libraries and rfsa filesystem pieces: libdspCV_skel, libdsp_streamer_qvrcam_receiver, libeye_tracking_dsp_sample_skel, libfastcv(a)dsp(_skel), libqvr_cam_dsp_driver_skel, libqvr_dsp_driver_skel, libapps_mem_heap and friends. These run on the DSP side of the camera/tracking pipeline.

## How to remake this dump

Pulled from the dsp/rfsa paths on the stock device - see tools/292_find_skel.sh and the cdsp scripts.

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.

## 中文

### 这是什么

Hexagon DSP 的 skel 库和 rfsa 文件系统组件：libdspCV_skel、libdsp_streamer_qvrcam_receiver、libeye_tracking_dsp_sample_skel、libfastcv(a)dsp(_skel)、libqvr_cam_dsp_driver_skel、libqvr_dsp_driver_skel、libapps_mem_heap 等。它们运行在相机/追踪流水线的 DSP 侧。

### 如何重新制作这些转储

重新获取：从原版设备的 dsp/rfsa 路径提取，见 tools/292_find_skel.sh 和 cdsp 相关脚本。

### 许可证说明

本仓库中的 AGPL v3 许可证仅适用于本 README 以及我们原创的文件（脚本、配置、补丁、笔记）。下面列出的二进制文件属于 Pico（字节跳动）及/或相关厂商（高通、Tobii、NDI 等）的专有固件或软件，不是我们的作品，不受 AGPL 授权，不会在本仓库中分发（已被 gitignore 排除），也不得再分发。如需获取，请从你合法拥有的设备或固件中自行提取。

## Русский

### Что это

Skel-библиотеки Hexagon DSP и части файловой системы rfsa: libdspCV_skel, libdsp_streamer_qvrcam_receiver, libeye_tracking_dsp_sample_skel, libfastcv(a)dsp(_skel), libqvr_cam_dsp_driver_skel, libqvr_dsp_driver_skel, libapps_mem_heap и др. Работают на стороне DSP в конвейере камер/трекинга.

### Как воспроизвести дамп

Воспроизведение: извлекаются из dsp/rfsa-путей стокового устройства — см. tools/292_find_skel.sh и скрипты cdsp.

### Лицензия

Лицензия AGPL v3 в этом репозитории распространяется ТОЛЬКО на этот README и другие наши собственные работы (скрипты, конфиги, патчи, заметки). Перечисленные ниже бинарные файлы являются проприетарной прошивкой или ПО компании Pico (ByteDance) и/или соответствующих вендоров (Qualcomm, Tobii, NDI и др.). Они не являются нашей работой, не покрываются AGPL, не распространяются в этом репозитории (исключены через gitignore) и не подлежат повторному распространению. Чтобы получить их, извлеките их из оборудования или прошивки, которыми вы легально владеете.

## Files in this folder / 本目录文件 / Файлы в этой папке

15 files / 共 15 个文件 / всего файлов: 15

```
  15.6 MiB  libVIOMapping_6dof_skel.so
  53.0 KiB  libapps_mem_heap.so
  32.6 KiB  libdspCV_skel.so
   7.4 KiB  libdsp_streamer_qvrcam_receiver.so
  32.8 KiB  libeye_tracking_dsp_sample_skel.so
   1.2 MiB  libfastcvadsp.so
 532.3 KiB  libfastcvdsp_skel.so
  90.2 KiB  libqvr_cam_dsp_driver_skel.so
  96.2 KiB  libqvr_dsp_driver_skel.so
  33.5 KiB  libqvr_mapper_skel.so
  93.5 KiB  libscveBlobDescriptor_skel.so
 326.5 KiB  libscveT2T_skel.so
  28.9 KiB  libsns_low_lat_stream_skel.so
   1.1 MiB  libtobii_eyecore_skel.so
  20.3 MiB  libtracker_6dof_skel.so
```
