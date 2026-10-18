#include "props.h"

#ifdef __ANDROID__
#include <sys/system_properties.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

static bool propGet(const char* key, char* out, int outSize) {
#ifdef __ANDROID__
    char b[PROP_VALUE_MAX];
    const int n = __system_property_get(key, b);
    if (n <= 0 || n >= outSize) return false;
    memcpy(out, b, n + 1);
    return true;
#else
    const char* v = getenv(key);
    if (!v || !*v) return false;
    snprintf(out, (size_t)outSize, "%s", v);
    return true;
#endif
}

float propF(const char* key, float dflt) {
    char b[256];
    return propGet(key, b, sizeof(b)) ? (float)atof(b) : dflt;
}

int propI(const char* key, int dflt) {
    char b[256];
    return propGet(key, b, sizeof(b)) ? atoi(b) : dflt;
}

int propS(const char* key, char* out, int outSize) {
    if (outSize <= 0) return 0;
    if (!propGet(key, out, outSize)) { out[0] = 0; return 0; }
    return (int)strlen(out);
}
