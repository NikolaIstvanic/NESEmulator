#include "Mapper0.hpp"

Mapper0::Mapper0(uint8_t prgBanks, uint8_t chrBanks) :
    Mapper(prgBanks, chrBanks) { }

void Mapper0::reset() { }

bool Mapper0::cpuMapRead8(uint16_t addr, uint32_t& mapped_addr) {
    if (0x8000 <= addr && addr <= 0xFFFF) {
        mapped_addr = addr & (prgBanks > 1 ? 0x7FFF : 0x3FFF);
        return true;
    }
    return false;
}

bool Mapper0::cpuMapWrite8(uint16_t addr, uint32_t& mapped_addr) {
    if (0x8000 <= addr && addr <= 0xFFFF) {
        mapped_addr = addr & (prgBanks > 1 ? 0x7FFF : 0x3FFF);
        return true;
    }
    return false;
}

bool Mapper0::ppuMapRead8(uint16_t addr, uint32_t& mapped_addr) {
    if (0x0000 <= addr && addr <= 0x1FFF) {
        mapped_addr = addr;
        return true;
    }
    return false;
}

bool Mapper0::ppuMapWrite8(uint16_t addr, uint32_t& mapped_addr) {
    if (0x0000 <= addr && addr <= 0x1FFF && chrBanks == 0) {
        mapped_addr = addr;
        return true;
    }
    return false;
}

