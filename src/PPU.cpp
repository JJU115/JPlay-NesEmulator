#include <cstdint>
#include "PPU.hpp"
#include <SDL_thread.h>
#include <SDL_timer.h>
#include <time.h>
#include "Palette.hpp"

extern SDL_cond* mainPPUCond;
extern SDL_mutex* mainThreadMutex;
extern std::condition_variable CPU_COND;
extern bool pause;
extern bool Gamelog;
extern bool quit;
extern long CPUCycleCount;
extern SDL_mutex* CpuPpuMutex;

std::ofstream P_LOG;


uint8_t PPU::FETCH(uint16_t ADDR) {
    
    if ((ADDR >= 0x3000) && (ADDR <= 0x3EFF))
        ADDR &= 0x2EFF;

    if ((ADDR >= 0x3F20) && (ADDR <= 0x3FFF))
        ADDR &= 0x3F1F;


    //Pattern Tables
    if (ADDR <= 0x1FFF)
        return ROM->PPU_ACCESS(ADDR, 0, true);

    //Nametables - Addresses need to be properly translated to array indices, 0x2000 will be index 0 for example and so on
    if (ADDR <= 0x2FFF)
        return VRAM[ROM->PPU_ACCESS(ADDR, 0, true, true)];
        
    //Palette RAM
    if (ADDR <= 0x3F1F) {
        if ((ADDR % 4) == 0)
            return PALETTES[0];
        else
            return PALETTES[ADDR & 0xFF];
    }

    return 0;
}


void PPU::WRITE(uint16_t ADDR, uint8_t DATA) {
    
    if ((ADDR >= 0x3000) && (ADDR <= 0x3EFF))
        ADDR &= 0x2EFF;

    if ((ADDR >= 0x3F20) && (ADDR <= 0x3FFF))
        ADDR &= 0x3F1F;

    if (ADDR <= 0x1FFF)
        ROM->PPU_ACCESS(ADDR, DATA, false); //Only if the board has CHR RAM
    
    //Nametables
    if ((ADDR >= 0x2000) && (ADDR <= 0x2FFF))
        VRAM[ROM->PPU_ACCESS(ADDR, 0, true, true)] = DATA;
        

    //Palette RAM
    if ((ADDR >= 0x3F00) && (ADDR <= 0x3F1F)) {
        if (((ADDR % 4) == 0) && (ADDR < 0x3F10))
            PALETTES[ADDR & 0xFF] = PALETTES[(ADDR & 0xFF) + 0x10] = DATA % 0x40;
        else if ((ADDR % 4) == 0)
            PALETTES[ADDR & 0xFF] = PALETTES[(ADDR & 0xFF) - 0x10] = DATA % 0x40;
        else
            PALETTES[ADDR & 0xFF] = DATA % 0x40;

    }
}


void PPU::RESET() {
    PPUCTRL = 0;
    PPUMASK = 0;
    WRITE_TOGGLE = false;
    PPUSCROLL = 0;
    PPUDATA = 0;
    ODD_FRAME = false;
    Reset = false;
}




void PPU::CYCLE(uint16_t N) {
    if (quit)
        return;

    while(pause)
        std::this_thread::yield();

    while (N-- > 0) {
        ++TICK;
        SDL_LockMutex(CpuPpuMutex);
        ++cycleCount;
        SDL_UnlockMutex(CpuPpuMutex);

        while ((cycleCount > 14) && !quit)
            std::this_thread::yield();

    }
}


