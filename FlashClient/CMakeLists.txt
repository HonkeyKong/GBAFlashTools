# GBA Link Flash Client
# Author: Ryan Souders

cmake_minimum_required(VERSION 3.13)

set(DEVKITPRO /opt/devkitpro)
set(CMAKE_TOOLCHAIN_FILE ${DEVKITPRO}/cmake/GBA.cmake)

project(FlashClient C CXX ASM)


include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/../common/res)
include_directories(${CMAKE_SOURCE_DIR}/../common/include)

set(BG_SRC "${CMAKE_SOURCE_DIR}/../common/res/guibg.c")
set(FONT_SRC "${CMAKE_SOURCE_DIR}/../common/res/font.c")
set(BG_PNG "${CMAKE_SOURCE_DIR}/../common/res/guibg.png")
set(FONT_PNG "${CMAKE_SOURCE_DIR}/../common/res/font.png")

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

add_custom_target(generate_gfx_src ALL DEPENDS ${FONT_SRC} ${BG_SRC})

set( SOURCE_FILES
    src/main.cpp
    src/flash.cpp
    ../common/src/text.cpp
    ${BG_SRC}
    ${FONT_SRC} )

add_executable( flashclient ${SOURCE_FILES} )
add_dependencies(flashclient generate_gfx_src)

set_property(TARGET flashclient PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET flashclient PROPERTY CXX_STANDARD 17)

set_target_properties(flashclient PROPERTIES GBA_MULTIBOOT TRUE)
set_target_properties(flashclient PROPERTIES GBA_TITLE "Flash Client")

target_link_libraries( flashclient gba )
gba_create_rom(flashclient MULTIBOOT ${GBA_MULTIBOOT} TITLE ${GBA_TITLE})
