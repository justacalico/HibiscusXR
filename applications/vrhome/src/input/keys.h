#pragma once

// Key codes we treat as the headset's confirm button. Values mirror the
// Android AKEYCODE_* constants but are defined here so this header stays
// usable on the host.
constexpr int kKeyEnter      = 66;   // AKEYCODE_ENTER
constexpr int kKeyDpadCenter = 23;   // AKEYCODE_DPAD_CENTER
constexpr int kKeyButtonA    = 96;   // AKEYCODE_BUTTON_A
// kPicoConfirm (1001) lives in common/config.h

bool isConfirm(int code);
