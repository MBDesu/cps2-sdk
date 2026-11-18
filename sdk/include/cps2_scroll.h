#pragma once
#ifndef CPS2_SDK_SCROLL_H
#define CPS2_SDK_SCROLL_H

#include "cps2_hw.h"
#include "cps2_palette.h"

#include <stdint.h>

namespace cps2 {

enum class LayerMask : uint16_t {
  None = 0,
  Obj = 1u << 0,
  Scroll1 = 1u << 1,
  Scroll2 = 1u << 2,
  Scroll3 = 1u << 3,
  All = Obj | Scroll1 | Scroll2 | Scroll3,
};

constexpr LayerMask operator|(LayerMask a, LayerMask b) {
  return static_cast<LayerMask>(static_cast<uint16_t>(a) |
                                static_cast<uint16_t>(b));
}

enum class LayerId : uint8_t {
  Obj = 0,
  Scroll1 = 1,
  Scroll2 = 2,
  Scroll3 = 3,
};

enum class TileFlip : uint16_t {
  None = 0,
  X = 1u << 5,
  Y = 1u << 6,
  XY = X | Y,
};

constexpr TileFlip operator|(TileFlip a, TileFlip b) {
  return static_cast<TileFlip>(static_cast<uint16_t>(a) |
                               static_cast<uint16_t>(b));
}

namespace video {

inline void set_layer_order(LayerId back_l0, LayerId mid_l1, LayerId front_l2,
                            LayerId top_l3, LayerMask enable) {
  cpsb().layer_ctrl =
      static_cast<uint16_t>(((static_cast<uint16_t>(back_l0) & 3u) << 6) |
                            ((static_cast<uint16_t>(mid_l1) & 3u) << 8) |
                            ((static_cast<uint16_t>(front_l2) & 3u) << 10) |
                            ((static_cast<uint16_t>(top_l3) & 3u) << 12) |
                            (static_cast<uint16_t>(enable) & 0x000fu));
}

inline void init(LayerMask enable = LayerMask::All) {
  auto &a = cpsa();
  a.obj_base = 0x7000u;
  a.scroll1_base = 0x9040u;
  a.scroll2_base = 0x9080u;
  a.scroll3_base = 0x90c0u;

  a.scroll1_x = 0;
  a.scroll1_y = 0;
  a.scroll2_x = 0;
  a.scroll2_y = 0;
  a.scroll3_x = 0;
  a.scroll3_y = 0;
  a.video_ctrl = 0x003eu;

  set_layer_order(LayerId::Scroll3, LayerId::Scroll2, LayerId::Scroll1,
                  LayerId::Obj, enable);
  auto &b = cpsb();
  b.pri_mask[0] = 0x0000u;
  b.pri_mask[1] = 0x0000u;
  b.pri_mask[2] = 0x0000u;
  b.pri_mask[3] = 0x0000u;
  reg_obj_pri() = 0x6420u;
  b.pal_ctrl = 0x003fu;
}

inline void set_background(uint16_t color) {
  palette::set_entry(PalPage::Scroll3, 31u, 1u, color);
  constexpr uint16_t kSolid32x32Tile = 0x0002u;
  volatile uint16_t *map = &gfx_ram()[0x6000u];
  for (uint32_t i = 0; i < 64u * 64u; ++i) {
    *(map + i * 2u) = kSolid32x32Tile;
    *(map + i * 2u + 1u) = 31u;
  }
  palette::commit();
}

} // namespace video

class ScrollPlane {
public:
  void clear(const TileSheet &sheet, uint8_t palette = 0,
             uint32_t tile_offset = 0) const {
    clear(sheet.tile(tile_offset), palette);
  }

  void fill_rect(int col, int row, int width, int height,
                 const TileSheet &sheet, uint32_t tile_offset = 0,
                 uint8_t palette = 0) const {
    fill_rect(col, row, width, height, sheet.tile(tile_offset), palette);
  }
  constexpr explicit ScrollPlane(uint32_t word_base, uint8_t tile_size = 8)
      : word_base_(word_base), tile_size_(tile_size) {}

  constexpr uint32_t entry_index(int col, int row) const {
    if (tile_size_ == 16) {
      return static_cast<uint32_t>(((row & 0x30) << 6) | ((col & 0x3f) << 4) |
                                   (row & 0x0f));
    }
    if (tile_size_ == 32) {
      return static_cast<uint32_t>(((row & 0x38) << 6) | ((col & 0x3f) << 3) |
                                   (row & 0x07));
    }
    return static_cast<uint32_t>(((row & 0x20) << 6) | ((col & 0x3f) << 5) |
                                 (row & 0x1f));
  }

  void clear_transparent(uint8_t palette = 0) const {
    const uint16_t blank_tile =
        (tile_size_ == 8) ? 0x0020u : ((tile_size_ == 16) ? 0x0011u : 0x0001u);
    clear(blank_tile, palette);
  }

