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
HOST_CXXFLAGS := -std=c++17 -O0 -g -Wall -Wextra -Isrc

# render/platform code shared by both .so's
SHARED_SRCS := src/common/props.cpp src/common/jni.cpp \
               src/math/mat4.cpp src/math/head.cpp \
               src/text/utf8.cpp src/text/glyphs.cpp \
               src/text/font.cpp src/text/draw.cpp \
               src/render/egl.cpp src/render/frame.cpp \
               src/render/shaders.cpp src/render/warp.cpp \
               src/render/scene_geo.cpp src/sensor/sensor.cpp \
               src/sensor/qvr.cpp
ENV_SRCS := src/env/main.cpp src/render/scene.cpp
HUD_SRCS := src/hud/hud_main.cpp src/render/chrome.cpp \
            src/panels/panels.cpp src/panels/layout.cpp \
            src/dock/dock.cpp src/dock/layout.cpp \
            src/bridge/bridge.cpp \
            src/input/input.cpp src/input/keys.cpp

ENV_OBJS := $(patsubst src/%.cpp,$(OUT)/obj/%.o,$(SHARED_SRCS) $(ENV_SRCS))
HUD_OBJS := $(patsubst src/%.cpp,$(OUT)/obj/%.o,$(SHARED_SRCS) $(HUD_SRCS))
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
HOST_SRCS := src/math/mat4.cpp src/math/head.cpp src/panels/layout.cpp \
             src/text/utf8.cpp src/text/glyphs.cpp src/render/scene_geo.cpp \
             src/input/keys.cpp src/render/warp.cpp src/dock/layout.cpp
TEST_SRCS := $(wildcard tests/*.cpp)
TEST_BIN  := $(OUT)/tests/runtests

.PHONY: all apk test install clean toolcheck

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
	$(CLANGXX) -c -fPIC -O2 -std=c++17 -I $(GLUE) -MMD -MP -o $@ $<

-include $(ENV_OBJS:.o=.d) $(HUD_OBJS:.o=.d)

$(ENV_SO): $(ENV_OBJS) $(GLUE_O)
	@mkdir -p $(dir $@)
	$(CLANGXX) -shared -fPIC -O2 -std=c++17 -static-libstdc++ \
	    -o $@ $^ -landroid -lEGL -lGLESv2 -llog -ldl -lm -u ANativeActivity_onCreate

$(HUD_SO): $(HUD_OBJS)
	@mkdir -p $(dir $@)
	$(CLANGXX) -shared -fPIC -O2 -std=c++17 -static-libstdc++ \
	    -o $@ $^ -landroid -lEGL -lGLESv2 -llog -ljnigraphics -ldl -lm

$(CLASSES): $(JAVA_SRCS) $(STUB_SRCS) | toolcheck
	@mkdir -p $(OUT)/classes
	rm -rf $(OUT)/classes/gitlab
	javac -source 8 -target 8 -cp "$(JAR)" -d $(OUT)/classes \
	    $(JAVA_SRCS) $(STUB_SRCS)
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
	@mkdir -p $(OUT)
	$(AAPT2) link -I "$(JAR)" --manifest hud/AndroidManifest.xml \
	    --min-sdk-version 26 --target-sdk-version 29 -o $@ $(OUT)/res.zip

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
	        -dname "CN=vrhome,O=pn2,C=CN"; \
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
	$(CXX) $(HOST_CXXFLAGS) -o $@ $(TEST_SRCS) $(HOST_SRCS) -lm

test: $(TEST_BIN)
	./$(TEST_BIN)

install: $(APK) $(HUD_APK)
	adb install -r $(APK)
	adb install -r $(HUD_APK)

clean:
	rm -rf $(OUT)
