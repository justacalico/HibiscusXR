#include "props.h"

#include <sys/system_properties.h>
#include <cstdlib>
#include <cstring>

float propF(const char* key, float dflt) {
    char b[PROP_VALUE_MAX];
    if (__system_property_get(key, b) > 0) return (float)atof(b);
    return dflt;
}

int propI(const char* key, int dflt) {
    char b[PROP_VALUE_MAX];
    if (__system_property_get(key, b) > 0) return atoi(b);
    return dflt;
}

int propS(const char* key, char* out, int outSize) {
    if (outSize <= 0) return 0;
    char b[PROP_VALUE_MAX];
    const int n = __system_property_get(key, b);
    if (n <= 0) { out[0] = 0; return 0; }
    const int m = n < outSize - 1 ? n : outSize - 1;
    memcpy(out, b, m);
    out[m] = 0;
    return m;
}
