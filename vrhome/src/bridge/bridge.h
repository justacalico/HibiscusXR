#pragma once

#include <android_native_app_glue.h>
#include <jni.h>

#include <string>

struct Engine;

// attach the calling thread to the VM and return its env
JNIEnv* threadEnv(android_app* app);

// FindClass on a natively-attached thread only sees the boot classpath; app
// classes must go through the activity's ClassLoader
jclass loadAppClass(JNIEnv* env, jobject activity, const char* name);

// construct ShellBridge and cache every method/field id we call
void initBridge(Engine* e);

// queue an app launch from any thread (LauncherActivity JNI + test hook)
void queueLaunch(const char* pkg);

// drain everything the bridge has queued; run on the render thread
void pumpBridge(Engine* e);

// HOME presses reach us as onNewIntent on PanelActivity - consumed in the
// render loop where the gaze yaw is current
bool takeWantRecenter();
