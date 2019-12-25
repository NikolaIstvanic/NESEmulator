#include <cstring>

#include "PPU.hpp"

PPU::PPU() {
    palScreen = {{
        olc::Pixel(84, 84, 84),    olc::Pixel(0, 30, 116),    olc::Pixel(8, 16, 144),
        olc::Pixel(48, 0, 136),    olc::Pixel(68, 0, 100),    olc::Pixel(92, 0, 48),
        olc::Pixel(84, 4, 0),      olc::Pixel(60, 24, 0),     olc::Pixel(32, 42, 0), 
        olc::Pixel(8, 58, 0),      olc::Pixel(0, 64, 0),      olc::Pixel(0, 60, 0),
        olc::Pixel(0, 50, 60),     olc::Pixel(0, 0, 0),       olc::Pixel(0, 0, 0),
        olc::Pixel(0, 0, 0),       olc::Pixel(152, 150, 152), olc::Pixel(8, 76, 196),
        olc::Pixel(48, 50, 236),   olc::Pixel(92, 30, 228),   olc::Pixel(136, 20, 176),
        olc::Pixel(160, 20, 100),  olc::Pixel(152, 34, 32),   olc::Pixel(120, 60, 0),
        olc::Pixel(84, 90, 0),     olc::Pixel(40, 114, 0),    olc::Pixel(8, 124, 0),
        olc::Pixel(0, 118, 40),    olc::Pixel(0, 102, 120),   olc::Pixel(0, 0, 0),
        olc::Pixel(0, 0, 0),       olc::Pixel(0, 0, 0),       olc::Pixel(236, 238, 236),
        olc::Pixel(76, 154, 236),  olc::Pixel(120, 124, 236), olc::Pixel(176, 98, 236),
        olc::Pixel(228, 84, 236),  olc::Pixel(236, 88, 180),  olc::Pixel(236, 106, 100),
        olc::Pixel(212, 136, 32),  olc::Pixel(160, 170, 0),   olc::Pixel(116, 196, 0),
        olc::Pixel(76, 208, 32),   olc::Pixel(56, 204, 108),  olc::Pixel(56, 180, 204),
        olc::Pixel(60, 60, 60),    olc::Pixel(0, 0, 0),       olc::Pixel(0, 0, 0),
        olc::Pixel(236, 238, 236), olc::Pixel(168, 204, 236), olc::Pixel(188, 188, 236),
        olc::Pixel(212, 178, 236), olc::Pixel(236, 174, 236), olc::Pixel(236, 174, 212),
        olc::Pixel(236, 180, 176), olc::Pixel(228, 196, 144), olc::Pixel(204, 210, 120),
        olc::Pixel(180, 222, 120), olc::Pixel(168, 226, 144), olc::Pixel(152, 226, 180),
        olc::Pixel(160, 214, 228), olc::Pixel(160, 162, 160), olc::Pixel(0, 0, 0),
        olc::Pixel(0, 0, 0)
    }};
}

void PPU::reset() {
    PPU_ctrl.val = 0x00;
    PPU_mask.val = 0x00;
    PPU_stat.val = 0x00;
    tram_addr.val = 0x0000;
    vram_addr.val = 0x0000;
    scanline = 0;
    cycle = 0;
    address_latch = 0x00;
    ppu_data_buffer = 0x00;
    fine_x = 0x00;
    bg_next_tile_id = 0x00;
    bg_next_tile_attr = 0x00;
    bg_next_tile_lsb = 0x00;
    bg_next_tile_msb = 0x00;
    bg_shifter_pattern_lo = 0x0000;
    bg_shifter_pattern_hi = 0x0000;
    bg_shifter_attr_lo = 0x0000;
    bg_shifter_attr_hi = 0x0000;
}

void PPU::connectCartridge(const std::shared_ptr<Cartridge>& cartridge) {
    this->cartridge = cartridge;
}

olc::Sprite& PPU::getScreen() { return sprScreen; }

