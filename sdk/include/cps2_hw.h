#pragma once
#ifndef CPS2_SDK_HW_HPP
#define CPS2_SDK_HW_HPP

#include <stdint.h>

namespace cps2 {

inline constexpr int kScreenWidth = 384;
inline constexpr int kScreenHeight = 224;

inline constexpr int kScr1Left = 8;
inline constexpr int kScr1Top = 2;
inline constexpr int kScr1Cols = 48;
inline constexpr int kScr1Rows = 28;

struct CpsARegs {
  volatile uint16_t obj_base;       /* +$00: $800100 */
  volatile uint16_t scroll1_base;   /* +$02: $800102 */
  volatile uint16_t scroll2_base;   /* +$04: $800104 */
  volatile uint16_t scroll3_base;   /* +$06: $800106 */
  volatile uint16_t rowscroll_base; /* +$08: $800108 */
  volatile uint16_t palette_base;   /* +$0a: $80010a */
  volatile uint16_t scroll1_x;      /* +$0c: $80010c */
  volatile uint16_t scroll1_y;      /* +$0e: $80010e */
  volatile uint16_t scroll2_x;      /* +$10: $800110 */
  volatile uint16_t scroll2_y;      /* +$12: $800112 */
  volatile uint16_t scroll3_x;      /* +$14: $800114 */
  volatile uint16_t scroll3_y;      /* +$16: $800116 */
};

struct CpsBRegs {
  volatile uint16_t mult1;       /* +$00: $800140 */
  volatile uint16_t mult2;       /* +$02: $800142 */
  volatile uint16_t mult_res_lo; /* +$04: $800144 */
  volatile uint16_t mult_res_hi; /* +$06: $800146 */
  uint16_t _pad0[15];            /* +$08..+$24 */
  volatile uint16_t layer_ctrl;  /* +$26: $800166 */
  volatile uint16_t pri_mask[4]; /* +$28..+$2e */
  volatile uint16_t pal_ctrl;    /* +$30: $800170 */
  volatile uint16_t board_id;    /* +$32: $800172 */
};

inline CpsARegs &cpsa() { return *reinterpret_cast<CpsARegs *>(0x800100u); }

inline CpsBRegs &cpsb() { return *reinterpret_cast<CpsBRegs *>(0x800140u); }

inline volatile uint16_t &reg_in0() {
  return *reinterpret_cast<volatile uint16_t *>(0x804000u);
}

inline volatile uint16_t &reg_in1() {
  return *reinterpret_cast<volatile uint16_t *>(0x804010u);
}

inline volatile uint16_t &reg_in2() {
  return *reinterpret_cast<volatile uint16_t *>(0x804020u);
}

inline volatile uint16_t &reg_obj_bank() {
  return *reinterpret_cast<volatile uint16_t *>(0x8040e0u);
}

inline volatile uint16_t &reg_obj_pri() {
  return *reinterpret_cast<volatile uint16_t *>(0x400004u);
}

inline volatile uint16_t *gfx_ram() {
  return reinterpret_cast<volatile uint16_t *>(0x900000u);
}

inline volatile uint16_t *obj_ram() {
  return reinterpret_cast<volatile uint16_t *>(0x700000u);
}

inline void halt_until_vblank() { __asm__ volatile("stop #0x2000"); }

void on_vblank();

} // namespace cps2

#endif // CPS2_SDK_HW_HPP
