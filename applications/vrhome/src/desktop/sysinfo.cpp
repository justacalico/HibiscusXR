#include "sysinfo.h"

#include <cstdlib>

static std::string trimmed(const std::string& s) {
    const size_t b = s.find_first_not_of(" \t\r\n");
    const size_t e = s.find_last_not_of(" \t\r\n");
    return b == std::string::npos ? "" : s.substr(b, e - b + 1);
}

int parseCapacity(const std::string& text) {
    const std::string t = trimmed(text);
    if (t.empty()) return -1;
    const int v = atoi(t.c_str());
    return v < 0 ? 0 : v > 100 ? 100 : v;
}

bool parseCharging(const std::string& text) {
    const std::string t = trimmed(text);
    return t == "Charging" || t == "Full";
}

std::string parseWifiSsid(const std::string& text) {
    size_t at = 0;
    while (at <= text.size()) {
        const size_t eol = text.find('\n', at);
        const std::string line = text.substr(at,
            eol == std::string::npos ? eol : eol - at);
        at = eol == std::string::npos ? text.size() + 1 : eol + 1;
        if (line.rfind("yes:", 0) == 0) return line.substr(4);
    }
    return "";
}

bool parseNmRadio(const std::string& text) {
    return trimmed(text).rfind("enabled", 0) == 0;
}

bool parseEthLink(const std::string& text) {
    size_t at = 0;
    while (at <= text.size()) {
        const size_t eol = text.find('\n', at);
        const std::string line = text.substr(at,
            eol == std::string::npos ? eol : eol - at);
        at = eol == std::string::npos ? text.size() + 1 : eol + 1;
        if (line.rfind("ethernet:connected", 0) == 0) return true;
    }
    return false;
}

float parseWpctlVol(const std::string& text) {
    const size_t colon = text.find(':');
    if (colon == std::string::npos) return -1.0f;
    return (float)atof(text.c_str() + colon + 1);
}

bool parseWpctlMuted(const std::string& text) {
    return text.find("[MUTED]") != std::string::npos;
}

float parseBrightness(const std::string& text) {
    // dev,class,current,pct,max - pct field carries a trailing %
    size_t p = 0;
    for (int i = 0; i < 3; ++i) {
        p = text.find(',', p);
        if (p == std::string::npos) return -1.0f;
        ++p;
    }
    const size_t e = text.find(',', p);
    const std::string f = text.substr(p,
        e == std::string::npos ? e : e - p);
    const float v = (float)atof(f.c_str());
    return v < 0.0f ? 0.0f : v > 100.0f ? 1.0f : v / 100.0f;
}