void PPU::GENERATE_SIGNAL() {
    OAM_PRIMARY.assign(256, 0);
    OAM_SECONDARY.reserve(32);
    cycleCount = 0;
    
    VRAM.fill(0);
    SLINE_NUM = 0;
    P_LOG.open("PPU_LOG.txt", std::ios::trunc | std::ios::out);
    
    while (!quit) {
        //These two calls ensure the main thread is waiting on its condition variable before the PPU goes any further
        if (SDL_LockMutex(mainThreadMutex) != 0)
            std::cout << "Lock failed\n";
        if (SDL_UnlockMutex(mainThreadMutex) != 0)
            std::cout << "Unlock failed\n";

        PRE_RENDER();

        while (SLINE_NUM < 240) {
            SCANLINE(SLINE_NUM++);
        }
        ++SLINE_NUM;
        TICK = 0;
        SDL_CondSignal(mainPPUCond);
       
        CYCLE(341); //Post-render
        
        TICK = 0;
        ++SLINE_NUM;
        CYCLE(1);
    
        //Here, 82523 or 82522 PPU cycles have elapsed, CPU should be at 27507 cycles (with 2/3 through one more)
        while ((cycleCount > 2) && !quit)
            std::this_thread::yield();    

        PPUSTATUS |= 0x80; //VBLANK flag is actually only set in the 2nd tick of scanline 241 
        NMI_OCC = 1;

        if (NMI_OCC && NMI_OUT && !SuppressNmi)
            GEN_NMI = 1;      

        SuppressNmi = false; 
        CYCLE(340);
        ++SLINE_NUM;
        TICK = 0;
      
        //20 vblank scanlines
        CYCLE(20 * 341);  
        NMI_OCC = 0;
       
        while ((cycleCount > 2) && !quit) //29781 every other frame
            std::this_thread::yield();
        
        CPUCycleCount = 0;
        SLINE_NUM = 0;
        ODD_FRAME = (ODD_FRAME) ? false : true;
    }
}


//increment fine y at dot 256 of pre-render and visible scanlines
//also, since pre-render should make the same memory accesses as a regular scanline, the vram address register changes
void PPU::PRE_RENDER() {
    //auto t1 = std::chrono::high_resolution_clock::now();
    TICK = 0;

    CYCLE();
    PPUSTATUS &= 0x1F;  //Signal end of Vblank and clear sprite overflow

    while (TICK < 321) {
        if ((TICK % 8 == 0) && (TICK < 256))
           X_INCREMENT();
        
        if (TICK == 256)
            Y_INCREMENT();

        if (TICK == 257) {
            //Copy all horizontal bits from t to v at tick 257 if rendering enabled
            if ((PPUMASK & 0x08) || (PPUMASK & 0x10)) {
                VRAM_ADDR &= 0xFBE0;
                VRAM_ADDR |= (VRAM_TEMP & 0x041F);
            }
        }

        if ((TICK >= 257) && (TICK <= 320))
            OAMADDR = 0;

        if ((TICK >= 280) && (TICK <= 304)) {
            if ((PPUMASK & 0x08) || (PPUMASK & 0x10)) {
                VRAM_ADDR &= 0x041F;
                VRAM_ADDR |= (VRAM_TEMP & 0xFBE0);
            }
        }

        if ((TICK == 285) && (PPUMASK & 0x18))
            ROM->Scanline();

        CYCLE();
    }


    PRE_SLINE_TILE_FETCH();

    //Last 4 cycles of scanline (skip last on odd frames when BG rendering enabled)
    if (ODD_FRAME && (PPUMASK & 0x08))
        CYCLE(3);
    else
        CYCLE(4);
}


