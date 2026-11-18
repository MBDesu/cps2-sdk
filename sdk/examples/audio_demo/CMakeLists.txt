set(ELF "${CMAKE_CURRENT_BINARY_DIR}/audio_demo.elf")
set(BIN "${CMAKE_CURRENT_BINARY_DIR}/audio_demo.bin")
set(ROM_DIR "${CMAKE_CURRENT_BINARY_DIR}/rom")
set(ZIP "${CMAKE_CURRENT_BINARY_DIR}/audio_demo.zip")

set(ASSETS_H "${CMAKE_CURRENT_BINARY_DIR}/assets.h")
set(LASER_WAV "${CMAKE_CURRENT_SOURCE_DIR}/assets/laser.wav")
set(CHIME_WAV "${CMAKE_CURRENT_SOURCE_DIR}/assets/chime.wav")
set(EXPLOSION_WAV "${CMAKE_CURRENT_SOURCE_DIR}/assets/explosion.wav")
set(BASS_WAV "${CMAKE_CURRENT_SOURCE_DIR}/assets/bass.wav")

set(PACK_ARGS
    --sfx LASER assets/laser.wav
    --sfx CHIME assets/chime.wav
    --sfx EXPLOSION assets/explosion.wav
    --bgm BASS assets/bass.wav
)

add_custom_command(
    OUTPUT "${ASSETS_H}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${ROM_DIR}"
    COMMAND "${CPS2_PACKER}" --prog /dev/null --out-dir "${ROM_DIR}"
            --name hb --header "${ASSETS_H}" ${PACK_ARGS}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS "${LASER_WAV}" "${CHIME_WAV}" "${EXPLOSION_WAV}" "${BASS_WAV}"
            "${CPS2_PACKER}"
    COMMENT "Generating assets.h for audio_demo"
)

add_executable(audio_demo.elf main.cpp "${ASSETS_H}" "${CPS2_CRT0}")
set_target_properties(audio_demo.elf PROPERTIES SUFFIX "")
target_include_directories(audio_demo.elf PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(audio_demo.elf PRIVATE cps2_sdk)
add_dependencies(audio_demo.elf cps2_crt0)

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
            hb.01 hb.03 hb.11 hb.13 hb.14 hb.15 hb.16 hb.17 hb.18 hb.19 hb.20 ${KEY_FILE}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS audio_demo.elf "${LASER_WAV}" "${CHIME_WAV}" "${EXPLOSION_WAV}" "${BASS_WAV}"
            "${CPS2_PACKER}"
    COMMENT "Packing audio_demo.zip"
)

add_custom_target(audio_demo ALL DEPENDS "${ZIP}")
