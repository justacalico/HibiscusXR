#pragma once

#include <string>
#include <vector>

// one [subdir] group from an index.theme file. type mirrors the spec:
// 0 Fixed, 1 Scalable, 2 Threshold
struct IconSubdir {
    std::string dir;
    int size = 0;
    int scale = 1;
    int type = 0;
    int minSize = 0;
    int maxSize = 0;
    int threshold = 2;
};

struct IconTheme {
    std::vector<std::string> parents;
    std::vector<IconSubdir> subs;
    bool valid = false;
};

bool parseIndexTheme(const std::string& text, IconTheme* out);

// spec lookup: 0 wants exact/allowed fit, anything larger is the
// "minimal distance" fallback score - lower is closer
int iconSizeFit(const IconSubdir& s, int want);

// roots are icon search dirs in order ($XDG_DATA_HOME/icons, ~/.icons,
// $XDG_DATA_DIRS/icons, /usr/share/pixmaps). themes lists the wanted
// theme plus fallbacks; "hicolor" is appended automatically. Returns the
// icon file path or "".
std::string findIconFile(const std::vector<std::string>& roots,
                         const std::vector<std::string>& themes,
                         const std::string& name, int size);

std::vector<std::string> iconSearchRoots();
