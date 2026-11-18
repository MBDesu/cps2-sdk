#pragma once
#ifndef CPS2_SDK_PALETTE_H
#define CPS2_SDK_PALETTE_H

#include "cps2_hw.h"
#include <stdint.h>

namespace cps2 {

struct TileSheet {
  uint32_t first_tile;
  uint32_t tile_count;
  const uint16_t *palette;

  constexpr uint16_t tile(uint32_t offset = 0) const {
    const uint32_t clamped =
        (tile_count > 0u && offset >= tile_count) ? (tile_count - 1u) : offset;
    return static_cast<uint16_t>(first_tile + clamped);
  }
};

enum class PalPage : uint32_t {
  Obj = 0,
  Scroll1 = 1,
  Scroll2 = 2,
  Scroll3 = 3,
};

constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t bright = 0x0f) {
  return static_cast<uint16_t>(((bright & 0x0fu) << 12) | ((r & 0x0fu) << 8) |
                               ((g & 0x0fu) << 4) | (b & 0x0fu));
}

namespace palette {

inline void set_backdrop(uint16_t color) { gfx_ram()[0x0bffu] = color; }

inline void set_entry(PalPage page, uint32_t slot, uint32_t entry,
                      uint16_t color) {
  const uint32_t offset = ((static_cast<uint32_t>(page) & 3u) * 0x200u) +
                          ((slot & 31u) * 16u) + (entry & 15u);
  gfx_ram()[offset] = color;
}

inline void load(PalPage page, uint32_t slot, const uint16_t *colors16) {
  const uint32_t base =
      ((static_cast<uint32_t>(page) & 3u) * 0x200u) + ((slot & 31u) * 16u);
  volatile uint16_t *dst = &gfx_ram()[base];
  for (uint32_t i = 0; i < 16u; ++i) {
    asm volatile("" : "+r"(dst));
    dst[i] = colors16[i];
  }
}

inline void load(PalPage page, uint32_t slot, const TileSheet &sheet) {
  load(page, slot, sheet.palette);
}

inline void load_faded(PalPage page, uint32_t slot, const uint16_t *colors16,
                       uint8_t level) {
  const uint32_t base =
      ((static_cast<uint32_t>(page) & 3u) * 0x200u) + ((slot & 31u) * 16u);
  volatile uint16_t *dst = &gfx_ram()[base];
  const uint32_t scale = (level > 15u) ? 15u : level;
  for (uint32_t i = 0; i < 16u; ++i) {
    asm volatile("" : "+r"(dst));
    const uint16_t c = colors16[i];
    const uint16_t bright =
        static_cast<uint16_t>((((c >> 12) & 0x0fu) * scale) / 15u);
    dst[i] = static_cast<uint16_t>((bright << 12) | (c & 0x0fffu));
  }
}

inline void commit() { cpsa().palette_base = 0x9000u; }

} // namespace palette
} // namespace cps2

#endif // CPS2_SDK_PALETTE_H
