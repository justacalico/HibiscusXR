#pragma once

#ifdef __ANDROID__
#include <jni.h>
#endif

struct HudEngine;

#ifdef __ANDROID__
void initBridge(HudEngine* e, JNIEnv* env, jobject br);
#endif
void queueLaunch(const char* pkg);
void pumpBridge(HudEngine* e);
void syncUiStrings(HudEngine* e);
void wantRecenter();
bool takeWantRecenter();
