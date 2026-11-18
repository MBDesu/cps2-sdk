#include "../include/cps2.h"
#include "assets.h"

namespace {
cps2::Input g_input;
uint8_t g_cursor = 0;
int16_t g_pan_x = 192;
bool g_paused = false;

constexpr const char *kLabels[] = {
    "1: LASER         (BTN 1)",
    "2: POWERUP       (BTN 2)",
    "3: EXPLOSION     (BTN 3)",
    "4: SYNTH BASS    (START)",
};

constexpr uint16_t kSampleIds[] = {
    LASER_ID,
    CHIME_ID,
    EXPLOSION_ID,
    BASS_ID,
};
} // namespace

void cps2::on_vblank() {
  g_input.poll();

  if (g_input.pressed(cps2::Button::Down)) {
    g_cursor = static_cast<uint8_t>((g_cursor + 1u) & 3u);
  }
  if (g_input.pressed(cps2::Button::Up)) {
    g_cursor = static_cast<uint8_t>((g_cursor + 3u) & 3u);
  }
  if (g_input.held(cps2::Button::Left) && g_pan_x > 8) {
    g_pan_x -= 4;
  }
  if (g_input.held(cps2::Button::Right) && g_pan_x < 375) {
    g_pan_x += 4;
  }

  if (g_input.pressed(cps2::Button::B1)) {
    const uint16_t id = kSampleIds[g_cursor];
    if (id == BASS_ID) {
      g_paused = false;
      cps2::sound::play_bgm(id);
    } else {
      cps2::sound::play_sfx_at(id, g_pan_x);
    }
  }
  if (g_input.pressed(cps2::Button::B2)) {
    cps2::sound::play_sfx_at(CHIME_ID, g_pan_x);
  }
  if (g_input.pressed(cps2::Button::B3)) {
    cps2::sound::play_sfx_at(EXPLOSION_ID, g_pan_x);
  }
  if (g_input.pressed(cps2::Button::B4)) {
    g_paused = !g_paused;
    if (g_paused) {
      cps2::sound::pause();
    } else {
      cps2::sound::resume();
    }
  }
  if (g_input.pressed(cps2::Button::B5)) {
    g_paused = false;
    cps2::sound::fade_out(4u);
  }
  if (g_input.pressed(cps2::Button::B6)) {
    cps2::sound::stop_bgm();
  }
  if (g_input.pressed(cps2::Button::Start)) {
    g_paused = false;
    cps2::sound::play_bgm(BASS_ID);
  }
  if (g_input.pressed(cps2::Button::Coin)) {
    g_paused = false;
    cps2::sound::stop_all();
  }

  for (uint8_t i = 0; i < 4u; ++i) {
    cps2::Scroll1.put(cps2::kScr1Left + 11, cps2::kScr1Top + 8 + i * 2,
                      (i == g_cursor) ? '>' : ' ', 1);
  }

  const uint8_t pan = cps2::sound::pan_from_x(g_pan_x);
  cps2::Scroll1.print_hex(cps2::kScr1Left + 17, cps2::kScr1Top + 17, 1, pan, 2);
  cps2::Scroll1.print(cps2::kScr1Left + 30, cps2::kScr1Top + 17, 1,
                      cps2::sound::is_ready() ? "READY" : "BUSY ");
  cps2::Scroll1.print(cps2::kScr1Left + 24, cps2::kScr1Top + 19, 1,
                      g_paused ? "PAUSED " : "ACTIVE ");

  {
    cps2::ScopedSpriteBatch batch;
    batch.draw(cps2::Sprite{
        .x = static_cast<int16_t>(g_pan_x - 8),
        .y = 166,
        .tile = 0x0040u,
        .palette = 0,
        .priority = 7,
    });
  }

  cps2::palette::commit();
}

int main() {
  cps2::video::init(cps2::LayerMask::Scroll3 | cps2::LayerMask::Scroll1 |
                    cps2::LayerMask::Obj);
  cps2::sprite::clear_all();

  constexpr uint16_t bg = cps2::rgb(0x02, 0x04, 0x09);
  cps2::video::set_background(bg);

  cps2::palette::set_entry(cps2::PalPage::Scroll1, 0, 1,
                           cps2::rgb(0x0f, 0x0f, 0x0f));
  cps2::palette::set_entry(cps2::PalPage::Scroll1, 1, 1,
                           cps2::rgb(0x04, 0x0f, 0x0a));
  cps2::palette::set_entry(cps2::PalPage::Obj, 0, 1,
                           cps2::rgb(0x0f, 0x0c, 0x02));
  cps2::palette::commit();

  cps2::sound::init();
  cps2::sound::set_volume(0x7fu);
  cps2::sound::play_bgm(BASS_ID);

  cps2::Scroll1.clear_transparent();
  cps2::Scroll1.box(cps2::kScr1Left + 2, cps2::kScr1Top + 2, 44, 24, 0);
  cps2::Scroll1.print(cps2::kScr1Left + 13, cps2::kScr1Top + 4, 1,
                      "CPS2 QSOUND AUDIO DEMO");

  for (uint8_t i = 0; i < 4u; ++i) {
    cps2::Scroll1.print(cps2::kScr1Left + 13, cps2::kScr1Top + 8 + i * 2, 0,
                        kLabels[i]);
  }

  cps2::Scroll1.print(cps2::kScr1Left + 12, cps2::kScr1Top + 17, 0, "PAN:");
  cps2::Scroll1.print(cps2::kScr1Left + 25, cps2::kScr1Top + 17, 0, "Z80:");
  cps2::Scroll1.print(cps2::kScr1Left + 17, cps2::kScr1Top + 19, 0, "STATE:");
  cps2::Scroll1.print(cps2::kScr1Left + 10, cps2::kScr1Top + 22, 0,
                      "B4:PAUSE B5:FADE B6:STOP BGM");
  cps2::Scroll1.print(cps2::kScr1Left + 10, cps2::kScr1Top + 24, 0,
                      "L/R:PAN  START:BGM  COIN:STOP");

  for (;;) {
    cps2::halt_until_vblank();
  }
}
