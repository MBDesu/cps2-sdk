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

add_executable(cps2_packer
    cps2_packer.cpp
    cps2_crypt.cpp
)
target_include_directories(cps2_packer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${stb_SOURCE_DIR}"
)
