#include "icontheme.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <map>

namespace fs = std::filesystem;

static bool allDigits(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) if (c < '0' || c > '9') return false;
    return true;
}

bool parseIndexTheme(const std::string& text, IconTheme* out) {
    out->parents.clear();
    out->subs.clear();
    out->valid = false;
    bool inTheme = false;
    IconSubdir* cur = nullptr;
    std::map<std::string, IconSubdir> subs;
    std::vector<std::string> order;
    size_t at = 0;
    auto trim = [](std::string s) {
        const size_t b = s.find_first_not_of(" \t\r");
        const size_t e = s.find_last_not_of(" \t\r");
        return b == std::string::npos ? "" : s.substr(b, e - b + 1);
    };
    while (at <= text.size()) {
        size_t eol = text.find('\n', at);
        if (eol == std::string::npos) eol = text.size();
        const std::string line = trim(text.substr(at, eol - at));
        at = eol + 1;
        if (line.empty() || line[0] == '#') continue;
        if (line.front() == '[' && line.back() == ']') {
            const std::string g = line.substr(1, line.size() - 2);
            inTheme = g == "Icon Theme";
            cur = nullptr;
            if (!inTheme && subs.count(g) == 0) {
                subs[g].dir = g;
                order.push_back(g);
            }
            if (!inTheme) cur = &subs[g];
            continue;
        }
        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = trim(line.substr(0, eq));
        const std::string val = trim(line.substr(eq + 1));
        if (inTheme) {
            out->valid = true;
            if (key == "Inherits") {
                size_t p = 0;
                while (p <= val.size()) {
                    const size_t c = val.find(',', p);
                    const std::string t = trim(val.substr(p,
                        c == std::string::npos ? c : c - p));
                    if (!t.empty()) out->parents.push_back(t);
                    if (c == std::string::npos) break;
                    p = c + 1;
                }
            }
            continue;
        }
        if (!cur) continue;
        const int n = atoi(val.c_str());
        if (key == "Size") cur->size = n;
        else if (key == "Scale") cur->scale = n > 0 ? n : 1;
        else if (key == "MaxSize") cur->maxSize = n;
        else if (key == "MinSize") cur->minSize = n;
        else if (key == "Threshold") cur->threshold = n;
        else if (key == "Type")
            cur->type = val == "Scalable" ? 1 : val == "Threshold" ? 2 : 0;
    }
    for (const std::string& d : order) out->subs.push_back(subs[d]);
    for (IconSubdir& s : out->subs) {
        if (!s.minSize) s.minSize = s.size;
        if (!s.maxSize) s.maxSize = s.size;
    }
    return out->valid;
}

// 0 = usable under the spec's match rule; positive = distance for the
// closest-size fallback
int iconSizeFit(const IconSubdir& s, int want) {
    const int w = want / (s.scale > 0 ? s.scale : 1);
    if (s.type == 0)
        return s.size == w ? 0 : abs(s.size - w) + 1000;
    if (s.type == 1) {
        if (w >= s.minSize && w <= s.maxSize) return 0;
        return w < s.minSize ? s.minSize - w : w - s.maxSize;
    }
    return abs(s.size - w) <= s.threshold ? 0
                                        : abs(s.size - w) + 1000;
}

bool subdirFromName(const std::string& name, IconSubdir* out) {
    std::string base = name;
    out->scale = 1;
    const size_t at = name.find('@');
    if (at != std::string::npos) {
        base = name.substr(0, at);
        std::string s = name.substr(at + 1);
        if (!s.empty() && s.back() == 'x') s.pop_back();
        if (!allDigits(s)) return false;
        out->scale = atoi(s.c_str());
        if (out->scale <= 0) return false;
    }
    if (base == "scalable") {
        out->type = 1;
        out->size = 0;
        out->minSize = 16;
        out->maxSize = 1024;
        return true;
    }
    const size_t x = base.find('x');
    if (x == std::string::npos || !allDigits(base.substr(0, x)) ||
            !allDigits(base.substr(x + 1))) return false;
    out->type = 0;
    out->size = atoi(base.c_str());
    out->minSize = out->maxSize = out->size;
    return true;
}

