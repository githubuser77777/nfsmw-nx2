#include <cassert>
#include <cstdint>
#include <vector>

#include <rex/memory/guest_address.h>
#include "../../app/src/nfsmw_pegamento_objeto.h"

int main() {
  using rex::memory::FoldGuestAddress;
  struct Range {
    uint64_t start;
    uint64_t end;
    uint64_t backing;
  };
  constexpr Range ranges[] = {
      {0x00000000, 0x7EFFFFFF, 0x00000000},
      {0x7F000000, 0x7FFFFFFF, 0xA0000000},
      {0x80000000, 0x8FFFFFFF, 0x80000000},
      {0x90000000, 0x9FFFFFFF, 0x80000000},
      {0xA0000000, 0xBFFFFFFF, 0xA0000000},
      {0xC0000000, 0xDFFFFFFF, 0xA0000000},
      {0xE0000000, 0xFFFFFFFF, 0xA0001000},
  };
  for (const auto& range : ranges) {
    for (uint64_t address = range.start; address <= range.end; address += 0x1000) {
      const auto expected = range.backing + address - range.start;
      assert(FoldGuestAddress(uint32_t(address)) == expected);
      assert(FoldGuestAddress(uint32_t(address + 0xFFF)) == expected + 0xFFF);
    }
  }
  uint32_t address = 0x12345678;
  for (int i = 0; i < 100000; ++i) {
    address ^= address << 13;
    address ^= address >> 17;
    address ^= address << 5;
    for (const auto& range : ranges) {
      if (address >= range.start && address <= range.end) {
        assert(FoldGuestAddress(address) == range.backing + address - range.start);
        break;
      }
    }
  }
  auto check_window = [](uint32_t start) {
    const nfsmw::pegamento_nativo::FoldedGuestObject window(start);
    for (uint32_t offset = 0; offset <= 32; ++offset) {
      assert(window.Offset(offset) == FoldGuestAddress(start + offset));
    }
    assert(window.Offset(0x10000000u) == FoldGuestAddress(start + 0x10000000u));
  };
  constexpr uint32_t boundaries[] = {
      0x01000000u, 0x7F000000u, 0x80000000u, 0x90000000u, 0xA0000000u,
      0xC0000000u, 0xE0000000u, 0u,
  };
  for (uint32_t boundary : boundaries) {
    for (uint32_t i = 0; i <= 32; ++i) {
      check_window(boundary - i);
      check_window(boundary + i);
    }
  }
  for (int i = 0; i < 100000; ++i) {
    address ^= address << 13;
    address ^= address >> 17;
    address ^= address << 5;
    check_window(address);
  }
  assert(FoldGuestAddress(0x7EFFFFFF) == 0x7EFFFFFF);
  assert(FoldGuestAddress(0x7F000000) == 0xA0000000);
  assert(FoldGuestAddress(0x7FFFFFFF) == 0xA0FFFFFF);
  assert(FoldGuestAddress(0x80000000) == 0x80000000);
  assert(FoldGuestAddress(0x90000000) == 0x80000000);
  assert(FoldGuestAddress(0x9FFFFFFF) == 0x8FFFFFFF);
  assert(FoldGuestAddress(0xA0000000) == 0xA0000000);
  assert(FoldGuestAddress(0xC0000000) == 0xA0000000);
  assert(FoldGuestAddress(0xE0000000) == 0xA0001000);
  assert(FoldGuestAddress(0xFFFFFFFF) + 1 == rex::memory::kFoldedGuestArenaSize);

  std::vector<uint8_t> physical(0x3000);
  auto at = [&](uint32_t address) -> uint8_t& {
    return physical.at(FoldGuestAddress(address) - 0xA0000000);
  };
  at(0xA0001234) = 0x56;
  assert(at(0xC0001234) == 0x56);
  assert(at(0xE0000234) == 0x56);
  assert(at(0x7F001234) == 0x56);
  at(0xE0000234) = 0x78;
  assert(at(0xA0001234) == 0x78);
}
