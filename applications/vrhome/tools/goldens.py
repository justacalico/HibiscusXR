#!/usr/bin/env python3
# render the dash's golden frames. Every scene in tests/goldens/scenes.json
# is a list of MCP tool calls; this boots out/dash-preview -headless per
# scene, drives it over stdio MCP and keeps the dash_screenshot once the
# frame stops changing.
#
# Goldens are self-updating: a frame that drifted rewrites its PNG, so the
# files always track the code - no update flag, no per-scene test code,
# adding a scene is a manifest line. `--check` verifies instead of
# rewriting and fails on any diff; it writes nothing.
import base64
import json
import os
import select
import struct
import subprocess
import sys
import threading
import time
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GOLDEN_DIR = ROOT / "tests" / "goldens"
PREVIEW = ROOT / "out" / "dash-preview"

PIX_TOL = 8      # per-channel delta a pixel may drift before it counts
PCT_TOL = 0.5    # percent of pixels allowed to drift before a rewrite
SETTLE_MS = 6000  # budget for a scene to reach a static frame
POLL_MS = 200

# the deterministic render context: llvmpipe so pixels don't depend on the
# GPU, UTC + a pinned clock so the dock's time text can't churn the frame
RENDER_ENV = {
    "TZ": "UTC",
    "debug.vrhome.clock": "14:37",
    "LIBGL_ALWAYS_SOFTWARE": "1",
    "GALLIUM_DRIVER": "llvmpipe",
}


