#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Mapper.hpp"

class Cartridge {
    public:
        Cartridge(const std::string& name);
        ~Cartridge() = default;
        void reset();
 
        bool cpuRead8(uint16_t addr, uint8_t& data);
        bool ppuRead8(uint16_t addr, uint8_t& data);
        bool cpuWrite8(uint16_t addr, uint8_t& data);
        bool ppuWrite8(uint16_t addr, uint8_t& data);

        enum MIRROR {
            HORIZONTAL,
            VERTICAL,
            ONESCREEN_LO,
            ONESCREEN_HI
        } mirror = HORIZONTAL;

        bool valid = false;

    private:
        std::vector<uint8_t> PRG;
        std::vector<uint8_t> CHR;
        std::shared_ptr<Mapper> mapper;

        struct Header {
            char name[4];
            uint8_t PRGROMSize;
            uint8_t CHRROMSize;
            uint8_t mapper1;
            uint8_t mapper2;
            uint8_t prgRAMSize;
            uint8_t tvSystem1;
            uint8_t tvSystem2;
            char unused[5];
        };
};