static const char* kIconExts[] = {".png", ".svg", ".xpm"};

static std::string tryFile(const std::string& dir, const std::string& name,
                           const char* ext) {
    if (ext) {
        const std::string p = dir + "/" + name + ext;
        std::error_code ec;
        return fs::is_regular_file(p, ec) ? p : "";
    }
    for (const char* e : kIconExts) {
        const std::string p = dir + "/" + name + e;
        std::error_code ec;
        if (fs::is_regular_file(p, ec)) return p;
    }
    return "";
}

// a theme dir with no index.theme still counts (Steam fills
// ~/.local/share/icons/hicolor without one): the "<size>x<size>" and
// "scalable" directory names carry the same info the index would
static bool scanThemeDirs(const std::string& dir, IconTheme* out) {
    out->parents.clear();
    out->subs.clear();
    out->valid = false;
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) return false;
    for (const auto& de : fs::directory_iterator(dir, ec)) {
        if (!de.is_directory(ec)) continue;
        IconSubdir s;
        const std::string name = de.path().filename().string();
        if (!subdirFromName(name, &s)) continue;
        s.dir = name;
        out->subs.push_back(s);
        for (const auto& ch : fs::directory_iterator(de.path(), ec)) {
            if (!ch.is_directory(ec)) continue;
            IconSubdir cs = s;
            cs.dir = name + "/" + ch.path().filename().string();
            out->subs.push_back(cs);
        }
    }
    // scalable fits any size, so put it last: a sized png in a fixed dir
    // must win over a same-named svg the dash cannot decode anyway
    std::stable_partition(out->subs.begin(), out->subs.end(),
                          [](const IconSubdir& s) { return s.type != 1; });
    out->valid = !out->subs.empty();
    return out->valid;
}

static bool loadTheme(const std::string& root, const std::string& theme,
                      IconTheme* out) {
    const std::string p = root + "/" + theme + "/index.theme";
    std::error_code ec;
    if (!fs::is_regular_file(p, ec))
        return scanThemeDirs(root + "/" + theme, out);
    FILE* f = fopen(p.c_str(), "rb");
    if (!f) return false;
    std::string text;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) text.append(buf, n);
    fclose(f);
    return parseIndexTheme(text, out);
}

std::string findIconFile(const std::vector<std::string>& roots,
                         const std::vector<std::string>& themesIn,
                         const std::string& name, int size,
                         const char* ext) {
    if (name.empty()) return "";
    if (name.front() == '/') {
        std::error_code ec;
        if (ext) {
            const size_t n = strlen(ext);
            if (name.size() < n ||
                name.compare(name.size() - n, n, ext) != 0) return "";
        }
        return fs::is_regular_file(name, ec) ? name : "";
    }
    std::vector<std::string> themes = themesIn;
    themes.push_back("hicolor");
    // index.theme parse cache: root/theme pairs repeat across subdir
    // loops, so keep them for the whole call
    std::map<std::string, IconTheme> cache;
    for (int pass = 0; pass < 2; ++pass) {
        for (size_t ti = 0; ti < themes.size(); ++ti) {
            const std::string theme = themes[ti];
            for (const std::string& root : roots) {
                const std::string key = root + "|" + theme;
                if (cache.count(key) == 0) {
                    IconTheme t;
                    if (!loadTheme(root, theme, &t)) t.valid = false;
                    cache[key] = t;
                    for (const std::string& par : t.parents) {
                        bool seen = false;
                        for (const std::string& x : themes)
                            if (x == par) { seen = true; break; }
                        if (!seen) themes.push_back(par);
                    }
                }
                const IconTheme& t = cache[key];
                if (!t.valid) continue;
                // exact-fit dirs first, then every other dir in
                // closest-size order until one actually holds the icon
                std::vector<std::pair<int, std::string>> near;
                for (const IconSubdir& s : t.subs) {
                    const int fit = iconSizeFit(s, size);
                    const std::string dir =
                        root + "/" + theme + "/" + s.dir;
                    if (fit == 0) {
                        const std::string p = tryFile(dir, name, ext);
                        if (!p.empty()) return p;
                    } else if (pass == 1) {
                        near.emplace_back(fit, dir);
                    }
                }
                std::stable_sort(near.begin(), near.end(),
                    [](const std::pair<int, std::string>& a,
                       const std::pair<int, std::string>& b) {
                        return a.first < b.first;
                    });
                for (const auto& cd : near) {
                    const std::string p = tryFile(cd.second, name, ext);
                    if (!p.empty()) return p;
                }
            }
        }
    }
    for (const std::string& root : roots) {
        const std::string p = tryFile(root, name, ext);
        if (!p.empty()) return p;
    }
    return "";
}

