#include <cstdint>
#include "PPU.hpp"


uint16_t TICK;


void PPU::CYCLE() {
    TICK++;
}



void PPU::GENERATE_SIGNAL(Cartridge& NES) {

    ROM = &NES;
    uint16_t SLINE_NUM = 0;

    while (true) {

        PRE_RENDER();

        while (SLINE_NUM++ < 240)
            SCANLINE();

        POST_RENDER();
        SLINE_NUM++;

        while (SLINE_NUM++ < 261)
            VBLANK();

        SLINE_NUM = 0;

        break; //Only render one frame for now
    }
}



void PPU::PRE_RENDER() {

    TICK = 0;
    uint8_t NTABLE_BYTE;

    while (TICK < 321)
        CYCLE();

    //NTABLE_BYTE = 

}


void PPU::SCANLINE() {

}


void PPU::POST_RENDER() {

}


void PPU::VBLANK() {

}


