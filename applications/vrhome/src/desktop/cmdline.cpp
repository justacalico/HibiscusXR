#include "cmdline.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

std::string shellJoin(const std::vector<std::string>& argv) {
    std::string out;
    for (const std::string& a : argv) {
        if (!out.empty()) out += ' ';
        out += '\'';
        for (char c : a) {
            if (c == '\'') out += "'\\''";
            else out += c;
        }
        out += '\'';
    }
    return out;
}

// emulators worth trying, in order; flag is how each takes the command
static const struct { const char* name; const char* flag; } kTerms[] = {
    {"foot", "--"},        {"kitty", "--"},      {"alacritty", "-e"},
    {"kgx", "--"},         {"gnome-terminal", "--"}, {"konsole", "-e"},
    {"xfce4-terminal", "-e"}, {"lxterminal", "-e"}, {"xterm", "-e"},
};

std::vector<std::string> terminalWrap(const std::string& term,
                                      const std::vector<std::string>& argv) {
    std::vector<std::string> out;
    out.push_back(term);
    for (const auto& t : kTerms)
        if (term == t.name) { out.push_back(t.flag); break; }
    out.insert(out.end(), argv.begin(), argv.end());
    return out;
}

static bool onPath(const char* name) {
    const char* path = getenv("PATH");
    if (!path) return false;
    std::string cand;
    for (const char* at = path;;) {
        const char* colon = strchr(at, ':');
        const size_t len = colon ? (size_t)(colon - at) : strlen(at);
        cand.assign(at, len);
        cand += '/';
        cand += name;
        if (access(cand.c_str(), X_OK) == 0) return true;
        if (!colon) return false;
        at = colon + 1;
    }
}

std::string findTerminal() {
    for (const auto& t : kTerms)
        if (onPath(t.name)) return t.name;
    return "";
}

bool argFlag(int argc, char** argv, const char* name) {
    for (int i = 1; i < argc; ++i)
        if (!strcmp(argv[i], name)) return true;
    return false;
}

// vars that would let a client open windows or run activations on the
// host session instead of on this compositor
static const char* kConfinedEnv[] = {
    "DISPLAY", "XAUTHORITY", "WAYLAND_DISPLAY", "WAYLAND_SOCKET",
    "MIR_SOCKET",
    "DBUS_SESSION_BUS_ADDRESS", "DBUS_STARTER_BUS_TYPE",
    "DBUS_STARTER_ADDRESS", "SESSION_MANAGER", "DESKTOP_STARTUP_ID",
    "GDK_BACKEND", "QT_QPA_PLATFORM", "SDL_VIDEODRIVER",
    "CLUTTER_BACKEND", "MOZ_ENABLE_WAYLAND", "MOZ_DISABLE_WAYLAND",
    "ELECTRON_OZONE_PLATFORM_HINT", "OZONE_PLATFORM",
    "WINIT_UNIX_BACKEND", "XDG_SESSION_TYPE",
};

bool confinedEnvName(const std::string& name) {
    for (const char* k : kConfinedEnv)
        if (name == k) return true;
    return false;
}

std::vector<std::string> spawnEnvAssigns(const std::string& waylandSock,
                                         const std::string& sessionBus) {
    return {
        "WAYLAND_DISPLAY=" + waylandSock,
        "XDG_SESSION_TYPE=wayland",
        "GDK_BACKEND=wayland",
        "QT_QPA_PLATFORM=wayland",
        "CLUTTER_BACKEND=wayland",
        "SDL_VIDEODRIVER=wayland",
        "ELECTRON_OZONE_PLATFORM_HINT=wayland",
        "WINIT_UNIX_BACKEND=wayland",
        "OZONE_PLATFORM=wayland",
        "MOZ_ENABLE_WAYLAND=1",
        "DBUS_SESSION_BUS_ADDRESS=" + sessionBus,
        "DISPLAY=",
        "XAUTHORITY=",
        "WAYLAND_SOCKET=",
        "MIR_SOCKET=",
        "SESSION_MANAGER=",
        "DESKTOP_STARTUP_ID=",
        "DBUS_STARTER_BUS_TYPE=",
        "DBUS_STARTER_ADDRESS=",
        "MOZ_DISABLE_WAYLAND=",
    };
}

