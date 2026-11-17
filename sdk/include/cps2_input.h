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

constexpr Button operator|(Button a, Button b) {
  return static_cast<Button>(static_cast<uint16_t>(a) |
                             static_cast<uint16_t>(b));
}

constexpr SysButton operator|(SysButton a, SysButton b) {
  return static_cast<SysButton>(static_cast<uint8_t>(a) |
                                static_cast<uint8_t>(b));
}

class Input {
public:
  void poll() {
    const uint16_t in0 = static_cast<uint16_t>(~reg_in0());
    const uint16_t in1 = static_cast<uint16_t>(~reg_in1());
    const uint16_t in2 = static_cast<uint16_t>(~reg_in2());

    uint16_t p1 = static_cast<uint16_t>(in0 & 0x007fu);
    p1 |= static_cast<uint16_t>((in1 & 0x0007u) << 7);
    if (in2 & (1u << 0))
      p1 |= static_cast<uint16_t>(Button::Start);
    if (in2 & (1u << 4))
      p1 |= static_cast<uint16_t>(Button::Coin);

    uint16_t p2 = static_cast<uint16_t>((in0 >> 8) & 0x007fu);
    p2 |= static_cast<uint16_t>(((in1 >> 8) & 0x0007u) << 7);
    if (in2 & (1u << 1))
      p2 |= static_cast<uint16_t>(Button::Start);
    if (in2 & (1u << 5))
      p2 |= static_cast<uint16_t>(Button::Coin);

    const uint16_t raw[2] = {p1, p2};
    for (int i = 0; i < 2; ++i) {
      const uint16_t prev = held_[i];
      const uint16_t curr = raw[i];
      pressed_[i] = static_cast<uint16_t>(curr & ~prev);
      released_[i] = static_cast<uint16_t>(prev & ~curr);
      held_[i] = curr;
    }

    uint8_t sys = 0;
    if (in2 & (1u << 2))
      sys |= static_cast<uint8_t>(SysButton::Service);
    if (in2 & (1u << 6))
      sys |= static_cast<uint8_t>(SysButton::Test);
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
