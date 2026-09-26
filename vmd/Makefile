# vmd - Hibiscus virtual machine driver (host side, Linux only).
#
# Boots the same system image under qemu, feeds the guest's hsvr vmd
# driver head poses, and shows or submits the guest framebuffer.
#
#   make           builds out/vmd
#   out/vmd -desktopsim   windowed sim (also -novr, -pc)
#   out/vmd -openxr       OpenXR app mode for WiVRn/Monado
#   out/vmd -selftest     pose-channel smoke test

CXX ?= g++
PKGS = glfw3 epoxy openxr
CFLAGS = -O2 -g -Wall -Wextra -std=c++17 \
         -I../drivers/vmd/proto $(shell pkg-config --cflags $(PKGS))
LIBS = $(shell pkg-config --libs $(PKGS)) -lX11 -lpthread

SRC = src/main.cpp src/pose.cpp src/vm.cpp src/sim.cpp src/xr.cpp
OUT = out/vmd

$(OUT): $(SRC) ../drivers/vmd/proto/vmd_proto.h
	@mkdir -p out
	$(CXX) $(CFLAGS) -o $@ $(SRC) $(LIBS)

clean:
	rm -rf out

.PHONY: clean
