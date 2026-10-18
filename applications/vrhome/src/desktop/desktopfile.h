#pragma once

#include <string>
#include <vector>
#include <sys/types.h>

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

// strips the ".desktop" suffix off a desktop file id
std::string deskIdBase(const std::string& id);

// an xdg app_id vs a launch slot's candidates - StartupWMClass and the
// bare desktop id (what sandboxed toolkits report). Exact match, or a
// whole-segment prefix either way.
bool appIdMatch(const std::string& appId, const std::string& wmClass,
                const std::string& deskId);

// wl client pid vs the spawn pid: direct hit, or the client sits in the
// launcher's process group - spawn() setsid()s, so wrappers like
// flatpak keep the group through their fork/exec chain
bool spawnPidMatch(pid_t clientPid, pid_t clientPgid, pid_t spawnPid);