olc::Pixel& PPU::getColor(uint8_t palette, uint8_t pixel) {
    return palScreen[ppuRead8(0x3F00 + (palette << 2) + pixel) & 0x3F];
}

void PPU::step() {
    auto incrementScrollX = [&]() {
        if (PPU_mask.render_bg || PPU_mask.render_sprites) {
            if (vram_addr.coarse_x == 31) {
                vram_addr.coarse_x = 0;
                vram_addr.nametable_x = ~vram_addr.nametable_x;
            } else {
                vram_addr.coarse_x++;
            }
        }
    };

    auto incrementScrollY = [&]() {
        if (PPU_mask.render_bg || PPU_mask.render_sprites) {
            if (vram_addr.fine_y < 7) {
                vram_addr.fine_y++;
            } else {
                vram_addr.fine_y = 0;
                if (vram_addr.coarse_y == 29) {
                    vram_addr.coarse_y = 0;
                    vram_addr.nametable_y = ~vram_addr.nametable_y;
                } else if (vram_addr.coarse_y == 31) {
                    vram_addr.coarse_y = 0;
                } else {
                    vram_addr.coarse_y++;
                }
            }
        }   
    };

    auto transferAddressX = [&]() {
        if (PPU_mask.render_bg || PPU_mask.render_sprites) {
            vram_addr.nametable_x = tram_addr.nametable_x;
            vram_addr.coarse_x = tram_addr.coarse_x;
        }
    };

    auto transferAddressY = [&]() {
        if (PPU_mask.render_bg || PPU_mask.render_sprites) {
            vram_addr.fine_y = tram_addr.fine_y;
            vram_addr.nametable_y = tram_addr.nametable_y;
            vram_addr.coarse_y = tram_addr.coarse_y;
        }
    };

    auto loadBackgroundShifters = [&]() {
        bg_shifter_pattern_lo = (bg_shifter_pattern_lo & 0xFF00)
            | bg_next_tile_lsb;
        bg_shifter_pattern_hi = (bg_shifter_pattern_hi & 0xFF00)
            | bg_next_tile_msb;
        bg_shifter_attr_lo = (bg_shifter_attr_lo & 0xFF00)
            | ((bg_next_tile_attr & 0x01) ? 0xFF : 0x00);
        bg_shifter_attr_hi = (bg_shifter_attr_hi & 0xFF00)
            | ((bg_next_tile_attr & 0x02) ? 0xFF : 0x00);
    };

    auto updateShifters = [&]() {
        if (PPU_mask.render_bg) {
            bg_shifter_pattern_lo <<= 1;
            bg_shifter_pattern_hi <<= 1;
            bg_shifter_attr_lo <<= 1;
            bg_shifter_attr_hi <<= 1;
        }
        if (PPU_mask.render_sprites && cycle >= 1 && cycle < 258) {
            for (int i = 0; i < sprite_count; i++) {
                if (spriteScanline[i].x > 0) {
                    spriteScanline[i].x--;
                } else {
                    sprite_shifter_pattern_lo[i] <<= 1;
                    sprite_shifter_pattern_hi[i] <<= 1;
                }
            }
        }
    };

    if (scanline >= -1 && scanline < 240) {
        if (scanline == 0 && cycle == 0) {
            cycle = 1;
        }
        if (scanline == -1 && cycle == 1) {
            PPU_stat.vblank = 0;
            PPU_stat.sprite_overflow = 0;
            PPU_stat.sprite_zero_hit = 0;

            for (int i = 0; i < 8; i++) {
                sprite_shifter_pattern_lo[i] = 0;
                sprite_shifter_pattern_hi[i] = 0;
            }
        }
        if ((cycle >= 2 && cycle < 258) || (cycle >= 321 && cycle < 338)) {
            updateShifters();

            switch ((cycle - 1) % 8) {
                case 0:
                    loadBackgroundShifters();
                    bg_next_tile_id = ppuRead8(0x2000 | (vram_addr.val & 0x0FFF));
                    break;
                case 2:
                    bg_next_tile_attr = ppuRead8(0x23C0
                            | (vram_addr.nametable_y << 11)
                            | (vram_addr.nametable_x << 10)
                            | ((vram_addr.coarse_y >> 2) << 3)
                            | (vram_addr.coarse_x >> 2));
                    if (vram_addr.coarse_y & 0x02) {
                        bg_next_tile_attr >>= 4;
                    }
                    if (vram_addr.coarse_x & 0x02) {
                        bg_next_tile_attr >>= 2;
                    }
                    bg_next_tile_attr &= 0x03;
                    break;
                case 4:
                    bg_next_tile_lsb = ppuRead8((PPU_ctrl.pattern_bg << 12)
                        + ((uint16_t) bg_next_tile_id << 4) + vram_addr.fine_y);
                    break;
                case 6:
                    bg_next_tile_msb = ppuRead8((PPU_ctrl.pattern_bg << 12)
                        + ((uint16_t) bg_next_tile_id << 4) + vram_addr.fine_y + 8);
                    break;
                case 7:
                    incrementScrollX();
                    break;
            }
        }
        if (cycle == 256) {
            incrementScrollY();
        }
        if (cycle == 257) {
            loadBackgroundShifters();
            transferAddressX();
        }
        if (cycle == 338 || cycle == 340) {
            bg_next_tile_id = ppuRead8(0x2000 | (vram_addr.val & 0x0FFF));
        }
        if (scanline == -1 && cycle >= 280 && cycle < 305) {
            transferAddressY();
        }
        if (cycle == 257 && scanline >= 0) {
            std::memset(spriteScanline, 0xFF, 8 * sizeof(OAMEntry));
            sprite_count = 0;

            for (int i = 0; i < 8; i++) {
                sprite_shifter_pattern_lo[i] = 0;
                sprite_shifter_pattern_hi[i] = 0;
            }

            uint8_t nOAMEntry = 0;
            spriteZeroHitPossible = false;

            while (nOAMEntry < 64 && sprite_count < 9) {
                int16_t diff = ((int16_t) scanline - (int16_t) OAM[nOAMEntry].y);

                if (diff >= 0 && diff < (PPU_ctrl.sprite_size ? 16 : 8)) {
                    if (sprite_count < 8) {
                        if (nOAMEntry == 0) {
                            spriteZeroHitPossible = true;
                        }
                        memcpy(&spriteScanline[sprite_count], &OAM[nOAMEntry],
                                sizeof(OAMEntry));
                        sprite_count++;
                    }
                }
                nOAMEntry++;
            }
            PPU_stat.sprite_overflow = (sprite_count > 8);
        }

        if (cycle == 340) {
            for (uint8_t i = 0; i < sprite_count; i++) {
                uint8_t sprite_pattern_bits_lo, sprite_pattern_bits_hi;
                uint16_t sprite_pattern_addr_lo, sprite_pattern_addr_hi;

                if (PPU_ctrl.sprite_size) {
                    if (!(spriteScanline[i].attr & 0x80)) {
                        if (scanline - spriteScanline[i].y < 8) {
                            sprite_pattern_addr_lo =
                                ((spriteScanline[i].id & 0x01) << 12)
                                | ((spriteScanline[i].id & 0xFE) << 4)
                                | ((scanline - spriteScanline[i].y) & 0x07);
                        } else {
                            sprite_pattern_addr_lo =
                                ((spriteScanline[i].id & 0x01) << 12)
                                | (((spriteScanline[i].id & 0xFE) + 1) << 4)
                                | ((scanline - spriteScanline[i].y) & 0x07);
                        }
                    } else {
                        if (scanline - spriteScanline[i].y < 8) {
                            sprite_pattern_addr_lo =
                                ((spriteScanline[i].id & 0x01) << 12)
                                | (((spriteScanline[i].id & 0xFE) + 1) << 4)
                                | (7 - ((scanline - spriteScanline[i].y) & 0x07));
                        } else {
                            sprite_pattern_addr_lo =
                                ((spriteScanline[i].id & 0x01) << 12)
                                | ((spriteScanline[i].id & 0xFE) << 4)
                                | (7 - ((scanline - spriteScanline[i].y) & 0x07));
                        }
                    }
                } else {
                    if (!(spriteScanline[i].attr & 0x80)) {
                        sprite_pattern_addr_lo = (PPU_ctrl.pattern_sprite << 12)
                            | (spriteScanline[i].id << 4)
                            | (scanline - spriteScanline[i].y);
                    } else {
                        sprite_pattern_addr_lo = (PPU_ctrl.pattern_sprite << 12)
                            | (spriteScanline[i].id << 4)
                            | (7 - (scanline - spriteScanline[i].y));
                    }
                }
                sprite_pattern_addr_hi = sprite_pattern_addr_lo + 8;
                sprite_pattern_bits_lo = ppuRead8(sprite_pattern_addr_lo);
                sprite_pattern_bits_hi = ppuRead8(sprite_pattern_addr_hi);

                if (spriteScanline[i].attr & 0x40) {
                    auto flip = [](uint8_t b) {
                        b = (b & 0xF0) >> 4 | (b & 0x0f) << 4;
                        b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
                        b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
                        return b;
                    };
                    sprite_pattern_bits_lo = flip(sprite_pattern_bits_lo);
                    sprite_pattern_bits_hi = flip(sprite_pattern_bits_hi);
                }
                sprite_shifter_pattern_lo[i] = sprite_pattern_bits_lo;
                sprite_shifter_pattern_hi[i] = sprite_pattern_bits_hi;
            }
        }
    }

    if (scanline >= 241 && scanline < 261) {
        if (scanline == 241 && cycle == 1) {
            PPU_stat.vblank = 1;

            if (PPU_ctrl.enable_nmi) {
                nmi = true;
            }
        }
    }

    uint8_t bg_pixel = 0x00;
    uint8_t bg_palette = 0x00;

    if (PPU_mask.render_bg) {
        uint16_t bit_mux = 0x8000 >> fine_x;
        uint8_t p0 = (bg_shifter_pattern_lo & bit_mux) > 0;
        uint8_t p1 = (bg_shifter_pattern_hi & bit_mux) > 0;
        bg_pixel = (p1 << 1) | p0;

        uint8_t b0 = (bg_shifter_attr_lo & bit_mux) > 0;
        uint8_t b1 = (bg_shifter_attr_hi & bit_mux) > 0;
        bg_palette = (b1 << 1) | b0;
    }

    uint8_t fg_pixel = 0x00;
    uint8_t fg_palette = 0x00;
    uint8_t fg_priority = 0x00;

    if (PPU_mask.render_sprites) {
        spriteZeroBeingRendered = false;

        for (int i = 0; i < sprite_count; i++) {
            if (spriteScanline[i].x == 0) {
                uint8_t fg_pixel_lo = (sprite_shifter_pattern_lo[i] & 0x80) > 0;
                uint8_t fg_pixel_hi = (sprite_shifter_pattern_hi[i] & 0x80) > 0;
                fg_pixel = (fg_pixel_hi << 1) | fg_pixel_lo;
                fg_palette = (spriteScanline[i].attr & 0x03) + 0x04;
                fg_priority = (spriteScanline[i].attr & 0x20) == 0;

                if (fg_pixel != 0) {
                    if (i == 0) {
                        spriteZeroBeingRendered = true;
                    }
                    break;
                }
            }
        }
    }

    uint8_t pixel = 0x00;
    uint8_t palette = 0x00;

    if (bg_pixel == 0 && fg_pixel == 0) {
        pixel = 0x00;
        palette = 0x00;
    } else if (bg_pixel == 0 && fg_pixel > 0) {
        pixel = fg_pixel;
        palette = fg_palette;
    } else if (bg_pixel > 0 && fg_pixel == 0) {
        pixel = bg_pixel;
        palette = bg_palette;
    } else if (bg_pixel > 0 && fg_pixel > 0) {
        if (fg_priority) {
            pixel = fg_pixel;
            palette = fg_palette;
        } else {
            pixel = bg_pixel;
            palette = bg_palette;
        }
        if (spriteZeroHitPossible && spriteZeroBeingRendered) {
            if (PPU_mask.render_bg && PPU_mask.render_sprites) {
                if (~(PPU_mask.render_bg_left | PPU_mask.render_sprites_left)) {
                    if (cycle >= 9 && cycle < 258) {
                        PPU_stat.sprite_zero_hit = 1;
                    }
                } else {
                    if (cycle >= 1 && cycle < 258) {
                        PPU_stat.sprite_zero_hit = 1;
                    }
                }
            }
        }
    }

    sprScreen.SetPixel(cycle - 1, scanline, getColor(palette, pixel));

    cycle++;
    if (cycle >= 341) {
        cycle = 0;
        scanline++;

        if (scanline >= 261) {
            scanline = -1;
            frame_done = true;
        }
    }
}

