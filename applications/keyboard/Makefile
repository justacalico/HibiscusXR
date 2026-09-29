# One APK without Gradle: javac -> d8 -> aapt2 -> zipalign -> apksigner.
#
#   out/pn2keyboard.apk  gitlab.neosalsa.keyboard - the floating IME
#
# Env overrides match vrhome's Makefile:
#   ANDROID_SDK_ROOT  Android SDK dir   (default /opt/android-sdk)
#   BT                build-tools dir   (default: newest under $SDK/build-tools)
#   JAR               platform jar      (default: android-29)
#   KEYS              signing key dir   (default ../build/keys - platform.pk8 +
#                                       platform.x509.pem; falls back to a
#                                       generated debug keystore)

SDK  := $(or $(ANDROID_SDK_ROOT),/opt/android-sdk)
BT   ?= $(lastword $(sort $(wildcard $(SDK)/build-tools/*)))
JAR  ?= $(SDK)/platforms/android-29/android.jar
KEYS ?= $(CURDIR)/../build/keys

OUT := out
APK := $(OUT)/pn2keyboard.apk

AAPT2    := $(BT)/aapt2
D8       := $(BT)/d8
ZIPALIGN := $(BT)/zipalign
APKSIGNER := $(BT)/apksigner

JAVA_SRCS := $(shell find java -name '*.java')
RES_SRCS  := $(wildcard res/*/*)

UNALIGNED := $(OUT)/unaligned.apk
DEX       := $(OUT)/dex/classes.dex

.PHONY: all apk clean toolcheck
all: apk
apk: $(APK)

toolcheck:
	@for p in "$(JAR)" "$(AAPT2)" "$(D8)" "$(ZIPALIGN)" "$(APKSIGNER)"; do \
	    [ -e "$$p" ] || { echo "missing: $$p" >&2; exit 1; }; \
	done

$(OUT)/classes/.stamp: $(JAVA_SRCS) $(UNALIGNED) | toolcheck
	@mkdir -p $(OUT)/classes
	javac -source 8 -target 8 -cp "$(JAR)" -d $(OUT)/classes $(JAVA_SRCS) \
	    $(OUT)/gen/gitlab/neosalsa/keyboard/R.java
	@touch $@

$(DEX): $(OUT)/classes/.stamp | toolcheck
	@mkdir -p $(OUT)/dex
	rm -f $@
	$(D8) --classpath "$(JAR)" --output $(OUT)/dex \
	    $(OUT)/classes/gitlab/neosalsa/keyboard/*.class

$(OUT)/res.zip: $(RES_SRCS) | toolcheck
	@mkdir -p $(OUT)
	$(AAPT2) compile --dir res -o $@

$(UNALIGNED): AndroidManifest.xml $(OUT)/res.zip | toolcheck
	@mkdir -p $(OUT)/gen
	$(AAPT2) link -I "$(JAR)" --manifest AndroidManifest.xml \
	    --min-sdk-version 26 --target-sdk-version 29 --java $(OUT)/gen \
	    -o $@ $(OUT)/res.zip

$(APK): $(UNALIGNED) $(DEX) | toolcheck
	cp $(UNALIGNED) $(OUT)/injected.apk
	python3 -c 'import sys, zipfile; \
	    apk, dex = sys.argv[1:3]; \
	    z = zipfile.ZipFile(apk, "a", zipfile.ZIP_STORED); \
	    z.writestr("classes.dex", open(dex, "rb").read()); \
	    z.close()' $(OUT)/injected.apk $(DEX)
	$(ZIPALIGN) -f 4 $(OUT)/injected.apk $@
	@if [ -f "$(KEYS)/platform.pk8" ] && [ -f "$(KEYS)/platform.x509.pem" ]; then \
	    $(APKSIGNER) sign --key "$(KEYS)/platform.pk8" \
	        --cert "$(KEYS)/platform.x509.pem" $@ && \
	    echo "$@: signed with platform key"; \
	else \
	    [ -f debug.keystore ] || keytool -genkeypair -v \
	        -keystore debug.keystore -alias keyboard -keyalg RSA -keysize 2048 \
	        -validity 10000 -storepass android -keypass android \
	        -dname "CN=keyboard,O=pn2,C=CN"; \
	    $(APKSIGNER) sign --ks debug.keystore --ks-pass pass:android $@; \
	    echo "$@: signed with debug key"; \
	fi
	@echo "built: $@ ($$(stat -c%s $@) bytes)"

install: $(APK)
	adb install -r $(APK)

clean:
	rm -rf $(OUT)
