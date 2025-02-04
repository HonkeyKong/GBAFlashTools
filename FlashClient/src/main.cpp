#include <gba_input.h>
#include <LinkSPI.hpp>
#include <flash.hpp>
#include <text.hpp>

// Allocate the buffer at the upper half of EWRAM (end of EWRAM)
#define BUFFER_ADDRESS 0x02020000
#define BUFFER_SIZE    (128 * 1024)
uint8_t* sectorBuffer = (uint8_t*)BUFFER_ADDRESS;

LinkSPI *linkSPI = nullptr;

#define ACK_FLASH_COMPLETE 0xACEDACED

bool chunkFlashed = false; // Track if the current chunk has been flashed

void forceFlash(uint32_t flashAddress) {
    ClearText();
    RenderLine("Force flashing...", 10);

    EraseSector(flashAddress);
    WriteData(flashAddress, sectorBuffer, BUFFER_SIZE);

    if (VerifyData(flashAddress, sectorBuffer, BUFFER_SIZE)) {
        chunkFlashed = true;
        RenderLine("Force flash successful!", 11);
    } else {
        RenderLine("Force flash failed!", 12);
    }
}

int main() {
    InitText();
    SetupBackground();
    RenderLine("Swap carts and press A.", 2);

    uint16_t keys = 0;
    while(!(keys & KEY_A)) {
        scanKeys();
        keys = keysDown();
        if(keys & KEY_A) break;
    }

    linkSPI = new LinkSPI();
    linkSPI->activate(LinkSPI::Mode::SLAVE, LinkSPI::DataSize::SIZE_32BIT);

    RenderLine("Syncing with sender...", 3);
    while (true) {
        uint32_t syncStep1 = linkSPI->transfer(0x5555AAAA);
        if (syncStep1 == 0xAAAA5555) {
            uint32_t syncStep2 = linkSPI->transfer(0x22221111);
            if (syncStep2 == 0x11112222) {
                ClearText();
                RenderLine("Sender ready!", 4);
                break;
            }
        }
    }

    uint32_t flashAddress = 0;
    uint32_t bytesReceived = 0;

    while (true) {
        uint32_t packet = linkSPI->transfer(0xFFFFFFFF);

        if (packet == 0xBBBBBBBB) {
            ClearText();
            RenderLine("Transfer complete!", 8);
            RenderLine("Total Received: ", 9);
            RenderText(HexString32(bytesReceived), 16, 9);
            break;
        }

        if (packet != 0x12345678) {
            continue;
        }

        ClearText();
        RenderLine("Receiving 128KB chunk...", 5);

        // memset(sectorBuffer, 0xFF, BUFFER_SIZE);
        chunkFlashed = false;

        uint32_t chunkBytesReceived = 0;

        while (chunkBytesReceived < BUFFER_SIZE) {
            linkSPI->transfer(0xDEADBEEF);

            RenderLine("Receiving 8KB block...", 6);

            uint32_t blockSize = (BUFFER_SIZE - chunkBytesReceived > 8192) ? 8192 : (BUFFER_SIZE - chunkBytesReceived);
            uint32_t checksum = 0;

            for (uint32_t i = 0; i < blockSize; i++, chunkBytesReceived++, bytesReceived++) {
                uint8_t byte = linkSPI->transfer(0x00000000);
                sectorBuffer[chunkBytesReceived] = byte;
                checksum = (checksum + byte) & 0xFFFF;
            }

            uint32_t receivedChecksum = linkSPI->transfer(checksum);
            if (receivedChecksum != checksum) {
                ClearText();
                RenderLine("Checksum mismatch!", 7);
                RenderLine("Requesting resend...", 8);
                RenderLine("Calc: ", 9);
                RenderLine("Recv: ", 10);
                RenderText(HexString32(checksum), 6, 9);
                RenderText(HexString32(receivedChecksum), 6, 10);

                chunkBytesReceived = (chunkBytesReceived >= blockSize) ? (chunkBytesReceived - blockSize) : 0;
                bytesReceived = (bytesReceived >= blockSize) ? (bytesReceived - blockSize) : 0;
                continue;
            }

            RenderLine("Block receive success!",10);
            RenderLine("Bytes Received:", 11);
            RenderText(HexString32(bytesReceived), 16, 11);
        }

        ClearText();
        RenderLine("Flashing to cartridge...", 12);

        EraseSector(flashAddress);
        WriteData(flashAddress, sectorBuffer, BUFFER_SIZE);

        if (VerifyData(flashAddress, sectorBuffer, BUFFER_SIZE)) {
            chunkFlashed = true;
            RenderLine("Flashed successfully!", 13);
        } else {
            RenderLine("Verification failed!", 14);
            forceFlash(flashAddress);
        }

        flashAddress += BUFFER_SIZE;

        while (true) {
            uint32_t senderSignal = linkSPI->transfer(0xFFFFFFFF);

            if (senderSignal == 0xCAFEBABE) {
                if (!chunkFlashed) {
                    RenderLine("Missed flash, retrying.", 15);
                    forceFlash(flashAddress - BUFFER_SIZE);
                }

                linkSPI->transfer(ACK_FLASH_COMPLETE);
                RenderLine("ACK sent to sender!", 16);
                break;
            }
        }
    }

    while (true);
    return 0;
}
