#pragma once

#include <map>
#include <string>
#include <vector>

// Minimal JSON value for the MCP wire format: stdio transport is one
// compact message per line, so this only needs to parse and write the
// JSON-RPC envelopes MCP sends.
struct Json {
    enum Type { Null, Bool, Num, Str, Arr, Obj };

    Type t = Null;
    bool b = false;
    double n = 0.0;
    std::string s;
    std::vector<Json> a;
    std::map<std::string, Json> o;

    static Json obj() { Json j; j.t = Obj; return j; }
    static Json arr() { Json j; j.t = Arr; return j; }
    static Json str(const std::string& v) { Json j; j.t = Str; j.s = v; return j; }
    static Json num(double v) { Json j; j.t = Num; j.n = v; return j; }
    static Json boolean(bool v) { Json j; j.t = Bool; j.b = v; return j; }

    bool isNull() const { return t == Null; }
    bool isObj() const { return t == Obj; }
    bool isArr() const { return t == Arr; }
    bool isStr() const { return t == Str; }
    bool isNum() const { return t == Num; }
    bool isBool() const { return t == Bool; }

    const Json* get(const std::string& k) const;
    Json& set(const std::string& k, const Json& v);
    void push(const Json& v);

    std::string asStr(const std::string& dflt = "") const {
        return t == Str ? s : dflt;
    }
    double asNum(double dflt = 0.0) const { return t == Num ? n : dflt; }
    bool asBool(bool dflt = false) const { return t == Bool ? b : dflt; }
};

// strict parse: trailing garbage after the top value fails
bool jsonParse(const std::string& in, Json* out);
std::string jsonWrite(const Json& j);
