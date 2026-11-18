#pragma once
#ifndef CPS2_SDK_INPUT_H
#define CPS2_SDK_INPUT_H

#include "cps2_hw.h"

#include <stdint.h>

namespace cps2 {

enum class Button : uint16_t {
  None = 0,
  Right = 1u << 0,
  Left = 1u << 1,
  Down = 1u << 2,
  Up = 1u << 3,
  B1 = 1u << 4,
  B2 = 1u << 5,
  B3 = 1u << 6,
  B4 = 1u << 7,
  B5 = 1u << 8,
  B6 = 1u << 9,
  Start = 1u << 10,
  Coin = 1u << 11,
};

enum class SysButton : uint8_t {
  None = 0,
  Service = 1u << 0,
  Test = 1u << 1,
};

enum class In2Bit : uint16_t {
  EepromDo = 1u << 0,
  P1Start = 1u << 8,
  P2Start = 1u << 9,
  Service = 1u << 10,
  P1Coin = 1u << 12,
  P2Coin = 1u << 13,
  Test = 1u << 14,
};

constexpr Button operator|(Button a, Button b) {
  return static_cast<Button>(static_cast<uint16_t>(a) |
                             static_cast<uint16_t>(b));
}

constexpr SysButton operator|(SysButton a, SysButton b) {
  return static_cast<SysButton>(static_cast<uint8_t>(a) |
                                static_cast<uint8_t>(b));
}

constexpr bool has_bit(uint16_t reg, In2Bit bit) {
  return (reg & static_cast<uint16_t>(bit)) != 0;
}

namespace detail {
inline constexpr uint16_t kIn0PlayerMask =
    0x007fu; // Bits 0..6: Right, Left, Down, Up, B1, B2, B3
inline constexpr uint16_t kIn1PlayerMask = 0x0007u; // Bits 0..2: B4, B5, B6
inline constexpr uint8_t kP2ByteShift =
    8u; // Player 2 occupies bits 8..15 of IN0/IN1
inline constexpr uint8_t kKickButtonShift =
    7u; // B4..B6 sit at bits 7..9 in Button

constexpr uint16_t unpack_player_buttons(uint16_t in0, uint16_t in1,
                                         uint8_t byte_shift) {
  const uint16_t joy_and_punches =
      static_cast<uint16_t>((in0 >> byte_shift) & kIn0PlayerMask);
  const uint16_t kicks = static_cast<uint16_t>(
      ((in1 >> byte_shift) & kIn1PlayerMask) << kKickButtonShift);
  return static_cast<uint16_t>(joy_and_punches | kicks);
}

constexpr uint8_t unpack_sys_buttons(uint16_t in2) {
  uint8_t sys = 0;
  if (has_bit(in2, In2Bit::Service)) {
    sys |= static_cast<uint8_t>(SysButton::Service);
  }
  if (has_bit(in2, In2Bit::Test)) {
    sys |= static_cast<uint8_t>(SysButton::Test);
  }
  return sys;
}
} // namespace detail

class Input {
public:
  void poll() {
    const uint16_t in0 = static_cast<uint16_t>(~reg_in0());
    const uint16_t in1 = static_cast<uint16_t>(~reg_in1());
    const uint16_t in2 = static_cast<uint16_t>(~reg_in2());

    uint16_t p1 = detail::unpack_player_buttons(in0, in1, 0u);
    if (has_bit(in2, In2Bit::P1Start)) {
      p1 |= static_cast<uint16_t>(Button::Start);
    }
    if (has_bit(in2, In2Bit::P1Coin)) {
      p1 |= static_cast<uint16_t>(Button::Coin);
    }

    uint16_t p2 = detail::unpack_player_buttons(in0, in1, detail::kP2ByteShift);
    if (has_bit(in2, In2Bit::P2Start)) {
      p2 |= static_cast<uint16_t>(Button::Start);
    }
    if (has_bit(in2, In2Bit::P2Coin)) {
      p2 |= static_cast<uint16_t>(Button::Coin);
    }

    const uint16_t raw[2] = {p1, p2};
    for (int i = 0; i < 2; ++i) {
      const uint16_t prev = held_[i];
      const uint16_t curr = raw[i];
      pressed_[i] = static_cast<uint16_t>(curr & ~prev);
      released_[i] = static_cast<uint16_t>(prev & ~curr);
      held_[i] = curr;
    }

    const uint8_t sys = detail::unpack_sys_buttons(in2);
    sys_pressed_ = static_cast<uint8_t>(sys & ~sys_held_);
    sys_held_ = sys;
  }

  [[nodiscard]] bool held(Button btn, int player = 0) const {
    return (held_[player & 1] & static_cast<uint16_t>(btn)) != 0;
  }

  [[nodiscard]] bool pressed(Button btn, int player = 0) const {
    return (pressed_[player & 1] & static_cast<uint16_t>(btn)) != 0;
  }

  [[nodiscard]] bool released(Button btn, int player = 0) const {
    return (released_[player & 1] & static_cast<uint16_t>(btn)) != 0;
  }

  [[nodiscard]] bool sys_held(SysButton btn) const {
    return (sys_held_ & static_cast<uint8_t>(btn)) != 0;
  }

  [[nodiscard]] bool sys_pressed(SysButton btn) const {
    return (sys_pressed_ & static_cast<uint8_t>(btn)) != 0;
  }

private:
  uint16_t held_[2]{};
  uint16_t pressed_[2]{};
  uint16_t released_[2]{};
  uint8_t sys_held_ = 0;
  uint8_t sys_pressed_ = 0;
};

} // namespace cps2

#endif // CPS2_SDK_INPUT_H