  void set_pos(int16_t x, int16_t y) const {
    auto &a = cpsa();
    if (tile_size_ == 8) {
      a.scroll1_x = x;
      a.scroll1_y = y;
    } else if (tile_size_ == 16) {
      a.scroll2_x = x;
      a.scroll2_y = y;
    } else {
      a.scroll3_x = x;
      a.scroll3_y = y;
    }
  }

  void put(int col, int row, uint16_t tile, uint8_t palette = 0,
           TileFlip flip = TileFlip::None) const {
    volatile uint16_t *map = &gfx_ram()[word_base_];
    const uint32_t idx = entry_index(col, row);
    *(map + idx * 2u) = tile;
    *(map + idx * 2u + 1u) =
        static_cast<uint16_t>((palette & 0x1fu) | static_cast<uint16_t>(flip));
  }

  void clear(uint16_t tile = 0x0000u, uint8_t palette = 0) const {
    volatile uint16_t *map = &gfx_ram()[word_base_];
    const uint16_t attr = static_cast<uint16_t>(palette & 0x1fu);
    for (uint32_t i = 0; i < 64u * 64u; ++i) {
      *(map + i * 2u) = tile;
      *(map + i * 2u + 1u) = attr;
    }
  }

  void box(int col, int row, int width, int height, uint16_t tile,
           uint8_t palette = 0) const {
    for (int x = 0; x < width; ++x) {
      put(col + x, row, tile, palette);
      put(col + x, row + height - 1, tile, palette);
    }
    for (int y = 0; y < height; ++y) {
      put(col, row + y, tile, palette);
      put(col + width - 1, row + y, tile, palette);
    }
  }

  void print(int col, int row, uint8_t palette, const char *str) const {
    while (*str) {
      const auto ch = static_cast<uint8_t>(*str++);
      put(col++, row, static_cast<uint16_t>(ch), palette);
    }
  }

  void print_hex(int col, int row, uint8_t palette, uint32_t value,
                 int digits = 4) const {
    for (int i = digits - 1; i >= 0; --i) {
      const int nib = static_cast<int>(value & 0x0fu);
      const uint16_t ch =
          static_cast<uint16_t>(nib < 10 ? ('0' + nib) : ('A' + (nib - 10)));
      put(col + i, row, ch, palette);
      value >>= 4;
    }
  }

  void print_dec(int col, int row, uint8_t palette, uint32_t value) const {
    char buf[12];
    int len = 0;
    if (value == 0u) {
      buf[len++] = '0';
    } else {
      char rev[12];
      int rlen = 0;
      while (value > 0u) {
        rev[rlen++] = static_cast<char>('0' + (value % 10u));
        value /= 10u;
      }
      while (rlen > 0) {
        buf[len++] = rev[--rlen];
      }
    }
    buf[len] = '\0';
    print(col, row, palette, buf);
  }

  void fill_rect(int col, int row, int width, int height, uint16_t tile,
                 uint8_t pal = 0, TileFlip flip = TileFlip::None) const {
    for (int r = 0; r < height; ++r) {
      for (int c = 0; c < width; ++c) {
        put(col + c, row + r, tile, pal, flip);
      }
    }
  }

  void print_uint(int col, int row, uint8_t pal, uint32_t value,
                  int min_digits = 1) const {
    static constexpr uint32_t kPowers[10] = {
        1000000000u, 100000000u, 10000000u, 1000000u, 100000u,
        10000u,      1000u,      100u,      10u,      1u};

    char buf[11];
    int cursor = 0;
    bool started = false;
    for (int i = 0; i < 10; ++i) {
      const uint32_t p = kPowers[i];
      uint8_t digit = 0;
      while (value >= p) {
        value -= p;
        ++digit;
      }
      const int remaining = 10 - i;
      if (digit > 0 || started || remaining <= min_digits) {
        buf[cursor++] = static_cast<char>('0' + digit);
        started = true;
      }
    }
    buf[cursor] = '\0';
    print(col, row, pal, buf);
  }

private:
  uint32_t word_base_;
  uint8_t tile_size_;
};

inline constexpr ScrollPlane Scroll1{0x2000u, 8};
inline constexpr ScrollPlane Scroll2{0x4000u, 16};
inline constexpr ScrollPlane Scroll3{0x6000u, 32};

inline void set_scroll1_pos(int16_t x, int16_t y) {
  cpsa().scroll1_x = static_cast<uint16_t>(x);
  cpsa().scroll1_y = static_cast<uint16_t>(y);
}

inline void set_scroll2_pos(int16_t x, int16_t y) {
  cpsa().scroll2_x = static_cast<uint16_t>(x);
  cpsa().scroll2_y = static_cast<uint16_t>(y);
}

inline void set_scroll3_pos(int16_t x, int16_t y) {
  cpsa().scroll3_x = static_cast<uint16_t>(x);
  cpsa().scroll3_y = static_cast<uint16_t>(y);
}

} // namespace cps2

#endif // CPS2_SDK_SCROLL_H
