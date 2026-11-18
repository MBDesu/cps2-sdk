# cps2-sdk

C++20 and C17 SDK and ROM packer for Capcom CPS2 hardware.

## Layout

- `sdk/` -- Freestanding `m68k-elf` C++20 headers (`cps2.h`, `cps2_*.h`), vanilla C17 compatibility header (`cps2_c.h`), `crt0.s` startup code, 16-channel Z80 QSound driver (`sdk/z80/`), linker script, and example ROMs (`cpp_hello`, `c_hello`, `sprite_demo`, `big_sprite_demo`, `audio_demo`).
- `tools/` -- Host `cps2_packer` utility for converting PNGs into CPS2 4bpp planar graphics ROMs (`.13`–`.20`), packing WAV audio into QSound sample ROMs (`.11`–`.12`) with an embedded Z80 QSound driver (`.01`), generating `assets.h` headers, and encrypting 68000 code with `.key` files.

## Requirements

- CMake 3.20+
- Host C++20 compiler (`clang++` or `g++`)
- Python 3 (for assembling `sdk/z80/qsound_driver.s` via `sdk/z80/asm_z80.py`)
- `m68k-elf-gcc` / `m68k-elf-g++`, `m68k-elf-objcopy`, and `vasmm68k_mot`

### macOS

```bash
brew install m68k-elf-gcc vasm
```

### Linux

