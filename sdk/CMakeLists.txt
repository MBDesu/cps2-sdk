cmake_minimum_required(VERSION 3.20...4.0)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/m68k-toolchain.cmake"
        CACHE FILEPATH "m68k cross-toolchain file")
endif()

project(cps2_sdk LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CPS2_ENCRYPT "Encrypt 68000 ROMs with CPS2 security key" ON)
option(CPS2_BUILD_EXAMPLES "Build SDK example ROMs" ON)

find_program(CPS2_PACKER
    NAMES cps2_packer
    HINTS "${CMAKE_CURRENT_SOURCE_DIR}/../tools/build"
)

if(NOT CPS2_PACKER)
    message(FATAL_ERROR
        "cps2_packer not found. Build tools first:\n"
        "  cmake -S tools -B tools/build && cmake --build tools/build -j")
endif()
set(CPS2_PACKER "${CPS2_PACKER}" CACHE FILEPATH "Path to cps2_packer")


set(CPS2_CRT0 "${CMAKE_BINARY_DIR}/crt0.o"
    CACHE INTERNAL "Path to crt0.o")
add_custom_command(
    OUTPUT "${CPS2_CRT0}"
    COMMAND "${CMAKE_ASM_COMPILER}" -Felf -m68000
            -I"${CMAKE_SOURCE_DIR}/include" -quiet
            "${CMAKE_SOURCE_DIR}/src/crt0.s" -o "${CPS2_CRT0}"
    DEPENDS "${CMAKE_SOURCE_DIR}/src/crt0.s"
    COMMENT "Assembling crt0.s"
)
add_custom_target(cps2_crt0 DEPENDS "${CPS2_CRT0}")

add_library(cps2_sdk INTERFACE)
target_include_directories(cps2_sdk INTERFACE
    "${CMAKE_SOURCE_DIR}/include"
)
target_compile_features(cps2_sdk INTERFACE cxx_std_20)
target_compile_options(cps2_sdk INTERFACE
    -m68000 -O2 -Wall -Wextra
    -ffreestanding -fno-exceptions -fno-jump-tables -nostdlib
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti -fno-threadsafe-statics -fno-use-cxa-atexit>
)
target_link_options(cps2_sdk INTERFACE
    -m68000 -T "${CMAKE_SOURCE_DIR}/ld/cps2.ld" -nostdlib
    -Wl,--build-id=none
)
target_link_libraries(cps2_sdk INTERFACE gcc)

# optional vanilla C11/C17 compatibility wrapper library
add_library(cps2_sdk_c OBJECT
    "${CMAKE_SOURCE_DIR}/src/cps2_c.cpp"
)
target_link_libraries(cps2_sdk_c PUBLIC cps2_sdk)

set(CPS2_Z80_DRIVER "${CMAKE_CURRENT_BINARY_DIR}/z80/qsound_driver.bin")
add_subdirectory(z80)

if(CPS2_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

file(WRITE "${CMAKE_SOURCE_DIR}/compile_flags.txt"
    "--target=m68k-unknown-elf\n"
    "-std=c++20\n"
    "-ffreestanding\n"
    "-I${CMAKE_SOURCE_DIR}/include\n"
)
