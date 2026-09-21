# Hardware

Pico Neo 2 - `A7B10` / `PICOA7B10`.

| | |
|---|---|
| SoC | Qualcomm SDM845, Adreno 630 |
| Stock OS | Android 8.1.0, SDK 27, VNDK 27 |
| Stock build | PUI 4.1.3 b346 (2021-04-09), security patch 2019-01-05 |
| Kernel | 4.9.65-perf+ (sdm845 launch BSP, never rebased) |
| Partitioning | Non-A/B, UFS 6 LUNs, discrete `recovery`, no `super` |
| Panel | JDI 4K, dual DSI 1080x3840 per link -> 3840x2160 @ 72 Hz, DSC 540x8 |
| Tracking cameras | OmniVision ov9282 stereo, 2560x800 |
| Eye cameras | OmniVision ov6211 stereo, 800x400 (Eye SKU only, Tobii) |

Every value in the device tree was read out of the stock firmware, not copied
from a similar sdm845 device. See the
[system/android](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/system/android)
for the full tree.
