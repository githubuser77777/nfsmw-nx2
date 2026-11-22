#pragma once

#include <cstdint>

namespace rex::memory {

// Offset in the Switch 2 arena after folding the Xbox 360 mirrored ranges.
constexpr uint64_t FoldGuestAddress(uint32_t address) noexcept {
  if (address < 0x90000000u) {
    if ((address & 0xFF000000u) == 0x7F000000u) {
      return uint64_t(address) + 0x21000000ull;
    }
    return address;
  }
  if (address < 0xA0000000u) {
    return uint64_t(address) - 0x10000000ull;
  }
  if (address < 0xC0000000u) {
    return address;
  }
  if (address < 0xE0000000u) {
    return uint64_t(address) - 0x20000000ull;
  }
  return uint64_t(address) - 0x40000000ull + 0x1000;
}

constexpr uint64_t kFoldedGuestArenaSize = 0xC0001000ull;

}  // namespace rex::memory
