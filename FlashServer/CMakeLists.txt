# GBA Link Flash Server
# Author: Ryan Souders

cmake_minimum_required(VERSION 3.13)

set(DEVKITPRO /opt/devkitpro)
set(CMAKE_TOOLCHAIN_FILE ${DEVKITPRO}/cmake/GBA.cmake)

project(FlashServer C CXX ASM)

include_directories(${CMAKE_SOURCE_DIR}/res)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/../common/res)
include_directories(${CMAKE_SOURCE_DIR}/../common/include)

set(ROM_ASM "${CMAKE_SOURCE_DIR}/res/rom.s")
set(ROM_HEADER "${CMAKE_SOURCE_DIR}/res/rom.h")
set(ROM_BINARY "${CMAKE_SOURCE_DIR}/res/rom.gba")
set(BG_SRC "${CMAKE_SOURCE_DIR}/../common/res/guibg.c")
set(FONT_SRC "${CMAKE_SOURCE_DIR}/../common/res/font.c")
set(BG_PNG "${CMAKE_SOURCE_DIR}/../common/res/guibg.png")
set(FONT_PNG "${CMAKE_SOURCE_DIR}/../common/res/font.png")

# Phony target to pad the ROM to 128KB (131072 bytes)
add_custom_target(pad_rom
    COMMAND padbin 131072 ${ROM_BINARY}
    DEPENDS ${ROM_BINARY}
    COMMENT "Padding ROM binary to 128KB boundary..."
    VERBATIM
)

add_custom_command(
    OUTPUT ${FONT_SRC}
    COMMAND grit ${FONT_PNG} -gt -gB4 -pn16 -pT1 -ftc -o${FONT_SRC}
    DEPENDS ${FONT_PNG}
    COMMENT "Converting font..."
    VERBATIM
)

add_custom_command(
    OUTPUT ${BG_SRC}
    COMMAND grit ${BG_PNG} -gt -gB4 -mRtf -pn16 -ftc -o${BG_SRC}
    DEPENDS ${BG_PNG}
    COMMENT "Converting background..."
    VERBATIM
)

# Custom command to convert the padded ROM to assembly and header
add_custom_command(
    OUTPUT ${ROM_HEADER} ${ROM_ASM}
    COMMAND bin2s -H ${ROM_HEADER} ${ROM_BINARY} > ${ROM_ASM}
    DEPENDS ${ROM_BINARY}  # Ensure it runs after padding
    COMMENT "Converting padded ROM binary to header and assembly files..."
    VERBATIM
)

# Add custom targets to chain dependencies
# Ensure the GFX/ROM files are generated before the executable
add_custom_target(generate_gfx_src ALL DEPENDS ${FONT_SRC} ${BG_SRC})
add_custom_target(generate_rom_src ALL DEPENDS ${ROM_HEADER} ${ROM_ASM})

add_dependencies(generate_rom_src pad_rom)

# Add newly-generated source files to the list
set(SOURCE_FILES
    ../common/src/text.cpp
    src/main.cpp
    ${FONT_SRC}
    ${BG_SRC}
    res/rom.s )

add_executable(flashserver ${SOURCE_FILES})
add_dependencies(flashserver generate_rom_src generate_gfx_src)

# Set C++ standard to C++17
set_property(TARGET flashserver PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET flashserver PROPERTY CXX_STANDARD 17)

# Set custom title for header
set_target_properties(flashserver PROPERTIES GBA_TITLE "Flash Server")

target_link_libraries(flashserver gba)
gba_create_rom(flashserver TITLE ${GBA_TITLE})
