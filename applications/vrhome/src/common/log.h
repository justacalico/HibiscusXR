#pragma once

#ifdef __ANDROID__
#include <android/log.h>
#define TAG "vrhome"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#else
#include <cstdio>
#define LOGI(...) do { fprintf(stderr, "vrhome: " ); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } while (0)
#define LOGE(...) do { fprintf(stderr, "vrhome: " ); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } while (0)
#endif
