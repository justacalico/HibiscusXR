Hisbuscus rules;

1. All code should be testable; every line
2. All code changes should be able to replicated in git DO NOT WRITE DEVICE SPEFIIC CHANGES. Fix the actural code.
3. The user may have notes in a DNC.md file when oworking in worktrees this file wont be visible so make sure to read them.
4. This OS is to be ran on multiple devices not just the neo 2 ensure code will work with that and docs reflect.
5. Hibiscus is an open source Android based OS for vr headsets in general, the Pico Neo 2 is just the first device it happens to support. Never describe the OS, the shell, or the apps as being "for the Pico Neo 2". Device names (pn2, Pico Neo 2, A7B10) only belong in device specfic code like drivers/<name>/, the device trees, dumps and research notes. Shared code, apps, docs and variables stay device neutral.
