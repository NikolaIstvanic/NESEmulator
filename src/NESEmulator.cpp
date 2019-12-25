#include <iostream>

#include "NES.hpp"

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

class NESEmulator : public olc::PixelGameEngine {
    public:
        NESEmulator(std::string romPath) {
            sAppName = "NES Emulator";
            this->romPath = romPath;
        }

    private: 
        NES nes;
        float residualTime = 0.0f;
        std::string romPath;

        bool OnUserCreate() override {
            std::shared_ptr<Cartridge> cart = std::make_shared<Cartridge>(romPath);

            if (!cart->valid) {
                std::cout << "File not found!" << std::endl;
                return false;
            }

            nes.loadCartridge(cart);
            nes.reset();
            return true;
        }

        bool OnUserUpdate(float elapsedTime) override {
            nes.controller[0] = 0x00;
            nes.controller[0] |= GetKey(olc::Key::K).bHeld ? 0x80 : 0x00;
            nes.controller[0] |= GetKey(olc::Key::J).bHeld ? 0x40 : 0x00;
            nes.controller[0] |= GetKey(olc::Key::U).bHeld ? 0x20 : 0x00;
            nes.controller[0] |= GetKey(olc::Key::I).bHeld ? 0x10 : 0x00;
            nes.controller[0] |= GetKey(olc::Key::W).bHeld ? 0x08 : 0x00;
            nes.controller[0] |= GetKey(olc::Key::S).bHeld ? 0x04 : 0x00;
            nes.controller[0] |= GetKey(olc::Key::A).bHeld ? 0x02 : 0x00;
            nes.controller[0] |= GetKey(olc::Key::D).bHeld ? 0x01 : 0x00;

            if (GetKey(olc::Key::R).bPressed) {
                nes.reset();
            }
            if (GetKey(olc::Key::ESCAPE).bPressed) {
                return false;
            }

            if (residualTime > 0.0) {
                residualTime -= elapsedTime;
            } else {
                residualTime += (1.0f / 60.0f) - elapsedTime;
                do { nes.step(); } while (!nes.ppu.frame_done);
                nes.ppu.frame_done = false;
            }

            DrawSprite(0, 0, &nes.ppu.getScreen(), 2);
            return true;
        }
};

int main(int argc, char* argv[]) {
    if (argc > 1) {
        NESEmulator emu(argv[1]);
        if (emu.Construct(480, 480, 2, 2)) {
            emu.Start();
        }
    } else {
        std::cout << "Provide path to ROM" << std::endl;
    }
    return 0;
}

