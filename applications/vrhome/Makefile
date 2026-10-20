# Two APKs without Gradle: clang -> javac -> d8 -> aapt2 -> zipalign ->
# apksigner. `make test` builds and runs the host unit tests.
#
#   out/vrhome.apk  env: gitlab.neosalsa.home - HOME activity + scenery
#   out/vrhud.apk   hud: gitlab.neosalsa.hud  - system overlay, panels + menu
#
# Env overrides:
#   ANDROID_SDK_ROOT  Android SDK dir   (default /opt/android-sdk)
#   NDK               Android NDK dir   (default: newest under $SDK/ndk)
#   BT                build-tools dir   (default: newest under $SDK/build-tools)
#   JAR               platform jar      (default: android-29)
#   KEYS              signing key dir   (default ../build/keys - platform.pk8 +
#                                       platform.x509.pem; falls back to a
#                                       generated debug keystore)

SDK  := $(or $(ANDROID_SDK_ROOT),/opt/android-sdk)
NDK  ?= $(lastword $(sort $(wildcard $(SDK)/ndk/*)))
BT   ?= $(lastword $(sort $(wildcard $(SDK)/build-tools/*)))
JAR  ?= $(SDK)/platforms/android-29/android.jar
KEYS ?= $(CURDIR)/../build/keys

OUT := out
APK     := $(OUT)/vrhome.apk
HUD_APK := $(OUT)/vrhud.apk

CLANG    := $(NDK)/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang
CLANGXX  := $(NDK)/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang++
GLUE     := $(NDK)/sources/android/native_app_glue
AAPT2    := $(BT)/aapt2
D8       := $(BT)/d8
ZIPALIGN := $(BT)/zipalign
APKSIGNER := $(BT)/apksigner

CXX ?= c++
HOST_CXXFLAGS := -std=c++17 -O0 -g -Wall -Wextra -Isrc -Iout/gen

# what the wayland dash's About card reports as the Hibiscus version when
# the host has no /etc/hibiscus-release: the dist pipeline's stamp wins,
# else the tree's own describe, else dev
HIBISCUS_VERSION ?= $(shell git describe --tags --always --dirty 2>/dev/null || echo dev)
HOST_CXXFLAGS += -DHIBISCUS_VERSION=\"$(HIBISCUS_VERSION)\"

# render/platform code shared by both .so's
SHARED_SRCS := src/common/props.cpp src/common/jni.cpp \
               src/common/palette.cpp \
               src/math/mat4.cpp src/math/head.cpp \
               src/text/utf8.cpp src/text/glyphs.cpp \
               src/text/font.cpp src/text/draw.cpp \
               src/render/egl.cpp src/render/frame.cpp \
               src/render/resources.cpp src/render/shaders.cpp src/render/warp.cpp \
               src/render/scene_geo.cpp src/render/mesh.cpp \
               src/sensor/sensor.cpp \
               src/sensor/qvr.cpp
ENV_SRCS := src/env/main.cpp src/env/cam.cpp src/render/scene.cpp \
            src/render/pt_geo.cpp src/env/homeenv.cpp \
            src/env/envmap.cpp src/env/zipfile.cpp
HUD_SRCS := src/hud/hud_main.cpp src/hud/dash.cpp src/hud/debug_hooks.cpp \
            src/render/chrome.cpp \
            src/render/shape.cpp src/render/ctrl_render.cpp \
            src/panels/panels.cpp src/panels/layout.cpp \
            src/anim/anim.cpp \
            src/pill/pill.cpp \
            src/grid/grid.cpp src/grid/layout.cpp \
            src/kbd/kbd.cpp src/kbd/glue.cpp \
            src/dock/dock.cpp src/dock/layout.cpp \
            src/quick/quick.cpp src/quick/layout.cpp src/quick/icons.cpp \
            src/set/set.cpp src/set/layout.cpp src/set/icons.cpp \
            src/render/iprim.cpp src/render/icons.cpp \
            src/render/iconatlas.cpp \
            src/notif/notif.cpp src/notif/layout.cpp \
            src/notif/shade.cpp \
            src/sysmsg/sysmsg.cpp src/sysmsg/layout.cpp \
            src/bridge/bridge.cpp src/bridge/core.cpp \
            src/input/input.cpp src/input/keys.cpp \
            src/input/input_state.cpp src/input/aim.cpp src/input/ctrl.cpp \
            src/input/ctrl_debug.cpp src/input/pick.cpp

# pure C, shared verbatim with the pn2 monado driver
HUD_CSRCS := src/input/ctrl_state.c

ENV_OBJS := $(patsubst src/%.cpp,$(OUT)/obj/%.o,$(SHARED_SRCS) $(ENV_SRCS))
HUD_OBJS := $(patsubst src/%.cpp,$(OUT)/obj/%.o,$(SHARED_SRCS) $(HUD_SRCS)) \
            $(patsubst src/%.c,$(OUT)/obj/%.o,$(HUD_CSRCS))
JAVA_SRCS   := $(shell find java -name '*.java')
# compile-time stubs for @hide framework classes (InputChannel,
# InputEventReceiver); never passed to d8 - the real classes resolve at
# runtime off the boot classpath
STUB_SRCS   := $(shell find stubs -name '*.java')
RES_SRCS    := $(wildcard res/*/*)

GLUE_O    := $(OUT)/glue.o
ENV_SO    := $(OUT)/lib/arm64-v8a/libvrhome.so
HUD_SO    := $(OUT)/lib/arm64-v8a/libvrhud.so
CLASSES   := $(OUT)/classes/.stamp
ENV_DEX   := $(OUT)/dex_home/classes.dex
HUD_DEX   := $(OUT)/dex_hud/classes.dex
ENV_UNALIGNED  := $(OUT)/unaligned.apk
HUD_UNALIGNED  := $(OUT)/unaligned-hud.apk

# pure modules only - no Android/GL includes, so they build on the host
HOST_SRCS := src/common/palette.cpp src/math/mat4.cpp src/math/head.cpp \
             src/anim/anim.cpp \
             src/panels/layout.cpp src/kbd/kbd.cpp src/pill/pill.cpp \
             src/grid/layout.cpp \
             src/text/utf8.cpp src/text/glyphs.cpp src/render/scene_geo.cpp \
             src/render/pt_geo.cpp \
             src/input/keys.cpp src/render/warp.cpp src/dock/layout.cpp \
             src/notif/layout.cpp src/sysmsg/layout.cpp \
             src/input/input_state.cpp \
             src/input/aim.cpp src/render/mesh.cpp src/input/ctrl_state.c \
             src/input/pick.cpp src/quick/layout.cpp src/set/layout.cpp \
             src/input/ctrl_debug.cpp src/env/envmap.cpp src/env/zipfile.cpp \
             src/mcp/json.cpp src/mcp/enc.cpp src/mcp/server.cpp \
             src/mcp/tools.cpp \
             src/render/iconatlas.cpp \
             src/desktop/desktopfile.cpp src/desktop/icontheme.cpp \
             src/desktop/popuppos.cpp src/desktop/pixconv.cpp \
             src/desktop/sysinfo.cpp src/desktop/cmdline.cpp \
             src/desktop/proctrack.cpp
TEST_SRCS := $(wildcard tests/*.cpp)
TEST_BIN  := $(OUT)/tests/runtests

DESKTOP_SRCS := src/desktop/preview.cpp src/desktop/harness.cpp \
                src/desktop/cmdline.cpp \
                src/hud/dash.cpp \
                src/bridge/core.cpp src/common/props.cpp \
                src/common/palette.cpp src/math/mat4.cpp src/math/head.cpp \
                src/text/utf8.cpp src/text/glyphs.cpp src/text/font.cpp \
                src/text/draw.cpp src/render/resources.cpp \
                src/render/shaders.cpp src/render/shape.cpp \
                src/render/chrome.cpp src/render/ctrl_render.cpp \
                src/render/icons.cpp src/render/iconatlas.cpp \
                src/render/scene_geo.cpp src/render/mesh.cpp \
                src/panels/panels.cpp src/panels/layout.cpp \
                src/anim/anim.cpp src/pill/pill.cpp \
                src/grid/grid.cpp src/grid/layout.cpp \
                src/kbd/kbd.cpp src/kbd/glue.cpp \
                src/dock/dock.cpp src/dock/layout.cpp \
                src/quick/quick.cpp src/quick/layout.cpp \
                src/quick/icons.cpp \
                src/set/set.cpp src/set/layout.cpp src/set/icons.cpp \
                src/render/iprim.cpp \
                src/notif/notif.cpp src/notif/layout.cpp \
                src/notif/shade.cpp \
                src/sysmsg/sysmsg.cpp src/sysmsg/layout.cpp \
                src/input/input.cpp src/input/keys.cpp \
                src/input/input_state.cpp src/input/aim.cpp \
                src/input/ctrl.cpp src/input/ctrl_debug.cpp \
                src/input/ctrl_state.c src/input/pick.cpp \
                src/mcp/json.cpp src/mcp/enc.cpp src/mcp/server.cpp \
                src/mcp/tools.cpp
PREVIEW_BIN := $(OUT)/dash-preview

# dash on desktop linux as a real wayland compositor: same shared code as
# the preview plus the compositor/backend glue. Host needs wayland-server,
# xkbcommon and libpng on top of the preview's deps.
WL_SRCS := src/desktop/wlmain.cpp src/desktop/wlbackend.cpp \
           src/desktop/wlcomp.cpp src/desktop/harness.cpp \
           src/desktop/desktopfile.cpp src/desktop/icontheme.cpp \
           src/desktop/popuppos.cpp src/desktop/pixconv.cpp \
           src/desktop/sysinfo.cpp \
           src/desktop/proctrack.cpp src/desktop/wldmabuf.cpp \
           src/desktop/wlegl.cpp \
           $(filter-out src/desktop/preview.cpp \
             src/desktop/harness.cpp,$(DESKTOP_SRCS))
WL_BIN := $(OUT)/dash-wayland
XDG_XML := /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml
ACT_XML := /usr/share/wayland-protocols/staging/xdg-activation/xdg-activation-v1.xml
DMA_XML := /usr/share/wayland-protocols/stable/linux-dmabuf/linux-dmabuf-v1.xml
WL_GEN := $(OUT)/gen/xdg-shell-server-protocol.h \
          $(OUT)/gen/xdg-activation-v1-server-protocol.h \
          $(OUT)/gen/linux-dmabuf-v1-server-protocol.h \
          $(OUT)/gen/xdg-shell-protocol.o \
          $(OUT)/gen/xdg-activation-v1-protocol.o \
          $(OUT)/gen/linux-dmabuf-v1-protocol.o

.PHONY: all apk test preview dash-wayland install clean toolcheck

all: apk
apk: $(APK) $(HUD_APK)

toolcheck:
	@for p in "$(CLANG)" "$(CLANGXX)" "$(GLUE)/android_native_app_glue.c" \
	          "$(JAR)" "$(AAPT2)" "$(D8)" "$(ZIPALIGN)" "$(APKSIGNER)"; do \
	    [ -e "$$p" ] || { echo "missing: $$p" >&2; exit 1; }; \
	done

# glue is C; building it with clang++ miscompiles the void* conversions
$(GLUE_O): $(GLUE)/android_native_app_glue.c | toolcheck
	@mkdir -p $(dir $@)
	$(CLANG) -c -fPIC -O2 -I $(GLUE) -o $@ $<

$(OUT)/obj/%.o: src/%.cpp | toolcheck
	@mkdir -p $(dir $@)
	$(CLANGXX) -c -fPIC -O2 -std=c++17 -I $(GLUE) -I $(OUT)/gen -MMD -MP -o $@ $<

# pure-C sources shared with the monado driver; -x c++ keeps the dialect
# identical to the host build, where g++ also compiles .c as C++
$(OUT)/obj/%.o: src/%.c | toolcheck
	@mkdir -p $(dir $@)
	$(CLANGXX) -c -fPIC -O2 -std=c++17 -x c++ -I $(GLUE) -MMD -MP -o $@ $<

# embedded controller meshes: the OBJ text lands as byte arrays under
# out/gen for ctrl_render.cpp to parse at first draw
$(OUT)/gen/ctrl_l.inc: models/ctrl_l.obj tools/obj2c.py
	@mkdir -p $(OUT)/gen
	python3 tools/obj2c.py $< kObjL $@

$(OUT)/gen/ctrl_r.inc: models/ctrl_r.obj tools/obj2c.py
	@mkdir -p $(OUT)/gen
	python3 tools/obj2c.py $< kObjR $@

$(OUT)/obj/render/ctrl_render.o: $(OUT)/gen/ctrl_l.inc $(OUT)/gen/ctrl_r.inc

# baked Material Symbols glyph atlas for the dash's icons
$(OUT)/gen/icons.inc $(OUT)/gen/icon_ids.h: $(wildcard art/icons/*.svg) \
                                          tools/icons2c.py
	@mkdir -p $(OUT)/gen
	python3 tools/icons2c.py art/icons $(OUT)/gen

$(OUT)/obj/render/icons.o $(OUT)/obj/render/iconatlas.o \
$(TEST_BIN) $(PREVIEW_BIN) $(WL_BIN): $(OUT)/gen/icons.inc \
                                      $(OUT)/gen/icon_ids.h

-include $(ENV_OBJS:.o=.d) $(HUD_OBJS:.o=.d)

$(ENV_SO): $(ENV_OBJS) $(GLUE_O)
	@mkdir -p $(dir $@)
	$(CLANGXX) -shared -fPIC -O2 -std=c++17 -static-libstdc++ \
	    -o $@ $^ -landroid -lcamera2ndk -lEGL -lGLESv2 -llog -ldl -lz -lm -u ANativeActivity_onCreate

$(HUD_SO): $(HUD_OBJS)
	@mkdir -p $(dir $@)
	$(CLANGXX) -shared -fPIC -O2 -std=c++17 -static-libstdc++ \
	    -o $@ $^ -landroid -lEGL -lGLESv2 -llog -ljnigraphics -ldl -lm

# R.java for the hud package is generated by the hud link step below, so
# the hud manifest's res references resolve to real ids in java code
$(CLASSES): $(JAVA_SRCS) $(STUB_SRCS) $(HUD_UNALIGNED) | toolcheck
	@mkdir -p $(OUT)/classes
	rm -rf $(OUT)/classes/gitlab
	javac -source 8 -target 8 -cp "$(JAR)" -d $(OUT)/classes \
	    $(JAVA_SRCS) $(STUB_SRCS) \
	    $(OUT)/gen/gitlab/neosalsa/hud/R.java
	@touch $@

$(ENV_DEX): $(CLASSES) | toolcheck
	@mkdir -p $(OUT)/dex_home
	rm -f $@
	$(D8) --classpath "$(JAR)" --output $(OUT)/dex_home \
	    $(OUT)/classes/gitlab/neosalsa/home/*.class

$(HUD_DEX): $(CLASSES) | toolcheck
	@mkdir -p $(OUT)/dex_hud
	rm -f $@
	$(D8) --classpath "$(JAR)" --output $(OUT)/dex_hud \
	    $(OUT)/classes/gitlab/neosalsa/hud/*.class

$(OUT)/res.zip: $(RES_SRCS) | toolcheck
	@mkdir -p $(OUT)
	$(AAPT2) compile --dir res -o $@

$(ENV_UNALIGNED): AndroidManifest.xml $(OUT)/res.zip | toolcheck
	@mkdir -p $(OUT)
	$(AAPT2) link -I "$(JAR)" --manifest AndroidManifest.xml \
	    --min-sdk-version 26 --target-sdk-version 29 -o $@ $(OUT)/res.zip

$(HUD_UNALIGNED): hud/AndroidManifest.xml $(OUT)/res.zip | toolcheck
	@mkdir -p $(OUT)/gen
	$(AAPT2) link -I "$(JAR)" --manifest hud/AndroidManifest.xml \
	    --min-sdk-version 26 --target-sdk-version 29 --java $(OUT)/gen \
	    -o $@ $(OUT)/res.zip

# aapt2 can't add entries afterwards: inject the .so and dex into a fresh
# copy of the apk, both stored uncompressed
define repack
	cp $(1) $(2)
	python3 -c 'import sys, zipfile; \
	    apk, so, dex = sys.argv[1:4]; \
	    z = zipfile.ZipFile(apk, "a", zipfile.ZIP_STORED); \
	    z.writestr("lib/arm64-v8a/$(3)", open(so, "rb").read()); \
	    z.writestr("classes.dex", open(dex, "rb").read()); \
	    z.close()' $(2) $(4) $(5)
	$(ZIPALIGN) -f 4 $(2) $(6)
	@if [ -f "$(KEYS)/platform.pk8" ] && [ -f "$(KEYS)/platform.x509.pem" ]; then \
	    $(APKSIGNER) sign --key "$(KEYS)/platform.pk8" \
	        --cert "$(KEYS)/platform.x509.pem" $(6) && \
	    echo "$(6): signed with platform key"; \
	else \
	    [ -f debug.keystore ] || keytool -genkeypair -v \
	        -keystore debug.keystore -alias vrhome -keyalg RSA -keysize 2048 \
	        -validity 10000 -storepass android -keypass android \
	        -dname "CN=vrhome,O=hibiscus,C=CN"; \
	    $(APKSIGNER) sign --ks debug.keystore --ks-pass pass:android $(6); \
	    echo "$(6): signed with debug key (system perms will NOT be granted)"; \
	fi
endef

$(APK): $(ENV_UNALIGNED) $(ENV_SO) $(ENV_DEX) | toolcheck
	$(call repack,$(ENV_UNALIGNED),$(OUT)/injected.apk,libvrhome.so,$(ENV_SO),$(ENV_DEX),$(APK))
	@echo "built: $(APK) ($$(stat -c%s $(APK)) bytes)"

$(HUD_APK): $(HUD_UNALIGNED) $(HUD_SO) $(HUD_DEX) | toolcheck
	$(call repack,$(HUD_UNALIGNED),$(OUT)/injected-hud.apk,libvrhud.so,$(HUD_SO),$(HUD_DEX),$(HUD_APK))
	@echo "built: $(HUD_APK) ($$(stat -c%s $(HUD_APK)) bytes)"

$(TEST_BIN): $(TEST_SRCS) $(HOST_SRCS) tests/test.h
	@mkdir -p $(dir $@)
	$(CXX) $(HOST_CXXFLAGS) -o $@ $(TEST_SRCS) $(HOST_SRCS) -lz -lm

test: $(TEST_BIN)
	./$(TEST_BIN)

$(PREVIEW_BIN): $(DESKTOP_SRCS) $(OUT)/gen/ctrl_l.inc $(OUT)/gen/ctrl_r.inc
	@mkdir -p $(dir $@)
	$(CXX) $(HOST_CXXFLAGS) -DVRHOME_DESKTOP -I$(OUT)/gen -o $@ \
	    $(DESKTOP_SRCS) -lglfw -lGLESv2 -lEGL -lz -lm

preview: $(PREVIEW_BIN)
	./$(PREVIEW_BIN)

$(OUT)/gen/xdg-shell-server-protocol.h: $(XDG_XML)
	@mkdir -p $(OUT)/gen
	wayland-scanner server-header $< $@

$(OUT)/gen/xdg-shell-protocol.c: $(XDG_XML)
	@mkdir -p $(OUT)/gen
	wayland-scanner private-code $< $@

$(OUT)/gen/xdg-shell-protocol.o: $(OUT)/gen/xdg-shell-protocol.c
	$(CC) -O2 -I$(OUT)/gen -c -o $@ $<

$(OUT)/gen/xdg-activation-v1-server-protocol.h: $(ACT_XML)
	@mkdir -p $(OUT)/gen
	wayland-scanner server-header $< $@

$(OUT)/gen/xdg-activation-v1-protocol.c: $(ACT_XML)
	@mkdir -p $(OUT)/gen
	wayland-scanner private-code $< $@

$(OUT)/gen/xdg-activation-v1-protocol.o: $(OUT)/gen/xdg-activation-v1-protocol.c
	$(CC) -O2 -I$(OUT)/gen -c -o $@ $<

$(OUT)/gen/linux-dmabuf-v1-server-protocol.h: $(DMA_XML)
	@mkdir -p $(OUT)/gen
	wayland-scanner server-header $< $@

$(OUT)/gen/linux-dmabuf-v1-protocol.c: $(DMA_XML)
	@mkdir -p $(OUT)/gen
	wayland-scanner private-code $< $@

$(OUT)/gen/linux-dmabuf-v1-protocol.o: $(OUT)/gen/linux-dmabuf-v1-protocol.c
	$(CC) -O2 -I$(OUT)/gen -c -o $@ $<

$(WL_BIN): $(WL_SRCS) $(WL_GEN) \
           $(OUT)/gen/ctrl_l.inc $(OUT)/gen/ctrl_r.inc
	@mkdir -p $(dir $@)
	$(CXX) $(HOST_CXXFLAGS) -DVRHOME_DESKTOP -I$(OUT)/gen -o $@ \
	    $(WL_SRCS) $(OUT)/gen/xdg-shell-protocol.o \
	    $(OUT)/gen/xdg-activation-v1-protocol.o \
	    $(OUT)/gen/linux-dmabuf-v1-protocol.o \
	    $$(pkg-config --libs --cflags wayland-server xkbcommon libpng \
	            libdrm) \
	    -lglfw -lGLESv2 -lEGL -lz -lm

dash-wayland: $(WL_BIN)
	./$(WL_BIN)

install: $(APK) $(HUD_APK)
	adb install -r $(APK)
	adb install -r $(HUD_APK)

clean:
	rm -rf $(OUT)
