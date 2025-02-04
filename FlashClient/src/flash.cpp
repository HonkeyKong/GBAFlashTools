#include <flash.hpp>

#define GBA_ROM (uint16_t*)FLASH_BASE
#define SECTOR_SIZE 8192      // 8KB per sector
#define FLASH_BASE 0x08000000 // Start of cartridge ROM space
#define _FLASH_WRITE(pa, pd) { *(GBA_ROM + ((pa) / 2)) = (pd); __asm("nop"); }

bool bitSwapped = false;
uint16_t region0Sectors = 0;
uint32_t region0Size = 0;
uint16_t region1Sectors = 0;
uint32_t region1Size = 0;

uint8_t readByte(int addr) {
    uint8_t data = *(GBA_ROM + (addr / 2));
    if (bitSwapped) {
        data = (data & 0xFC) | ((data & 1) << 1) | ((data & 2) >> 1);
    }
    return data;
}

// Query the ROM chip using CFI and display results
bool QueryCFI() {
    _FLASH_WRITE(0x0000, 0xF0); // Reset the chip to normal mode
    _FLASH_WRITE(0xAA, 0x98);  // Enter CFI mode

    uint16_t Q = *(uint16_t*)(FLASH_BASE + 0x20);
    uint16_t R = *(uint16_t*)(FLASH_BASE + 0x22);
    uint16_t Y = *(uint16_t*)(FLASH_BASE + 0x24);

    if (Q == 'Q' && R == 'R' && Y == 'Y') {
        bitSwapped = false;
    } else if (Q == 'R' && R == 'Q' && Y == 'Z') {
        bitSwapped = true;
    } else {
        // RenderLine("CFI QUERY FAILED!", currentLine++);
        return false;
    }

    // Detect sector layout
    uint8_t regionCount = readByte(0x58); // Adjust for D0/D1 swap automatically
    // RENDER_LINE_WITH_VALUE("REGIONS: ", regionCount);
    // RenderLine("REGIONS: ", currentLine);
    // RenderText(HexString(regionCount), 9, currentLine++);

    for (uint8_t region = 0; region < regionCount; ++region) {
        uint32_t baseOffset = 0x5A + region * 8;
        // uint32_t baseOffset = 0x5A + region << 3;
        uint16_t sectorCount = readByte(baseOffset) | (readByte(baseOffset + 2) << 8);
        uint16_t sectorSize = readByte(baseOffset + 4) | (readByte(baseOffset + 6) << 8);

        if (region == 0) {
            region0Sectors = sectorCount + 1;
            region0Size = sectorSize * 256; // 8KB sectors for region 0
        } else {
            region1Sectors = sectorCount + 1;
            region1Size = sectorSize * 256; // Larger sectors for region 1
        }

        // RENDER_LINE_WITH_VALUE("REGION ", region);
        // RenderLine("REGION ", currentLine);
        // RenderText(HexString(region), 7, currentLine++);
        // RenderText(HexString(region0Sectors), 0, currentLine);
        // RenderText("X" , 3, currentLine);
        // RenderText(HexString(region0Size), 5, currentLine++);
    }

    _FLASH_WRITE(0x0000, 0xF0); // Exit CFI mode
    return true;
}

uint16_t DetectChipType() {
    _FLASH_WRITE(0xAAA, 0xA9); // Enter auto-select mode
    _FLASH_WRITE(0x555, 0x56);
    _FLASH_WRITE(0xAAA, 0x90);

    uint16_t manufacturerID = *(volatile uint16_t*)(FLASH_BASE + 0x00);
    uint16_t deviceID = *(volatile uint16_t*)(FLASH_BASE + 0x02);

    _FLASH_WRITE(0x0000, 0xF0); // Exit auto-select mode

    if (manufacturerID == 0x01 && deviceID == 0x227E) {
        return 16; // S29GL128N (16MB)
    } else if (manufacturerID == 0x01 && deviceID == 0x2200) {
        return 8; // S29GL064N (8MB)
    } else {
        return 0; // Unknown chip
    }
}

void EraseSector(uint32_t address) {
    _FLASH_WRITE(0xAAA, 0xA9);
    _FLASH_WRITE(0x555, 0x56);
    _FLASH_WRITE(0xAAA, 0x80);
    _FLASH_WRITE(0xAAA, 0xA9);
    _FLASH_WRITE(0x555, 0x56);
    _FLASH_WRITE(address, 0x30); // Sector erase command

    // Wait for completion
    while (*(volatile uint16_t*)(FLASH_BASE + address) != 0xFFFF) {
        __asm("nop");
    }
}

bool VerifyData(uint32_t address, const uint8_t* data, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        uint8_t readValue = *(uint8_t*)(FLASH_BASE + address + i);
        uint8_t expectedValue = data[i];

        // Log each mismatch for debugging
        if (readValue != expectedValue) {
            return false;
        }
    }
    return true;
}

void WriteData(uint32_t address, const uint8_t* data, uint32_t size) {
    for (uint32_t i = 0; i < size; i += 2) {
        uint16_t word = data[i] | (data[i + 1] << 8); // Combine two bytes into a 16-bit word
        _FLASH_WRITE(0xAAA, 0xA9);
        _FLASH_WRITE(0x555, 0x56);
        _FLASH_WRITE(0xAAA, 0xA0);
        _FLASH_WRITE(address + i, word);

        // Wait for the write to complete
        while (*(uint16_t*)(FLASH_BASE + address + i) != word) {
            __asm("nop");
        }
    }
}

uint32_t GetRegionSectorCount(uint32_t region) {
    switch (region) {
        case 0: return 8;   // First region: 8 sectors
        case 1: return 127; // Second region: 127 sectors
        default: return 0;  // No more regions
    }
}

uint32_t GetRegionSectorSize(uint32_t region) {
    switch (region) {
        case 0: return 8192;   // First region: 8KB sectors
        case 1: return 65536;  // Second region: 64KB sectors
        default: return 0;     // No more regions
    }
}
