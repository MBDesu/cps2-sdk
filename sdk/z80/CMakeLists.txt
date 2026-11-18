set(Z80_DRIVER_BIN "${CMAKE_CURRENT_BINARY_DIR}/qsound_driver.bin")

add_custom_command(
    OUTPUT "${Z80_DRIVER_BIN}"
    COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/asm_z80.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/qsound_driver.s"
            "${Z80_DRIVER_BIN}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/asm_z80.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/qsound_driver.s"
    COMMENT "Assembling Z80 QSound driver (qsound_driver.bin)"
)

add_custom_target(cps2_z80_driver ALL DEPENDS "${Z80_DRIVER_BIN}")

