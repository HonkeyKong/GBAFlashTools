#ifndef TEXT_HPP
#define TEXT_HPP

#include <cstdint>

#define REG_DMA0SAD     (*(volatile uint32_t*)0x40000B0) // Source Address
#define REG_DMA0DAD     (*(volatile uint32_t*)0x40000B4) // Destination Address
#define REG_DMA0CNT_L   (*(volatile uint16_t*)0x40000B8) // Word Count (lower half)
#define REG_DMA0CNT_H   (*(volatile uint16_t*)0x40000BA) // Control (upper half)
#define REG_DMA0CNT     (*(volatile uint32_t*)0x40000B8) // Combined Control Register

void InitText();
void ClearText();
void SetupBackground();
const char* HexString32(uint32_t value);
void RenderText(const char *text, uint8_t x, uint8_t y);
void RenderLine(const char *text, uint8_t line);

#endif // TEXT_HPP