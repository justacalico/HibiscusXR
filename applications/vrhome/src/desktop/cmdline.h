#pragma once

#include <string>
#include <vector>

// single-quote every arg and join: 'foo' 'it'\''s' - safe for
// /bin/sh -c no matter what the .desktop Exec line produced
std::string shellJoin(const std::vector<std::string>& argv);

// wrap a Terminal=true argv so it runs inside a terminal emulator:
// the flag column covers each emulator's "run this command" syntax
std::vector<std::string> terminalWrap(const std::string& term,
                                      const std::vector<std::string>& argv);

// first terminal emulator found on PATH, "" if none
std::string findTerminal();
