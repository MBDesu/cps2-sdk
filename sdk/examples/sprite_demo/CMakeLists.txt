set(ELF "${CMAKE_CURRENT_BINARY_DIR}/sprite_demo.elf")
set(BIN "${CMAKE_CURRENT_BINARY_DIR}/sprite_demo.bin")
set(ROM_DIR "${CMAKE_CURRENT_BINARY_DIR}/rom")
set(ZIP "${CMAKE_CURRENT_BINARY_DIR}/sprite_demo.zip")
set(ASSETS_H "${CMAKE_CURRENT_BINARY_DIR}/assets.h")

set(HERO_PNG "${CMAKE_CURRENT_SOURCE_DIR}/assets/hero.png")
set(BG_PNG "${CMAKE_CURRENT_SOURCE_DIR}/assets/bg_tiles.png")
set(PACK_ARGS --sprite HERO assets/hero.png 16 --png BG assets/bg_tiles.png 16)

add_custom_command(
    OUTPUT "${HERO_PNG}" "${BG_PNG}"
    COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/gen_assets.py"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/gen_assets.py"
    COMMENT "Generating sprite_demo PNG assets"
)

add_custom_command(
    OUTPUT "${ASSETS_H}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${ROM_DIR}"
    COMMAND "${CPS2_PACKER}" --prog /dev/null --out-dir "${ROM_DIR}"
            --name hb --header "${ASSETS_H}" ${PACK_ARGS}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS "${HERO_PNG}" "${BG_PNG}" "${CPS2_PACKER}"
    COMMENT "Generating assets.h for sprite_demo"
)

add_executable(sprite_demo.elf main.cpp "${ASSETS_H}" "${CPS2_CRT0}")
set_target_properties(sprite_demo.elf PROPERTIES SUFFIX "")
target_include_directories(sprite_demo.elf PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(sprite_demo.elf PRIVATE cps2_sdk)
add_dependencies(sprite_demo.elf cps2_crt0)

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
    DEPENDS sprite_demo.elf "${CPS2_PACKER}"
    COMMENT "Packing sprite_demo.zip"
)

add_custom_target(sprite_demo ALL DEPENDS "${ZIP}")
