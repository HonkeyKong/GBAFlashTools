#ifndef FLASH_HPP
#define FLASH_HPP

// #include <string>
#include <cstdint>
// #include <gba_types.h>
// #include <gba_systemcalls.h>

bool QueryCFI();
uint16_t DetectChipType();
uint8_t readByte(int addr);
void EraseSector(uint32_t address);
uint32_t GetRegionSectorSize(uint32_t region);
uint32_t GetRegionSectorCount(uint32_t region);
void WriteData(uint32_t address, const uint8_t* data, uint32_t size);
bool VerifyData(uint32_t address, const uint8_t* data, uint32_t size);

#endif // FLASH_HPP