uint8_t PPU::cpuRead8(uint16_t addr, bool bReadOnly) {
    uint8_t data = 0x00;

	if (bReadOnly) {
        switch (addr) {
            case PPU_CTRL:
                data = PPU_ctrl.val;
                break;
            case PPU_MASK:
                data = PPU_mask.val;
                break;
            case PPU_STAT:
                data = PPU_stat.val;
                break;
        }
    } else {
        switch (addr) {
            case PPU_STAT:
                data = (PPU_stat.val & 0xE0) | (ppu_data_buffer & 0x1F);
                PPU_stat.vblank = 0;
                address_latch = 0;
                break;

            case OAM_DATA:
                data = pOAM[oam_addr];
                break;

            case PPU_DATA:
                data = ppu_data_buffer;
                ppu_data_buffer = ppuRead8(vram_addr.val);
                if (vram_addr.val >= 0x3F00) {
                    data = ppu_data_buffer;
                }
                vram_addr.val += PPU_ctrl.increment_mode ? 32 : 1;
                break;
        }
    }
    return data;
}

uint8_t PPU::ppuRead8(uint16_t addr, bool bReadOnly) {
    uint8_t data = 0x00;
    addr &= 0x3FFF;

    if (cartridge->ppuRead8(addr, data)) {

    } else if (0x0000 <= addr && addr <= 0x1FFF) {
        data = patternTable[(addr & 0x1000) >> 12][addr & 0x0FFF];
    } else if (0x2000 <= addr && addr <= 0x3EFF) {
        addr &= 0x0FFF;

        if (cartridge->mirror == Cartridge::MIRROR::VERTICAL) {
            if (0x0000 <= addr && addr <= 0x03FF) {
                data = nameTable[0][addr & 0x03FF];
            } else if (0x0400 <= addr && addr <= 0x07FF) {
                data = nameTable[1][addr & 0x03FF];
            } else if (0x0800 <= addr && addr <= 0x0BFF) {
                data = nameTable[0][addr & 0x03FF];
            } else if (0x0C00 <= addr && addr <= 0x0FFF) {
                data = nameTable[1][addr & 0x03FF];
            }
        } else if (cartridge->mirror == Cartridge::MIRROR::HORIZONTAL) {
            if (0x0000 <= addr && addr <= 0x03FF) {
                data = nameTable[0][addr & 0x03FF];
            } else if (0x0400 <= addr && addr <= 0x07FF) {
                data = nameTable[0][addr & 0x03FF];
            } else if (0x0800 <= addr && addr <= 0x0BFF) {
                data = nameTable[1][addr & 0x03FF];
            } else if (0x0C00 <= addr && addr <= 0x0FFF) {
                data = nameTable[1][addr & 0x03FF];
            }
        }
    } else if (0x3F00 <= addr && addr <= 0x3FFF) {
        addr &= 0x001F;

        if (addr == 0x0010) {
            addr = 0x0000;
        } else if (addr == 0x0014) {
            addr = 0x0004;
        } else if (addr == 0x0018) {
            addr = 0x0008;
        } else if (addr == 0x001C) {
            addr = 0x000C;
        }
        data = paletteTable[addr];
    }
    return data;
}

