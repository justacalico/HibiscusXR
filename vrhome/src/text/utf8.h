#pragma once

// Decode the next UTF-8 codepoint and advance the cursor past it.
// Malformed lead bytes are returned as-is so a bad string can't wedge the
// layout loop.
int nextCp(const char*& p);
