#pragma once

#include <cstdint>
#include <rex/memory/guest_address.h>

namespace nfsmw::pegamento_nativo {

class FoldedGuestObject {
 public:
  explicit constexpr FoldedGuestObject(uint32_t address) noexcept
      : address_(address), folded_(rex::memory::FoldGuestAddress(address)),
        linear_((address & 0x00FFFFFFu) <= 0x00FFFFE3u) {}

  constexpr uint64_t Offset(uint32_t offset) const noexcept {
    return linear_ && offset <= 28 ? folded_ + offset : rex::memory::FoldGuestAddress(address_ + offset);
  }

 private:
  uint32_t address_;
  uint64_t folded_;
  bool linear_;
};

}  // namespace nfsmw::pegamento_nativo
