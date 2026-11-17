#pragma once
#ifndef CPS2_SDK_CPS2_C_MODULE
#define CPS2_SDK_CPS2_C_MODULE
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Layer masks
enum {
  CPS2_LAYER_OBJ = 1u << 0,
  CPS2_LAYER_SCR1 = 1u << 1,
  CPS2_LAYER_SCR2 = 1u << 2,
  CPS2_LAYER_SCR3 = 1u << 3,
  CPS2_LAYER_ALL = 0x0fu
};

// Palette pages
enum {
  CPS2_PAL_OBJ = 0,
  CPS2_PAL_SCR1 = 1,
  CPS2_PAL_SCR2 = 2,
  CPS2_PAL_SCR3 = 3
};

// Buttons
enum {
  CPS2_BTN_RIGHT = 1u << 0,
  CPS2_BTN_LEFT = 1u << 1,
  CPS2_BTN_DOWN = 1u << 2,
  CPS2_BTN_UP = 1u << 3,
  CPS2_BTN_B1 = 1u << 4,
  CPS2_BTN_B2 = 1u << 5,
  CPS2_BTN_B3 = 1u << 6,
  CPS2_BTN_START = 1u << 8,
  CPS2_BTN_COIN = 1u << 9
};

// Viewport constants
enum {
  CPS2_SCR1_LEFT = 8,
  CPS2_SCR1_TOP = 2,
  CPS2_SCR1_COLS = 48,
  CPS2_SCR1_ROWS = 28,
};

// Sprite flip flags
enum {
  CPS2_SPR_NONE = 0u,
  CPS2_SPR_FLIP_X = 1u << 0,
  CPS2_SPR_FLIP_Y = 1u << 1,
  CPS2_SPR_FLIP_XY = 3u
};

typedef struct Cps2SpriteAnim {
  uint32_t base_tile;
  uint16_t frame_count;
  uint16_t tiles_per_frame;
  uint8_t ticks_per_frame;
  bool loop;
  uint16_t frame;
  uint8_t timer;
  bool finished;
} Cps2SpriteAnim;

void cps2_scroll_fill_rect(uint8_t plane, int col, int row, int width,
                           int height, uint16_t tile, uint8_t pal);
void cps2_scroll_box(uint8_t plane, int col, int row, int width, int height,
                     uint8_t pal);
void cps2_scroll_print_uint(uint8_t plane, int col, int row, uint8_t pal,
                            uint32_t value, int min_digits);

void cps2_batch_draw_meta(int16_t x, int16_t y, uint32_t first_tile,
                          uint8_t w_tiles, uint8_t h_tiles, uint8_t pal,
                          uint8_t prio, uint8_t flip);
void cps2_anim_reset(Cps2SpriteAnim *anim);
void cps2_anim_tick(Cps2SpriteAnim *anim);
uint32_t cps2_anim_tile(const Cps2SpriteAnim *anim);

// User-implemented VBlank callback
void cps2_on_vblank(void);

// System & Video
void cps2_halt_until_vblank(void);
void cps2_video_init(uint16_t layer_mask);
void cps2_scroll2_pos(int16_t x, int16_t y);
void cps2_scroll3_pos(int16_t x, int16_t y);

// Palette
uint16_t cps2_pal_color(uint8_t r, uint8_t g, uint8_t b, uint8_t bright);
void cps2_palette_set_backdrop(uint16_t color);
void cps2_palette_set_entry(uint8_t page, uint8_t slot, uint8_t index,
                            uint16_t color);
void cps2_palette_load(uint8_t page, uint8_t slot, const uint16_t *colors16);
void cps2_palette_commit(void);

// Scroll Planes (1, 2, 3)
void cps2_scroll_clear(uint8_t plane, uint16_t tile, uint8_t pal);
void cps2_scroll_put(uint8_t plane, int col, int row, uint16_t tile,
                     uint8_t pal);
void cps2_scroll_print(uint8_t plane, int col, int row, uint8_t pal,
                       const char *str);
void cps2_scroll_print_hex(uint8_t plane, int col, int row, uint8_t pal,
                           uint32_t val, int digits);

// Input
void cps2_input_poll(void);
bool cps2_input_held(uint16_t button, int player);
bool cps2_input_pressed(uint16_t button, int player);

// Sprites
void cps2_sprite_clear_all(void);
void cps2_batch_begin(void);
void cps2_batch_draw(int16_t x, int16_t y, uint32_t tile, uint8_t pal,
                     uint8_t prio, uint8_t flip);
void cps2_batch_end(void);

#ifdef __cplusplus
}
#endif
#endif