static std::string envOr(const char* var, const std::string& fallback) {
    const char* v = getenv(var);
    return (v && *v) ? v : fallback;
}

static bool readText(const std::string& path, std::string* out) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) out->append(buf, n);
    fclose(f);
    return true;
}

std::string confGroupValue(const std::string& text,
                           const std::string& group,
                           const std::string& key) {
    auto trim = [](std::string s) {
        const size_t b = s.find_first_not_of(" \t\r");
        const size_t e = s.find_last_not_of(" \t\r");
        return b == std::string::npos ? "" : s.substr(b, e - b + 1);
    };
    std::string cur;
    size_t at = 0;
    while (at <= text.size()) {
        size_t eol = text.find('\n', at);
        if (eol == std::string::npos) eol = text.size();
        const std::string line = trim(text.substr(at, eol - at));
        at = eol + 1;
        if (line.empty() || line[0] == '#') continue;
        if (line.front() == '[' && line.back() == ']') {
            cur = line.substr(1, line.size() - 2);
            continue;
        }
        if (cur != group) continue;
        const size_t eq = line.find('=');
        if (eq == std::string::npos || trim(line.substr(0, eq)) != key)
            continue;
        std::string v = trim(line.substr(eq + 1));
        if (v.size() > 1 &&
                ((v.front() == '"' && v.back() == '"') ||
                 (v.front() == '\'' && v.back() == '\'')))
            v = v.substr(1, v.size() - 2);
        return v;
    }
    return "";
}

// kdeglobals, gtk3/4 settings.ini and .gtkrc-2.0 all carry the icon
// theme under their own key; first hit wins, then the built-in
// fallbacks fill out the rest of the chain
std::vector<std::string> iconThemeNames() {
    std::vector<std::string> names;
    auto add = [&names](const std::string& t) {
        if (t.empty()) return;
        for (const std::string& x : names) if (x == t) return;
        names.push_back(t);
    };
    const std::string cfg = envOr("XDG_CONFIG_HOME",
                                  envOr("HOME", "/tmp") + "/.config");
    const std::string home = envOr("HOME", "/tmp");
    const char* spots[][3] = {
        {"/kdeglobals", "Icons", "Theme"},
        {"/gtk-4.0/settings.ini", "Settings", "gtk-icon-theme-name"},
        {"/gtk-3.0/settings.ini", "Settings", "gtk-icon-theme-name"},
    };
    std::string text;
    for (const auto& s : spots) {
        text.clear();
        if (readText(cfg + s[0], &text))
            add(confGroupValue(text, s[1], s[2]));
    }
    text.clear();
    if (readText(home + "/.gtkrc-2.0", &text))
        add(confGroupValue(text, "", "gtk-icon-theme-name"));
    add("Papirus");
    add("Adwaita");
    add("breeze");
    return names;
}

std::vector<std::string> iconSearchRoots() {
    std::vector<std::string> roots;
    const std::string homeDir = envOr("HOME", "/tmp");
    roots.push_back(envOr("XDG_DATA_HOME",
                          homeDir + "/.local/share") + "/icons");
    roots.push_back(homeDir + "/.icons");
    const std::string xdirs =
        envOr("XDG_DATA_DIRS", "/usr/local/share:/usr/share");
    size_t at = 0;
    while (at <= xdirs.size()) {
        const size_t colon = xdirs.find(':', at);
        const std::string d = xdirs.substr(at,
            colon == std::string::npos ? colon : colon - at);
        if (!d.empty()) roots.push_back(d + "/icons");
        if (colon == std::string::npos) break;
        at = colon + 1;
    }
    roots.push_back("/usr/share/pixmaps");
    return roots;
}
