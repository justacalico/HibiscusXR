#include "icontheme.h"

#include <cstdlib>
#include <filesystem>
#include <map>

namespace fs = std::filesystem;

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

static const char* kIconExts[] = {".png", ".svg", ".xpm"};

static std::string tryFile(const std::string& dir,
                           const std::string& name) {
    for (const char* ext : kIconExts) {
        const std::string p = dir + "/" + name + ext;
        std::error_code ec;
        if (fs::is_regular_file(p, ec)) return p;
    }
    return "";
}

static bool loadTheme(const std::string& root, const std::string& theme,
                      IconTheme* out) {
    const std::string p = root + "/" + theme + "/index.theme";
    std::error_code ec;
    if (!fs::is_regular_file(p, ec)) return false;
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
                         const std::string& name, int size) {
    if (name.empty()) return "";
    if (name.front() == '/') {
        std::error_code ec;
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
                int bestFit = -1;
                std::string bestDir;
                for (const IconSubdir& s : t.subs) {
                    const int fit = iconSizeFit(s, size);
                    const std::string dir =
                        root + "/" + theme + "/" + s.dir;
                    if (fit == 0) {
                        const std::string p = tryFile(dir, name);
                        if (!p.empty()) return p;
                    } else if (pass == 1 &&
                               (bestFit < 0 || fit < bestFit)) {
                        bestFit = fit;
                        bestDir = dir;
                    }
                }
                if (pass == 1 && !bestDir.empty()) {
                    const std::string p = tryFile(bestDir, name);
                    if (!p.empty()) return p;
                }
            }
        }
    }
    for (const std::string& root : roots) {
        const std::string p = tryFile(root, name);
        if (!p.empty()) return p;
    }
    return "";
}

static std::string envOr(const char* var, const std::string& fallback) {
    const char* v = getenv(var);
    return (v && *v) ? v : fallback;
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
