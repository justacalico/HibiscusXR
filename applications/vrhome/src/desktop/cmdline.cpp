#include "cmdline.h"

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
