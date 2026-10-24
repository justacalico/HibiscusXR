#pragma once

#include <string>
#include <vector>
#include <sys/types.h>

// single-quote every arg and join: 'foo' 'it'\''s' - safe for
// /bin/sh -c no matter what the .desktop Exec line produced
std::string shellJoin(const std::vector<std::string>& argv);

// wrap a Terminal=true argv so it runs inside a terminal emulator:
// the flag column covers each emulator's "run this command" syntax
std::vector<std::string> terminalWrap(const std::string& term,
                                      const std::vector<std::string>& argv);

// first terminal emulator found on PATH, "" if none
std::string findTerminal();

// true when argv carries the flag, e.g. argFlag(argc, argv, "-mcp");
// argv[0] is skipped
bool argFlag(int argc, char** argv, const char* name);

// true when name is an env var that can route a client's windows or
// activation back to the host session (display sockets, the session bus
// address, toolkit backend picks)
bool confinedEnvName(const std::string& name);

// the environment every spawned client ends up with: NAME=value forces
// a var, NAME= with an empty value means "must be unset". waylandSock is
// this compositor's socket name, sessionBus the private bus address
// ("" when no private bus is running)
std::vector<std::string> spawnEnvAssigns(const std::string& waylandSock,
                                         const std::string& sessionBus);

// the address line out of `dbus-daemon --print-address --print-pid`
// output; *pid takes the all-digits line, -1 when absent
std::string sessionBusAddr(const std::string& out, pid_t* pid);

// rewrite an Exec argv so wrappers can't escape the session: an `env`
// prefix gets the forced assignments spliced in before its command
// (-S/--split-string payloads are tokenised inline first, and existing
// assignments/unsets of confined vars are dropped), and `flatpak run`
// gets the wayland socket pinned plus its routing flags dropped.
// Anything else comes back unchanged.
std::vector<std::string> confineExec(
    const std::vector<std::string>& argv,
    const std::vector<std::string>& forceEnv);
