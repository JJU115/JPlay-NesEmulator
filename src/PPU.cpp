#include <cstdint>
#include "PPU.hpp"

std::mutex PPU_MTX;
std::condition_variable PPU_CV;
std::unique_lock<std::mutex> PPU_LCK(PPU_MTX);

uint16_t TICK;
std::ofstream P_LOG;

//Fetch function seems to exist solely for calling Cartridge functions, will keep for now but may get rid of later 
uint8_t PPU::FETCH(uint16_t ADDR) {
    //std::cout << "PPU read from " << std::hex << int(ADDR) << '\n';
    if ((ADDR >= 0x3000) && (ADDR <= 0x3EFF))
        ADDR &= 0x2EFF;

    if ((ADDR >= 0x3F20) && (ADDR <= 0x3FFF))
        ADDR &= 0x3F1F;


    //Pattern Tables
    if (ADDR <= 0x1FFF)
        return ROM->PPU_ACCESS(ADDR, true);

    //Nametables - Addresses need to be properly translated to array indices, 0x2000 will be index 0 for example and so on
    if (ADDR <= 0x2FFF) {
        uint16_t NT_ADDR = ROM->PPU_ACCESS(ADDR, true, true);
        if (NT_ADDR < 0x2800)
            return VRAM[NT_ADDR % 0x2000];
        else
            return VRAM[(NT_ADDR - 0x0400) % 0x2800];
    }
        
    //Palette RAM
    if (ADDR <= 0x3F1F) {
        if ((ADDR % 4) == 0)
            return PALETTES[0];
        else
            return PALETTES[(ADDR & 0x00FF) - ((ADDR - 0x3F00)/4)];        
    }


    return 0;
}


void PPU::WRITE(uint16_t ADDR, uint8_t DATA) {
    //P_LOG << "PPU write of " << std::hex << int(DATA) << " to " << std::hex << int(ADDR) << '\n';
    if ((ADDR >= 0x3000) && (ADDR <= 0x3EFF))
        ADDR &= 0x2EFF;

    if ((ADDR >= 0x3F20) && (ADDR <= 0x3FFF))
        ADDR &= 0x3F1F;

    if (ADDR <= 0x1FFF)
        ROM->PPU_ACCESS(ADDR, true); //Doesn't do anything since writing to change cartridge CHR data probably isn't allowed

    //Nametables
    if ((ADDR >= 0x2000) && (ADDR <= 0x2FFF)) {
        uint16_t NT_ADDR = ROM->PPU_ACCESS(ADDR, true, true);
        if (NT_ADDR < 0x2800)
            VRAM[NT_ADDR % 0x2000] = DATA;
        else
            VRAM[NT_ADDR - 0x0400] = DATA;
    }
        

    //Palette RAM
    if ((ADDR >= 0x3F00) && (ADDR <= 0x3F1F)) {
        if ((ADDR % 4) == 0)
            PALETTES[0] = DATA;
        else
            PALETTES[(ADDR & 0x00FF) - ((ADDR - 0x3F00)/4)] = DATA;

    }
    //P_LOG << "After write: " << std::hex << int(FETCH(ADDR)) << '\n';
}


void PPU::CYCLE(uint16_t N) {
    while (N-- > 0) {
        PPU_CV.wait(PPU_LCK);
        //if (debug_wait)
          //  std::this_thread::sleep_for(std::chrono::milliseconds(100));
        TICK++;
        //P_LOG << "PPU Tick\n";
    }
    //P_LOG << "PPU Tick\n";
}


