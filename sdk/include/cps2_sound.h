#pragma once
#ifndef CPS2_SDK_SOUND_H
#define CPS2_SDK_SOUND_H

#include <stdint.h>

namespace cps2 {

inline constexpr uint32_t kQSoundSharedBase = 0x618000u;
inline constexpr uint32_t kQSoundSharedSize = 0x002000u;

// standard Capcom QSound Z80 system commands
enum class SoundSysCmd : uint16_t {
  StopAll = 0xff00u,
  FadeOut = 0xff01u,
  Pause = 0xff02u,
  Resume = 0xff03u,
  StopBgm = 0xff04u,
  StereoMode = 0xff10u,
  MonoMode = 0xff11u,
};

namespace sound {

// access shared Z80 RAM (68000 sees Z80 bytes at odd offsets: base + index * 2
// + 1)
inline volatile uint8_t &shared_byte(uint16_t z80_offset) {
  return *reinterpret_cast<volatile uint8_t *>(
      kQSoundSharedBase + (static_cast<uint32_t>(z80_offset) << 1) + 1u);
}

inline void write_shared(uint16_t z80_offset, uint8_t value) {
  shared_byte(z80_offset) = value;
}

[[nodiscard]] inline uint8_t read_shared(uint16_t z80_offset) {
  return shared_byte(z80_offset);
}

// release the Z80 sound CPU from reset via the CPS2 board output latch
// (0x804040)
inline void init() {
  write_shared(0x0000u, 0x01u); // Mark busy until Z80 boots and clears 0x0000
  *reinterpret_cast<volatile uint16_t *>(0x804040u) = 0x0808u;
}

enum class SoundPan : uint8_t {
  Left = 0x00u,
  Center = 0x10u,
  Right = 0x20u,
};

[[nodiscard]] constexpr uint8_t pan_from_x(int16_t screen_x) {
  if (screen_x <= 0) {
    return 0x00u;
  }
  if (screen_x >= 383) {
    return 0x20u;
  }
  return static_cast<uint8_t>((static_cast<uint32_t>(screen_x) * 32u) / 383u);
}

// send a raw 16-bit command + optional parameter byte to the Z80 command queue
inline void send_command(uint16_t cmd, uint8_t param = 0x00u) {
  for (uint16_t spin = 0; spin < 2048u; ++spin) {
    if (read_shared(0x0000u) == 0x00u) {
      break;
    }
  }
  write_shared(0x0002u, static_cast<uint8_t>((cmd >> 8) & 0xffu));
  write_shared(0x0003u, static_cast<uint8_t>(cmd & 0xffu));
  write_shared(0x0004u, param);
  // Strobe command ready (non-zero triggers Z80 IRQ/poll pickup)
  write_shared(0x0000u, 0x01u);
}

inline void play_sfx(uint16_t sfx_id, uint8_t pan = 0x10u) {
  send_command(sfx_id, pan);
}

inline void play_sfx_at(uint16_t sfx_id, int16_t screen_x) {
  play_sfx(sfx_id, pan_from_x(screen_x));
}

// check if the Z80 driver has finished booting and cleared the init flag
[[nodiscard]] inline bool is_ready() { return read_shared(0x0000u) == 0x00u; }

inline void play_bgm(uint16_t bgm_id) { send_command(bgm_id, 0x10u); }

inline void stop_bgm() {
  send_command(static_cast<uint16_t>(SoundSysCmd::StopBgm));
}

inline void pause() { send_command(static_cast<uint16_t>(SoundSysCmd::Pause)); }

inline void resume() {
  send_command(static_cast<uint16_t>(SoundSysCmd::Resume));
}

inline void stop_all() {
  send_command(static_cast<uint16_t>(SoundSysCmd::StopAll));
}

inline void fade_out(uint8_t speed = 0x04u) {
  send_command(static_cast<uint16_t>(SoundSysCmd::FadeOut), speed);
}

inline void set_volume(uint8_t volume) {
  const uint8_t clamped = (volume > 0x7fu) ? 0x7fu : volume;
  write_shared(0x0008u, clamped);
}

} // namespace sound
} // namespace cps2

#endif // CPS2_SDK_SOUND_H
