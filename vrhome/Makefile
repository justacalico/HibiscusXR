# vrhome.apk without Gradle: clang -> javac -> d8 -> aapt2 -> zipalign ->
# apksigner. `make test` builds and runs the host unit tests.
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
APK := $(OUT)/vrhome.apk

CLANG    := $(NDK)/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang
CLANGXX  := $(NDK)/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang++
GLUE     := $(NDK)/sources/android/native_app_glue
AAPT2    := $(BT)/aapt2
D8       := $(BT)/d8
ZIPALIGN := $(BT)/zipalign
APKSIGNER := $(BT)/apksigner

CXX ?= c++
HOST_CXXFLAGS := -std=c++17 -O0 -g -Wall -Wextra -Isrc

NATIVE_SRCS := $(wildcard src/*.cpp src/*/*.cpp)
NATIVE_OBJS := $(patsubst src/%.cpp,$(OUT)/obj/%.o,$(NATIVE_SRCS))
JPKG        := gitlab/neosalsa/home
JAVA_SRCS   := $(wildcard java/$(JPKG)/*.java)
RES_SRCS    := $(wildcard res/*/*)

GLUE_O    := $(OUT)/glue.o
SO        := $(OUT)/lib/arm64-v8a/libvrhome.so
CLASSES   := $(OUT)/classes/.stamp
DEX       := $(OUT)/dex/classes.dex
UNALIGNED := $(OUT)/unaligned.apk

# pure modules only - no Android/GL includes, so they build on the host
HOST_SRCS := src/math/mat4.cpp src/math/head.cpp src/panels/layout.cpp \
             src/text/utf8.cpp src/text/glyphs.cpp src/render/scene_geo.cpp \
             src/input/keys.cpp
TEST_SRCS := $(wildcard tests/*.cpp)
TEST_BIN  := $(OUT)/tests/runtests

.PHONY: all apk test install clean toolcheck

all: apk
apk: $(APK)

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

-include $(NATIVE_OBJS:.o=.d)

$(SO): $(NATIVE_OBJS) $(GLUE_O)
	@mkdir -p $(dir $@)
	$(CLANGXX) -shared -fPIC -O2 -std=c++17 -static-libstdc++ \
	    -o $@ $^ -landroid -lEGL -lGLESv2 -llog -lm -u ANativeActivity_onCreate

$(CLASSES): $(JAVA_SRCS) | toolcheck
	@mkdir -p $(OUT)/classes
	javac -source 8 -target 8 -cp "$(JAR)" -d $(OUT)/classes $(JAVA_SRCS)
	@touch $@

$(DEX): $(CLASSES) | toolcheck
	@mkdir -p $(OUT)/dex
	rm -f $@
	$(D8) --classpath "$(JAR)" --output $(OUT)/dex \
	    $(OUT)/classes/$(JPKG)/*.class

$(OUT)/res.zip: $(RES_SRCS) | toolcheck
	@mkdir -p $(OUT)
	$(AAPT2) compile --dir res -o $@

$(UNALIGNED): AndroidManifest.xml $(OUT)/res.zip | toolcheck
	@mkdir -p $(OUT)
	$(AAPT2) link -I "$(JAR)" --manifest AndroidManifest.xml \
	    --min-sdk-version 26 --target-sdk-version 29 -o $@ $(OUT)/res.zip

# aapt2 can't add entries afterwards: inject the .so and dex into a fresh
# copy of the apk, both stored uncompressed
$(APK): $(UNALIGNED) $(SO) $(DEX) | toolcheck
	cp $(UNALIGNED) $(OUT)/injected.apk
	python3 -c 'import sys, zipfile; \
	    apk, so, dex = sys.argv[1:4]; \
	    z = zipfile.ZipFile(apk, "a", zipfile.ZIP_STORED); \
	    z.writestr("lib/arm64-v8a/libvrhome.so", open(so, "rb").read()); \
	    z.writestr("classes.dex", open(dex, "rb").read()); \
	    z.close()' $(OUT)/injected.apk $(SO) $(DEX)
	$(ZIPALIGN) -f 4 $(OUT)/injected.apk $(APK)
	@if [ -f "$(KEYS)/platform.pk8" ] && [ -f "$(KEYS)/platform.x509.pem" ]; then \
	    $(APKSIGNER) sign --key "$(KEYS)/platform.pk8" \
	        --cert "$(KEYS)/platform.x509.pem" $(APK) && \
	    echo "signed with platform key"; \
	else \
	    [ -f debug.keystore ] || keytool -genkeypair -v \
	        -keystore debug.keystore -alias vrhome -keyalg RSA -keysize 2048 \
	        -validity 10000 -storepass android -keypass android \
	        -dname "CN=vrhome,O=pn2,C=CN"; \
	    $(APKSIGNER) sign --ks debug.keystore --ks-pass pass:android $(APK); \
	    echo "signed with debug key (system perms will NOT be granted)"; \
	fi
	@$(APKSIGNER) verify --print-certs $(APK) | head -2
	@echo "built: $(APK) ($$(stat -c%s $(APK)) bytes)"

$(TEST_BIN): $(TEST_SRCS) $(HOST_SRCS) tests/test.h
	@mkdir -p $(dir $@)
	$(CXX) $(HOST_CXXFLAGS) -o $@ $(TEST_SRCS) $(HOST_SRCS) -lm

test: $(TEST_BIN)
	./$(TEST_BIN)

install: $(APK)
	adb install -r $(APK)

clean:
	rm -rf $(OUT)