void PPU::GENERATE_SIGNAL() {
    VRAM.fill(0);
    debug_wait = false;
    uint16_t SLINE_NUM = 0;
    NMI_OCC = 0;
    NMI_OUT = 0;
    GEN_NMI = 0;
    SDL_Event evt;
    P_LOG.open("PPU_LOG.txt", std::ios::trunc | std::ios::out);
    while (true) {

        SDL_PollEvent(&evt);

        if ((PPUMASK & 0x08) != 0) {
            debug_wait = true;
            VRAM_ADDR = 0;
            VRAM_TEMP = 0;
            PPUCTRL = 0x10;
            PALETTES[0] = 0x3F;
            PALETTES[1] = 0x21;
            PALETTES[3] = 0x12;
        }

        PRE_RENDER();

        while (SLINE_NUM++ < 240) {
            if ((PPUMASK & 0x08) != 0)
                SCANLINE(SLINE_NUM);
        }

        //P_LOG << "Entering post render\n";
        CYCLE(341); //Post-render, don't think any of the below happens as it does in every other scanline but will keep commented here for now
        /*Y_INCREMENT();
        CYCLE();
        if ((PPUMASK & 0x08) || (PPUMASK & 0x10)) {
            VRAM_ADDR &= 0xFBE0;
            VRAM_ADDR |= (VRAM_TEMP & 0x041F);
        }
        CYCLE(85);*/
        SLINE_NUM++;
        //P_LOG << "Post render done";
        PPUSTATUS |= 0x80; //VBLANK flag is actually only set in the 2nd tick of scanline 241
        NMI_OCC = 1;
        //VBLANK will cause NMI if PPUCTRL allows it, there's some complication here since NMIs can still happen during vblank, will need to resolve later
        if (NMI_OCC && NMI_OUT)
            GEN_NMI = 1;
            
        while (SLINE_NUM++ < 262) { //Every scanline does the 256 increment and 257 copy but as with pre-render, not sure if it happens in the vblank period, so commented here for now
            CYCLE(341);
            /*Y_INCREMENT();
            CYCLE();
            if ((PPUMASK & 0x08) || (PPUMASK & 0x10)) {
                VRAM_ADDR &= 0xFBE0;
                VRAM_ADDR |= (VRAM_TEMP & 0x041F);
            }
            CYCLE(85);*/
        }

        NMI_OCC = 0;
        SLINE_NUM = 0;
        ODD_FRAME = ~ODD_FRAME;
        P_LOG << "One frame rendered\n";
        if (debug_wait)
            break; //Only render one frame for now
        
    }

    delete Screen;
}


//increment fine y at dot 256 of pre-render and visible scanlines
//also, since pre-render should make the same memory accesses as a regular scanline, the vram address register changes
//Nesdev says vertical scroll bits are reloaded during pre-render if rendering is enabled?
void PPU::PRE_RENDER() {
    TICK = 0;
    uint8_t NTABLE_BYTE;

    CYCLE();
    PPUSTATUS &= 0x7F;  //Signal end of Vblank

    while (TICK < 321) {
        if ((TICK % 8 == 0) && (TICK < 256))
           X_INCREMENT();
        
        if (TICK == 256)
            Y_INCREMENT();

        if (TICK == 257) {
            OAMADDR = 0;
            //Copy all horizontal bits from t to v at tick 257 if rendering enabled
            if ((PPUMASK & 0x08) || (PPUMASK & 0x10)) {
                VRAM_ADDR &= 0xFBE0;
                VRAM_ADDR |= (VRAM_TEMP & 0x041F);
            }
        }

        if ((TICK >= 280) && (TICK <= 304)) {
            if ((PPUMASK & 0x08) || (PPUMASK & 0x10)) {
                VRAM_ADDR &= 0x041F;
                VRAM_ADDR |= (VRAM_TEMP & 0xFBE0);
            }
        }

        CYCLE();
    }

    PRE_SLINE_TILE_FETCH();

    //Last 4 cycles of scanline (skip last on odd frames)
    if (ODD_FRAME)
        CYCLE(3);
    else
        CYCLE(4);
    P_LOG << "Pre render done\n";
}


