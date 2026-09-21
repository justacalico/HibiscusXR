#pragma once

#include <jni.h>

struct HudEngine;

// cache the method/field ids we call on an already-built ShellBridge; `br` is
// the java object HudService constructed (it owns the covered-listener wiring)
void initBridge(HudEngine* e, JNIEnv* env, jobject br);

// queue an app launch from any thread (open-package broadcast + test hook)
void queueLaunch(const char* pkg);

// drain everything the bridge has queued; run on the render thread
void pumpBridge(HudEngine* e);

// ask the render thread to recenter the ring on the next frame (summon key
// in home space, plus the automatic recenter when a covered app lets go)
void wantRecenter();
bool takeWantRecenter();
