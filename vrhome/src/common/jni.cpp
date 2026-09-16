#include "jni.h"

JNIEnv* threadEnv(JavaVM* vm) {
    JNIEnv* env = nullptr;
    vm->AttachCurrentThread(&env, nullptr);
    return env;
}

jclass loadAppClass(JNIEnv* env, jobject ctx, const char* name) {
    jclass ctxCls = env->GetObjectClass(ctx);
    jmethodID getCL = env->GetMethodID(ctxCls, "getClassLoader",
                                     "()Ljava/lang/ClassLoader;");
    jobject cl = env->CallObjectMethod(ctx, getCL);
    jclass clCls = env->FindClass("java/lang/ClassLoader");
    jmethodID load = env->GetMethodID(clCls, "loadClass",
                                      "(Ljava/lang/String;)Ljava/lang/Class;");
    jstring jn = env->NewStringUTF(name);
    jclass c = (jclass)env->CallObjectMethod(cl, load, jn);
    env->DeleteLocalRef(jn);
    return c;
}