void PPU::cpuWrite8(uint16_t addr, uint8_t data) {
    switch (addr) {
        case PPU_CTRL:
            PPU_ctrl.val = data;
            tram_addr.nametable_x = PPU_ctrl.nametable_x;
            tram_addr.nametable_y = PPU_ctrl.nametable_y;
            break;

        case PPU_MASK:
            PPU_mask.val = data;
            break;

        case OAM_ADDR:
            oam_addr = data;
            break;

        case OAM_DATA:
            pOAM[oam_addr] = data;
            break;

        case PPU_SCRL:
            if (address_latch == 0) {
                fine_x = data & 0x07;
                tram_addr.coarse_x = data >> 3;
                address_latch = 1;
            } else {
                tram_addr.fine_y = data & 0x07;
                tram_addr.coarse_y = data >> 3; 
                address_latch = 0;
            }
            break;

        case PPU_ADDR:
            if (address_latch == 0) {
                tram_addr.val = (uint16_t) ((data & 0x3F) << 8)
                    | (tram_addr.val & 0x00FF);
                address_latch = 1;
            } else {
                tram_addr.val = (tram_addr.val & 0xFF00) | data;
                vram_addr = tram_addr;
                address_latch = 0;
            }
            break;

        case PPU_DATA:
            ppuWrite8(vram_addr.val, data);
            vram_addr.val += PPU_ctrl.increment_mode ? 32 : 1;
            break;
    }
}