//Sprite evaluation for the next scanline occurs at the same time
void PPU::SCANLINE(uint16_t SLINE) {
    //OAM_SECONDARY.assign(32, 255);  //Potentially unnecessary op
    OAM_SECONDARY.clear();
    TICK = 0;
    //uint8_t NTABLE_BYTE, PTABLE_LOW, PTABLE_HIGH, ATTR_BYTE;
    activeSprites = 0;
    //uint16_t BGPIXEL, SPPIXEL;
    fineXSelect = 0x8000 >> Fine_x;
    //uint32_t COL;

    spriteZeroRenderedNext = false;
    spriteZeroHit = false; //true if zero hit has occurred
    spriteZeroLoaded = false;
    spriteHasPriority = false;

    hideLeftBg = !(PPUMASK & 0x02);
    hideLeftSpr = !(PPUMASK & 0x04);

    //For sprite eval
    n = 0;
    m = 0;
    step = 1;
    offset = 1;
    foundSprites = 0;
    auto In_range = [](uint8_t yPos, uint16_t slineNum, bool sprSize) {
        int16_t diff = yPos - slineNum;
        return (sprSize) ? ((diff <= -1) && (diff >= -16)) : ((diff <= -1) && (diff >= -8));
    };

    auto IncN = [&]() {
        step = ((n + 1) > 63) ? 5 : (foundSprites < 8) ? 1 : 4;
        n++;
    };


    //Cycle 0 is idle
    CYCLE();
    

    //Cycles 1-256: Fetch tile data starting at tile 3 of current scanline (first two fetched from previous scanline)
    //Remember the vram address updates
    while (TICK < 257) {
        switch (TICK % 8) {
            case 0:
                //Fetch PTable high
                PTABLE_HIGH = FETCH(((PPUCTRL & 0x10) << 8) | ((NTABLE_BYTE << 4) + 8 + ((VRAM_ADDR & 0x7000) >> 12)));
                
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
                ATTR_NEXT >>= 2;
                break;
            case 2:
                //Fetch nametable byte
                NTABLE_BYTE = FETCH(0x2000 | (VRAM_ADDR & 0x0FFF));
                break;
            case 4:
                //Fetch attribute byte
                ATTR_BYTE = FETCH(0x23C0 | (VRAM_ADDR & 0x0C00) | ((VRAM_ADDR >> 4) & 0x38) | ((VRAM_ADDR >> 2) & 0x07));
                
                if (((VRAM_ADDR & 0x03E0) >> 5) & 0x02) 
                    ATTR_BYTE >>= 4;
                if ((VRAM_ADDR & 0x001F) & 0x02) 
                    ATTR_BYTE >>= 2;
                
                ATTR_NEXT |= ((ATTR_BYTE & 0x03) << 2);
            
                break;
            case 6:
                //Fetch ptable low
                PTABLE_LOW = FETCH(((PPUCTRL & 0x10) << 8) | ((NTABLE_BYTE << 4) + ((VRAM_ADDR & 0x7000) >> 12)));
                break;
            default:
                break;
        }
        


        BGPIXEL = 0x3F00;
        //Background pixel composition
        if ((PPUMASK & 0x08))  {
            BGPIXEL |= (((BGSHIFT_ONE & fineXSelect) >> (15 - Fine_x)) | ((BGSHIFT_TWO & fineXSelect) >> (14 - Fine_x)));
            //Form the pixel color
            BGPIXEL |= ((ATTRSHIFT_ONE & (0x80 >> Fine_x)) >> (7 - Fine_x) << 2) | (((ATTRSHIFT_TWO & (0x80 >> Fine_x)) >> (7 - Fine_x)) << 3);
        } 


        SPPIXEL = 0x3F10;
        //Sprite pixel composition
        if ((PPUMASK & 0x10) && SLINE != 1) {
            for (uint8_t i=0; i<SPR_XPOS.size(); i++) {
                //If sprite pattern is all zero, ignore it and move on
                if (!SPR_PAT[i << 1] && !SPR_PAT[(i << 1) + 1])
                    continue;

                //If sprite x-pos is 0 and sprite isn't already active, actvate it
                if ((SPR_XPOS[i] == 0) && !(activeSprites & (0x80 >> i)))
                    activeSprites |= (0x80 >> i);

                //If sprite is active and non-transparent pixel has not already been found
                if ((activeSprites & (0x80 >> i)) && ((SPPIXEL & 0x03) == 0)) {
                    SPPIXEL |= ((SPR_ATTRS[i] & 0x03) << 2) | ((SPR_PAT[i*2] & 0x80) >> 7) | ((SPR_PAT[i*2 + 1] & 0x80) >> 6);
                    spriteZeroLoaded = (i == 0);
                    spriteHasPriority = !(SPR_ATTRS[i] & 0x20);
                }

                //Decrement all x positions and shift active sprite pattern data
                //Exhausted sprites aren't removed, just skipped
                SPR_XPOS[i]--;
                if (activeSprites & (0x80 >> i)) {
                    SPR_PAT[i << 1] <<= 1;
                    SPR_PAT[(i << 1) + 1] <<= 1;
                }
            }
        }

        SPPIXEL = (hideLeftSpr && (TICK < 9)) ? 0x3F10 : SPPIXEL;
        BGPIXEL = (hideLeftBg && (TICK < 9)) ? 0x3F00 : BGPIXEL;


        //Check for sprite zero hit, only if all rendering enabled, the zero hit hasn't already happened, and sprite zero is being rendered
        if ((PPUMASK & 0x08) && (PPUMASK & 0x10) && !spriteZeroHit && spriteZeroRendered)
            spriteZeroHit = (spriteZeroLoaded && ((SPPIXEL & 0x03) != 0) && ((BGPIXEL & 0x03) != 0));

        if (spriteZeroHit && (PPUMASK & 0x08) && (TICK != 256))
            PPUSTATUS |= 0x40;

        //Multiplex
        if ((SPPIXEL & 0x03) && (BGPIXEL & 0x03))
            COL = (spriteHasPriority) ? FETCH(SPPIXEL) : FETCH(BGPIXEL);
        else if ((SPPIXEL & 0x03))
            COL = FETCH(SPPIXEL);
        else
            COL = FETCH(BGPIXEL);

        framePixels[(SLINE * 256) + TICK - 1] = SYSTEM_PAL[COL];

        //Shift registers once
        //Pattern table data for one tile are stored in the lower 8 bytes of the two shift registers
        //Low entry in BGSHIFT_ONE, high entry in BGSHIFT_TWO
        //The attribute byte for the current tile will be in ATTRSHIFT_ONE, the next tile in ATTRSHIFT_TWO
        BGSHIFT_ONE <<= 1;
        BGSHIFT_TWO <<= 1;
        ATTRSHIFT_ONE <<= 1;
        ATTRSHIFT_TWO <<= 1;
        ATTRSHIFT_ONE |= (ATTR_NEXT & 0x01);
        ATTRSHIFT_TWO |= ((ATTR_NEXT & 0x02) >> 1);


        //Sprite eval - happens if either background or sprite rendering is enabled
        //Consider making 2nd OAM variable size and avoid fully assigning content at start of scanline function 
         if (TICK >= 65 && ((PPUMASK & 0x10) || (PPUMASK & 0x08))) {
            switch (step) {
                case 1: //Reads OAM y-cord and determines if in range
                    if ((TICK % 2) == 1)
                        data = OAM_PRIMARY[n << 2];
                    else if (foundSprites < 8) {
                        //OAM_SECONDARY[foundSprites << 2] = data;
                        if (In_range(data, SLINE+1, PPUCTRL & 0x20)) {
                            step = 2;
                            OAM_SECONDARY.push_back(data);
                        } else
                            IncN();     
                    }
                    break;
                case 2: //Copies next 3 bytes if y-cord in range
                    if ((TICK % 2) == 1) {
                        data = OAM_PRIMARY[(n << 2) + offset];
                    } else {
                        //OAM_SECONDARY[(foundSprites << 2) + offset++] = data;
                        OAM_SECONDARY.push_back(data);
                       /* if (offset == 4) { //All bytes copied
                            offset = 1;
                            spriteZeroRenderedNext = (n == 0) ? true : spriteZeroRenderedNext;
                            ++foundSprites;
                            IncN();
                            break;
                        }*/ 
                        if (++offset == 4) {
                            offset = 1;
                            spriteZeroRenderedNext = (n == 0) ? true : spriteZeroRenderedNext;
                            ++foundSprites;
                            IncN();
                            break;
                        }
                    }                                 
                    break;
                case 4: //Evaluating OAM contents
                    if (In_range(OAM_PRIMARY[(n << 2) + m], SLINE+1, PPUCTRL & 0x20)) {
                        PPUSTATUS |= 0x20;
                        step = 5; //Supposed to increment n and m but they're not accessed again during this scanline
                    } else {
                        if ((n + 1) > 63)
                            step = 5;
                        else {
                            n++;
                            m = (m + 1) % 4;
                        }
                    }
                    break;
                case 5: //Sprite eval effectively finished, no further operations needed
                    break;
            }
        }

        CYCLE();
    }
    
    spriteZeroRendered = spriteZeroRenderedNext;

    //Copy all horizontal bits from t to v at tick 257 if rendering enabled
    if ((PPUMASK & 0x08) || (PPUMASK & 0x10)) {
        VRAM_ADDR &= 0xFBE0;
        VRAM_ADDR |= (VRAM_TEMP & 0x041F);
    }

   
    //Cycles 257-320 - Fetch tile data for sprites on next scanline - OAMADDR register determines which is sprite 0
    SPR_ATTRS.clear();
    SPR_XPOS.clear();
    SPR_PAT.clear();
    uint16_t SPR_PAT_ADDR;
    for(int i=0; i < 8; i++) {
        //2nd OAM size is always going to be 32 as performed on first line of SCANLINE function
        if (foundSprites == 0) {
            CYCLE(8);
        } else {
            SPR_ATTRS.push_back(OAM_SECONDARY[(i << 2) + 2]);
            SPR_XPOS.push_back(OAM_SECONDARY[(i << 2) + 3]);

            //Pattern table data access different for 8*8 vs 8*16 sprites, see PPUCTRL
            //Also vertical flipping must be handled here, 8*16 sprites are flipped completely so last byte of bottom tile is fetched!
            if ((PPUCTRL & 0x20) == 0) { //8*8 sprites
                SPR_PAT_ADDR = (((PPUCTRL & 0x08) << 9) | (OAM_SECONDARY[(i << 2) + 1] << 4));
                //Vertically flipped or not?
                SPR_PAT_ADDR += (SPR_ATTRS.back() & 0x80) ? (7 - (SLINE - OAM_SECONDARY[i << 2])) : (SLINE - OAM_SECONDARY[i << 2]); 
                
                SPR_PAT.push_back(FETCH(SPR_PAT_ADDR));
                SPR_PAT.push_back(FETCH(SPR_PAT_ADDR + 8));
            } else { //8*16 sprites
                SPR_PAT_ADDR = (((OAM_SECONDARY[(i << 2) + 1] & 0x01) << 12) | (((OAM_SECONDARY[(i << 2) + 1] & 0xFE)) << 4));

                //The SLINE - OAM_SECONDARY is recalculated a lot, consider assigning it to a variable at start of loop iteration
                if (SPR_ATTRS.back() & 0x80) {
                    SPR_PAT_ADDR += 23 - (SLINE - OAM_SECONDARY[i << 2]) - (((SLINE - OAM_SECONDARY[i << 2]) > 7) << 3);
                } else {
                    //If current scanline less the y pos is > 7 then add 8 to the address
                    SPR_PAT_ADDR += (((SLINE - OAM_SECONDARY[i << 2]) > 7) << 3) + (SLINE - OAM_SECONDARY[i << 2]);
                }

                SPR_PAT.push_back(FETCH(SPR_PAT_ADDR));
                SPR_PAT.push_back(FETCH(SPR_PAT_ADDR + 8));
            }

            CYCLE(8);

            //Handle horizontal flipping here
            if (SPR_ATTRS.back() & 0x40) {
                auto flip_byte = [](uint8_t A) {
                    A = (A & 0xF0) >> 4 | (A & 0x0F) << 4;
                    A = (A & 0xCC) >> 2 | (A & 0x33) << 2;
                    A = (A & 0xAA) >> 1 | (A & 0x55) << 1;
                    return A;
                };

                SPR_PAT.back() = flip_byte(SPR_PAT.back());
                SPR_PAT[SPR_PAT.size() - 2] = flip_byte(SPR_PAT[SPR_PAT.size() - 2]);
            }
            --foundSprites;
        }
        
        if ((TICK == 281) && (PPUMASK & 0x18))
            ROM->Scanline();
    }

    //Cycle 321-336
    PRE_SLINE_TILE_FETCH();

    //Cycle 337-340
    CYCLE(4); //Supposed to be nametable fetches identical to fetches at start of next scanline
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
            } else {
                y = (y == 31) ? 0 : (y + 1);
            }
            VRAM_ADDR = (VRAM_ADDR & ~0x03E0) | (y << 5);
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