- Debian / Ubuntu:
  ```bash
  sudo apt install build-essential cmake python3 git gcc-m68k-linux-gnu g++-m68k-linux-gnu binutils-m68k-linux-gnu
  ```
  Build `vasmm68k_mot` from [sun.hasenbraten.de/vasm](http://hasenbraten.de) (`make CPU=m68k SYNTAX=mot`) and place it on your `PATH`.

- Arch Linux:
  ```bash
  yay -S m68k-elf-gcc m68k-elf-binutils vasm
  ```

### Windows (MSYS2 UCRT64)

1. Install [MSYS2](https://msys2.org) and open the MSYS2 UCRT64 shell:
   ```bash
   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-python git
   ```
2. Place `m68k-elf-gcc`, `m68k-elf-g++`, `m68k-elf-objcopy`, and `vasmm68k_mot.exe` in `C:\msys64\ucrt64\bin` (or on your `PATH`).

## Building

Build `tools` and `sdk` together:

```bash
make -j
```

Or build each directory separately:

```bash
cmake -S tools -B tools/build
cmake --build tools/build -j

cmake -S sdk -B sdk/build
cmake --build sdk/build -j
```

Output `.zip` ROM sets:
- `sdk/build/examples/cpp_hello/cpp_hello.zip`
- `sdk/build/examples/c_hello/c_hello.zip`
- `sdk/build/examples/sprite_demo/sprite_demo.zip`
- `sdk/build/examples/big_sprite_demo/big_sprite_demo.zip`
- `sdk/build/examples/audio_demo/audio_demo.zip`

CMake builds example ROMs encrypted by default (`CPS2_ENCRYPT=ON`). Pass `-DCPS2_ENCRYPT=OFF` when configuring `sdk` to build unencrypted ROMs without a `.key` file:

```bash
cmake -S sdk -B sdk/build -DCPS2_ENCRYPT=OFF
cmake --build sdk/build -j
```

## Contributing

- Keep the C17 shim in sync: Whenever you add or modify public types, constants, or functions in `sdk/include/cps2*.h`, expose matching `extern "C"` wrappers in `sdk/include/cps2_c.h` and implement them in `sdk/src/cps2_c.cpp`.
- Verify all targets: Before submitting changes, run `make -j` from the repository root and confirm that all five example `.zip` ROMs build without compiler warnings.

## Minimal C++20 Example

Link against `cps2_sdk` and include `"cps2.h"`:

```cpp
#include "cps2.h"

namespace {
cps2::Input g_input;
} // namespace

void cps2::on_vblank() {
  g_input.poll();

  {
    cps2::ScopedSpriteBatch batch;
    batch.draw(cps2::Sprite{.x = 184, .y = 160, .tile = 0x0040});
  }

  cps2::palette::commit();
}

int main() {
  cps2::video::init(cps2::LayerMask::Obj | cps2::LayerMask::Scroll1);
  cps2::sprite::clear_all();

  cps2::palette::set_backdrop(cps2::rgb(0x01, 0x02, 0x05));
  cps2::palette::set_entry(cps2::PalPage::Scroll1, 0, 1,
                           cps2::rgb(0x0f, 0x0f, 0x0f));
  cps2::palette::commit();

  cps2::Scroll1.clear_transparent();
  cps2::Scroll1.print(cps2::kScr1Left + 2, cps2::kScr1Top + 2, 0,
                      "HELLO CPS2!");

  for (;;) {
    cps2::halt_until_vblank();
  }
}
```

## Vanilla C17 Usage

To write a project in pure C (`main.c`), include `"cps2_c.h"`, implement `void cps2_on_vblank(void)`, and link against `cps2_sdk_c` instead of `cps2_sdk`:

```c
#include "cps2_c.h"

void cps2_on_vblank(void) {
  cps2_input_poll();
  cps2_palette_commit();
}

int main(void) {
  cps2_video_init(CPS2_LAYER_OBJ | CPS2_LAYER_SCR1);
  cps2_sprite_clear_all();

  cps2_palette_set_backdrop(cps2_pal_color(0x01, 0x02, 0x05, 0x0f));
  cps2_palette_set_entry(CPS2_PAL_SCR1, 0, 1,
                         cps2_pal_color(0x0f, 0x0f, 0x0f, 0x0f));
  cps2_palette_commit();

  cps2_scroll_clear_transparent(1, 0);
  cps2_scroll_print(1, 10, 6, 0, "HELLO CPS2 FROM C!");

  for (;;) {
    cps2_halt_until_vblank();
  }
}
```

In your target's `CMakeLists.txt`, link `cps2_sdk_c` and set `-std=c17`:

```cmake
add_executable(my_game.elf main.c "\${CPS2_CRT0}")
set_target_properties(my_game.elf PROPERTIES SUFFIX "")
target_compile_options(my_game.elf PRIVATE -std=c17)
target_link_libraries(my_game.elf PRIVATE cps2_sdk_c)
add_dependencies(my_game.elf cps2_crt0)
```
## `cps2_packer` CLI

```bash
./tools/build/cps2_packer \
  --prog game.bin \
  --elf game.elf \
  --out-dir dist/ \
  --name hb \
  --header build/assets.h \
  --encrypt \
  --sprite HERO assets/hero.png 16 \
  --png BG assets/bg_tiles.png 16 \
  --sfx LASER assets/laser.wav \
  --bgm THEME assets/theme.wav
```

- `--sprite <PREFIX> <PNG> <TILE_SIZE>` Packs a sprite sheet into the object graphics region (`\$000000`+) and emits `<PREFIX>_FIRST_TILE`, `<PREFIX>_TILE_COUNT`, `<PREFIX>_PALETTE`, and `<PREFIX>_SHEET`.
- `--png <PREFIX> <PNG> <TILE_SIZE>` Packs a tileset into the scroll graphics region (`\$400000`+) and emits `<PREFIX>_FIRST_TILE`, `<PREFIX>_TILE_COUNT`, `<PREFIX>_PALETTE`, and `<PREFIX>_SHEET`.
- `--bgm <PREFIX> <WAV>`: Same as `--sfx`, emitting `#define <PREFIX>_ID` (with bit `0x80` set) so `cps2::sound::play_bgm(<PREFIX>_ID)` loops the track on QSound channel 0 and automatically streams longer songs across consecutive 64 KB QSound banks.
- `--bgm <PREFIX> <WAV>`: Same as `--sfx`, but marks the track to loop continuously on QSound channel 0 (`cps2::sound::play_bgm(<PREFIX>_ID)`) and automatically streams longer songs across consecutive 64 KB QSound banks.
- `--z80 <BIN>`: Optional override to pack your own custom Z80 sound driver binary into `<name>.01` (and `<name>.02` if larger than 128 KB) instead of the embedded default driver.
- `--qsound <BIN>`: Low-level option to copy a pre-built raw 8-bit signed PCM binary directly into `<name>.11` (and `<name>.12` if larger than 2 MB).
- `--encrypt`: Encrypts executable 68000 sections from `--elf` and writes `<name>.key` using the default key (`0x12345678` / `0x9abcdef0`). When invoking `cps2_packer` directly, output is unencrypted unless `--encrypt`, `--key`, or `--key-file` is passed.
- `--key <K0_HEX> <K1_HEX>`: Encrypts using a custom 64-bit key pair (implies `--encrypt`) and writes a matching `<name>.key`.
- `--key-file <PATH>`: Encrypts using an existing 20-byte `.key` file (implies `--encrypt`) and copies it to `<out-dir>/<name>.key`.

### Custom Z80 Audio Drivers

Whenever you pass `--sfx` or `--bgm`, `cps2_packer` automatically bakes the SDK's built-in 16-channel QSound Z80 driver (`sdk/z80/qsound_driver.s`) into `<name>.01`. Simply call `cps2::sound::init()` (`cps2_sound_init()` in C) on startup to release the Z80 from reset; however, you can supply your own Z80 driver (using `sdk/z80/qsound_driver.s` as a reference and assembling with `python3 sdk/z80/asm_z80.py my_driver.s my_driver.bin` or `vasmz80_oldstyle`), then pass `--z80 my_driver.bin` to `cps2_packer` in your project's `CMakeLists.txt`. If you still use `--sfx` / `--bgm` alongside `--z80`, keep `0x0400..0x07ff` free in your Z80 memory map for `cps2_packer`'s sample descriptor table, or you can just replace `sdk/z80/qsound_driver.s` entirely if you want your own driver to be baked in automatically without passing any flags. The world's your oyster.

### Sprite Sheet & Tileset Rules

| Rule | Requirement | Why |
| :--- | :--- | :--- |
| Dimensions | Exact multiples of `<TILE_SIZE>` (`8`, `16`, or `32` px) | `cps2_packer` rejects partial edge tiles |
| Multi-Tile Frames | Stack frames vertically (`W*16` × `H*16*frames`) | `cps2_packer` scans row-by-row; vertical strips keep each frame's sub-tiles contiguous (`0..N-1`) for `Sprite` (`w_tiles` × `h_tiles`) and `SpriteAnim::from_sheet` |
| Single-Tile Frames | Horizontal or vertical 16 px strips | Each frame is 1 tile wide, so both directions pack sequentially |
| Transparency | `alpha < 128` = transparent (`0x0f`) | CPS2 hardware reserves color index `15` (`0x0f`) for transparency |
| Color Limit | Max 15 opaque colors per PNG | Index `0` is reserved (`0x0000`) and index `15` is transparent, leaving `1..14` for colors |

- Always pass `16` for `--sprite` tile size. CPS2 object hardware renders all sprites out of `16x16` blocks. Build larger characters (`32x32`, `32x48`, `64x64`, etc.) by setting `w_tiles` and `h_tiles` on `cps2::Sprite` (`Cps2Sprite` in C17), and pad smaller sprites (`8x8` bullets) inside a `16x16` transparent frame.
- Multiple sprite sheets and palettes: Pass `--sprite` once per sprite sheet—for example, separate sheets for the player, enemies, and projectiles, or separate sheets when one character has animations of different sizes (such as `32x48` walking frames vs. `48x48` punch frames). Load each sheet's palette into one of the 32 object palette slots (`0..31`) via `cps2::palette::load(cps2::PalPage::Obj, slot, <PREFIX>_SHEET)`, and set `Sprite::palette = slot` when drawing.

### Audio & WAV Rules

| Rule | Requirement | Why |
| :--- | :--- | :--- |
| WAV Format | Uncompressed 8-bit or 16-bit PCM `.wav` (mono or stereo) | `cps2_packer` mixes stereo to mono and converts to 8-bit signed PCM |
| Clip Length | `--sfx`: up to 1.36s (`32,736` samples); `--bgm`: up to 42.5s (`32` banks) | Each QSound hardware voice addresses `0x0000..0x7fff` (`s16`) per 64 KB bank; longer `--bgm` tracks are sliced across banks and chained by the Z80 252 Hz timer |
| Max Clips | Up to 127 total `--sfx` and `--bgm` entries per ROM | Z80 sample table (`0x0400..0x07ff` in `<name>.01`) reserves 128 × 16-byte slots (`id = 1..127`) |
| Startup Init | Call `cps2::sound::init()` (`cps2_sound_init()` in C) once at boot | Releases the Z80 co-processor from reset (`0x804040`) so the QSound driver boots |

## TODO
- Unit tests: ctest suites for `tools/` (`CPS2Crypt` round-trip, key encoding/decoding, planar 4bpp tile interleaving) and `sdk/` (`constexpr` RGB/tile packing and C/C++ ABI checks).
- Raster effects and hardware math (`sdk/include/cps2_raster.h`): Scanline row-scroll tables and CPS-B 16x16 hardware multiplier wrappers.
- More higher level abstractions in general
