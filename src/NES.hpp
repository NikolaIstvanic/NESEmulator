#pragma once

#include <cstdint>
#include <memory>

#include "Cartridge.hpp"
#include "CPU.hpp"
#include "PPU.hpp"

#define SIZE_CPU 2048

class NES {
    public:
        NES();
        ~NES() = default;
        void loadCartridge(const std::shared_ptr<Cartridge>& cartridge);
        void reset();
        void step();

        uint8_t cpuRead8(uint16_t addr, bool bReadOnly = false);
        uint16_t cpuRead16(uint16_t addr, bool bReadOnly = false);
        void cpuWrite8(uint16_t addr, uint8_t data);
        void cpuWrite16(uint16_t addr, uint16_t data);

        CPU cpu;
        PPU ppu;
        std::shared_ptr<Cartridge> cartridge;
        uint8_t controller[2];

    private:
        std::array<uint8_t, SIZE_CPU> cpuRAM;
        uint16_t clocks = 0;
        uint8_t dma_page = 0x00;
        uint8_t dma_addr = 0x00;
        uint8_t dma_data = 0x00;
        bool dma_transfer = false;
        bool dma_even = true;
        uint8_t controller_state[2];
};

