#include "../include/cps2.h"
#include "assets.h"

#include <stdint.h>

namespace {
volatile uint32_t g_frame = 0;
cps2::Input g_input;

constexpr int16_t kGroundY = 144;
int16_t g_px = 176;
int16_t g_py = kGroundY;
int16_t g_vy = 0;
bool g_on_ground = true;
bool g_facing_left = false;
bool g_moving = false;
int32_t g_world_x = 0;

auto g_idle_anim =
    cps2::SpriteAnim::from_sheet(FIGHTER_SHEET, 2, 2, 3, 20, true, 0u);

auto g_walk_anim =
    cps2::SpriteAnim::from_sheet(FIGHTER_SHEET, 2, 2, 3, 8, true, 12u);
} // namespace

void cps2::on_vblank() {
  g_frame = g_frame + 1;
  g_input.poll();

  g_moving = false;
  if (g_input.held(cps2::Button::Right)) {
    g_facing_left = false;
    g_moving = true;
    if (g_px < 232) {
      g_px += 2;
    } else {
      g_world_x += 2;
    }
  }
  if (g_input.held(cps2::Button::Left)) {
    g_facing_left = true;
    g_moving = true;
    if (g_px > 104) {
      g_px -= 2;
    } else {
      g_world_x -= 2;
    }
  }

  if (g_on_ground && g_input.pressed(cps2::Button::B1)) {
    g_vy = -8;
    g_on_ground = false;
  }

  if (!g_on_ground) {
    g_py += g_vy;
    if ((g_frame & 1u) == 0u) {
      ++g_vy;
    }
    if (g_py >= kGroundY) {
      g_py = kGroundY;
      g_vy = 0;
      g_on_ground = true;
    }
  }

  uint32_t current_tile = FIGHTER_FIRST_TILE;
  if (!g_on_ground) {
    current_tile = FIGHTER_FIRST_TILE + 12u;
  } else if (g_moving) {
    g_walk_anim.tick();
    g_idle_anim.reset();
    current_tile = g_walk_anim.current_tile();
  } else {
    g_idle_anim.tick();
    g_walk_anim.reset();
    current_tile = g_idle_anim.current_tile();
  }

  cps2::Scroll3.set_pos(static_cast<int16_t>(g_world_x >> 2), 0);
  cps2::Scroll2.set_pos(static_cast<int16_t>(g_world_x), 0);

  cps2::Scroll1.print_hex(cps2::kScr1Left + 9, cps2::kScr1Top + 1, 1,
                          static_cast<uint32_t>(g_world_x), 4);

  {
    cps2::ScopedSpriteBatch batch;
    batch.draw(cps2::Sprite{
        .x = g_px,
        .y = g_py,
        .tile = current_tile,
        .palette = 0,
        .priority = 7,
        .w_tiles = 2,
        .h_tiles = 3,
        .flip = g_facing_left ? cps2::SpriteFlip::X : cps2::SpriteFlip::None,
    });
  }

  cps2::palette::commit();
}

int main() {
  cps2::video::init(cps2::LayerMask::All);
  cps2::sprite::clear_all();

  constexpr uint16_t space_bg = cps2::rgb(0x01, 0x01, 0x03);
  cps2::palette::set_backdrop(space_bg);

  cps2::palette::load(cps2::PalPage::Obj, 0, FIGHTER_SHEET);
  cps2::palette::load(cps2::PalPage::Scroll2, 0, FLOOR_SHEET);
  cps2::palette::load(cps2::PalPage::Scroll3, 0, STARS_SHEET);

  cps2::palette::set_entry(cps2::PalPage::Scroll1, 0, 1,
                           cps2::rgb(0x0f, 0x0f, 0x0f));
  cps2::palette::set_entry(cps2::PalPage::Scroll1, 1, 1,
                           cps2::rgb(0x04, 0x0f, 0x0e));
  cps2::palette::commit();

  cps2::Scroll3.clear(STARS_SHEET, 0);
  cps2::Scroll2.clear_transparent();
  cps2::Scroll2.fill_rect(0, 12, 64, 1, FLOOR_SHEET, 0, 0);
  cps2::Scroll2.fill_rect(0, 13, 64, 3, FLOOR_SHEET, 1, 0);

  cps2::Scroll1.clear_transparent();
  cps2::Scroll1.print(cps2::kScr1Left + 1, cps2::kScr1Top + 1, 0, "WORLD X:");

  for (;;) {
    cps2::halt_until_vblank();
  }
}
