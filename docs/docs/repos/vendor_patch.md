# vendor_patch

[gitlab.com/neosalsa/vendor_patch](https://gitlab.com/neosalsa/vendor_patch)

The vendor-side patch files - what changes (or rather, what gets overridden)
on the stock `/vendor` partition. The vendor itself is never reflashed; these
are applied as init overlays from the system side.

```
manifest.orig.xml   the stock /vendor/manifest.xml, kept for reference/diff
pn2-snd.rc          loads the sdm845 audio kernel modules late, after apexd
                    mounts the runtime APEX (the "cannot execve" fix)
pn2-vintf.rc        VINTF manifest override - stops vold blocking on the
                    declared-but-absent IBootControl interface
```

Both correspond to bugs [#1 and #2](../internals/bugs-fixed.md).
