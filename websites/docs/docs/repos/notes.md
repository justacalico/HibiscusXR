# notes

[HibiscusXR/research/notes](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/research/notes)

The research log behind every fix - roughly 280 numbered files mirroring the
numbered tools scripts. Each `NN_*.log` / `NN_*.txt` is the captured output of
the matching `tools/*/NN_*.sh` run.

How to read it: pick a bug in
[Bugs fixed](../internals/bugs-fixed.md), find the tool script, then open the
same-numbered note for the raw evidence (logcat captures, disassembly dumps,
symbol scans, strace-style listings).

Oversized captures (like `reboot_watch.log`, 661 MB) stay local - they are
gitignored, everything in the repo is the distilled findings.