void PPU::PRE_SLINE_TILE_FETCH() {

    uint8_t NTABLE_BYTE = FETCH(0x2000 | (VRAM_ADDR & 0x0FFF));
    
    ATTR_NEXT = FETCH(0x23C0 | (VRAM_ADDR & 0x0C00) | ((VRAM_ADDR >> 4) & 0x38) | ((VRAM_ADDR >> 2) & 0x07));
    if (((VRAM_ADDR & 0x03E0) >> 5) & 0x02) 
        ATTR_NEXT >>= 4;
        
	if ((VRAM_ADDR & 0x001F) & 0x02) 
        ATTR_NEXT >>= 2;
	
    ATTR_NEXT &= 0x03;
    ATTRSHIFT_ONE = (ATTR_NEXT & 0x01) ? 0xFF : 0;
    
    BGSHIFT_ONE = FETCH(((PPUCTRL & 0x10) << 8) | ((NTABLE_BYTE << 4) + ((VRAM_ADDR & 0x7000) >> 12))) << 8; //fine y determines which byte of tile to get
    BGSHIFT_TWO = FETCH(((PPUCTRL & 0x10) << 8) | ((NTABLE_BYTE << 4) + 8 + ((VRAM_ADDR & 0x7000) >> 12))) << 8; //8 bytes higher
    
    X_INCREMENT();
    
    NTABLE_BYTE = FETCH(0x2000 | (VRAM_ADDR & 0x0FFF));
    
    ATTRSHIFT_TWO = (ATTR_NEXT & 0x02) ? 0xFF : 0;
    ATTR_NEXT = FETCH(0x23C0 | (VRAM_ADDR & 0x0C00) | ((VRAM_ADDR >> 4) & 0x38) | ((VRAM_ADDR >> 2) & 0x07));

    if (((VRAM_ADDR & 0x03E0) >> 5) & 0x02) 
        ATTR_NEXT >>= 4;
	if ((VRAM_ADDR & 0x001F) & 0x02) 
        ATTR_NEXT >>= 2;
	
    ATTR_NEXT &= 0x03;
    
    BGSHIFT_ONE |= FETCH(((PPUCTRL & 0x10) << 8) | ((NTABLE_BYTE << 4) + ((VRAM_ADDR & 0x7000) >> 12))); //fine y determines which byte of tile to get
    BGSHIFT_TWO |= FETCH(((PPUCTRL & 0x10) << 8) | ((NTABLE_BYTE << 4) + 8 + ((VRAM_ADDR & 0x7000) >> 12))); //8 bytes higher
    
    X_INCREMENT();
    
    CYCLE(16);
}


