set(ELF "${CMAKE_CURRENT_BINARY_DIR}/big_sprite_demo.elf")
set(BIN "${CMAKE_CURRENT_BINARY_DIR}/big_sprite_demo.bin")
set(ROM_DIR "${CMAKE_CURRENT_BINARY_DIR}/rom")
set(ZIP "${CMAKE_CURRENT_BINARY_DIR}/big_sprite_demo.zip")

set(ASSETS_H "${CMAKE_CURRENT_BINARY_DIR}/assets.h")
set(FIGHTER_PNG "${CMAKE_CURRENT_SOURCE_DIR}/assets/fighter.png")
set(FLOOR_PNG "${CMAKE_CURRENT_SOURCE_DIR}/assets/floor.png")
set(STARS_PNG "${CMAKE_CURRENT_SOURCE_DIR}/assets/stars.png")
set(PACK_ARGS --sprite FIGHTER assets/fighter.png 16 --png FLOOR assets/floor.png 16 --png STARS assets/stars.png 32)

add_custom_command(
    OUTPUT "${ASSETS_H}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${ROM_DIR}"
    COMMAND "${CPS2_PACKER}" --prog /dev/null --out-dir "${ROM_DIR}"
            --name hb --header "${ASSETS_H}" ${PACK_ARGS}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS "${FIGHTER_PNG}" "${FLOOR_PNG}" "${STARS_PNG}" "${CPS2_PACKER}"
    COMMENT "Generating assets.h for big_sprite_demo"
)

add_executable(big_sprite_demo.elf main.cpp "${ASSETS_H}" "${CPS2_CRT0}")
set_target_properties(big_sprite_demo.elf PROPERTIES SUFFIX "")
target_include_directories(big_sprite_demo.elf PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(big_sprite_demo.elf PRIVATE cps2_sdk)
add_dependencies(big_sprite_demo.elf cps2_crt0)

set(ENC_ARG $<$<BOOL:${CPS2_ENCRYPT}>:--encrypt>)
set(KEY_FILE $<$<BOOL:${CPS2_ENCRYPT}>:hb.key>)

add_custom_command(
    OUTPUT "${ZIP}"
    COMMAND "${CMAKE_OBJCOPY}" -O binary "${ELF}" "${BIN}"
    COMMAND "${CMAKE_COMMAND}" -E remove_directory "${ROM_DIR}"
    COMMAND "${CPS2_PACKER}" --prog "${BIN}" --elf "${ELF}"
            --out-dir "${ROM_DIR}" --name hb --header "${ASSETS_H}"
            ${ENC_ARG} ${PACK_ARGS}
    COMMAND "${CMAKE_COMMAND}" -E chdir "${ROM_DIR}"
            "${CMAKE_COMMAND}" -E tar cf "${ZIP}" --format=zip
            hb.03 hb.13 hb.14 hb.15 hb.16 hb.17 hb.18 hb.19 hb.20 ${KEY_FILE}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS big_sprite_demo.elf "${FIGHTER_PNG}" "${FLOOR_PNG}" "${STARS_PNG}" "${CPS2_PACKER}"
    COMMENT "Packing big_sprite_demo.zip"
)

add_custom_target(big_sprite_demo ALL DEPENDS "${ZIP}")
