#pragma once

#include <string>
#include <vector>

#include "../set/item.h"

// Parsers for the host's status feeds: /sys battery files and the
// NetworkManager / PipeWire / brightnessctl command outputs. All take the
// raw text so unit tests feed fixtures; the backend does the IO.

int  parseCapacity(const std::string& text);       // "87\n" -> 87, else -1
bool parseCharging(const std::string& text);       // Charging/Full -> true

// `nmcli -t -f general.state dev wifi` isn't the command; use
// `nmcli -t -f active,ssid dev wifi`: "yes:name" marks the live link
std::string parseWifiSsid(const std::string& text);
bool parseNmRadio(const std::string& text);        // "enabled"/"disabled"

// `nmcli -t -f type,state dev`: an "ethernet:connected" row means the
// wired link is up
bool parseEthLink(const std::string& text);

// `wpctl get-volume @DEFAULT_AUDIO_SINK@` -> "Volume: 0.63 [MUTED]"
float parseWpctlVol(const std::string& text);
bool parseWpctlMuted(const std::string& text);

// `brightnessctl -m` -> "dev,class,cur,pct,max"
float parseBrightness(const std::string& text);

// ------------------------------------------------------- settings card ---

// /etc/hibiscus-release: an os-release style VERSION="x" line wins, else
// the dist stamp's bare single-line version
std::string parseHibiscusRelease(const std::string& text);

// /proc/cpuinfo: the first "model name" value (ARM hosts carry
// "Hardware"/"Processor" instead, tried the same way), "" when absent
std::string parseCpuModel(const std::string& text);

// /proc/meminfo "MemTotal: <n> kB" -> "<n> GB" in GiB, "" when absent
std::string parseMemTotal(const std::string& text);

// glGetString(GL_RENDERER) strings carry driver noise after '(' and '/'
// ("NVIDIA ... /PCIe/SSE2", "AMD ... (radeonsi, ...)"): cut there
std::string cleanGpuName(const std::string& name);

// the About card's System value: cpu, mem and gpu joined with ", ";
// empty fields drop out so a partial probe still reads sensibly
std::string specLine(const std::string& cpu, const std::string& mem,
                     const std::string& gpu);

// `nmcli -t -f active,ssid,signal,security dev wifi` rows. -t mode
// backslash-escapes ':' inside fields; "yes" marks the live link,
// signal 0-100 -> level 0-4, "--"/empty security -> open. saved carries
// the index into `cons` (nmcli -t -f name con show) when a profile
// with the same name exists
void parseWifiList(const std::string& text,
                   const std::vector<std::string>& cons,
                   std::vector<SetWifi>* out);

// `nmcli -t -f name con show` -> one profile name per line
std::vector<std::string> parseConNames(const std::string& text);

// `bluetoothctl show` -> "Powered: yes"
bool parseBtPowered(const std::string& text);

// `bluetoothctl devices` rows "Device <addr> <name>" - all bonded;
// `conns` is the `bluetoothctl devices Connected` output marking the
// live ones
void parseBtDevs(const std::string& text, const std::string& conns,
                 std::vector<SetBtDev>* out);
