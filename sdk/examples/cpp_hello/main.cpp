#include "../include/cps2.h"

namespace {
volatile uint32_t g_frame_count = 0;
cps2::Input g_input;
int16_t g_cam_x = 0;
int16_t g_cam_y = 0;
} // namespace

void cps2::on_vblank() {
  g_frame_count = g_frame_count + 1;
  g_input.poll();

  if (g_input.held(cps2::Button::Right))
    g_cam_x += 2;
  if (g_input.held(cps2::Button::Left))
    g_cam_x -= 2;
  if (g_input.held(cps2::Button::Down))
    g_cam_y += 2;
  if (g_input.held(cps2::Button::Up))
    g_cam_y -= 2;

  cps2::set_scroll1_pos(g_cam_x, g_cam_y);

  const uint8_t c = static_cast<uint8_t>((g_frame_count >> 2) & 0x0fu);
  cps2::palette::set_entry(cps2::PalPage::Scroll1, 0, 1,
                           cps2::rgb(0x0f, 0x0f - (c >> 1), c));
  cps2::palette::commit();
}

int main() {
  cps2::video::init(cps2::LayerMask::Scroll1 | cps2::LayerMask::Obj);
  cps2::sprite::clear_all();

  constexpr uint16_t bg_color = cps2::rgb(0x01, 0x02, 0x05);
  cps2::palette::set_backdrop(bg_color);
  cps2::palette::set_entry(cps2::PalPage::Scroll1, 0, 0, bg_color);
  cps2::palette::set_entry(cps2::PalPage::Scroll1, 0, 1,
                           cps2::rgb(0x0f, 0x0e, 0x04));
  cps2::palette::set_entry(cps2::PalPage::Scroll1, 1, 1,
                           cps2::rgb(0x02, 0x0f, 0x0f));
  cps2::palette::commit();

  cps2::Scroll1.clear(' ', 0);
  cps2::Scroll1.print(cps2::kScr1Left + 15, cps2::kScr1Top + 10, 0,
                      "HELLO CPS2 C++20!");
  cps2::Scroll1.print(cps2::kScr1Left + 11, cps2::kScr1Top + 13, 1,
                      "USE D-PAD / WASD TO SCROLL");
  cps2::Scroll1.print(cps2::kScr1Left + 12, cps2::kScr1Top + 16, 1,
                      "CMAKE + ENCRYPTED READY");

  cps2::Scroll1.box(cps2::kScr1Left, cps2::kScr1Top, cps2::kScr1Cols,
                    cps2::kScr1Rows, 0x0001, 0);

  for (;;) {
    cps2::halt_until_vblank();
  }
}
