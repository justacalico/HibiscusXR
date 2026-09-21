# lens

[gitlab.com/neosalsa/lens](https://gitlab.com/neosalsa/lens)

Lens and tracking configuration pulled from `/vendor/etc/qvr` on stock.

```
svrapi_config.txt / _default   the file that carries the real lens intrinsics
                               (read by libsvrapi.so)
6dof_config.xml                6DoF solver configuration
config_default.txt             qvr defaults
device_calibration.xml         per-device calibration
qvrservice_config_default.txt  qvrservice defaults
axisOffset.txt                 lens axis offsets
```

These are stock Pico/Qualcomm data files kept for reference - they drive the
distortion mesh and tracking config, and are what the `hmdInfo` values
(3840x2160, lensSeparation 0.062, eyeTextureFov 80) come from.
