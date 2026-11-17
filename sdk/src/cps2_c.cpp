#include "cps2_c.h"
#include "cps2.h"

namespace {
cps2::Input g_c_input;
cps2::SpriteBatch g_c_batch;

} // namespace

extern "C" __attribute__((weak)) void cps2_on_vblank(void) {}

void cps2::on_vblank() { cps2_on_vblank(); }

extern "C" {

void cps2_halt_until_vblank(void) { cps2::halt_until_vblank(); }
void cps2_video_init(uint16_t mask) {
  cps2::video::init(static_cast<cps2::LayerMask>(mask));
}
void cps2_scroll2_pos(int16_t x, int16_t y) { cps2::set_scroll2_pos(x, y); }
void cps2_scroll3_pos(int16_t x, int16_t y) { cps2::set_scroll3_pos(x, y); }

uint16_t cps2_pal_color(uint8_t r, uint8_t g, uint8_t b, uint8_t bright) {
  return cps2::rgb(r, g, b, bright);
}
void cps2_palette_set_backdrop(uint16_t c) { cps2::palette::set_backdrop(c); }
void cps2_palette_set_entry(uint8_t p, uint8_t s, uint8_t i, uint16_t c) {
  cps2::palette::set_entry(static_cast<cps2::PalPage>(p), s, i, c);
}
void cps2_palette_load(uint8_t p, uint8_t s, const uint16_t *c16) {
  cps2::palette::load(static_cast<cps2::PalPage>(p), s, c16);
}
void cps2_palette_commit(void) { cps2::palette::commit(); }

void cps2_scroll_clear(uint8_t p, uint16_t tile, uint8_t pal) {
  switch (p) {
  case 2:
    cps2::Scroll2.clear(tile, pal);
    break;
  case 3:
    cps2::Scroll3.clear(tile, pal);
    break;
  default:
    cps2::Scroll1.clear(tile, pal);
    break;
  }
}
void cps2_scroll_put(uint8_t p, int col, int row, uint16_t tile, uint8_t pal) {
  switch (p) {
  case 2:
    cps2::Scroll2.put(col, row, tile, pal);
    break;
  case 3:
    cps2::Scroll3.put(col, row, tile, pal);
    break;
  default:
    cps2::Scroll1.put(col, row, tile, pal);
    break;
  }
}
void cps2_scroll_print(uint8_t p, int col, int row, uint8_t pal,
                       const char *s) {
  switch (p) {
  case 2:
    cps2::Scroll2.print(col, row, pal, s);
    break;
  case 3:
    cps2::Scroll3.print(col, row, pal, s);
    break;
  default:
    cps2::Scroll1.print(col, row, pal, s);
    break;
  }
}
void cps2_scroll_print_hex(uint8_t p, int col, int row, uint8_t pal, uint32_t v,
                           int d) {
  switch (p) {
  case 2:
    cps2::Scroll2.print_hex(col, row, pal, v, d);
    break;
  case 3:
    cps2::Scroll3.print_hex(col, row, pal, v, d);
    break;
  default:
    cps2::Scroll1.print_hex(col, row, pal, v, d);
    break;
  }
}

void cps2_scroll_fill_rect(uint8_t p, int col, int row, int w, int h,
                           uint16_t tile, uint8_t pal) {
  switch (p) {
  case 2:
    cps2::Scroll2.fill_rect(col, row, w, h, tile, pal);
    break;
  case 3:
    cps2::Scroll3.fill_rect(col, row, w, h, tile, pal);
    break;
  default:
    cps2::Scroll1.fill_rect(col, row, w, h, tile, pal);
    break;
  }
}

void cps2_scroll_box(uint8_t p, int col, int row, int w, int h, uint8_t pal) {
  switch (p) {
  case 2:
    cps2::Scroll2.box(col, row, w, h, 0x0001u, pal);
    break;
  case 3:
    cps2::Scroll3.box(col, row, w, h, 0x0001u, pal);
    break;
  default:
    cps2::Scroll1.box(col, row, w, h, 0x0001u, pal);
    break;
  }
}

void cps2_scroll_print_uint(uint8_t p, int col, int row, uint8_t pal,
                            uint32_t v, int min_digits) {
  switch (p) {
  case 2:
    cps2::Scroll2.print_uint(col, row, pal, v, min_digits);
    break;
  case 3:
    cps2::Scroll3.print_uint(col, row, pal, v, min_digits);
    break;
  default:
    cps2::Scroll1.print_uint(col, row, pal, v, min_digits);
    break;
  }
}

void cps2_batch_draw_meta(int16_t x, int16_t y, uint32_t first_tile,
                          uint8_t w_tiles, uint8_t h_tiles, uint8_t pal,
                          uint8_t prio, uint8_t flip) {
  g_c_batch.draw_meta(x, y, first_tile, w_tiles, h_tiles, pal, prio,
                      static_cast<cps2::SpriteFlip>(flip));
}

void cps2_anim_reset(Cps2SpriteAnim *a) {
  a->frame = 0;
  a->timer = 0;
  a->finished = false;
}

void cps2_anim_tick(Cps2SpriteAnim *a) {
  if (a->finished || a->frame_count <= 1 || a->ticks_per_frame == 0)
    return;
  if (++a->timer >= a->ticks_per_frame) {
    a->timer = 0;
    if (a->frame + 1 < a->frame_count) {
      ++a->frame;
    } else if (a->loop) {
      a->frame = 0;
    } else {
      a->finished = true;
    }
  }
}

uint32_t cps2_anim_tile(const Cps2SpriteAnim *a) {
  return a->base_tile + static_cast<uint32_t>(a->frame) * a->tiles_per_frame;
}

void cps2_input_poll(void) { g_c_input.poll(); }
bool cps2_input_held(uint16_t btn, int player) {
  return g_c_input.held(static_cast<cps2::Button>(btn), player);
}
bool cps2_input_pressed(uint16_t btn, int player) {
  return g_c_input.pressed(static_cast<cps2::Button>(btn), player);
}

void cps2_sprite_clear_all(void) { cps2::sprite::clear_all(); }
void cps2_batch_begin(void) { g_c_batch.begin(); }
void cps2_batch_draw(int16_t x, int16_t y, uint32_t tile, uint8_t pal,
                     uint8_t prio, uint8_t flip) {
  g_c_batch.draw(x, y, tile, pal, prio, static_cast<cps2::SpriteFlip>(flip));
}
void cps2_batch_end(void) { g_c_batch.end(); }

} // extern "C"
