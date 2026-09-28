# cted

On-device half of HCTE (`applications/cte`). A small TCP daemon that gives
the desktop tool a shell-capability channel without needing adb on the
host. Compiled into the image by `system/tools/build/380_img_cted.sh` and
kept **off by default** - same opt-in stance as wireless adb:

```sh
adb shell su -c 'setprop persist.hibiscus.cted 1'   # start, survives reboot
adb shell su -c 'setprop persist.hibiscus.cted 0'   # stop
```

## Wire protocol

Port `7340`, one command per connection. The server writes a `CTE/1`
banner on accept; the client writes one command line; the reply is either
a single `+`-prefixed result or a stream until disconnect.

| Command | Reply |
| --- | --- |
| `PING` | `+PONG` |
| `INFO` | `+JSON <len>\n` + JSON: `model`, `device`, `tracking`, `battery`, `props{}` |
| `PROPS` | `+TEXT <len>\n` + raw `getprop` output |
| `FRAMES` | repeating `FRAME <len>\n` + PNG bytes (screencap, ~2.5 fps) |
| `POSE` | `POSELOG <line>\n` stream of the pose logcat tags (`pn2pose`, `hibiscuspose`) |
| `CTRL` | `CTRL <idx> live= batt= px= py= pz= qx= qy= qz= qw= trk= sx= sy= a= b= menu= sys= trig= grip=` lines |
| `LOG` | `LOG <line>\n` logcat stream |
| `INSTALL <len>` | `<len>` raw apk bytes, then `LOG` progress lines and `+OK`/`+ERR` |
| anything else | `+ERR unknown command` |

`POSE` also enables the dump switches before tailing:
`setprop debug.pn2.posedump 1` and `/data/local/tmp/xr/posedump`. Head
poses only exist while a VR app runs on the headset.

## The generic pose feed (`hibiscuspose`)

Drivers emit one stable line per served relation on logcat tag
`hibiscuspose` while posedump is on:

```
HMD ts=<ns> qx=.. qy=.. qz=.. qw=.. px=.. py=.. pz=.. st=<state>
```

`st` mirrors the driver's tracking state: `3` = position+orientation
(6DoF), `1` = orientation only (3DoF). The pn2 driver emits these from
both its qvrd and IMU-fusion paths (`drivers/pn2/monado/pn2_hmd.c`).
New drivers get the same surface by emitting the same line shape - the
tooling doesn't care who wrote it.

## Controllers

`CTRL` mmaps `/sdcard/CtrlShareMem` - the shared file CVService rewrites
with both controllers' pose/keys/battery - and decodes it with
`ctrl_state.c`, the same file vrhome and the pn2 monado driver compile.
Devices without the file simply stream nothing.

## Files

- `cted.c` - the daemon (portable POSIX, builds for Android and host)
- `hibiscus-cted.rc` - init service, `disabled` + property triggers
- `tests/run.sh` - host smoke test of the wire protocol
