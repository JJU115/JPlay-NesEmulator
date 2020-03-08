#include <cstdint>
#include "PPU.hpp"


uint16_t TICK;


//Fetch function seems to exist solely for calling Cartridge functions, will keep for now but may get rid of later 
uint8_t PPU::FETCH(uint16_t ADDR) {

    if ((ADDR >= 0x3000) && (ADDR <= 0x3EFF))
        ADDR &= 0x3EFF;

    if ((ADDR >= 0x3F20) && (ADDR <= 0x3FFF))
        ADDR &= 0x3F1F;


    //Pattern Tables
    if ((ADDR >= 0) && (ADDR <= 0x1FFF))
        return ROM->PPU_ACCESS(ADDR);

    //Nametables
    if ((ADDR >= 0x2000) && (ADDR <= 0x2FFF))
        return VRAM[ROM->PPU_ACCESS(ADDR, true)];

    //Palette RAM
    if ((ADDR >= 0x3F00) && (ADDR <= 0x3F1F))
        return 0;


    return 0;
}


void PPU::CYCLE(uint16_t N=1) {
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
        ODD_FRAME = ~ODD_FRAME;

        break; //Only render one frame for now
    }
}


//increment fine y at dot 256 of pre-render and visible scanlines
//also, since pre-render should make the same memory accesses as a regular scanline, the vram address register changes
void PPU::PRE_RENDER() {

    TICK = 0;
    uint8_t NTABLE_BYTE;

    CYCLE();
    while (TICK < 321) {
        if ((TICK % 8 == 0) && (TICK < 256))
           X_INCREMENT();
        
        if (TICK == 256)
            Y_INCREMENT();

        CYCLE();
    }

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
    uint8_t NTABLE_BYTE, PTABLE_LOW, PTABLE_HIGH, ATTR_BYTE;
    bool ZERO_HIT = false; //Placeholder for now, starting at cycle 2, check if opaque pixel of sprite 0 overlaps an opaque background pixel

    //Cycle 0 is idle
    CYCLE();

    //Cycles 1-256: Fetch tile data starting at tile 3 of current scanline (first two fetched from previous scanline)
    //Remember the vram address updates
    while (TICK < 257) {

        if (TICK >=4)
            RENDER_PIXEL();

        //Shift registers once
        //Pattern table data for one tile are stored in the lower 8 bytes of the two shift registers
        //Low entry in BGSHIFT_ONE, high entry in BGSHIFT_TWO
        //The attribute byte for the current tile will be in ATTRSHIFT_ONE, the next tile in ATTRSHIFT_TWO
        //Nesdev says shifters only shift starting at cycle 2? Does it even make a difference?
        BGSHIFT_ONE = (BGSHIFT_ONE >> 1);
        BGSHIFT_TWO = (BGSHIFT_TWO >> 1);
        ATTRSHIFT_ONE = (ATTRSHIFT_ONE >> 1);

        switch (TICK % 8) {
            case 0:
                //Fetch PTable high
                PTABLE_HIGH = FETCH(((PPUCTRL & 0x10) >> 1) + (NTABLE_BYTE * 16) + ((VRAM_ADDR & 0x7000) >> 12));
                if ((TICK % 256) != 0)
                    X_INCREMENT();
            case 1:
                //Reload shift regs - First reload only on cycle 9
                if (TICK == 1)
                    break;
                BGSHIFT_ONE |= (PTABLE_LOW << 8);
                BGSHIFT_TWO |= (PTABLE_HIGH << 8);
                ATTRSHIFT_ONE = ATTRSHIFT_TWO;
                ATTRSHIFT_TWO = ATTR_BYTE;
                break;
            case 2:
                //Fetch nametable byte
                NTABLE_BYTE = FETCH(0x2000 | (VRAM_ADDR & 0x0FFF)); //Change based on base nametable select of ppuctrl register?
                break;
            case 4:
                //Fetch attribute byte
                ATTR_BYTE = FETCH(0x23C0 | (VRAM_ADDR & 0x0C00) | ((VRAM_ADDR >> 4) & 0x38) | ((VRAM_ADDR >> 2) & 0x07));
                break;
            case 6:
                //Fetch ptable low
                PTABLE_LOW = FETCH(((PPUCTRL & 0x10) >> 1) + (NTABLE_BYTE * 16) + ((VRAM_ADDR & 0x7000) >> 12));
                break;
        }

        CYCLE();
    }
    Y_INCREMENT();

    //Cycles 257-320 - Fetch tile data for sprites on next scanline
    Sprite CUR;
    uint16_t SPR_PAT_ADDR;
    for(int i=0; i < 8; i++) {
        if (OAM_SECONDARY.size() <= i) {
            CYCLE(8);
        } else {
            CUR = OAM_SECONDARY[i]; //Remember, lower index in 2nd OAM means sprite has a higher priority
            CYCLE(2);
            SPR_ATTRS.push_back(CUR.ATTR);
            CYCLE();
            SPR_XPOS.push_back(CUR.X_POS);
            CYCLE();

            //Pattern table data access different for 8*8 vs 8*16 sprites, see PPUCTRL
            if ((PPUCTRL & 0x20) == 0) {
                SPR_PAT_ADDR = (((PPUCTRL & 0x08) << 9) | ((CUR.IND/32) << 8) | ((CUR.IND % 32) << 4) | ((VRAM_ADDR & 0x7000) >> 12));
                SPR_PAT.push_back(FETCH(SPR_PAT_ADDR));
                CYCLE(2);
                SPR_PAT.push_back(FETCH(SPR_PAT_ADDR + 8));
                CYCLE(2);
            } else {
                SPR_PAT_ADDR = (((CUR.IND % 2) << 12) | ((CUR.IND - (CUR.IND % 2)) << 4) | ((VRAM_ADDR & 0x7000) >> 12));
                FETCH(SPR_PAT_ADDR);
                CYCLE(2);
                FETCH(SPR_PAT_ADDR + 8);
                CYCLE(2);
            }

        }

    }

    //Cycle 321-336
    PRE_SLINE_TILE_FETCH();

    //Cycle 337-340
    CYCLE(4); //Supposed to be nametable fetches identical to fetches at start of next scanline
}


