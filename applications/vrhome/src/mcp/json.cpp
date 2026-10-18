#include "json.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

const Json* Json::get(const std::string& k) const {
    if (t != Obj) return nullptr;
    auto it = o.find(k);
    return it == o.end() ? nullptr : &it->second;
}

Json& Json::set(const std::string& k, const Json& v) {
    t = Obj;
    o[k] = v;
    return o[k];
}

void Json::push(const Json& v) {
    t = Arr;
    a.push_back(v);
}

namespace {

struct P {
    const char* cur;
    const char* end;
};

void ws(P& p) {
    while (p.cur < p.end && (*p.cur == ' ' || *p.cur == '\t' ||
                             *p.cur == '\r' || *p.cur == '\n'))
        ++p.cur;
}

bool hex4(P& p, unsigned* out) {
    if (p.end - p.cur < 4) return false;
    unsigned v = 0;
    for (int i = 0; i < 4; ++i) {
        const char c = p.cur[i];
        v <<= 4;
        if (c >= '0' && c <= '9') v |= (unsigned)(c - '0');
        else if (c >= 'a' && c <= 'f') v |= (unsigned)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') v |= (unsigned)(c - 'A' + 10);
        else return false;
    }
    p.cur += 4;
    *out = v;
    return true;
}

void utf8(std::string* out, unsigned cp) {
    if (cp < 0x80) {
        out->push_back((char)cp);
    } else if (cp < 0x800) {
        out->push_back((char)(0xC0 | (cp >> 6)));
        out->push_back((char)(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        out->push_back((char)(0xE0 | (cp >> 12)));
        out->push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
        out->push_back((char)(0x80 | (cp & 0x3F)));
    } else {
        out->push_back((char)(0xF0 | (cp >> 18)));
        out->push_back((char)(0x80 | ((cp >> 12) & 0x3F)));
        out->push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
        out->push_back((char)(0x80 | (cp & 0x3F)));
    }
}

bool jstr(P& p, std::string* out) {
    if (p.cur >= p.end || *p.cur != '"') return false;
    ++p.cur;
    out->clear();
    while (p.cur < p.end) {
        const char c = *p.cur++;
        if (c == '"') return true;
        if (c == '\\') {
            if (p.cur >= p.end) return false;
            switch (*p.cur++) {
                case '"': out->push_back('"'); break;
                case '\\': out->push_back('\\'); break;
                case '/': out->push_back('/'); break;
                case 'b': out->push_back('\b'); break;
                case 'f': out->push_back('\f'); break;
                case 'n': out->push_back('\n'); break;
                case 'r': out->push_back('\r'); break;
                case 't': out->push_back('\t'); break;
                case 'u': {
                    unsigned cp;
                    if (!hex4(p, &cp)) return false;
                    if (cp >= 0xD800 && cp < 0xDC00) {
                        // lone or unpaired surrogate: only a following
                        // \uDC00..DFFF completes the pair
                        if (p.end - p.cur >= 2 && p.cur[0] == '\\' &&
                            p.cur[1] == 'u') {
                            p.cur += 2;
                            unsigned lo;
                            if (!hex4(p, &lo) || lo < 0xDC00 || lo > 0xDFFF)
                                return false;
                            cp = 0x10000 + ((cp - 0xD800) << 10) +
                                 (lo - 0xDC00);
                        } else {
                            return false;
                        }
                    }
                    utf8(out, cp);
                    break;
                }
                default: return false;
            }
        } else if ((unsigned char)c < 0x20) {
            return false;
        } else {
            out->push_back(c);
        }
    }
    return false;
}

bool value(P& p, Json* out);

bool jnum(P& p, Json* out) {
    const char* start = p.cur;
    if (p.cur < p.end && *p.cur == '-') ++p.cur;
    if (p.cur >= p.end) return false;
    if (*p.cur == '0') {
        ++p.cur;
    } else if (*p.cur >= '1' && *p.cur <= '9') {
        while (p.cur < p.end && *p.cur >= '0' && *p.cur <= '9') ++p.cur;
    } else {
        return false;
    }
    if (p.cur < p.end && *p.cur == '.') {
        ++p.cur;
        if (p.cur >= p.end || *p.cur < '0' || *p.cur > '9') return false;
        while (p.cur < p.end && *p.cur >= '0' && *p.cur <= '9') ++p.cur;
    }
    if (p.cur < p.end && (*p.cur == 'e' || *p.cur == 'E')) {
        ++p.cur;
        if (p.cur < p.end && (*p.cur == '+' || *p.cur == '-')) ++p.cur;
        if (p.cur >= p.end || *p.cur < '0' || *p.cur > '9') return false;
        while (p.cur < p.end && *p.cur >= '0' && *p.cur <= '9') ++p.cur;
    }
    *out = Json::num(strtod(std::string(start, p.cur).c_str(), nullptr));
    return true;
}

bool jarr(P& p, Json* out) {
    ++p.cur;
    *out = Json::arr();
    ws(p);
    if (p.cur < p.end && *p.cur == ']') { ++p.cur; return true; }
    for (;;) {
        Json v;
        if (!value(p, &v)) return false;
        out->push(v);
        ws(p);
        if (p.cur >= p.end) return false;
        if (*p.cur == ',') { ++p.cur; ws(p); continue; }
        if (*p.cur == ']') { ++p.cur; return true; }
        return false;
    }
}

bool jobj(P& p, Json* out) {
    ++p.cur;
    *out = Json::obj();
    ws(p);
    if (p.cur < p.end && *p.cur == '}') { ++p.cur; return true; }
    for (;;) {
        ws(p);
        std::string k;
        if (!jstr(p, &k)) return false;
        ws(p);
        if (p.cur >= p.end || *p.cur != ':') return false;
        ++p.cur;
        Json v;
        if (!value(p, &v)) return false;
        out->set(k, v);
        ws(p);
        if (p.cur >= p.end) return false;
        if (*p.cur == ',') { ++p.cur; continue; }
        if (*p.cur == '}') { ++p.cur; return true; }
        return false;
    }
}

bool lit(P& p, const char* w) {
    const size_t n = strlen(w);
    if ((size_t)(p.end - p.cur) < n || memcmp(p.cur, w, n)) return false;
    p.cur += n;
    return true;
}

bool value(P& p, Json* out) {
    ws(p);
    if (p.cur >= p.end) return false;
    switch (*p.cur) {
        case '{': return jobj(p, out);
        case '[': return jarr(p, out);
        case '"': {
            std::string s;
            if (!jstr(p, &s)) return false;
            *out = Json::str(s);
            return true;
        }
        case 't':
            if (!lit(p, "true")) return false;
            *out = Json::boolean(true);
            return true;
        case 'f':
            if (!lit(p, "false")) return false;
            *out = Json::boolean(false);
            return true;
        case 'n':
            if (!lit(p, "null")) return false;
            *out = Json();
            return true;
        default:
            return jnum(p, out);
    }
}

void wstr(std::string* out, const std::string& s) {
    out->push_back('"');
    for (const unsigned char c : s) {
        switch (c) {
            case '"': out->append("\\\""); break;
            case '\\': out->append("\\\\"); break;
            case '\b': out->append("\\b"); break;
            case '\f': out->append("\\f"); break;
            case '\n': out->append("\\n"); break;
            case '\r': out->append("\\r"); break;
            case '\t': out->append("\\t"); break;
            default:
                if (c < 0x20) {
                    char b[8];
                    snprintf(b, sizeof(b), "\\u%04x", c);
                    out->append(b);
                } else {
                    out->push_back((char)c);
                }
        }
    }
    out->push_back('"');
}

void wnum(std::string* out, double v) {
    if (!std::isfinite(v)) { out->push_back('0'); return; }
    char b[40];
    snprintf(b, sizeof(b), "%.15g", v);
    if (strtod(b, nullptr) != v) snprintf(b, sizeof(b), "%.17g", v);
    out->append(b);
}

void wval(std::string* out, const Json& j) {
    switch (j.t) {
        case Json::Null: out->append("null"); break;
        case Json::Bool: out->append(j.b ? "true" : "false"); break;
        case Json::Num: wnum(out, j.n); break;
        case Json::Str: wstr(out, j.s); break;
        case Json::Arr: {
            out->push_back('[');
            for (size_t i = 0; i < j.a.size(); ++i) {
                if (i) out->push_back(',');
                wval(out, j.a[i]);
            }
            out->push_back(']');
            break;
        }
        case Json::Obj: {
            out->push_back('{');
            bool first = true;
            for (const auto& kv : j.o) {
                if (!first) out->push_back(',');
                first = false;
                wstr(out, kv.first);
                out->push_back(':');
                wval(out, kv.second);
            }
            out->push_back('}');
            break;
        }
    }
}

} // namespace

bool jsonParse(const std::string& in, Json* out) {
    P p{in.data(), in.data() + in.size()};
    if (!value(p, out)) return false;
    ws(p);
    return p.cur == p.end;
}

std::string jsonWrite(const Json& j) {
    std::string out;
    wval(&out, j);
    return out;
}
