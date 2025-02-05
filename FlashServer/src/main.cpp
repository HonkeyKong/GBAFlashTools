// #include <gba_interrupt.h>
#include <LinkSPI.hpp>
#include <text.hpp>
#include <cstdint>
#include <rom.h>

LinkSPI *linkSPI = nullptr;

#define ACK_FLASH_COMPLETE 0xACEDACED

// Counters for tracking
uint32_t successCount = 0;
uint32_t mismatchCount = 0;

int main() {
    InitText();
    SetupBackground();
    RenderLine("Initializing link...", 2);

    linkSPI = new LinkSPI();
    linkSPI->activate(LinkSPI::Mode::MASTER_256KBPS, LinkSPI::DataSize::SIZE_32BIT);

    const uint8_t* dataPtr = rom_gba;
    uint32_t dataSize = rom_gba_size;
    uint32_t bytesSent = 0;

    // Handshake
    RenderLine("Syncing with receiver...", 3);
    while (true) {
        uint32_t syncStep1 = linkSPI->transfer(0xAAAA5555);
        if (syncStep1 == 0x5555AAAA) {
            uint32_t syncStep2 = linkSPI->transfer(0x11112222);
            if (syncStep2 == 0x22221111) {
                ClearText();
                RenderLine("Receiver ready!", 4);
                break;
            }
        }
    }

    while (bytesSent < dataSize) {
        ClearText();
        RenderLine("Sending 128KB chunk...  ", 5);
        linkSPI->transfer(0x12345678);  // Start of chunk

        uint32_t chunkSize = (dataSize - bytesSent > 128 * 1024) ? (128 * 1024) : (dataSize - bytesSent);
        uint32_t chunkBytesSent = 0;

        while (chunkBytesSent < chunkSize) {
            uint32_t readySignal = linkSPI->transfer(0xCAFEBABE);
            if (readySignal != 0xDEADBEEF) {
                RenderLine("Sync lost, resending... ", 6);
                continue;
            }

            RenderLine("Sending 8KB block...    ", 6);

            uint32_t blockSize = (chunkSize - chunkBytesSent > 8192) ? 8192 : (chunkSize - chunkBytesSent);
            uint32_t checksum = 0;

            for (uint32_t i = 0; i < blockSize; i++, bytesSent++, chunkBytesSent++) {
                uint8_t byte = dataPtr[bytesSent];
                checksum = (checksum + byte) & 0xFFFF;
                linkSPI->transfer(byte);
            }

            uint32_t ack = linkSPI->transfer(checksum);
            if (ack != checksum) {
                mismatchCount++;
                ClearText();
                RenderLine("Checksum fail, retrying.", 5);
                RenderLine("Calc: ", 6);
                RenderLine("Recv: ", 7);
                RenderText(HexString32(checksum), 9, 8);
                RenderText(HexString32(ack), 9, 9);
                bytesSent -= blockSize;
                chunkBytesSent -= blockSize;
                continue;
            }

            successCount++;
            RenderLine("Block sent successfully!", 10);
            RenderLine("Sent: ", 11);
            RenderText(HexString32(bytesSent), 8, 11);
        }

        RenderLine("Waiting for flash ACK...", 12);
        while (true) {
            uint32_t readySignal = linkSPI->transfer(0xCAFEBABE);
            if (readySignal == ACK_FLASH_COMPLETE) {
                RenderLine("Flash acknowledged!", 13);
                break;
            }
        }
    }

    linkSPI->transfer(0xBBBBBBBB);  // Signal transfer complete

    ClearText();
    RenderLine("Transfer complete!", 8);
    RenderLine("Total Sent: ", 9);
    RenderText(HexString32(bytesSent), 15, 9);
    RenderLine("Successes: ", 10);
    RenderText(HexString32(successCount), 15, 10);
    RenderLine("Mismatches:", 11);
    RenderText(HexString32(mismatchCount), 15, 11);

    while (true);
    return 0;
}
