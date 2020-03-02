#include <cstdint>
#include "PPU.hpp"


uint16_t TICK;


void PPU::CYCLE(uint8_t N=1) {
    while (N-- > 0)
        TICK++;
}


void PPU::RENDER_PIXEL() {

}



void PPU::GENERATE_SIGNAL(Cartridge& NES) {

    ROM = &NES;
    uint16_t SLINE_NUM = 0;

    while (true) {

        PRE_RENDER();

        while (SLINE_NUM++ < 240)
            SCANLINE(SLINE_NUM);

        CYCLE(341); //Post-render
        SLINE_NUM++;

        PPUSTATUS |= 0x80; //VBLANK flag is actually only set in the 2nd tick of scanline 241
        //VBLANK will cause NMI if PPUCTRL allows it, 
        while (SLINE_NUM++ < 261)
            CYCLE(341);

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

    PRE_SLINE_TILE_FETCH();

    //Last 4 cycles of scanline (skip last on odd frames)
    if (ODD_FRAME)
        CYCLE(3);
    else
        CYCLE(4);
}


//Sprite evaluation for the next scanline occurs at the same time, will probably multithread 
void PPU::SCANLINE(uint16_t SLINE) {

    TICK = 0;
    uint8_t NTABLE_BYTE;
    bool ZERO_HIT = false; //Placeholder for now

    //Cycle 0 is idle
    CYCLE();

    //Cycles 1-256: Fetch tile data starting at tile 3 of current scanline (first two fetched from previous scanline)
    while (TICK < 257) {
        switch (TICK % 8) {
            case 0:
                //Fetch PTable high
                break;
            case 1:
                //Reload shift regs - First reload only on cycle 9
                if (TICK == 1)
                    break;
                break;
            case 2:
                //Fetch nametable byte
                break;
            case 4:
                //Fetch attribute byte
                break;
            case 6:
                //Fetch ptable low
                break;
        }

        if ((TICK >= 2 && ZERO_HIT) || TICK >=4)
            RENDER_PIXEL();

        //Shift registers once

        CYCLE();
    }
    //Reload shift regs for cycle 257

    //Cycles 257-320 - Fetch tile data for sprites on next scanline
    Sprite CUR;
    for(int i=0; i < 8; i++) {
        if (OAM_SECONDARY.size() <= i) {
            CYCLE(8);
        } else {
            CUR = OAM_SECONDARY[i];
            CYCLE(2);
            SPR_ATTRS.push_back(CUR.ATTR);
            CYCLE();
            SPR_XPOS.push_back(CUR.X_POS);
            CYCLE();

            //Pattern table data access different for 8*8 vs 8*16 sprites, see PPUCTRL
            if ((PPUCTRL & 0x20) == 0) {
                //ROM.FETCH_CHRROM(((PPUCTRL & 0x08) << 4) | ((CUR.IND * 16) << 4) | fine_y);
                CYCLE(2);
                //ROM.FETCH_CHRROM(((PPUCTRL & 0x08) << 4) | ((CUR.IND * 16) << 4) | fine_y + 8);
                CYCLE(2);
            } else {
                //ROM.FETCH_CHRROM(((0x01 ^ (CUR.IND % 2)) << 12) | (CUR.IND - (CUR.IND % 2)) << 4) | fine_y);
                CYCLE(2);
                //ROM.FETCH_CHRROM(((0x01 ^ (CUR.IND % 2)) << 12) | (CUR.IND - (CUR.IND % 2)) << 4) | fine_y + 8);
                CYCLE(2);
            }

        }

    }

    //Cycle 321-336
    PRE_SLINE_TILE_FETCH();

    //Cycle 337-340
    CYCLE(4); //Supposed to be nametable fetches identical to fetches at start of next scanline
}


void PPU::PRE_SLINE_TILE_FETCH() {
    CYCLE(2);
    //NTABLE_BYTE = FETCH(0x2000 | (VRAM_ADDR & 0x0FFF));
    CYCLE(2);
    //ATTRSHIFT_ONE = FETCH(0x23C0 | (VRAM_ADDR & 0x0C00) | ((VRAM_ADDR >> 4) & 0x38) | ((VRAM_ADDR >> 2) & 0X07));
    CYCLE(2);
    //BGSHIFT_ONE =  ROM.FETCH_CHRROM(NTABLE_BYTE); //fine y determines which byte of tile to get
    CYCLE(2);
    //BGSHIFT_ONE |= (ROM.FETCH_CHRROM(NTABLE_BYTE) << 8); //8 bytes higher
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
    //BGSHIFT_TWO |= (ROM.FETCH_CHRROM(NTABLE_BYTE) << 8); //8 bytes higher
    if ((VRAM_ADDR & 0x001F) == 31) { // if coarse X == 31
        VRAM_ADDR &= ~0x001F;          // coarse X = 0
        VRAM_ADDR ^= 0x0400;           // switch horizontal nametable
    } else
        VRAM_ADDR += 1;                // increment coarse X
}