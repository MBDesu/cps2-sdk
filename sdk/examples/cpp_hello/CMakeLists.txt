set(ELF "${CMAKE_CURRENT_BINARY_DIR}/cpp_hello.elf")
set(BIN "${CMAKE_CURRENT_BINARY_DIR}/cpp_hello.bin")
set(ROM_DIR "${CMAKE_CURRENT_BINARY_DIR}/rom")
set(ZIP "${CMAKE_CURRENT_BINARY_DIR}/cpp_hello.zip")

add_executable(cpp_hello.elf main.cpp "${CPS2_CRT0}")
set_target_properties(cpp_hello.elf PROPERTIES SUFFIX "")
target_link_libraries(cpp_hello.elf PRIVATE cps2_sdk)
add_dependencies(cpp_hello.elf cps2_crt0)

set(ENC_ARG $<$<BOOL:${CPS2_ENCRYPT}>:--encrypt>)
set(KEY_FILE $<$<BOOL:${CPS2_ENCRYPT}>:hb.key>)

add_custom_command(
    OUTPUT "${ZIP}"
    COMMAND "${CMAKE_OBJCOPY}" -O binary "${ELF}" "${BIN}"
    COMMAND "${CMAKE_COMMAND}" -E remove_directory "${ROM_DIR}"
    COMMAND "${CPS2_PACKER}" --prog "${BIN}" --elf "${ELF}"
            --out-dir "${ROM_DIR}" --name hb ${ENC_ARG}
    COMMAND "${CMAKE_COMMAND}" -E chdir "${ROM_DIR}"
            "${CMAKE_COMMAND}" -E tar cf "${ZIP}" --format=zip
            hb.03 hb.13 hb.14 hb.15 hb.16 hb.17 hb.18 hb.19 hb.20 ${KEY_FILE}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS cpp_hello.elf "${CPS2_PACKER}"
    COMMENT "Packing cpp_hello.zip"
)

add_custom_target(cpp_hello ALL DEPENDS "${ZIP}")