//CPU will call this function
void PPU::REG_WRITE(uint8_t DATA, uint8_t REG, long cycle) {
  
    uint16_t T;
    switch (REG) {
        case 0:
            if (!(PPUCTRL & 0x80) && (DATA & 0x80) && (PPUSTATUS & 0x80)) {
                GEN_NMI = 1;
                NmiDelay = true;
            }
            
            PPUCTRL = DATA;
            NMI_OUT = (DATA & 0x80);
            VRAM_TEMP &= 0xF3FF;
            VRAM_TEMP |= (((DATA & 0x03) << 10)); 
            if (Gamelog)
                P_LOG << "Wrote " << int(DATA) << " to control at sline: " << SLINE_NUM << "\n";
            break;
        case 1: //Possible mid-frame state change
            PPUMASK = DATA;
            break;
        case 2:
            break;
        case 3:
            OAMADDR = DATA;
            break;
        case 4:
            OAMDATA = DATA;
            OAM_PRIMARY[OAMADDR++] = DATA;
            break;
        case 5:
            if (WRITE_TOGGLE) {
                VRAM_TEMP &= 0x0C1F;
                VRAM_TEMP |= ((DATA & 0x07) << 12) | ((DATA & 0xF8) << 2);
            } else {
                VRAM_TEMP &= 0xFFE0;
                VRAM_TEMP |= ((DATA & 0xF8) >> 3);
                Fine_x = DATA & 0x07;
            }
            WRITE_TOGGLE = (WRITE_TOGGLE) ? false : true;
            break;
        case 6:
            if (WRITE_TOGGLE) {
                VRAM_TEMP = ((VRAM_TEMP & 0xFF00) | DATA);
                T = VRAM_ADDR & 0x1000;
        
                VRAM_ADDR = VRAM_TEMP & 0x3FFF;
                if ((T == 0) && (VRAM_ADDR & 0x1000))
                    ROM->Scanline();
                
            } else {
                VRAM_TEMP &= 0x00FF;
                VRAM_TEMP = ((DATA & 0x3F) << 8);
            }
            WRITE_TOGGLE = (WRITE_TOGGLE) ? false : true;
            break;
        case 7:
            WRITE(VRAM_ADDR, DATA);
            T = VRAM_ADDR & 0x1000;
            VRAM_ADDR += ((PPUCTRL & 0x04) != 0) ? 32 : 1;
            if ((T == 0) && (VRAM_ADDR & 0x1000))
                ROM->Scanline();
            break;
    }
}


