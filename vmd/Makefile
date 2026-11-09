# vmd - Hibiscus virtual machine driver (host side, Linux only).
#
# Boots the same system image under qemu, feeds the guest's hsvr vmd
# driver head poses, and shows or submits the guest framebuffer.
#
#   make           builds out/vmd
#   make ARCH=aarch64 SYSROOT=<dir>
#                  cross build -> out/aarch64/vmd, with the aarch64
#                  Fedora sysroot from system/linux/images/fetch-sysroot.sh
#                  --vmd. PKG_CONFIG_SYSROOT_DIR/LIBDIR are pointed into
#                  it, so a plain cross g++ is all the host needs.
#   make guest          build the Hibiscus Linux guest set into
#                       out/guest-<arch>/ (vm/make-guest.sh; GUEST_ARCH,
#                       SYSROOT apply)
#   make dist           standalone out/dist/vmd-linux-<arch> with the
#                       guest set embedded - runs the OS with no -img.
#                       EMBED=0 ships the guest as a sibling tar.xz
#   out/vmd -desktopsim   windowed sim (also -novr, -pc)
#   out/vmd -openxr       OpenXR app mode for WiVRn/Monado
#   out/vmd -selftest     pose-channel smoke test
#   out/vmd -guestinfo    what guest set this binary would boot

ARCH    ?= $(shell uname -m)
ifeq ($(ARCH),aarch64)
CROSS   ?= aarch64-linux-gnu-
endif
ifeq ($(origin CXX),default)
CXX     := $(CROSS)g++
endif
SYSROOT ?=
PKGS    = glfw3 epoxy openxr
PC      = pkg-config

ifneq ($(SYSROOT),)
# rpath-link: cross lds that do not search -L dirs for a shared lib's
# own deps (Debian/Ubuntu toolchains) still find libxcb/jsoncpp & co
SYSFLAGS = --sysroot=$(SYSROOT) -Wl,-rpath-link,$(SYSROOT)/usr/lib64
# a bare cross g++ ships no c++ headers (Fedora's); when the compiler
# cannot parse <string> on its own, libstdc++-devel in the sysroot
# provides the generic dir plus the *-linux arch dir - isystem, not
# idirafter, so the headers' #include_next still reaches the C library.
# A cross g++ with its own headers keeps them: the sysroot's headers
# can be newer than the compiler understands
ifeq ($(shell printf '#include <string>\n' | \
        $(CXX) --sysroot=$(SYSROOT) -x c++ -fsyntax-only - 2>/dev/null \
        && echo yes),)
CXXVER  := $(notdir $(firstword $(wildcard $(SYSROOT)/usr/include/c++/*)))
ifneq ($(CXXVER),)
CXXARCH := $(notdir $(firstword $(wildcard \
            $(SYSROOT)/usr/include/c++/$(CXXVER)/*-linux)))
SYSFLAGS += -isystem $(SYSROOT)/usr/include/c++/$(CXXVER)/$(CXXARCH) \
            -isystem $(SYSROOT)/usr/include/c++/$(CXXVER)
endif
endif
empty   :=
space   := $(empty) $(empty)
PCDIRS  := $(SYSROOT)/usr/lib64/pkgconfig \
           $(SYSROOT)/usr/lib/$(ARCH)-linux-gnu/pkgconfig \
           $(SYSROOT)/usr/lib/pkgconfig $(SYSROOT)/usr/share/pkgconfig
PC      = env PKG_CONFIG_SYSROOT_DIR=$(SYSROOT) \
          PKG_CONFIG_LIBDIR=$(subst $(space),:,$(strip $(PCDIRS))) pkg-config
endif

CFLAGS = -O2 -g -Wall -Wextra -std=c++17 $(SYSFLAGS) \
         -I../drivers/vmd/proto $(shell $(PC) --cflags $(PKGS))
LIBS = $(shell $(PC) --libs $(PKGS)) -lX11 -lpthread

SRC = src/main.cpp src/pose.cpp src/vm.cpp src/sim.cpp src/xr.cpp \
      src/guest.cpp
ifeq ($(ARCH),$(shell uname -m))
OUT = out/vmd
else
OUT = out/$(ARCH)/vmd
endif

$(OUT): $(SRC) ../drivers/vmd/proto/vmd_proto.h
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) -o $@ $(SRC) $(LIBS)

# The guest set is the Hibiscus Linux image vmd boots - kernel, module
# initramfs and a real rootfs (vm/make-guest.sh). It matches the binary
# arch by default so the host's own KVM can run it.
GUEST_ARCH ?= $(ARCH)
GUESTDIR = out/guest-$(GUEST_ARCH)
DISTBIN  = out/dist/vmd-linux-$(ARCH)

guest:
	SYSROOT="$(SYSROOT)" bash vm/make-guest.sh -a $(GUEST_ARCH) -o $(GUESTDIR)

# dist: the standalone binary with the guest set embedded - no -img
# needed, it unpacks itself and boots. EMBED=0 ships the bare tool next
# to a vmd-guest-<arch>.tar.xz instead.
EMBED ?= 1
dist: $(OUT)
	@mkdir -p out/dist
ifeq ($(EMBED),0)
	cp $(OUT) $(DISTBIN)
	@test -f $(GUESTDIR)/guest.env && \
		tar -cJf out/dist/vmd-guest-$(GUEST_ARCH).tar.xz -C $(GUESTDIR) . || true
else
	@test -f $(GUESTDIR)/guest.env || \
		$(MAKE) --no-print-directory guest
	tar -cJf out/dist/vmd-guest-$(GUEST_ARCH).tar.xz -C $(GUESTDIR) .
	cp $(OUT) $(DISTBIN)
	bash vm/embed-guest.sh $(DISTBIN) out/dist/vmd-guest-$(GUEST_ARCH).tar.xz
endif

clean:
	rm -rf out

.PHONY: clean guest dist
