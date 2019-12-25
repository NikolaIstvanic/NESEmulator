#include <fstream>

#include "Cartridge.hpp"
#include "Mapper0.hpp"

Cartridge::Cartridge(const std::string& name) {
    std::ifstream ifs;
    ifs.open(name, std::ifstream::binary);
    valid = false;

    if (ifs.is_open()) {
        Header header;

        ifs.read((char*) &header, sizeof(Header));
        if (header.mapper1 & 0x04) {
            ifs.seekg(512, std::ios_base::cur);
        }

        mirror = (header.mapper1 & 0x01) ? VERTICAL : HORIZONTAL;
        uint8_t mapperID = ((header.mapper2 >> 4) << 4) | (header.mapper1 >> 4);
        uint8_t prgBanks = header.PRGROMSize;
        PRG.resize(prgBanks * 16384);
        ifs.read((char*) PRG.data(), PRG.size());

        uint8_t chrBanks = header.CHRROMSize;
        CHR.resize(chrBanks == 0 ? 8192 : chrBanks * 8192);
        ifs.read((char*) CHR.data(), CHR.size());
        ifs.close();

        switch (mapperID) {
            case 0:
                mapper = std::make_shared<Mapper0>(prgBanks, chrBanks);
                break;
        }
        valid = true;
    }
}

void Cartridge::reset() {
    if (mapper != nullptr) {
        mapper->reset();
    }
}

bool Cartridge::cpuRead8(uint16_t addr, uint8_t& data) {
    uint32_t mapped_addr = 0;

    if (mapper->cpuMapRead8(addr, mapped_addr)) {
        data = PRG[mapped_addr];
        return true;
    }
    return false;
}

bool Cartridge::ppuRead8(uint16_t addr, uint8_t& data) {
    uint32_t mapped_addr = 0;

    if (mapper->ppuMapRead8(addr, mapped_addr)) {
        data = CHR[mapped_addr];
        return true;
    }
    return false;

}

bool Cartridge::cpuWrite8(uint16_t addr, uint8_t& data) {
    uint32_t mapped_addr = 0;

   if (mapper->cpuMapWrite8(addr, mapped_addr)) {
        PRG[mapped_addr] = data;
        return true;
    }
    return false;
}

bool Cartridge::ppuWrite8(uint16_t addr, uint8_t& data) {
    uint32_t mapped_addr = 0;

    if (mapper->ppuMapRead8(addr, mapped_addr)) {
        CHR[mapped_addr] = data;
        return true;
    }
    return false;
}

