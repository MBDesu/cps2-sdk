cmake_minimum_required(VERSION 3.20...4.0)
project(cps2_tools LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(stb)

set(Z80_ASM_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/../sdk/z80/asm_z80.py")
set(Z80_DRIVER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../sdk/z80/qsound_driver.s")
set(Z80_DRIVER_BIN "${CMAKE_CURRENT_BINARY_DIR}/qsound_driver.bin")
set(Z80_DRIVER_HDR "${CMAKE_CURRENT_BINARY_DIR}/default_z80_driver.h")

add_custom_command(
    OUTPUT "${Z80_DRIVER_BIN}" "${Z80_DRIVER_HDR}"
    COMMAND python3 "${Z80_ASM_SCRIPT}" "${Z80_DRIVER_SRC}" "${Z80_DRIVER_BIN}"
            --header "${Z80_DRIVER_HDR}"
    DEPENDS "${Z80_ASM_SCRIPT}" "${Z80_DRIVER_SRC}"
    COMMENT "Embedding default Z80 QSound driver into cps2_packer"
)

add_executable(cps2_packer
    cps2_packer.cpp
    cps2_crypt.cpp
    "${Z80_DRIVER_HDR}"
)
target_include_directories(cps2_packer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${stb_SOURCE_DIR}"
)

# add_executable(cps2_lift
#     cps2_lift.cpp
#     cps2_lift_decoder.cpp
# )
# target_include_directories(cps2_lift PRIVATE
#     "${CMAKE_CURRENT_SOURCE_DIR}"
# )
#
