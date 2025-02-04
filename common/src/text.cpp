#include <font.h>
#include <cstring>
#include <guibg.h>
#include <text.hpp>
#include <gba_video.h>


void ClearText() {
    memset(MAP_BASE_ADR(3), 0, 2048);
}

void RenderLine(const char *text, uint8_t line) {
    RenderText(text, 3, line); // Render at x = 3, y = line
}

void RenderText(const char* text, uint8_t x, uint8_t y) {
    uint16_t* tilemap = (uint16_t*)SCREEN_BASE_BLOCK(3); // Screen block 3 is BG0
    int offset = (y * 32) + x; // Calculate the starting offset in the tilemap

    for (int i = 0; text[i] != '\0'; ++i) {
        char c = text[i];
        if (c >= 'a' && c <= 'z') {
            c -= 32; // Convert lowercase to uppercase
        }
        tilemap[offset + i] = c | 0x1000; // Combine 0x1000 for palette 1
    }
}

void InitText() {
    SetMode(MODE_0 | BG0_ENABLE | BG0_ON );
    BGCTRL[0] = TILE_BASE(0) | MAP_BASE(3) | BG_16_COLOR | BG_SIZE_0 | BG_PRIORITY(0);

    memcpy(CHAR_BASE_ADR(0), fontTiles, fontTilesLen);
    memcpy((uint16_t*)BG_PALETTE + 16, fontPal, fontPalLen);

    // Clear out any random garbage that may be in the tilemap RAM.
    ClearText();
}

void SetupBackground() {
    SetMode(MODE_0 | BG0_ENABLE | BG0_ON | BG1_ENABLE | BG1_ON);  // Mode 0, BG0+BG1 active
    BGCTRL[0] = TILE_BASE(0) | MAP_BASE(3) | BG_16_COLOR | BG_SIZE_0 | BG_PRIORITY(0); //text
    BGCTRL[1] = TILE_BASE(1) | MAP_BASE(4) | BG_16_COLOR | BG_SIZE_0 | BG_PRIORITY(1); //background

    memcpy(CHAR_BASE_ADR(1), guibgTiles, guibgTilesLen);
    memcpy((uint16_t*)BG_PALETTE, guibgPal, guibgPalLen);
    memcpy(MAP_BASE_ADR(4), guibgMap, guibgMapLen);

    // There shouldn't be anything here, InitText clears BG0, but to be safe...
    ClearText();
}

// Returns a pointer to a static buffer containing the hex representation of `value`.
// Each call overwrites the same static buffer.
const char* HexString32(uint32_t value) {
    static char hexBuf[9]; // 8 hex digits + null terminator

    for (int i = 7; i >= 0; i--) {
        int nib = value & 0xF;
        hexBuf[i] = (nib < 10) ? (char)('0' + nib) : (char)('A' + (nib - 10));
        value >>= 4;
    }

    hexBuf[8] = '\0'; // Null-terminate
    return hexBuf;
}
