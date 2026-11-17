#include "../include/cps2.h"
#include "assets.h"

#include <stdint.h>

namespace {
volatile uint32_t g_frame = 0;
cps2::Input g_input;

int16_t g_px = 184;
int16_t g_py = 160;
int16_t g_vy = 0;
bool g_on_ground = true;
bool g_facing_left = false;
bool g_moving = false;
int32_t g_world_x = 0;

constexpr int16_t kGroundY = 160;
} // namespace

void cps2::on_vblank() {
  g_frame = g_frame + 1;
  g_input.poll();

  g_moving = false;
  if (g_input.held(cps2::Button::Right)) {
    g_facing_left = false;
    g_moving = true;
    if (g_px < 240) {
      g_px += 2;
    } else {
      g_world_x += 2;
    }
  }
  if (g_input.held(cps2::Button::Left)) {
    g_facing_left = true;
    g_moving = true;
    if (g_px > 110) {
      g_px -= 2;
    } else {
      g_world_x -= 2;
    }
  }

  if (g_on_ground && g_input.pressed(cps2::Button::B1)) {
    g_vy = -7;
    g_on_ground = false;
  }

  if (!g_on_ground) {
    g_py += g_vy;
    if ((g_frame & 1u) == 0u) {
      g_vy += 1;
    }
    if (g_py >= kGroundY) {
      g_py = kGroundY;
      g_vy = 0;
      g_on_ground = true;
    }
  }

  cps2::set_scroll3_pos(static_cast<int16_t>(g_world_x >> 2), 0);
  cps2::set_scroll2_pos(static_cast<int16_t>(g_world_x), 0);

  cps2::Scroll1.print_hex(cps2::kScr1Left + 9, cps2::kScr1Top + 1, 1,
                          static_cast<uint32_t>(g_world_x), 4);

  cps2::SpriteBatch batch;
  batch.begin();
  const uint32_t anim_offset = (g_moving && ((g_frame >> 3) & 1u)) ? 1u : 0u;
  const auto flip =
      g_facing_left ? cps2::SpriteFlip::X : cps2::SpriteFlip::None;
  batch.draw(g_px, g_py, HERO_FIRST_TILE + anim_offset, 0, 7, flip);
  batch.end();

  cps2::palette::commit();
}

int main() {
  cps2::video::init(cps2::LayerMask::All);
  cps2::sprite::clear_all();

  constexpr uint16_t space_bg = cps2::rgb(0x01, 0x01, 0x03);
  cps2::palette::set_backdrop(space_bg);

  cps2::palette::load(cps2::PalPage::Obj, 0, HERO_PALETTE);
  cps2::palette::load(cps2::PalPage::Scroll2, 0, BG_PALETTE);
  cps2::palette::load(cps2::PalPage::Scroll3, 0, BG_PALETTE);

  cps2::palette::set_entry(cps2::PalPage::Scroll1, 0, 1,
                           cps2::rgb(0x0f, 0x0f, 0x0f));
  cps2::palette::set_entry(cps2::PalPage::Scroll1, 1, 1,
                           cps2::rgb(0x04, 0x0f, 0x0e));
  cps2::palette::commit();

  cps2::Scroll3.clear(static_cast<uint16_t>(BG_FIRST_TILE), 0);
  cps2::Scroll2.clear(0x0000u, 0);
  for (int col = 0; col < 64; ++col) {
    for (int row = 12; row < 16; ++row) {
      cps2::Scroll2.put(col, row, static_cast<uint16_t>(BG_FIRST_TILE + 1u), 0);
    }
  }

  cps2::Scroll1.clear(' ', 0);
  cps2::Scroll1.print(cps2::kScr1Left + 1, cps2::kScr1Top + 1, 0, "WORLD X:");
  cps2::Scroll1.print(cps2::kScr1Left + 1, cps2::kScr1Top + 2, 0,
                      "D-PAD: MOVE   BTN1: JUMP");

  for (;;) {
    cps2::halt_until_vblank();
  }
}
