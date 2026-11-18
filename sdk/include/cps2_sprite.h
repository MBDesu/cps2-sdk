#pragma once
#ifndef CPS2_SDK_SPRITE_H
#define CPS2_SDK_SPRITE_H

#include "cps2_hw.h"
#include "cps2_palette.h"

#include <stdint.h>

namespace cps2 {

inline constexpr uint32_t kMaxSprites = 1024u;

enum class SpriteFlip : uint16_t {
  None = 0,
  X = 1u << 5,
  Y = 1u << 6,
  XY = X | Y,
};

struct Sprite {
  int16_t x = 0;
  int16_t y = 0;
  uint32_t tile = 0;
  uint8_t palette = 0;
  uint8_t priority = 7;
  uint8_t w_tiles = 1;
  uint8_t h_tiles = 1;
  SpriteFlip flip = SpriteFlip::None;
};

struct SpriteAnim {
  uint32_t base_tile = 0;
  uint16_t frame_count = 1;
  uint16_t tiles_per_frame = 1;
  uint8_t ticks_per_frame = 6;
  bool loop = true;

  uint16_t frame = 0;
  uint8_t timer = 0;
  bool finished = false;

  static constexpr SpriteAnim from_sheet(const TileSheet &sheet,
                                         uint8_t ticks_per_frame = 6,
                                         bool loop = true,
                                         uint32_t tile_offset = 0) {
    return SpriteAnim{
        .base_tile = sheet.first_tile + tile_offset,
        .frame_count = static_cast<uint16_t>(
            sheet.tile_count > tile_offset ? sheet.tile_count - tile_offset
                                           : 1u),
        .tiles_per_frame = 1,
        .ticks_per_frame = ticks_per_frame,
        .loop = loop,
    };
  }

  static constexpr SpriteAnim
  from_sheet(const TileSheet &sheet, uint16_t frame_count, uint8_t w_tiles,
             uint8_t h_tiles, uint8_t ticks_per_frame = 6, bool loop = true,
             uint32_t tile_offset = 0) {
    return SpriteAnim{
        .base_tile = sheet.first_tile + tile_offset,
        .frame_count = frame_count,
        .tiles_per_frame = static_cast<uint16_t>(
            static_cast<uint16_t>(w_tiles) * static_cast<uint16_t>(h_tiles)),
        .ticks_per_frame = ticks_per_frame,
        .loop = loop,
    };
  }

  void reset() {
    frame = 0;
    timer = 0;
    finished = false;
  }

  void tick() {
    if (finished || frame_count <= 1 || ticks_per_frame == 0) {
      return;
    }
    if (++timer >= ticks_per_frame) {
      timer = 0;
      if (frame + 1 < frame_count) {
        ++frame;
      } else if (loop) {
        frame = 0;
      } else {
        finished = true;
      }
    }
  }

  constexpr uint32_t current_tile() const {
    return base_tile + static_cast<uint32_t>(frame) * tiles_per_frame;
  }
};

constexpr SpriteFlip operator|(SpriteFlip a, SpriteFlip b) {
  return static_cast<SpriteFlip>(static_cast<uint16_t>(a) |
                                 static_cast<uint16_t>(b));
}

namespace sprite {

constexpr uint16_t encode_size(uint8_t width_blocks, uint8_t height_blocks) {
  const uint16_t w = static_cast<uint16_t>((width_blocks - 1u) & 0x0fu);
  const uint16_t h = static_cast<uint16_t>((height_blocks - 1u) & 0x0fu);
  return static_cast<uint16_t>((w << 8) | (h << 12));
}

constexpr bool is_offscreen(int16_t x, int16_t y, uint8_t width_blocks,
                            uint8_t height_blocks) {
  const int16_t px_w = static_cast<int16_t>(width_blocks * 16);
  const int16_t px_h = static_cast<int16_t>(height_blocks * 16);
  return (x + px_w <= 0) || (x >= kScreenWidth) || (y + px_h <= 0) ||
         (y >= kScreenHeight);
}

inline void set_ext(uint32_t index, int16_t x, int16_t y, uint32_t tile,
                    uint8_t palette, uint8_t priority, uint8_t width_blocks,
                    uint8_t height_blocks, SpriteFlip flip = SpriteFlip::None) {
  if (index >= kMaxSprites) {
    return;
  }
  volatile uint16_t *entry = &obj_ram()[index * 4u];
  const uint16_t sx = static_cast<uint16_t>(
      (x & 0x03ff) | ((static_cast<uint16_t>(priority & 0x07u)) << 13));
  const uint16_t sy = static_cast<uint16_t>(
      (y & 0x03ff) | (static_cast<uint16_t>(((tile >> 16) & 0x03u) << 13)));
  const uint16_t attr = static_cast<uint16_t>(
      (palette & 0x1fu) | (static_cast<uint16_t>(flip) & 0x0060u) |
      encode_size(width_blocks, height_blocks));

  *(entry + 0) = sx;
  *(entry + 1) = sy;
  *(entry + 2) = static_cast<uint16_t>(tile & 0xffffu);
  *(entry + 3) = attr;
}

inline void set(uint32_t index, int16_t x, int16_t y, uint32_t tile,
                uint8_t palette, uint8_t priority = 7,
                SpriteFlip flip = SpriteFlip::None) {
  set_ext(index, x, y, tile, palette, priority, 1u, 1u, flip);
}

inline void hide(uint32_t index) {
  if (index >= kMaxSprites) {
    return;
  }
  volatile uint16_t *entry = &obj_ram()[index * 4u];
  *(entry + 0) = 0x0000u;
  *(entry + 1) = 0x01f0u;
  *(entry + 2) = 0x0000u;
  *(entry + 3) = 0x0000u;
}

inline void end(uint32_t count) {
  if (count >= kMaxSprites) {
    return;
  }
  volatile uint16_t *entry = &obj_ram()[count * 4u];
  *(entry + 0) = 0x0000u;
  *(entry + 1) = 0x8000u;
  *(entry + 2) = 0x0000u;
  *(entry + 3) = 0xff00u;
}

inline void clear_all() { end(0u); }

} // namespace sprite

class SpriteBatch {
public:
  SpriteBatch() = default;

