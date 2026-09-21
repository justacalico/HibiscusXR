# keylayout

[gitlab.com/neosalsa/keylayout](https://gitlab.com/neosalsa/keylayout)

Input keylayout files for the headset's physical buttons.

```
dc_detect.kl   DC detect input device
gpio-keys.kl   GPIO keys (power, volume, confirm/back)
```

Installed to `/system/usr/keylayout/` - see `tools/input/` for the install and
verification scripts (`394_install_kl.sh`, `395_klcheck.sh`, `403_verify_keys`).
