#pragma once

#include <string>

// Parsers for the host's status feeds: /sys battery files and the
// NetworkManager / PipeWire / brightnessctl command outputs. All take the
// raw text so unit tests feed fixtures; the backend does the IO.

int  parseCapacity(const std::string& text);       // "87\n" -> 87, else -1
bool parseCharging(const std::string& text);       // Charging/Full -> true

// `nmcli -t -f general.state dev wifi` isn't the command; use
// `nmcli -t -f active,ssid dev wifi`: "yes:name" marks the live link
std::string parseWifiSsid(const std::string& text);
bool parseNmRadio(const std::string& text);        // "enabled"/"disabled"

// `wpctl get-volume @DEFAULT_AUDIO_SINK@` -> "Volume: 0.63 [MUTED]"
float parseWpctlVol(const std::string& text);
bool parseWpctlMuted(const std::string& text);

// `brightnessctl -m` -> "dev,class,cur,pct,max"
float parseBrightness(const std::string& text);