//Sprite evaluation for the next scanline occurs at the same time, will probably multithread 
//For now only rendering the background so sprite eval doesn't need to happen
//What is enabled or disabled if rendering is disabled? VRAM reg updates? Byte fetching?
//May eliminate one attribute shift register
void PPU::SCANLINE(uint16_t SLINE) {

    TICK = 0;
    uint8_t NTABLE_BYTE, PTABLE_LOW, PTABLE_HIGH, ATTR_BYTE;
    uint16_t PIXEL;
    bool ZERO_HIT = false; //Placeholder for now, starting at cycle 2, check if opaque pixel of sprite 0 overlaps an opaque background pixel

    //Cycle 0 is idle
    CYCLE();
    //P_LOG << SLINE << ":  ";
    //Cycles 1-256: Fetch tile data starting at tile 3 of current scanline (first two fetched from previous scanline)
    //Remember the vram address updates
    while (TICK < 257) {
        switch (TICK % 8) {
            case 0:
                //Fetch PTable high
                PTABLE_HIGH = FETCH(((PPUCTRL & 0x10) << 8) | ((NTABLE_BYTE * 16) + 8 + ((VRAM_ADDR & 0x7000) >> 12)));
                //P_LOG << "PTable high: " << std::hex << int(PTABLE_HIGH) << '\n';
                if (TICK != 256)
                    X_INCREMENT();
                else
                    Y_INCREMENT();
                break;
            case 1:
                //Reload shift regs - First reload only on cycle 9
                if (TICK == 1)
                    break;
                BGSHIFT_ONE |= PTABLE_LOW;
                BGSHIFT_TWO |= PTABLE_HIGH;
                ATTRSHIFT_ONE = ATTR_BYTE;
                ATTRSHIFT_TWO = ATTR_BYTE;
                break;
            case 2:
                //Fetch nametable byte
                NTABLE_BYTE = FETCH(0x2000 | (VRAM_ADDR & 0x0FFF)); //Change based on base nametable select of ppuctrl register?
                //P_LOG << "NTable byte: " << std::hex << int(NTABLE_BYTE) << '\n';
                break;
            case 4:
                //Fetch attribute byte - since one byte controls a 4*4 tile group, fetching becomes more complicated. Need to change this:
                ATTR_BYTE = FETCH(0x23C0 | (VRAM_ADDR & 0x0C00) | ((VRAM_ADDR >> 4) & 0x38) | ((VRAM_ADDR >> 2) & 0x07));
                //P_LOG << "Attribute byte: " << std::hex << int(ATTR_BYTE) << '\n';
                break;
            case 6:
                //Fetch ptable low
                PTABLE_LOW = FETCH(((PPUCTRL & 0x10) << 8) | ((NTABLE_BYTE * 16) + ((VRAM_ADDR & 0x7000) >> 12)));
                //P_LOG << "PTable low: " << std::hex << int(PTABLE_LOW) << '\n';
                break;
            default:
                break;
        }
        //P_LOG << "1st shifter: " << std::hex << int(BGSHIFT_ONE) << '\n';
        //P_LOG << "2nd shifter: " << std::hex << int(BGSHIFT_TWO) << '\n';
        PIXEL = 0x3F00;
        PIXEL |= ((BGSHIFT_ONE & 0x8000) != 0) ? 1 : 0;
        PIXEL |= ((BGSHIFT_TWO & 0x8000) != 0) ? 0x02 : 0;
        //Form the pixel color
        if (((SLINE/2) % 2) == 0)
            PIXEL |= (((TICK-1)/16) % 2 == 0) ? ((ATTRSHIFT_ONE & 0x03) << 2) : (ATTRSHIFT_ONE & 0x0C);
        else
            PIXEL |= (((TICK-1)/16) % 2 == 0) ? ((ATTRSHIFT_ONE & 0x30) >> 2) : ((ATTRSHIFT_ONE & 0xC0) >> 4);

        //nesdev says there's a delay in rendering and the 1st pixel is output only at cycle 4?
        Screen->RENDER_PIXEL((SLINE * 256) + TICK - 1, FETCH(PIXEL));

        //Shift registers once
        //Pattern table data for one tile are stored in the lower 8 bytes of the two shift registers
        //Low entry in BGSHIFT_ONE, high entry in BGSHIFT_TWO
        //The attribute byte for the current tile will be in ATTRSHIFT_ONE, the next tile in ATTRSHIFT_TWO
        //Nesdev says shifters only shift starting at cycle 2? Does it even make a difference?
        BGSHIFT_ONE <<= 1;
        BGSHIFT_TWO <<= 1;
        //ATTRSHIFT_ONE <<= 1; - Why shift this at all?

        CYCLE();
    }
    //Copy all horizontal bits from t to v at tick 257 if rendering enabled
    if ((PPUMASK & 0x08) || (PPUMASK & 0x10)) {
        VRAM_ADDR &= 0xFBE0;
        VRAM_ADDR |= (VRAM_TEMP & 0x041F);
    }
    //P_LOG << '\n';

    //Cycles 257-320 - Fetch tile data for sprites on next scanline
    Sprite CUR;
    uint16_t SPR_PAT_ADDR;
    for(int i=0; i < 8; i++) {
        OAMADDR = 0;
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
    P_LOG << "Scanline complete\n";
}


void PPU::Y_INCREMENT() {

    if ((PPUMASK & 0x08) || (PPUMASK & 0x10)) {
        if ((VRAM_ADDR & 0x7000) != 0x7000)
            VRAM_ADDR += 0x1000;
        else {
            VRAM_ADDR &= 0x0FFF;
            uint16_t y = (VRAM_ADDR & 0x03E0) >> 5;

            if (y == 29) {
                y = 0;     
                VRAM_ADDR ^= 0x0800;
                VRAM_ADDR = (VRAM_ADDR & ~0x03E0) | (y << 5);
            } else {
                y = (y == 31) ? 0 : (y + 1);
                VRAM_ADDR = (VRAM_ADDR & ~0x03E0) | (y << 5);
            }
        }
    }
}


void PPU::X_INCREMENT() {
    if ((PPUMASK & 0x08) || (PPUMASK & 0x10)) {
        if ((VRAM_ADDR & 0x001F) == 31) { 
            VRAM_ADDR &= ~0x001F;      
            VRAM_ADDR ^= 0x0400;          
        } else {
            VRAM_ADDR++;
        }
    }
}


//How does ppuctrl base nametable address play into fetching ntable bytes?
//If rendering is disabled, how does this act differently?
void PPU::PRE_SLINE_TILE_FETCH() {

    uint8_t NTABLE_BYTE;
    //P_LOG << "Preline tile fetch\n";
    //P_LOG << "VRAM: " << std::hex << VRAM_ADDR << '\n';
    CYCLE(2);
    NTABLE_BYTE = FETCH(0x2000 | (VRAM_ADDR & 0x0FFF));
    //P_LOG << "1st nametable byte: " << std::hex << int(NTABLE_BYTE) << '\n';
    //P_LOG << std::hex << int(NTABLE_BYTE) << " ";
    CYCLE(2);
    ATTRSHIFT_ONE = FETCH(0x23C0 | (VRAM_ADDR & 0x0C00) | ((VRAM_ADDR >> 4) & 0x38) | ((VRAM_ADDR >> 2) & 0x07));
    //P_LOG << "1st attribute byte: " << std::hex << int(ATTRSHIFT_ONE) << '\n';
    CYCLE(2);
    BGSHIFT_ONE = FETCH(((PPUCTRL & 0x10) << 8) | ((NTABLE_BYTE * 16) + ((VRAM_ADDR & 0x7000) >> 12))) << 8; //fine y determines which byte of tile to get
    //P_LOG << "1st pattern byte: " << std::hex << BGSHIFT_ONE << '\n';
    CYCLE(2);
    BGSHIFT_TWO = FETCH(((PPUCTRL & 0x10) << 8) | ((NTABLE_BYTE * 16) + 8 + ((VRAM_ADDR & 0x7000) >> 12))) << 8; //8 bytes higher
    //P_LOG << "2nd pattern byte: " << std::hex << BGSHIFT_TWO << '\n';
    X_INCREMENT();
    //P_LOG << "Tile 1 done\n";
    CYCLE(2);
    NTABLE_BYTE = FETCH(0x2000 | (VRAM_ADDR & 0x0FFF));
    //P_LOG << "2nd nametable byte: " << std::hex << int(NTABLE_BYTE) << '\n';
    //P_LOG << std::hex << int(NTABLE_BYTE) << " ";
    CYCLE(2);
    ATTRSHIFT_TWO = FETCH(0x23C0 | (VRAM_ADDR & 0x0C00) | ((VRAM_ADDR >> 4) & 0x38) | ((VRAM_ADDR >> 2) & 0x07));
    //P_LOG << "1st attribute byte: " << std::hex << int(ATTRSHIFT_ONE) << '\n';
    CYCLE(2);
    BGSHIFT_ONE |= FETCH(((PPUCTRL & 0x10) << 8) | ((NTABLE_BYTE * 16) + ((VRAM_ADDR & 0x7000) >> 12))); //fine y determines which byte of tile to get
    //P_LOG << "1st pattern byte: " << std::hex << BGSHIFT_ONE << '\n';
    CYCLE(2);
    BGSHIFT_TWO |= FETCH(((PPUCTRL & 0x10) << 8) | ((NTABLE_BYTE * 16) + 8 + ((VRAM_ADDR & 0x7000) >> 12))); //8 bytes higher
    //P_LOG << "2nd pattern byte: " << std::hex << BGSHIFT_TWO << '\n';
    X_INCREMENT();
    //P_LOG << "Tile 2 done\n";
}

//CPU will call this function
void PPU::REG_WRITE(uint8_t DATA, uint8_t REG) {
    //P_LOG << "Writing " << int(DATA) << " to register" << int(REG) << '\n';
    //P_LOG << "Toggle: " << WRITE_TOGGLE << '\n';
    switch (REG) {
        case 0: //fine-x is affected, but no x scrolling implemented right now
            PPUCTRL = DATA;
            NMI_OUT = (DATA & 0x80);
            VRAM_TEMP &= 0xF3FF;
            VRAM_TEMP |= (((DATA & 0x03) << 10)); 
            break;
        case 1:
            PPUMASK = DATA;
            break;
        case 2:
            break;
        case 3:
            OAMADDR = DATA;
            break;
        case 4:
            if ((PPUSTATUS & 0x80) == 0) {
                OAMDATA = DATA;
                switch (OAMADDR%4) {
                    case 0:
                        OAM_PRIMARY[OAMADDR/4].Y_POS = DATA;
                        break;
                    case 1:
                        OAM_PRIMARY[OAMADDR/4].IND = DATA;
                        break;
                    case 2:
                        OAM_PRIMARY[OAMADDR/4].ATTR = DATA;
                        break;
                    case 3:
                        OAM_PRIMARY[OAMADDR/4].X_POS = DATA;
                        break;  
                }
                OAMADDR++;
            }
            break;
        case 5:
            if (WRITE_TOGGLE) {
                VRAM_TEMP &= 0x0C1F;
                VRAM_TEMP |= ((DATA & 0x07) << 12) | ((DATA & 0xF8) << 5);
            } else {
                VRAM_TEMP &= 0xFFE0;
                VRAM_TEMP |= ((DATA & 0xF8) >> 3);
                //Fine_x = DATA & 0x07
            }
            WRITE_TOGGLE = (WRITE_TOGGLE) ? false : true;
            break;
        case 6:
            if (WRITE_TOGGLE) {
                VRAM_TEMP = ((VRAM_TEMP & 0xFF00) | DATA);
                VRAM_ADDR = VRAM_TEMP;
            } else {
                VRAM_TEMP &= 0x00FF;
                VRAM_TEMP = ((DATA & 0x3F) << 8);
            }
            WRITE_TOGGLE = (WRITE_TOGGLE) ? false : true;
            break;
        case 7:
            WRITE(VRAM_ADDR, DATA);
            VRAM_ADDR += ((PPUCTRL & 0x04) != 0) ? 32 : 1;
            break;
    }
  /*  P_LOG << "PPUSCROLL: " << std::hex << int(PPUSCROLL) << '\n';
    P_LOG << "PPUADDR: " << std::hex << int(PPUADDR) << '\n';
    P_LOG << "PPUDATA: " << std::hex << int(PPUDATA) << '\n';
    P_LOG << "VRAM: " << std::hex << int(VRAM_ADDR) << '\n';
    P_LOG << "Toggle: " << WRITE_TOGGLE << '\n';*/
}


uint8_t PPU::REG_READ(uint8_t REG) {
    static uint8_t data_buf = 0x00;
    //P_LOG << "PPU reg read of " << int(REG) << '\n';
    switch (REG) {
        case 0:
            break;
        case 1:
            break;
        case 2: {//Affects nmi generation, see http://wiki.nesdev.com/w/index.php/NMI
            uint8_t TEMP = (PPUSTATUS & 0x7F) | (NMI_OCC << 7);
            NMI_OCC = 0;
            PPUSTATUS &= 0x7F;
            WRITE_TOGGLE = 0;
            return TEMP; }
        case 3:
            break;
        case 4:
            switch (OAMADDR%4) {
                case 0:
                    return OAM_PRIMARY[OAMADDR/4].Y_POS;
                case 1:
                    return OAM_PRIMARY[OAMADDR/4].IND;
                case 2:
                    return OAM_PRIMARY[OAMADDR/4].ATTR;
                case 3:
                    return OAM_PRIMARY[OAMADDR/4].X_POS;  
            }
        case 5:
            break;
        case 6:
            break;
        case 7: {
            uint8_t TEMP = data_buf;
            data_buf = FETCH(VRAM_ADDR);
            VRAM_ADDR += ((PPUCTRL & 0x04) != 0) ? 32 : 1;
            return (VRAM_ADDR >= 0x3F00) ? data_buf : TEMP; }
    }

    return 0;
}


//Sprite eval does not happen on pre-render line, only on visible scanlines: 0-239
//Since there's no visible scanlines after 239, sprite eval doesn't strictly need to happen
//OAMADDR effectively selects sprite 0, see nesdev wiki
void PPU::SPRITE_EVAL(uint16_t SLINE_NUM) {

    OAM_SECONDARY.clear();
    //Wait until cycle 65 of scanline
    
    uint8_t n,m = 0;
    int16_t DIF;
    while (1) {
        if (OAM_SECONDARY.size() == 8) {
            OAM_SECONDARY.emplace_back(OAM_PRIMARY[n]); //Read y coordinate and copy into 2nd OAM

            //If y coordinate in range, keep sprite, else set index as -1 to signify out of range  
            //Take sprite size into account
            DIF = SLINE_NUM + 1 - OAM_PRIMARY[n].Y_POS;
            if ((PPUCTRL & 0x20) != 0) //8*16 sprite size
                OAM_SECONDARY[n].IND = ((DIF < 0) || (DIF > 15)) ? -1 : OAM_SECONDARY[n].IND;
            else //8*8 sprite size
                OAM_SECONDARY[n].IND = ((DIF < 0) || (DIF > 7)) ? -1 : OAM_SECONDARY[n].IND;
        }

        n++;
        if (n >= 64)
            break;
        else if (OAM_SECONDARY.size() < 8)
            continue;
        else if (OAM_SECONDARY.size() == 8)
            break;
    }

    while (n < 64) {
        DIF = SLINE_NUM + 1 - SPR_SELECT(m, OAM_PRIMARY[n]);
        if ((PPUCTRL & 0x20) != 0) { //8*16 sprite size
            if ((DIF < 0) || (DIF > 15)) {
                m++;
                if (n++ >= 64)
                    break;
            } else {
                PPUSTATUS |= 0x20;
                break;
            }
        } else {//8*8 sprite size
            if ((DIF < 0) || (DIF > 7)) {
                m++;
                if (n++ >= 64)
                    break;
            } else {
                PPUSTATUS |= 0x20;
                break;
            }
        }
    }
    //Wait until cycle 256


}


void PPU::nametable(std::array<uint8_t, 2048> N) {
    for (int i=0; i<32; i++) {
        for (int j=0; j<32; j++) {
            P_LOG << std::hex << int(N[i*32 + j]) << " "; 
        }
        P_LOG << '\n';
    }
}