set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR m68k)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

find_program(CMAKE_C_COMPILER
    NAMES m68k-elf-gcc m68k-linux-gnu-gcc
    REQUIRED
)
find_program(CMAKE_CXX_COMPILER
    NAMES m68k-elf-g++ m68k-linux-gnu-g++
    REQUIRED
)
find_program(CMAKE_ASM_COMPILER
    NAMES vasmm68k_mot
    REQUIRED
)
find_program(CMAKE_OBJCOPY
    NAMES m68k-elf-objcopy m68k-linux-gnu-objcopy
    REQUIRED
)
