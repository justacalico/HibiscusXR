#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Minimal read-only zip: the home environments are single zips pushed over
// adb, so all this needs is the central directory walk plus stored/deflate
// extraction. No zip64, no data descriptors - an entry flagged with one is
// skipped rather than guessed at. Pure: tests feed it byte buffers.
struct ZipEntry {
    std::string name;
    uint32_t method = 0;     // 0 stored, 8 deflate
    uint32_t csize = 0;      // compressed size
    uint32_t usize = 0;      // size once extracted
    uint32_t localOff = 0;   // offset of the local file header
};

// List every central-directory entry. Returns false when the buffer is not
// a zip at all (no end-of-central-directory record).
bool zipList(const uint8_t* data, size_t len, std::vector<ZipEntry>* out);

// Extract one entry by exact name. Stored entries are copied, deflate goes
// through zlib's raw inflater; the declared uncompressed size is verified.
bool zipRead(const uint8_t* data, size_t len, const char* name,
             std::vector<uint8_t>* out);
