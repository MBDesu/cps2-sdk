#pragma once
#ifndef SDK_CPS2_MODULE_H
#define SDK_CPS2_MODULE_H

#include "cps2_input.h"
#include "cps2_palette.h"
#include "cps2_scroll.h"
#include "cps2_sprite.h"

static inline uint16_t cps2_rgb(uint8_t bright, uint8_t r, uint8_t g,
                                uint8_t b) {
  return ((bright & 0xfu) << 12) | ((r & 0xfu) << 8) | ((g & 0xfu) << 4) |
         (b & 0xfu);
}

static inline uint32_t cps2_scroll1_idx(int col, int row) {
  return (uint32_t)(((row & 0x20) << 6) | ((col & 0x3f) << 5) | (row & 0x1f));
}

static inline void cps2_scroll1_put(volatile uint16_t *map, int col, int row,
                                    uint16_t tile, uint16_t attr) {
  const uint32_t idx = cps2_scroll1_idx(col, row);
  map[idx * 2] = tile;
  map[idx * 2 + 1] = attr;
}

#endif
