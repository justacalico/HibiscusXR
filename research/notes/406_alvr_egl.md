# 406 - ALVR alvr_initialize_opengl panic: driver rejects EGL robust-access

## The panic

```
panicked at alvr\graphics\src\lib.rs:245:65:
removal index (is 0) should be < len (is 0)
  10: alvr_initialize_opengl
  11: _Z12renderThreadPv
```

ALVR's graphics init is `instance.enumerate_adapters(Backends::GL).remove(0)`
on wgpu 24. The empty vec means wgpu's GL backend never registered - not a
GL crash, a silent init drop. The only visible wgpu warnings beforehand
("EGL_MESA_platform_surfaceless not available", "EGL says it can present to
the window but not natively") are both benign on Android.

## Root cause: eglCreateContext + EGL_CONTEXT_OPENGL_ROBUST_ACCESS

wgpu-hal 24 `Inner::create`, on any EGL >= 1.5 display without ANGLE
extensions, unconditionally appends `EGL_CONTEXT_OPENGL_ROBUST_ACCESS`
(0x31B2, the core EGL 1.5 token) to the context attributes. Confirmed by
instrumenting khronos-egl 6.0.0 (ALVR's exact version) on-device:

```
KHEGL create_context dpy=0x1 attrs=[0x3098,3, 0x31B2,1, 0x3038]
KHEGL -> ctx=0x0  (eglGetError == EGL_SUCCESS - driver fails silently)
```

The V@378 driver advertises EGL 1.5 + EGL_EXT_create_context_robustness but
its attrib parser only accepts the EXT token (0x30BF); the core token falls
through to the unknown-key path. Same bug class as gfx-rs/wgpu#7952 (fixed
upstream in wgpu 25 by retrying Core->Ext->none; ALVR v20.14.1 ships wgpu 24
and cannot be patched here).

Note: this affects every app that creates an EGL context with the core
robust-access token, not just ALVR - wgpu 24 clients, anything using
EGL_CONTEXT_OPENGL_ROBUST_ACCESS per spec.

## Where the check lives

`libGLESv2_adreno.so` (7MB), context-attrib setter at 0x15f9fc
(ctx*, key w21, value w19 -> bool). Dispatch handles 0x3098
(CONTEXT_CLIENT_VERSION), 0x30BF, a jump table for 0x30FB-0x3100
(CONTEXT_MINOR_VERSION etc), 0x3138/0x31B3/0x31BD and the 0x32C0 range.
Unknown keys land at 0x15fcf8 -> log "invalid attrib" -> return 0 ->
EGL_BAD_ATTRIBUTE.

## The patch (tools/patch/405_patch_gles_robust.py)

Rewrites the head of the unknown-key path at 0x15fcf8:

```
mov w14, #0x31b2
cmp w21, w14
b.eq 0x15fb4c        ; existing 0x30BF handler - sets the robustness bit
mov w0, wzr
b   0x15ff3c         ; epilogue, ret 0 (unchanged failure, minus the log)
```

0x31B2 now gets identical handling to 0x30BF. Everything else still fails
the same way; only the "invalid attrib" debug message is lost. Xref audit:
the only branches into the rewritten region are the three dispatch compares
landing on the block head itself - nothing jumps into the middle of it.

## Delivery

Stock blob is patched at image build (406_img_glespatch.sh, sources from
linklibs/notes/qlibs or adb pull), written to /etc/pn2/, and bind-mounted
over /vendor/lib64/egl/libGLESv2_adreno.so at early-init by pn2-egl.rc -
same mechanism as the VINTF manifest shadow. Vendor partition stays
byte-identical; full image only since the blob is proprietary.

Caveats:
- 64-bit blob only. /vendor/lib/egl/libGLESv2_adreno.so has the same bug but
  every wgpu client seen so far is arm64.
- 0x30FC (EGL_CONTEXT_FLAGS_KHR) is still rejected. wgpu only requests it
  for debug/validation builds; release builds never see it.
- CI needs the stock blob in the blobs package (upload-inputs.sh now copies
  it into linklibs/ from notes/qlibs) - PIN_BLOBS has to point at a package
  that contains it, or the build falls back to an adb pull off a connected
  device.

## Verified

Patched blob bind-mounted over the vendor lib on-device (visible inside the
app mount namespace - the early-init mount in the image lands before zygote
forks, so every app inherits it):

- eglCreateContext(ES3 + 0x31B2) -> SUCCESS (was NO_CONTEXT before)
- Real wgpu 24.0.1 repro binary (Backends::GL, same InstanceDescriptor as
  ALVR) -> `enumerate_adapters` returns 1 adapter, Vendor Qualcomm /
  Adreno 630 / "OpenGL ES 3.2 V@378.0" parsed fine
- alvr.client.stable v20.14.1 launches, passes GraphicsContext::new_gl
  (previously panicked on remove(0)), reaches the network-announce stage
  and waits for a streamer - the GL path is clear end to end
- Applies system-wide: the parser is shared by every process using the
  Adreno EGL stack, so all apps get the fix, not just ALVR