def png_decode(png):
    if png[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("not a png")
    pos, idat, w, h, depth, ctype = 8, b"", 0, 0, 0, 0
    while pos < len(png):
        n, tag = struct.unpack(">I4s", png[pos:pos + 8])
        data = png[pos + 8:pos + 8 + n]
        if tag == b"IHDR":
            w, h, depth, ctype, _, _, inter = struct.unpack(">IIBBBBB", data)
            if depth != 8 or ctype not in (2, 6) or inter:
                raise ValueError("unsupported png format")
        elif tag == b"IDAT":
            idat += data
        elif tag == b"IEND":
            break
        pos += 12 + n
    if not idat or w <= 0 or h <= 0:
        raise ValueError("truncated png")
    bpp = 4 if ctype == 6 else 3
    stride = w * bpp
    raw = zlib.decompress(idat)
    rgba = bytearray()
    prev = bytearray(stride)
    for y in range(h):
        row = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        f = raw[y * (stride + 1)]
        for x in range(stride):
            a = row[x - bpp] if x >= bpp else 0
            b = prev[x]
            c = prev[x - bpp] if x >= bpp else 0
            if f == 1:
                row[x] = (row[x] + a) & 0xFF
            elif f == 2:
                row[x] = (row[x] + b) & 0xFF
            elif f == 3:
                row[x] = (row[x] + (a + b) // 2) & 0xFF
            elif f == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                row[x] = (row[x] +
                          (a if pa <= pb and pa <= pc
                           else b if pb <= pc else c)) & 0xFF
        rgba += row
        prev = row
    if bpp == 3:
        out = bytearray(w * h * 4)
        for i in range(w * h):
            out[i * 4:i * 4 + 3] = rgba[i * 3:i * 3 + 3]
            out[i * 4 + 3] = 255
        rgba = out
    return w, h, bytes(rgba)


def changed_pct(a, b):
    if len(a) != len(b):
        return 100.0
    diff = 0
    for i in range(0, len(a), 4):
        if (abs(a[i] - b[i]) > PIX_TOL or abs(a[i + 1] - b[i + 1]) > PIX_TOL
                or abs(a[i + 2] - b[i + 2]) > PIX_TOL):
            diff += 1
    return diff / (len(a) // 4) * 100.0


class Dash:
    def __init__(self, preview):
        env = dict(os.environ)
        env.update(RENDER_ENV)
        self.proc = subprocess.Popen(
            [str(preview), "-headless"], cwd=ROOT, stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
        self.errs = []
        threading.Thread(
            target=lambda: [self.errs.append(l.decode(errors="replace"))
                            for l in self.proc.stderr],
            daemon=True).start()
        self.next_id = 0

    def call(self, method, params=None, timeout=15):
        self.next_id += 1
        rid = self.next_id
        req = {"jsonrpc": "2.0", "id": rid, "method": method}
        if params is not None:
            req["params"] = params
        self.proc.stdin.write(json.dumps(req).encode() + b"\n")
        self.proc.stdin.flush()
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if self.proc.poll() is not None:
                raise RuntimeError(
                    "dash exited: " + "".join(self.errs[-5:]).strip())
            r, _, _ = select.select([self.proc.stdout], [], [],
                                    deadline - time.monotonic())
            if not r:
                break
            line = self.proc.stdout.readline()
            if not line:
                raise RuntimeError("dash closed stdout")
            try:
                msg = json.loads(line)
            except ValueError:
                continue
            if msg.get("id") != rid:
                continue
            if "error" in msg:
                raise RuntimeError(msg["error"].get("message", "rpc error"))
            return msg.get("result", {})
        raise RuntimeError("timeout waiting for " + method)

    def tool(self, name, arguments=None):
        return self.call("tools/call",
                         {"name": name, "arguments": arguments or {}})

    def shot(self):
        res = self.tool("dash_screenshot")
        if res.get("isError"):
            raise RuntimeError("screenshot failed")
        for c in res.get("content", []):
            if c.get("mimeType") == "image/png":
                return base64.b64decode(c["data"])
        raise RuntimeError("no image in screenshot")

    def close(self):
        try:
            self.proc.stdin.close()
            self.proc.wait(timeout=10)
        except Exception:
            self.proc.kill()


def wait_stable(dash, settle_ms):
    deadline = time.monotonic() + settle_ms / 1000
    png = last = None
    while True:
        png = dash.shot()
        if last is not None and png == last:
            return png
        last = png
        if time.monotonic() > deadline:
            print("    warn: frame never settled, using last")
            return png
        time.sleep(POLL_MS / 1000)


def render(preview, steps, settle_ms, tool_names=None):
    dash = Dash(preview)
    try:
        dash.call("initialize",
                  {"protocolVersion": "2024-11-05", "capabilities": {},
                   "clientInfo": {"name": "goldens", "version": "1"}})
        if tool_names is not None:
            res = dash.call("tools/list")
            tool_names.update(t["name"] for t in res.get("tools", []))
        # let bring-up settle first: queued panel launches dismiss the
        # settings card, so a scene step sent too early gets eaten
        wait_stable(dash, settle_ms)
        for s in steps:
            dash.tool(s["tool"], s.get("arguments"))
        return wait_stable(dash, settle_ms)
    finally:
        dash.close()


def load_scenes(path):
    spec = json.loads(path.read_text())
    scenes = spec.get("scenes", spec)
    out = {}
    for name, v in scenes.items():
        if isinstance(v, list):
            out[name] = {"steps": v, "settle_ms": SETTLE_MS}
        else:
            out[name] = {"steps": v.get("steps", []),
                         "settle_ms": v.get("settle_ms", SETTLE_MS)}
    return out


def main():
    check = "--check" in sys.argv
    only = None
    if "--scene" in sys.argv:
        only = sys.argv[sys.argv.index("--scene") + 1]
    preview = Path(os.environ.get("DASH_PREVIEW", PREVIEW))
    gdir = Path(os.environ.get("GOLDEN_DIR", GOLDEN_DIR))

    if not preview.exists():
        print(f"goldens skipped: {preview} missing "
              "(build it with `make out/dash-preview`)")
        return 0
    scenes = load_scenes(gdir / "scenes.json")
    if only:
        scenes = {only: scenes[only]} if only in scenes else {}
        if not scenes:
            print(f"no scene named {only}", file=sys.stderr)
            return 1

    gdir.mkdir(parents=True, exist_ok=True)
    seen_tools = set()
    for s in scenes.values():
        seen_tools.update(st["tool"] for st in s["steps"])

    fails = 0
    dash_tools = set()
    counts = {"stable": 0, "new": 0, "updated": 0, "removed": 0}
    for name, scene in sorted(scenes.items()):
        path = gdir / f"{name}.png"
        try:
            png = render(preview, scene["steps"], scene["settle_ms"],
                         dash_tools if not dash_tools else None)
        except Exception as e:
            if "surfaceless egl" in str(e) or "headless" in str(e):
                print(f"SKIP {name}: {e}")
                print("goldens skipped: headless EGL unavailable on this host")
                return 0
            print(f"FAIL {name}: {e}")
            fails += 1
            continue
        try:
            w, h, rgba = png_decode(png)
        except ValueError as e:
            print(f"FAIL {name}: bad frame ({e})")
            fails += 1
            continue
        px = rgba[::4096]
        if max(px) - min(px) < 4:
            print(f"FAIL {name}: frame is blank")
            fails += 1
            continue
        if not path.exists():
            status = "new"
        elif png == path.read_bytes():
            status = "stable"
        else:
            try:
                gw, gh, grgba = png_decode(path.read_bytes())
            except ValueError:
                status = "new"
            else:
                pct = (100.0 if (w, h) != (gw, gh)
                       else changed_pct(rgba, grgba))
                status = ("stable" if pct <= PCT_TOL
                          else f"updated ({pct:.1f}% px)")
        if status == "stable":
            counts["stable"] += 1
        elif check:
            print(f"FAIL {name}: " +
                  ("missing golden" if status == "new"
                   else f"{status} vs golden"))
            fails += 1
            continue
        elif status == "new":
            counts["new"] += 1
        else:
            counts["updated"] += 1
        if not check and status != "stable":
            path.write_bytes(png)
        print(f"{'OK' if status == 'stable' else status.upper():>7} {name}")

    for old in sorted(gdir.glob("*.png")):
        if only or old.stem in scenes or not old.exists():
            continue
        if check:
            print(f"FAIL {old.stem}: no scene produces it")
            fails += 1
        else:
            old.unlink()
            counts["removed"] += 1
            print(f"REMOVED {old.stem}")

    if not check and dash_tools:
        skip = {"dash_state", "dash_screenshot"}
        uncovered = sorted(t for t in dash_tools
                           if t.startswith("dash_")
                           and t not in skip and t not in seen_tools)
        if uncovered:
            print("note: no scene drives " + ", ".join(uncovered))
    print(f"goldens: {counts['stable']} stable, {counts['new']} new, "
          f"{counts['updated']} updated, {counts['removed']} removed"
          + (", check mode (no writes)" if check else ""))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
