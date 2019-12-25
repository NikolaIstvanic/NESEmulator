#pragma once

#include <cstdint>
#include <memory>

#include "Cartridge.hpp"
#include "olcPixelGameEngine.h"

#define PPU_CTRL 0x0000
#define PPU_MASK 0x0001
#define PPU_STAT 0x0002
#define OAM_ADDR 0x0003
#define OAM_DATA 0x0004
#define PPU_SCRL 0x0005
#define PPU_ADDR 0x0006
#define PPU_DATA 0x0007

class PPU {
    public:
        PPU();
        ~PPU() = default;
        void reset();
        void connectCartridge(const std::shared_ptr<Cartridge>& cartridge);
        olc::Sprite& getScreen();
        olc::Pixel& getColor(uint8_t palette, uint8_t pixel);
        void step();

        uint8_t cpuRead8(uint16_t addr, bool bReadOnly = false);
        uint8_t ppuRead8(uint16_t addr, bool bReadOnly = false);
        void cpuWrite8(uint16_t addr, uint8_t data);
        void ppuWrite8(uint16_t addr, uint8_t data);

        bool frame_done = false;
        bool nmi = false;
        uint8_t* pOAM = (uint8_t*) OAM;

    private:
        union {
            struct {
                uint8_t nametable_x : 1;
                uint8_t nametable_y : 1;
                uint8_t increment_mode : 1;
                uint8_t pattern_sprite : 1;
                uint8_t pattern_bg : 1;
                uint8_t sprite_size : 1;
                uint8_t slave_mode : 1;
                uint8_t enable_nmi : 1;
            };
            uint8_t val = 0x00;
        } PPU_ctrl;

        union {
            struct {
                uint8_t grayscale : 1;
                uint8_t render_bg_left : 1;
                uint8_t render_sprites_left : 1;
                uint8_t render_bg : 1;
                uint8_t render_sprites : 1;
                uint8_t enhance_red : 1;
                uint8_t enhance_green : 1;
                uint8_t enhance_blue : 1;
            };
            uint8_t val = 0x00;
        } PPU_mask;

        union {
            struct {
                uint8_t unused : 5;
                uint8_t sprite_overflow : 1;
                uint8_t sprite_zero_hit : 1;
                uint8_t vblank : 1;
            };
            uint8_t val = 0x00;
        } PPU_stat;

        union loopy_reg {
            struct {
                uint16_t coarse_x : 5;
                uint16_t coarse_y : 5;
                uint16_t nametable_x : 1;
                uint16_t nametable_y : 1;
                uint16_t fine_y : 3;
                uint16_t unused : 1;
            };
            uint16_t val = 0x0000;
        };
        
        loopy_reg tram_addr;
        loopy_reg vram_addr;
        
        struct OAMEntry {
            uint8_t y;
            uint8_t id;
            uint8_t attr;
            uint8_t x;
        } OAM[64];

        OAMEntry spriteScanline[8];
        uint8_t oam_addr = 0x00;
        uint8_t sprite_count;
        bool spriteZeroHitPossible = false;
        bool spriteZeroBeingRendered = false;

        int16_t scanline = 0;
        int16_t cycle = 0;

        uint8_t patternTable[2][4096];
        uint8_t paletteTable[32];
        uint8_t nameTable[2][1024];
        std::array<olc::Pixel, 64> palScreen;
        olc::Sprite sprScreen = olc::Sprite(256, 240);

        uint8_t address_latch = 0x00;
        uint8_t ppu_data_buffer = 0x00;
        uint8_t fine_x = 0x00;

        uint8_t bg_next_tile_id = 0x00;
        uint8_t bg_next_tile_attr = 0x00;
        uint8_t bg_next_tile_lsb = 0x00;
        uint8_t bg_next_tile_msb = 0x00;

        uint16_t bg_shifter_pattern_lo = 0x0000;
        uint16_t bg_shifter_pattern_hi = 0x0000;
        uint16_t bg_shifter_attr_lo = 0x0000;
        uint16_t bg_shifter_attr_hi = 0x0000;

        uint8_t sprite_shifter_pattern_lo[8];
        uint8_t sprite_shifter_pattern_hi[8];

        std::shared_ptr<Cartridge> cartridge;
};

