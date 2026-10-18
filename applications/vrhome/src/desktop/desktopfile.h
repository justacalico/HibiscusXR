#pragma once

#include <string>
#include <vector>

// One launchable app read from a freedesktop .desktop file. argv is the
// Exec line already tokenised and field-code expanded.
struct DeskEntry {
    std::string id;        // spec desktop id, "org.foo.Bar.desktop"
    std::string name;
    std::string icon;      // themed name or absolute path
    std::string wmClass;   // StartupWMClass / app_id to expect
    std::vector<std::string> argv;
    bool terminal = false;
};

bool parseDesktopFile(const std::string& text, const std::string& id,
                      DeskEntry* out);
std::vector<std::string> execArgv(const std::string& exec,
                                  const std::string& name,
                                  const std::string& icon,
                                  const std::string& path);

// the applications/ dirs in XDG priority order (data home first)
std::vector<std::string> desktopAppDirs();
// scans every dir in order; first id wins like the spec's lookup does.
// Entries with Hidden/NoDisplay or no Exec are skipped.
std::vector<DeskEntry> scanDesktopFiles(
    const std::vector<std::string>& dirs);