uint8_t PPU::REG_READ(uint8_t REG, long cycle) {
    static uint8_t data_buf = 0x00;
   
    switch (REG) {
        case 0:
            break;
        case 1:
            break;
        case 2: {//Affects nmi generation, see http://wiki.nesdev.com/w/index.php/NMI
            uint8_t TEMP = (PPUSTATUS & 0x7F) | (NMI_OCC << 7);
            SuppressNmi = ((SLINE_NUM == 242) && (TICK < 3));

            NMI_OCC = 0;
            PPUSTATUS &= 0x7F;
            WRITE_TOGGLE = 0;
            return TEMP;  
                }
        case 3:
            break;
        case 4: //Attempting to read 0x2004 before cycle 65 on visible scanlines returns 0xFF
            return OAM_PRIMARY[OAMADDR];
        case 5:
            break;
        case 6:
            break;
        case 7: {
            uint8_t TEMP = data_buf;
            uint16_t T = VRAM_ADDR & 0x1000;
            bool palRead = (VRAM_ADDR > 0x3EFF);
            data_buf = FETCH(VRAM_ADDR);
            VRAM_ADDR += ((PPUCTRL & 0x04) != 0) ? 32 : 1;
            if ((T == 0) && (VRAM_ADDR & 0x1000))
                ROM->Scanline();
            return (palRead) ? data_buf : TEMP; }
    }

    return 0;
}


void PPU::nametable(std::array<uint8_t, 2048> N) {
    for (int i=0; i<32; i++) {
        for (int j=0; j<32; j++) {
            P_LOG << std::hex << int(N[i*32 + j + 1024]) << " "; 
        }
        P_LOG << '\n';
    }
    /*P_LOG << "\n\nPalette:\n";

    for (int k=0; k<25; k++) {
        P_LOG << std::hex << int(PALETTES[k]) << " ";
    }*/

    P_LOG << "\n\n\n";
}