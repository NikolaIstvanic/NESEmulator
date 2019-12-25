#pragma once

#include <cstdint>

class Mapper {
    public:
        Mapper(uint8_t prgBanks, uint8_t chrBanks);

        virtual void reset() = 0;
        virtual bool cpuMapRead8(uint16_t addr, uint32_t& mapped_addr) = 0;
        virtual bool cpuMapWrite8(uint16_t addr, uint32_t& mapped_addr) = 0;
        virtual bool ppuMapRead8(uint16_t addr, uint32_t& mapped_addr) = 0;
        virtual bool ppuMapWrite8(uint16_t addr, uint32_t& mapped_addr) = 0;

    protected:
        uint8_t prgBanks = 0;
        uint8_t chrBanks = 0;
};

