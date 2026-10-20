#include "sysinfo.h"

#include <cstdio>
#include <cstdlib>
#include <functional>

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

static void eachLine(const std::string& text,
                     const std::function<void(const std::string&)>& fn) {
    size_t at = 0;
    while (at <= text.size()) {
        const size_t eol = text.find('\n', at);
        fn(text.substr(at, eol == std::string::npos ? eol : eol - at));
        at = eol == std::string::npos ? text.size() + 1 : eol + 1;
    }
}

static std::string unquote(const std::string& s) {
    const std::string t = trimmed(s);
    return t.size() >= 2 && t.front() == '"' && t.back() == '"'
        ? t.substr(1, t.size() - 2) : t;
}

std::string parseHibiscusRelease(const std::string& text) {
    std::string ver, bare;
    eachLine(text, [&](const std::string& line) {
        if (line.rfind("VERSION=", 0) == 0)
            ver = unquote(line.substr(8));
        else if (bare.empty())
            bare = trimmed(line);
    });
    return ver.empty() ? bare : ver;
}

std::string parseCpuModel(const std::string& text) {
    std::string out;
    eachLine(text, [&](const std::string& line) {
        if (!out.empty()) return;
        const size_t c = line.find(':');
        if (c == std::string::npos) return;
        const std::string key = trimmed(line.substr(0, c));
        if (key == "model name" || key == "Hardware")
            out = trimmed(line.substr(c + 1));
    });
    return out;
}

std::string parseMemTotal(const std::string& text) {
    long long kb = -1;
    eachLine(text, [&](const std::string& line) {
        if (kb >= 0 || line.rfind("MemTotal:", 0) != 0) return;
        kb = atoll(line.c_str() + 9);
    });
    if (kb <= 0) return "";
    const long long gb = (kb + 512 * 1024) / (1024 * 1024);
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld GB", gb < 1 ? 1 : gb);
    return buf;
}

std::string cleanGpuName(const std::string& name) {
    size_t e = name.find_first_of("(/");
    return trimmed(e == std::string::npos ? name : name.substr(0, e));
}

std::string specLine(const std::string& cpu, const std::string& mem,
                     const std::string& gpu) {
    std::string out;
    for (const std::string* s : {&cpu, &mem, &gpu})
        if (!s->empty()) out += (out.empty() ? "" : ", ") + *s;
    return out;
}

void parseWifiList(const std::string& text,
                   const std::vector<std::string>& cons,
                   std::vector<SetWifi>* out) {
    out->clear();
    eachLine(text, [&](const std::string& line) {
        std::string f[4];
        int nf = 0;
        size_t at = 0;
        while (at <= line.size() && nf < 4) {
            if (at < line.size() && line[at] == '\\' &&
                at + 1 < line.size() && line[at + 1] == ':') {
                f[nf] += ':';
                at += 2;
                continue;
            }
            if (at == line.size() || line[at] == ':') {
                ++nf;
                ++at;
                continue;
            }
            f[nf] += line[at++];
        }
        if (nf < 3 || f[1].empty()) return;
        SetWifi n;
        n.ssid = f[1];
        n.conn = f[0] == "yes";
        const int sig = atoi(f[2].c_str());
        n.level = sig <= 0 ? 0 : sig >= 100 ? 4 : sig * 4 / 100;
        const std::string& s = f[3];
        if (s.find("WEP") != std::string::npos) n.sec = WSEC_WEP;
        else if (s.find("WPA") != std::string::npos ||
                 s.find("PSK") != std::string::npos ||
                 s.find("SAE") != std::string::npos ||
                 s.find("802") != std::string::npos) n.sec = WSEC_WPA;
        for (size_t i = 0; i < cons.size(); ++i)
            if (cons[i] == n.ssid) { n.saved = (int)i; break; }
        out->push_back(n);
    });
}

std::vector<std::string> parseConNames(const std::string& text) {
    std::vector<std::string> out;
    eachLine(text, [&](const std::string& line) {
        const std::string t = trimmed(line);
        if (!t.empty()) out.push_back(t);
    });
    return out;
}

bool parseBtPowered(const std::string& text) {
    bool on = false;
    eachLine(text, [&](const std::string& line) {
        const size_t c = line.find(':');
        if (c != std::string::npos &&
            trimmed(line.substr(0, c)) == "Powered")
            on = trimmed(line.substr(c + 1)) == "yes";
    });
    return on;
}

void parseBtDevs(const std::string& text, const std::string& conns,
                 std::vector<SetBtDev>* out) {
    out->clear();
    auto rows = [&](const std::string& src, bool conn) {
        eachLine(src, [&](const std::string& line) {
            if (line.rfind("Device ", 0) != 0) return;
            const size_t sp = line.find(' ', 7);
            const std::string addr = line.substr(7,
                sp == std::string::npos ? sp : sp - 7);
            if (addr.empty()) return;
            if (conn) {
                for (auto& d : *out)
                    if (d.addr == addr) d.conn = true;
                return;
            }
            SetBtDev d;
            d.addr = addr;
            d.name = sp == std::string::npos ? "" : trimmed(line.substr(sp));
            d.bonded = true;
            out->push_back(d);
        });
    };
    rows(text, false);
    rows(conns, true);
}
