# rfsa

[gitlab.com/neosalsa/rfsa](https://gitlab.com/neosalsa/rfsa)

## What this is

Hexagon DSP skel libraries and rfsa filesystem pieces: libdspCV_skel, libdsp_streamer_qvrcam_receiver, libeye_tracking_dsp_sample_skel, libfastcv(a)dsp(_skel), libqvr_cam_dsp_driver_skel, libqvr_dsp_driver_skel, libapps_mem_heap and friends. These run on the DSP side of the camera/tracking pipeline.

## How to remake this dump

Pulled from the dsp/rfsa paths on the stock device - see tools/292_find_skel.sh and the cdsp scripts.

## Files

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

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.
