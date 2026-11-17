# cps2-sdk

C++20 and C17 SDK and ROM packer for Capcom CPS2 hardware.

## Layout

- `sdk/` — Freestanding `m68k-elf` C++20 headers (`cps2.h`, `cps2_*.h`), vanilla C17 compatibility header (`cps2_c.h`), `crt0.s` startup code, linker script, and example ROMs (`cpp_hello`, `c_hello`, `sprite_demo`).
- `tools/` — Host `cps2_packer` utility for converting PNGs into CPS2 4bpp planar graphics ROMs (`.13`–`.20`), generating `assets.h` headers, and encrypting 68000 code with `.key` files.

## Requirements

- CMake 3.20+
- Host C++20 compiler (`clang++` or `g++`)
- Python 3 (for `sprite_demo` asset generation)
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

CMake builds example ROMs encrypted by default (`CPS2_ENCRYPT=ON`). Pass `-DCPS2_ENCRYPT=OFF` when configuring `sdk` to build unencrypted ROMs without a `.key` file:

```bash
cmake -S sdk -B sdk/build -DCPS2_ENCRYPT=OFF
cmake --build sdk/build -j
```

## Contributing

- Keep the C17 shim in sync: Whenever you add or modify public types, constants, or functions in `sdk/include/cps2*.h`, expose matching `extern "C"` wrappers in `sdk/include/cps2_c.h` and implement them in `sdk/src/cps2_c.cpp`.
- Verify all targets: Before submitting changes, run `make -j` from the repository root and confirm that `cpp_hello.zip`, `c_hello.zip`, and `sprite_demo.zip` build without compiler warnings.

## Minimal C++20 Example

Link against `cps2_sdk` and include `"cps2.h"`:

```cpp
#include "cps2.h"

namespace {
cps2::Input g_input;
cps2::SpriteBatch g_batch;
} // namespace

void cps2::on_vblank() {
  g_input.poll();

  g_batch.begin();
  g_batch.draw(184, 160, 0x0040, 0, 7);
  g_batch.end();

  cps2::palette::commit();
}

int main() {
  cps2::video::init(cps2::LayerMask::Obj | cps2::LayerMask::Scroll1);
  cps2::sprite::clear_all();

  cps2::palette::set_backdrop(cps2::rgb(0x01, 0x02, 0x05));
  cps2::palette::set_entry(cps2::PalPage::Scroll1, 0, 1,
                           cps2::rgb(0x0f, 0x0f, 0x0f));
  cps2::palette::commit();

  cps2::Scroll1.clear(' ', 0);
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

  cps2_scroll_clear(1, ' ', 0);
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
  --png BG assets/bg_tiles.png 16
```

- `--sprite <PREFIX> <PNG> <TILE_SIZE>` — Packs a sprite sheet into the object graphics region (`$000000`+) and emits `<PREFIX>_FIRST_TILE`, `<PREFIX>_TILE_COUNT`, and `<PREFIX>_PALETTE`.
- `--png <PREFIX> <PNG> <TILE_SIZE>` — Packs a tileset into the scroll graphics region (`$400000`+) and emits tile/palette constants.
- `--encrypt` — Encrypts executable 68000 sections from `--elf` and writes `<name>.key` using the default key (`0x12345678` / `0x9abcdef0`). When invoking `cps2_packer` directly, output is unencrypted unless `--encrypt`, `--key`, or `--key-file` is passed.
- `--key <K0_HEX> <K1_HEX>` — Encrypts using a custom 64-bit key pair (implies `--encrypt`) and writes a matching `<name>.key`.
- `--key-file <PATH>` — Encrypts using an existing 20-byte `.key` file (implies `--encrypt`) and copies it to `<out-dir>/<name>.key`.

## TODO
- Unit tests: ctest suites for `tools/` (`CPS2Crypt` round-trip, key encoding/decoding, planar 4bpp tile interleaving) and `sdk/` (`constexpr` RGB/tile packing and C/C++ ABI checks).
- QSound audio (`sdk/include/cps2_sound.h`): Z80 command queue helpers, SFX/BGM triggers, and master volume control.
- Raster effects and hardware math (`sdk/include/cps2_raster.h`): Scanline row-scroll tables and CPS-B 16x16 hardware multiplier wrappers.
- More higher level abstractions in general