  SpriteBatch(const SpriteBatch &) = delete;
  SpriteBatch &operator=(const SpriteBatch &) = delete;

  void begin() {
    count_ = 0u;
    ended_ = false;
  }

  void draw_ext(int16_t x, int16_t y, uint32_t tile, uint8_t palette,
                uint8_t priority, uint8_t width_blocks, uint8_t height_blocks,
                SpriteFlip flip = SpriteFlip::None) {
    if (count_ >= kMaxSprites ||
        sprite::is_offscreen(x, y, width_blocks, height_blocks)) {
      return;
    }
    sprite::set_ext(count_++, x, y, tile, palette, priority, width_blocks,
                    height_blocks, flip);
  }

  void draw(int16_t x, int16_t y, uint32_t tile, uint8_t palette = 0,
            uint8_t priority = 7, SpriteFlip flip = SpriteFlip::None) {
    draw_ext(x, y, tile, palette, priority, 1u, 1u, flip);
  }

  void draw(const Sprite &spr) {
    const uint8_t w = (spr.w_tiles > 0u) ? spr.w_tiles : 1u;
    const uint8_t h = (spr.h_tiles > 0u) ? spr.h_tiles : 1u;
    if (w == 1u && h == 1u) {
      draw_ext(spr.x, spr.y, spr.tile, spr.palette, spr.priority, 1u, 1u,
               spr.flip);
    } else {
      draw_meta(spr.x, spr.y, spr.tile, w, h, spr.palette, spr.priority,
                spr.flip);
    }
  }

  void draw_meta(int16_t x, int16_t y, uint32_t first_tile, uint8_t w_tiles,
                 uint8_t h_tiles, uint8_t pal, uint8_t prio = 7,
                 SpriteFlip flip = SpriteFlip::None) {
    const bool hflip = (static_cast<uint16_t>(flip) &
                        static_cast<uint16_t>(SpriteFlip::X)) != 0;
    const bool vflip = (static_cast<uint16_t>(flip) &
                        static_cast<uint16_t>(SpriteFlip::Y)) != 0;

    for (uint8_t ty = 0; ty < h_tiles; ++ty) {
      const uint8_t src_y = vflip ? static_cast<uint8_t>(h_tiles - 1 - ty) : ty;
      for (uint8_t tx = 0; tx < w_tiles; ++tx) {
        const uint8_t src_x =
            hflip ? static_cast<uint8_t>(w_tiles - 1 - tx) : tx;
        const uint32_t tile =
            first_tile + static_cast<uint32_t>(src_y) * w_tiles + src_x;
        draw(static_cast<int16_t>(x + tx * 16),
             static_cast<int16_t>(y + ty * 16), tile, pal, prio, flip);
      }
    }
  }

  void end() {
    if (!ended_) {
      sprite::end(count_);
      ended_ = true;
    }
  }

  [[nodiscard]] uint32_t count() const { return count_; }

private:
  uint32_t count_ = 0u;
  bool ended_ = false;
};

class ScopedSpriteBatch : public SpriteBatch {
public:
  ScopedSpriteBatch() { begin(); }
  ~ScopedSpriteBatch() { end(); }
};

} // namespace cps2

#endif // CPS2_SDK_SPRITE_H
