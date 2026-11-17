#include "cps2_c.h"

static volatile uint32_t g_frame = 0;

void cps2_on_vblank(void) {
  cps2_input_poll();
  ++g_frame;

  uint8_t pulse = (uint8_t)((g_frame >> 2) & 0x0fu);
  cps2_palette_set_entry(CPS2_PAL_SCR1, 1, 1,
                         cps2_pal_color(pulse, 0x0fu, 0x0fu - pulse, 0x0fu));
  cps2_palette_commit();

  cps2_scroll_print_uint(1, CPS2_SCR1_LEFT + 17, CPS2_SCR1_TOP + 12, 1, g_frame,
                         6);
}

int main(void) {
  cps2_video_init(CPS2_LAYER_OBJ | CPS2_LAYER_SCR1);
  cps2_sprite_clear_all();

  cps2_palette_set_backdrop(cps2_pal_color(0x01, 0x02, 0x05, 0x0f));
  cps2_palette_set_entry(CPS2_PAL_SCR1, 0, 1,
                         cps2_pal_color(0x0f, 0x0f, 0x0f, 0x0f));
  cps2_palette_set_entry(CPS2_PAL_SCR1, 1, 1,
                         cps2_pal_color(0x0f, 0x0f, 0x00, 0x0f));
  cps2_palette_commit();

  cps2_scroll_clear(1, ' ', 0);
  cps2_scroll_box(1, CPS2_SCR1_LEFT + 6, CPS2_SCR1_TOP + 4, 28, 12, 0);
  cps2_scroll_print(1, CPS2_SCR1_LEFT + 10, CPS2_SCR1_TOP + 6, 1,
                    "CPS2-SDK VANILLA C17");
  cps2_scroll_print(1, CPS2_SCR1_LEFT + 9, CPS2_SCR1_TOP + 9, 0,
                    "BUILT VIA CPS2_C.H SHIM");
  cps2_scroll_print(1, CPS2_SCR1_LEFT + 10, CPS2_SCR1_TOP + 12, 0, "FRAME:");

  for (;;) {
    cps2_halt_until_vblank();
  }
}
