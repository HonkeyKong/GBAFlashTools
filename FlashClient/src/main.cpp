#include <gba_input.h>
#include <LinkSPI.hpp>
#include <flash.hpp>
#include <text.hpp>

#define BUFFER_ADDRESS 0x02020000
#define BUFFER_SIZE    (128 * 1024)
uint8_t* sectorBuffer = (uint8_t*)BUFFER_ADDRESS;

LinkSPI *linkSPI = nullptr;

#define ACK_FLASH_COMPLETE 0xACEDACED

bool chunkFlashed = false;

void forceFlash(uint32_t flashAddress, const uint8_t* data, uint32_t size) {
    ClearText();
    RenderLine("Force flashing...", 10);

    EraseSector(flashAddress);
    WriteData(flashAddress, data, size);

    if (VerifyData(flashAddress, data, size)) {
        chunkFlashed = true;
        RenderLine("Force flash successful!", 11);
    } else {
        RenderLine("Force flash failed!", 12);
    }
}

void Flash8MBChip(uint32_t flashAddress) {
    ClearText();
    RenderLine("Flashing 8MB Chip...", 12);

    uint32_t chunkOffset = 0; // Offset for tracking buffer position

    // Handle first 64KB (8 × 8KB sectors)
    if (flashAddress < 0x10000) { 
        for (int i = 0; i < 8; i++) {
            uint32_t sectorAddr = flashAddress + (i * 8192);
            EraseSector(sectorAddr);
            WriteData(sectorAddr, &sectorBuffer[chunkOffset], 8192);

            if (!VerifyData(sectorAddr, &sectorBuffer[chunkOffset], 8192)) {
                RenderLine("Verification failed!", 13);
                forceFlash(sectorAddr, &sectorBuffer[chunkOffset], 8192);
            }
            chunkOffset += 8192;
        }
        flashAddress = 0x10000; // Move to the first 64KB sector after the 8KB sectors
    }

    // Handle remaining sectors (64KB each)
    while (chunkOffset < BUFFER_SIZE) {
        uint32_t sectorAddr = flashAddress;
        EraseSector(sectorAddr);
        WriteData(sectorAddr, &sectorBuffer[chunkOffset], 0x10000); // 64KB per sector

        if (!VerifyData(sectorAddr, &sectorBuffer[chunkOffset], 0x10000)) {
            RenderLine("Verification failed!", 14);
            forceFlash(sectorAddr, &sectorBuffer[chunkOffset], 0x10000);
        }

        chunkOffset += 0x10000;
        flashAddress += 0x10000;
    }
}

void Flash16MBChip(uint32_t flashAddress) {
    ClearText();
    RenderLine("Flashing 16MB Chip...", 12);

    EraseSector(flashAddress);
    WriteData(flashAddress, sectorBuffer, BUFFER_SIZE);

    if (!VerifyData(flashAddress, sectorBuffer, BUFFER_SIZE)) {
        RenderLine("Verification failed!", 13);
        forceFlash(flashAddress, sectorBuffer, BUFFER_SIZE);
    }
}

extern uint16_t mfrID, devID;

int main() {
    InitText();
    SetupBackground();
    RenderLine("Swap carts and press A.", 2);

    while (!(keysDown() & KEY_A)) {
        scanKeys();
    }

    ClearText();
    RenderLine("Detecting flash chip...", 3);
    uint16_t chipType = DetectChipType();

    if (chipType == 16) {
        RenderLine("Detected S29GL128N (16MB)", 4);
    } else if (chipType == 8) {
        RenderLine("Detected S29GL064N (8MB)", 4);
    } else {
        RenderLine("Unknown flash chip!", 4);
        while (true);
    }

    linkSPI = new LinkSPI();
    linkSPI->activate(LinkSPI::Mode::SLAVE, LinkSPI::DataSize::SIZE_32BIT);

    RenderLine("Syncing with sender...", 5);
    while (linkSPI->transfer(0x5555AAAA) != 0xAAAA5555 || linkSPI->transfer(0x22221111) != 0x11112222) {}

    ClearText();
    RenderLine("Sender ready!", 6);

    uint32_t flashAddress = 0;
    uint32_t bytesReceived = 0;

    while (true) {
        uint32_t packet = linkSPI->transfer(0xFFFFFFFF);

        if (packet == 0xBBBBBBBB) {
            ClearText();
            RenderLine("Transfer complete!", 7);
            RenderLine("Total Received:", 8);
            RenderText(HexString32(bytesReceived), 18, 8);
            break;
        }

        if (packet != 0x12345678) continue;

        ClearText();
        RenderLine("Receiving 128KB chunk...", 9);
        chunkFlashed = false;

        uint32_t chunkBytesReceived = 0;

        while (chunkBytesReceived < BUFFER_SIZE) {
            linkSPI->transfer(0xDEADBEEF);

            uint32_t blockSize = (BUFFER_SIZE - chunkBytesReceived > 8192) ? 8192 : (BUFFER_SIZE - chunkBytesReceived);
            uint32_t checksum = 0;

            for (uint32_t i = 0; i < blockSize; i++, chunkBytesReceived++, bytesReceived++) {
                uint8_t byte = linkSPI->transfer(0x00000000);
                sectorBuffer[chunkBytesReceived] = byte;
                checksum = (checksum + byte) & 0xFFFF;
            }

            uint32_t receivedChecksum = linkSPI->transfer(checksum);
            if (receivedChecksum != checksum) {
                RenderLine("Checksum mismatch!", 10);
                chunkBytesReceived -= blockSize;
                bytesReceived -= blockSize;
                continue;
            }

            RenderLine("Received: ", 10);
            RenderText(HexString32(bytesReceived), 13, 10);
        }

        if (chipType == 8) {
            Flash8MBChip(flashAddress);
        } else {
            Flash16MBChip(flashAddress);
        }

        flashAddress += BUFFER_SIZE;

        while (linkSPI->transfer(0xFFFFFFFF) != 0xCAFEBABE) {}
        linkSPI->transfer(ACK_FLASH_COMPLETE);
    }

    while (true);
    return 0;
}
