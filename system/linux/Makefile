# Hibiscus Linux - the OS proper. Everything here is plain userspace C
# over the hsvc service layer; nothing touches hardware directly except
# the drivers, and those only through kernel interfaces (v4l2, iio,
# evdev, drm).
#
#   make               host arch build into out/
#   make ARCH=aarch64  cross build via aarch64-linux-gnu-
#   make test          build + run the whole suite on the host
#   make dist          static single-file hsvc-vmd for x86_64 + aarch64
#                      (aarch64 linked 16k-page safe) into out/dist/
#   make dist-host     just the host arch's static hsvc-vmd
#   make shell         build the dash-wayland shell for ARCH into the
#                      arch's out/xr*/bin/, staged by mkimage when
#                      HSVC_XR=1
#   make install DESTDIR=/x PREFIX=/usr

ARCH      ?= $(shell uname -m)
ifeq ($(ARCH),aarch64)
CROSS     ?= aarch64-linux-gnu-
endif
CC        := $(CROSS)gcc
SYSROOT   ?=
CFLAGS    ?= -O2 -g
ifneq ($(SYSROOT),)
CFLAGS    += --sysroot=$(SYSROOT)
LDFLAGS   += --sysroot=$(SYSROOT)
endif
CFLAGS    += -Wall -Wextra -std=c99 -D_GNU_SOURCE -Iinclude -Idrivers
LDLIBS    := -lm
OUT       := out/$(ARCH)

LIB       := src/hsvc.c src/registry.c
LIBOBJ    := $(patsubst src/%.c,$(OUT)/obj/%.o,$(LIB))
BINS      := hsvcd hsvcctl hsvc-sim hsvc-eye hsvc-imu hsvc-ctrl hsvc-vmd

all: $(addprefix $(OUT)/bin/,$(BINS))

$(OUT)/obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OUT)/obj/drv/%.o: drivers/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OUT)/bin/hsvcd: $(LIBOBJ) $(OUT)/obj/broker.o $(OUT)/obj/hsvcd.o
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# vmd carries the broker too: with no daemon answering it forks its own
# private bus, which is what makes the single binary the whole stack.
$(OUT)/bin/hsvc-vmd: $(LIBOBJ) $(OUT)/obj/drv/hsvc-vmd.o $(OUT)/obj/broker.o
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

$(OUT)/bin/hsvcctl: $(LIBOBJ) $(OUT)/obj/hsvcctl.o
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

$(OUT)/bin/hsvc-%: $(LIBOBJ) $(OUT)/obj/drv/hsvc-%.o
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# Single static binaries, one per shipped arch: no runtime deps, no
# install, just run it. The aarch64 one gets max-page-size=16384 so the
# same ELF loads on 4k/16k/64k-page kernels. Needs a static libc for
# each toolchain: libc6-dev / glibc-static on the host, and on hosts
# with a bare cross gcc A64SYSROOT=<dir from fetch-sysroot.sh>.
X86CC   ?= gcc
A64CC   ?= aarch64-linux-gnu-gcc
A64SYSROOT ?=
ifneq ($(A64SYSROOT),)
A64CC   += --sysroot=$(A64SYSROOT)
endif
DIST    := out/dist/hsvc-vmd
VMDSRC  := src/hsvc.c src/registry.c src/broker.c drivers/hsvc-vmd.c
VMDHDR  := include/hsvc/proto.h include/hsvc/hsvc.h include/hsvc/registry.h \
           drivers/vmdmap.h

dist: $(DIST)-x86_64 $(DIST)-aarch64

dist-host: $(DIST)-$(ARCH)

$(DIST)-x86_64: XCC := $(X86CC)
$(DIST)-aarch64: XCC := $(A64CC)
$(DIST)-aarch64: XPAGE := -Wl,-z,max-page-size=16384

$(DIST)-%: $(VMDSRC) $(VMDHDR)
	@mkdir -p $(dir $@)
	$(XCC) $(CFLAGS) $(XPAGE) -static -o $@ $(VMDSRC) $(LDLIBS)

test:
	bash tests/run.sh

# The shell is the dash's own wayland compositor from applications/vrhome
# built for ARCH, staged under the arch's xr dir where mkimage's xr
# staging finds it (out/xr for the host arch, out/xr-<arch> for cross -
# same place xr/build-runtime.sh lands the matching runtime). A cross
# build wants the matching-arch sysroot:
#   make shell ARCH=aarch64 SYSROOT=<dir from fetch-sysroot.sh --shell>
VRHOME  := $(abspath ../../applications/vrhome)
XROUT   := out/xr
ifeq ($(ARCH),$(shell uname -m))
DASHBIN := $(VRHOME)/out/dash-wayland
else
DASHBIN := $(VRHOME)/out/$(ARCH)/dash-wayland
XROUT   := out/xr-$(ARCH)
endif

shell:
	$(MAKE) -C "$(VRHOME)" dash-wayland ARCH=$(ARCH) SYSROOT="$(SYSROOT)"
	install -d $(XROUT)/bin
	install -m755 "$(DASHBIN)" $(XROUT)/bin/dash-wayland

install: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m755 $(addprefix $(OUT)/bin/,$(BINS)) $(DESTDIR)$(PREFIX)/bin/
	install -d $(DESTDIR)$(PREFIX)/lib/systemd/system
	install -m644 units/*.service $(DESTDIR)$(PREFIX)/lib/systemd/system/
	install -d $(DESTDIR)$(PREFIX)/share/hibiscus/devices
	install -m644 devices/*.env $(DESTDIR)$(PREFIX)/share/hibiscus/devices/
	install -d $(DESTDIR)$(PREFIX)/include/hsvc
	install -m644 include/hsvc/*.h $(DESTDIR)$(PREFIX)/include/hsvc/

clean:
	rm -rf out

.PHONY: all dist dist-host test shell install clean
