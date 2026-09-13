#include "props.h"

#include <sys/system_properties.h>
#include <cstdlib>

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
