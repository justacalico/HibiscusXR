# persist_calib

[HibiscusXR/system/persist_calib](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/system/persist_calib)

Calibration data pulled from `/persist` - the one partition you cannot
rebuild.

```
camera/device_calibration.xml   camera calibration
lens/axisOffset.txt             lens axis offsets
```

`/persist` is per-device and written at the factory - losing it means losing
the unit's calibration. That is why it is first in the
[backup order](../internals/partitions.md). Extracted by
`tools/extract/361_persist_extract.sh` / `362_calibgen.sh`.
