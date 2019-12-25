#pragma once

#include "Mapper.hpp"

class Mapper0 : public Mapper {
    public:
        Mapper0(uint8_t prgBanks, uint8_t chrBanks);

        void reset() override;
        bool cpuMapRead8(uint16_t addr, uint32_t& mapped_addr) override;
        bool cpuMapWrite8(uint16_t addr, uint32_t& mapped_addr) override;
        bool ppuMapRead8(uint16_t addr, uint32_t& mapped_addr) override;
        bool ppuMapWrite8(uint16_t addr, uint32_t& mapped_addr) override;
};

