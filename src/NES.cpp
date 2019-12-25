#include <algorithm>

#include "NES.hpp"

NES::NES() {
    cpu.connectNES(this);
    std::fill(cpuRAM.begin(), cpuRAM.end(), 0x00);
}

void NES::loadCartridge(const std::shared_ptr<Cartridge>& cartridge) {
    this->cartridge = cartridge;
    ppu.connectCartridge(cartridge);
}

void NES::reset() {
    cartridge->reset();
    cpu.reset();
    ppu.reset();
    clocks = 0;
    dma_page = 0x00;
    dma_addr = 0x00;
    dma_data = 0x00;
    dma_transfer = false;
    dma_even = true;
}

void NES::step() {
    ppu.step();
    
    // PPU has 3x clock speed of CPU
    if (clocks % 3 == 0) {
        if (dma_transfer) {
            if (dma_even) {
                if (clocks % 2 == 1) {
                    dma_even = false;
                }
            } else {
                if (clocks % 2 == 0) {
                    dma_data = cpuRead8((dma_page << 8) | dma_addr);
                } else {
                    ppu.pOAM[dma_addr++] = dma_data;

                    if (dma_addr == 0x00) {
                        // Wrap around
                        dma_transfer = false;
                        dma_even = true;
                    }
                }
            }
        } else {
            cpu.step();
        }
    }

    if (ppu.nmi) {
        ppu.nmi = false;
        cpu.nmi();
    }
    clocks++;
}

uint8_t NES::cpuRead8(uint16_t addr, bool bReadOnly) {
    uint8_t data = 0x00;

    if (cartridge->cpuRead8(addr, data)) {

    } else if (0x0000 <= addr && addr <= 0x1FFF) {
        data = cpuRAM[addr & 0x07FF];
    } else if (0x2000 <= addr && addr <= 0x3FFF) {
        data = ppu.cpuRead8(addr & 0x0007, bReadOnly); 
    } else if (0x4016 <= addr && addr <= 0x4017) {
        data = (controller_state[addr & 0x0001] & 0x80) > 0;
        controller_state[addr & 0x0001] <<= 1;
    }
    return data;
}

uint16_t NES::cpuRead16(uint16_t addr, bool bReadOnly) {
    return (cpuRead8(addr+ 1, bReadOnly) << 8) | cpuRead8(addr, bReadOnly);
}

void NES::cpuWrite8(uint16_t addr, uint8_t data) {
    if (cartridge->cpuWrite8(addr, data)) {

    } else if (0x0000 <= addr && addr <= 0x1FFF) {
        cpuRAM[addr & 0x07FF] = data;
    } else if (0x2000 <= addr && addr <= 0x3FFF) {
        ppu.cpuWrite8(addr & 0x0007, data);
    } else if (addr == 0x4014) {
        dma_page = data;
        dma_addr = 0x00;
        dma_transfer = true;
    } else if (0x4016 <= addr && addr <= 0x4017) {
        controller_state[addr & 0x0001] = controller[addr & 0x0001];
    }
}

void NES::cpuWrite16(uint16_t addr, uint16_t data) {
    cpuWrite8(addr, data & 0x00FF);
    cpuWrite8(addr + 1, data >> 8);
}

