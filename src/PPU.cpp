#include <cstdint>
#include "PPU.hpp"


uint16_t TICK;


void PPU::CYCLE(uint8_t N=1) {
    while (N-- > 0)
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

    //could also do CYCLE(321);
    while (TICK < 321)
        CYCLE();

    //NTABLE_BYTE = FETCH(0x2000 | (VRAM_ADDR & 0x0FFF));
    CYCLE(2);
    //ATTRSHIFT_ONE = FETCH(0x23C0 | (VRAM_ADDR & 0x0C00) | ((VRAM_ADDR >> 4) & 0x38) | ((VRAM_ADDR >> 2) & 0X07));
    CYCLE(2);
    //BGSHIFT_ONE =  ROM.FETCH_CHRROM(NTABLE_BYTE); //fine y determines which byte of tile to get
    CYCLE(2);
    //BGSHIFT_ONE = ROM.FETCH_CHRROM(NTABLE_BYTE); //8 bytes higher
    if ((VRAM_ADDR & 0x001F) == 31) { // if coarse X == 31
        VRAM_ADDR &= ~0x001F;          // coarse X = 0
        VRAM_ADDR ^= 0x0400;           // switch horizontal nametable
    } else
        VRAM_ADDR += 1;                // increment coarse X

    CYCLE(2);

    //NTABLE_BYTE = FETCH(0x2000 | (VRAM_ADDR & 0x0FFF));
    CYCLE(2);
    //ATTRSHIFT_TWO = FETCH(0x23C0 | (VRAM_ADDR & 0x0C00) | ((VRAM_ADDR >> 4) & 0x38) | ((VRAM_ADDR >> 2) & 0X07));
    CYCLE(2);
    //BGSHIFT_TWO =  ROM.FETCH_CHRROM(NTABLE_BYTE); //fine y determines which byte of tile to get
    CYCLE(2);
    //BGSHIFT_TWO = ROM.FETCH_CHRROM(NTABLE_BYTE); //8 bytes higher
    if ((VRAM_ADDR & 0x001F) == 31) { // if coarse X == 31
        VRAM_ADDR &= ~0x001F;          // coarse X = 0
        VRAM_ADDR ^= 0x0400;           // switch horizontal nametable
    } else
        VRAM_ADDR += 1;                // increment coarse X

    //Last 2 of 2nd tile fetch and last 4 of scanline (skip last on odd frames)
    if (ODD_FRAME)
        CYCLE(5);
    else
        CYCLE(6);
}


void PPU::SCANLINE() {

    TICK = 0;
    uint8_t NTABLE_BYTE;

}


void PPU::POST_RENDER() {

}


void PPU::VBLANK() {

}


