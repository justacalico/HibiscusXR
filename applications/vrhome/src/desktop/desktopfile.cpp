#include "desktopfile.h"

#include <cstdlib>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

// Exec tokenisation: double quotes group, backslash escapes the next
// char inside and outside quotes
static std::vector<std::string> execTokens(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    bool quoted = false, esc = false, any = false;
    auto push = [&] {
        if (any || !cur.empty()) out.push_back(cur);
        cur.clear(); any = false;
    };
    for (char c : s) {
        if (esc) { cur += c; esc = false; any = true; continue; }
        if (c == '\\') { esc = true; continue; }
        if (quoted) {
            if (c == '"') quoted = false;
            else cur += c;
            any = true;
            continue;
        }
        if (c == '"') { quoted = true; any = true; continue; }
        if (c == ' ' || c == '\t') { push(); continue; }
        cur += c; any = true;
    }
    push();
    return out;
}

// expands the spec field codes: %f %F %u %U %d %D %n %N %v %m drop away
// (no file/url args when the dash spawns it), %i adds --icon, %c the
// name, %k the .desktop path, %% a literal percent
std::vector<std::string> execArgv(const std::string& exec,
                                  const std::string& name,
                                  const std::string& icon,
                                  const std::string& path) {
    std::vector<std::string> out;
    for (std::string t : execTokens(exec)) {
        std::string t2;
        for (size_t i = 0; i < t.size(); ++i) {
            if (t[i] != '%' || i + 1 >= t.size()) { t2 += t[i]; continue; }
            const char code = t[++i];
            if (code == '%') t2 += '%';
            else if (code == 'i') {
                if (!icon.empty()) {
                    if (!t2.empty()) { out.push_back(t2); t2.clear(); }
                    out.push_back("--icon");
                    out.push_back(icon);
                }
            } else if (code == 'c') t2 += name;
            else if (code == 'k') t2 += path;
        }
        if (!t2.empty()) out.push_back(t2);
    }
    return out;
}

static bool flagVal(const std::string& v) {
    return v == "true" || v == "1" || v == "yes";
}

bool parseDesktopFile(const std::string& text, const std::string& id,
                      DeskEntry* out) {
    bool inEntry = false;
    std::string type, exec, name, icon, wmClass;
    bool hidden = false, noDisplay = false, term = false;
    size_t at = 0;
    while (at <= text.size()) {
        size_t eol = text.find('\n', at);
        if (eol == std::string::npos) eol = text.size();
        std::string line = text.substr(at, eol - at);
        at = eol + 1;
        while (!line.empty() &&
               (line.back() == '\r' || line.back() == ' ' ||
                line.back() == '\t'))
            line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        if (line[0] == '[') {
            inEntry = line == "[Desktop Entry]";
            continue;
        }
        if (!inEntry) continue;
        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        // locale-suffixed keys ([de] etc.) lose to the plain key
        const std::string key = line.substr(0, eq);
        const std::string val = line.substr(eq + 1);
        if (key == "Type") type = val;
        else if (key == "Exec") exec = val;
        else if (key == "Name") { if (name.empty()) name = val; }
        else if (key.rfind("Name[", 0) == 0 && name.empty()) name = val;
        else if (key == "Icon") icon = val;
        else if (key == "StartupWMClass") wmClass = val;
        else if (key == "Hidden") hidden = flagVal(val);
        else if (key == "NoDisplay") noDisplay = flagVal(val);
        else if (key == "Terminal") term = flagVal(val);
        else if (key == "TryExec") {
            if (val.empty() || val[0] == '/') {
                if (val.empty() || !fs::exists(val)) return false;
            }
        }
    }
    if (type != "Application" || exec.empty() || hidden || noDisplay)
        return false;
    out->id = id;
    out->name = name.empty() ? id : name;
    out->icon = icon;
    out->wmClass = wmClass;
    out->terminal = term;
    out->argv = execArgv(exec, name, icon, id);
    return true;
}

static std::string envOr(const char* var, const std::string& fallback) {
    const char* v = getenv(var);
    return (v && *v) ? v : fallback;
}

std::vector<std::string> desktopAppDirs() {
    std::vector<std::string> dirs;
    const std::string home = envOr("XDG_DATA_HOME",
        envOr("HOME", "/tmp") + "/.local/share");
    dirs.push_back(home + "/applications");
    const std::string xdirs =
        envOr("XDG_DATA_DIRS", "/usr/local/share:/usr/share");
    size_t at = 0;
    while (at <= xdirs.size()) {
        const size_t colon = xdirs.find(':', at);
        const std::string d = xdirs.substr(at,
            colon == std::string::npos ? colon : colon - at);
        if (!d.empty()) dirs.push_back(d + "/applications");
        if (colon == std::string::npos) break;
        at = colon + 1;
    }
    return dirs;
}

std::vector<DeskEntry> scanDesktopFiles(
        const std::vector<std::string>& dirs) {
    std::vector<DeskEntry> out;
    for (const std::string& dir : dirs) {
        std::error_code ec;
        if (!fs::is_directory(dir, ec)) continue;
        for (const auto& de : fs::directory_iterator(dir, ec)) {
            if (!de.is_regular_file()) continue;
            const std::string file = de.path().filename().string();
            if (file.size() < 9 ||
                file.compare(file.size() - 8, 8, ".desktop") != 0)
                continue;
            const std::string path = de.path().string();
            const std::string id = file;
            bool dup = false;
            for (const DeskEntry& e : out)
                if (e.id == id) { dup = true; break; }
            if (dup) continue;
            FILE* f = fopen(path.c_str(), "rb");
            if (!f) continue;
            std::string text;
            char buf[4096];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
                text.append(buf, n);
            fclose(f);
            DeskEntry e;
            if (parseDesktopFile(text, id, &e)) out.push_back(e);
        }
    }
    return out;
}

std::string deskIdBase(const std::string& id) {
    const size_t dot = id.rfind(".desktop");
    return dot == std::string::npos ? id : id.substr(0, dot);
}

// "org.foo.Bar" matches "org.foo" and "org.foo.Bar.Baz", only on whole
// dot segments so sibling ids can't cross-bind
static bool segMatch(const std::string& a, const std::string& b) {
    if (a.empty() || b.empty()) return false;
    if (a == b) return true;
    return a.find(b + ".") == 0 || b.find(a + ".") == 0;
}

bool appIdMatch(const std::string& appId, const std::string& wmClass,
                const std::string& deskId) {
    return !appId.empty() &&
        (segMatch(appId, wmClass) || segMatch(appId, deskId));
}

bool spawnPidMatch(pid_t clientPid, pid_t clientPgid, pid_t spawnPid) {
    if (clientPid <= 0 || spawnPid <= 0) return false;
    return clientPid == spawnPid || clientPgid == spawnPid;
}
