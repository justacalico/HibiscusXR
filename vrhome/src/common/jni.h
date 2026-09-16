#pragma once

#include <jni.h>

// attach the calling thread to the VM and return its env
JNIEnv* threadEnv(JavaVM* vm);

// FindClass on a natively-attached thread only sees the boot classpath; app
// classes must go through the context's own ClassLoader
jclass loadAppClass(JNIEnv* env, jobject ctx, const char* name);