std::string sessionBusAddr(const std::string& out, pid_t* pid) {
    std::string addr;
    *pid = -1;
    size_t at = 0;
    while (at <= out.size()) {
        const size_t eol = out.find('\n', at);
        const std::string line = out.substr(at,
            eol == std::string::npos ? eol : eol - at);
        at = eol == std::string::npos ? out.size() + 1 : eol + 1;
        if (line.empty()) continue;
        bool digits = true;
        for (const char ch : line)
            if (!isdigit((unsigned char)ch)) { digits = false; break; }
        if (digits) *pid = (pid_t)strtol(line.c_str(), nullptr, 10);
        else if (addr.empty()) addr = line;
    }
    return addr;
}

static std::string argBase(const std::string& path) {
    const size_t slash = path.rfind('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

static bool envAssignName(const std::string& arg, std::string* name) {
    const size_t eq = arg.find('=');
    if (eq == 0 || eq == std::string::npos) return false;
    for (size_t i = 0; i < eq; ++i) {
        const char c = arg[i];
        if (!isalnum((unsigned char)c) && c != '_') return false;
    }
    *name = arg.substr(0, eq);
    return true;
}

// env -S tokenisation: whitespace outside quotes splits, '..' and ".."
// group, backslash escapes; \c ends the payload
static std::vector<std::string> envSplit(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    char quote = 0;
    bool any = false;
    for (size_t i = 0; i < s.size(); ++i) {
        const char c = s[i];
        if (quote == '\'') {
            if (c == '\'') quote = 0;
            else cur += c;
            continue;
        }
        if (quote == '"') {
            if (c == '"') quote = 0;
            else if (c == '\\' && i + 1 < s.size()) cur += s[++i];
            else cur += c;
            continue;
        }
        if (c == '\'' || c == '"') { quote = c; any = true; continue; }
        if (c == '\\' && i + 1 < s.size()) {
            const char e = s[++i];
            if (e == 'c') return out;
            if (e == 'n') cur += '\n';
            else if (e == 't') cur += '\t';
            else if (e == 'r') cur += '\r';
            else if (e == 'v') cur += '\v';
            else if (e == 'f') cur += '\f';
            else cur += e;
            any = true;
            continue;
        }
        if (isspace((unsigned char)c)) {
            if (any || !cur.empty()) {
                out.push_back(cur);
                cur.clear();
                any = false;
            }
            continue;
        }
        cur += c;
        any = true;
    }
    if (any || !cur.empty()) out.push_back(cur);
    return out;
}

static std::vector<std::string> confineEnvArgv(
        const std::vector<std::string>& argv,
        const std::vector<std::string>& forceEnv) {
    // splice -S payloads so the operand scan sees env's real token stream
    std::vector<std::string> a = {argv[0]};
    for (size_t i = 1; i < argv.size(); ++i) {
        const std::string& t = argv[i];
        std::string payload;
        bool split = true;
        if (t == "-S" || t == "--split-string") {
            if (i + 1 < argv.size()) payload = argv[++i];
        } else if (t.rfind("--split-string=", 0) == 0)
            payload = t.substr(15);
        else if (t.rfind("-S", 0) == 0 && t.size() > 2)
            payload = t.substr(2);
        else
            split = false;
        if (!split) { a.push_back(t); continue; }
        const std::vector<std::string> toks = envSplit(payload);
        a.insert(a.end(), toks.begin(), toks.end());
    }
    // pre-command operands pass through unless they re-point a confined
    // var; the forced assigns land right before the command so nothing
    // in the Exec line can outrank them
    std::vector<std::string> out = {a[0]};
    size_t cmd = a.size();
    for (size_t i = 1; i < a.size(); ++i) {
        const std::string& t = a[i];
        std::string name;
        if (envAssignName(t, &name)) {
            if (!confinedEnvName(name)) out.push_back(t);
            continue;
        }
        if (t == "-u" || t == "--unset" ||
                (t.rfind("-u", 0) == 0 && t.size() > 2 &&
                 t.rfind("--u", 0) != 0) ||
                t.rfind("--unset=", 0) == 0) {
            std::string tgt;
            bool ate = false;
            if (t == "-u" || t == "--unset") {
                if (i + 1 < a.size()) { tgt = a[++i]; ate = true; }
            } else if (t.rfind("--unset=", 0) == 0) tgt = t.substr(8);
            else tgt = t.substr(2);
            if (!confinedEnvName(tgt)) {
                out.push_back(t);
                if (ate) out.push_back(tgt);
            }
            continue;
        }
        if (!t.empty() && t[0] == '-') { out.push_back(t); continue; }
        cmd = i;
        break;
    }
    out.insert(out.end(), forceEnv.begin(), forceEnv.end());
    for (size_t i = cmd; i < a.size(); ++i) out.push_back(a[i]);
    return out;
}

// sockets flatpak could hand the sandbox a host display through; the
// injected flags replace any Exec-line socket flags naming these
static bool confinedSocket(const std::string& val) {
    const std::string s = val.substr(0, val.find(':'));
    return s == "x11" || s == "fallback-x11" || s == "wayland";
}

static std::vector<std::string> confineFlatpak(
        const std::vector<std::string>& argv) {
    size_t i = 1;
    while (i < argv.size() && !argv[i].empty() && argv[i][0] == '-')
        ++i;
    if (i >= argv.size() || argv[i] != "run") return argv;
    std::vector<std::string> out(argv.begin(), argv.begin() + i + 1);
    out.push_back("--socket=wayland");
    out.push_back("--nosocket=x11");
    out.push_back("--nosocket=fallback-x11");
    // flatpak eats option-shaped args anywhere before '--', even after
    // the app id, so the whole tail gets filtered
    for (size_t j = i + 1; j < argv.size(); ++j) {
        const std::string& t = argv[j];
        if (t == "--") {
            out.insert(out.end(), argv.begin() + j, argv.end());
            break;
        }
        std::string name;
        if (t.empty() || t[0] != '-' || envAssignName(t, &name)) {
            out.push_back(t);
            continue;
        }
        std::string opt = t, val;
        bool hasVal = false;
        const size_t eq = t.find('=');
        if (eq != std::string::npos) {
            opt = t.substr(0, eq);
            val = t.substr(eq + 1);
            hasVal = true;
        }
        const bool envOpt = opt == "--env" || opt == "--unset-env";
        const bool sockOpt = opt == "--socket" || opt == "--nosocket" ||
                             opt == "--socket-if";
        if (!envOpt && !sockOpt) { out.push_back(t); continue; }
        const bool splitForm = !hasVal && j + 1 < argv.size();
        if (splitForm) { val = argv[++j]; hasVal = true; }
        const bool drop = envOpt
            ? hasVal && confinedEnvName(val.substr(0, val.find('=')))
            : hasVal && confinedSocket(val);
        if (drop) continue;
        out.push_back(t);
        if (splitForm) out.push_back(val);
    }
    return out;
}

std::vector<std::string> confineExec(
        const std::vector<std::string>& argv,
        const std::vector<std::string>& forceEnv) {
    if (argv.empty()) return argv;
    const std::string prog = argBase(argv[0]);
    if (prog == "env") return confineEnvArgv(argv, forceEnv);
    if (prog == "flatpak") return confineFlatpak(argv);
    return argv;
}
