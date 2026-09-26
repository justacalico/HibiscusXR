#pragma once

// Android system property readers with defaults.
float propF(const char* key, float dflt);
int   propI(const char* key, int dflt);

// Copies a string property into out (always NUL-terminated); returns the
// length copied, 0 when unset.
int propS(const char* key, char* out, int outSize);
