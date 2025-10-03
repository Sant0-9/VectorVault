#pragma once

#include <cstdint>
#include <string_view>

namespace vectorvault {

constexpr std::string_view VERSION = "1.0.0";
constexpr uint32_t VERSION_MAJOR = 1;
constexpr uint32_t VERSION_MINOR = 0;
constexpr uint32_t VERSION_PATCH = 0;

// File format version for persistence
constexpr uint32_t FILE_FORMAT_VERSION = 1;

// Magic number for file validation
constexpr uint32_t FILE_MAGIC = 0x56564C54; // "VVLT" = VectorVauLT

} // namespace vectorvault