void PPU::Y_INCREMENT() {

    VRAM_ADDR = ((VRAM_ADDR & 0x7000) != 0x7000) ? (VRAM_ADDR + 0x1000) : (VRAM_ADDR & 0x8FFF);
    uint16_t y = (VRAM_ADDR & 0x03E0) >> 5;

    if (y == 29) {
        y = 0;     
        VRAM_ADDR ^= 0x0800;
    } else 
        y = (y == 31) ? 0 : (y + 1);

    VRAM_ADDR = ((VRAM_ADDR & ~0x03E0) | (y << 5));
}


void PPU::X_INCREMENT() {
    if ((VRAM_ADDR & 0x001F) == 31) { 
        VRAM_ADDR &= ~0x001F;      
        VRAM_ADDR ^= 0x0400;          
    } else {
        VRAM_ADDR++;
    }
}


//How does ppuctrl base nametable address play into fetching ntable bytes?
void PPU::PRE_SLINE_TILE_FETCH() {

    uint8_t NTABLE_BYTE;

    CYCLE(2);
    NTABLE_BYTE = FETCH(0x2000 | (VRAM_ADDR & 0x0FFF));
    CYCLE(2);
    ATTRSHIFT_ONE = FETCH(0x23C0 | (VRAM_ADDR & 0x0C00) | ((VRAM_ADDR >> 4) & 0x38) | ((VRAM_ADDR >> 2) & 0x07));
    CYCLE(2);
    BGSHIFT_ONE = FETCH(((PPUCTRL & 0x10) >> 1) + (NTABLE_BYTE * 16) + ((VRAM_ADDR & 0x7000) >> 12)); //fine y determines which byte of tile to get
    CYCLE(2);
    BGSHIFT_TWO = FETCH(((PPUCTRL & 0x10) >> 1) + (NTABLE_BYTE * 16) + 8 + ((VRAM_ADDR & 0x7000) >> 12)); //8 bytes higher
    X_INCREMENT();

    CYCLE(2);
    NTABLE_BYTE = FETCH(0x2000 | (VRAM_ADDR & 0x0FFF));
    CYCLE(2);
    ATTRSHIFT_TWO = FETCH(0x23C0 | (VRAM_ADDR & 0x0C00) | ((VRAM_ADDR >> 4) & 0x38) | ((VRAM_ADDR >> 2) & 0x07));
    CYCLE(2);
    BGSHIFT_ONE |= (FETCH(((PPUCTRL & 0x10) >> 1) + (NTABLE_BYTE * 16) + ((VRAM_ADDR & 0x7000) >> 12)) << 8); //fine y determines which byte of tile to get
    CYCLE(2);
    BGSHIFT_TWO |= (FETCH(((PPUCTRL & 0x10) >> 1) + (NTABLE_BYTE * 16) + 8 + ((VRAM_ADDR & 0x7000) >> 12)) << 8); //8 bytes higher
    X_INCREMENT();
}