void PPU::ppuWrite8(uint16_t addr, uint8_t data) {
    addr &= 0x3FFF;

    if (cartridge->ppuWrite8(addr, data)) {

    } else if (0x0000 <= addr && addr <= 0x1FFF) {
        patternTable[(addr & 0x1000) >> 12][addr & 0x0FFF] = data;
    } else if (0x2000 <= addr && addr <= 0x3EFF) {
        addr &= 0x0FFF;

        if (cartridge->mirror == Cartridge::MIRROR::VERTICAL) {
            if (0x0000 <= addr && addr <= 0x03FF) {
                nameTable[0][addr & 0x03FF] = data;
            } else if (0x0400 <= addr && addr <= 0x07FF) {
                nameTable[1][addr & 0x03FF] = data;
            } else if (0x0800 <= addr && addr <= 0x0BFF) {
                nameTable[0][addr & 0x03FF] = data;
            } else if (0x0C00 <= addr && addr <= 0x0FFF) {
                nameTable[1][addr & 0x03FF] = data;
            }
        } else if (cartridge->mirror == Cartridge::MIRROR::HORIZONTAL) {
            if (0x0000 <= addr && addr <= 0x03FF) {
                nameTable[0][addr & 0x03FF] = data;
            } else if (0x0400 <= addr && addr <= 0x07FF) {
                nameTable[0][addr & 0x03FF] = data;
            } else if (0x0800 <= addr && addr <= 0x0BFF) {
                nameTable[1][addr & 0x03FF] = data;
            } else if (0x0C00 <= addr && addr <= 0x0FFF) {
                nameTable[1][addr & 0x03FF] = data;
            }
        }
    } else if (0x3F00 <= addr && addr <= 0x3FFF) {
        addr &= 0x001F;
 
        if (addr == 0x0010) {
            addr = 0x0000;
        } else if (addr == 0x0014) {
            addr = 0x0004;
        } else if (addr == 0x0018) {
            addr = 0x0008;
        } else if (addr == 0x001C) {
            addr = 0x000C;
        }
        paletteTable[addr] = data;
    }
}

