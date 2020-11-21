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
extern long CPUCycleCount;
long PPUcount;

extern SDL_mutex* CpuPpuMutex;

std::ofstream P_LOG;


//Fetch function seems to exist solely for calling Cartridge functions, will keep for now but may get rid of later 
uint8_t PPU::FETCH(uint16_t ADDR) {
    //P_LOG << "PPU read from " << std::hex << int(ADDR) << '\n';
    if ((ADDR >= 0x3000) && (ADDR <= 0x3EFF))
        ADDR &= 0x2EFF;

    if ((ADDR >= 0x3F20) && (ADDR <= 0x3FFF))
        ADDR &= 0x3F1F;


    //Pattern Tables
    if (ADDR <= 0x1FFF)
        return ROM->PPU_ACCESS(ADDR, 0, true);

    //Nametables - Addresses need to be properly translated to array indices, 0x2000 will be index 0 for example and so on
    if (ADDR <= 0x2FFF) {
        /*uint16_t NT_ADDR = ROM->PPU_ACCESS(ADDR, true, true);
        if (NT_ADDR < 0x2800)
            return VRAM[NT_ADDR % 0x2000];
        else
            return VRAM[(NT_ADDR - 0x0400) % 0x2800];*/
        return VRAM[ROM->PPU_ACCESS(ADDR, 0, true, true)];
    }
        
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
    //P_LOG << "PPU write of " << std::hex << int(DATA) << " to " << std::hex << int(ADDR) << '\n';
    if ((ADDR >= 0x3000) && (ADDR <= 0x3EFF))
        ADDR &= 0x2EFF;

    if ((ADDR >= 0x3F20) && (ADDR <= 0x3FFF))
        ADDR &= 0x3F1F;

    if (ADDR <= 0x1FFF)
        ROM->PPU_ACCESS(ADDR, DATA, false); //Shouldn't do anything since writing to change cartridge CHR data probably isn't allowed
    
    //Nametables
    if ((ADDR >= 0x2000) && (ADDR <= 0x2FFF)) {
        /*uint16_t NT_ADDR = ROM->PPU_ACCESS(ADDR, true, true);
        if (NT_ADDR < 0x2800)
            VRAM[NT_ADDR % 0x2000] = DATA;
        else
            VRAM[NT_ADDR - 0x0400] = DATA;*/
        VRAM[ROM->PPU_ACCESS(ADDR, 0, true, true)] = DATA;
        //P_LOG << "Wrote " << std::hex << int(DATA) << " to " << ROM->PPU_ACCESS(ADDR, true, true) << '\n'; 
    }
        

    //Palette RAM
    if ((ADDR >= 0x3F00) && (ADDR <= 0x3F1F)) {
        if (((ADDR % 4) == 0) && (ADDR < 0x3F10))
            PALETTES[ADDR & 0xFF] = PALETTES[(ADDR & 0xFF) + 0x10] = DATA;
        else if ((ADDR % 4) == 0)
            PALETTES[ADDR & 0xFF] = PALETTES[(ADDR & 0xFF) - 0x10] = DATA;
        else
            PALETTES[ADDR & 0xFF] = DATA;

            

    }
}


void PPU::RESET() {
    PPUCTRL = 0;
    PPUMASK = 0;
    WRITE_TOGGLE = false;
    PPUSCROLL = 0;
    PPUDATA = 0;
    ODD_FRAME = false;
    dots = 0;
    PPUcount = 0;
    Reset = false;
}




//Try putting in a yield statement here if cycleCount is above 3 or some other higher value
void PPU::CYCLE(uint16_t N) {
    while(pause)
        std::this_thread::yield();

    while (N-- > 0) {
        
        PPUcount++;
        TICK++;
        dots++;
        SDL_LockMutex(CpuPpuMutex);
        cycleCount++;
        SDL_UnlockMutex(CpuPpuMutex);

        while (cycleCount > 14)
            std::this_thread::yield();
    }
    //Temporary for now to resolve background render timing issues

    //P_LOG << "PPU Tick\n";
}


void PPU::GENERATE_SIGNAL() {
    OAM_PRIMARY.assign(256, 0);
    OAM_SECONDARY.assign(32, 255); //Problem here?
    cycleCount = 0;
    PPUcount = 0;
    //lock_t t;
    VRAM.fill(0);
    SLINE_NUM = 0;
    P_LOG.open("PPU_LOG.txt", std::ios::trunc | std::ios::out);
    int ext = 0;
    dots = 0;
    while (true) {
        //auto t1 = std::chrono::high_resolution_clock::now();
        //std::cout << "Start frame\n";
        //For testing dktest.nes only
        /*if ((PPUMASK & 0x08) != 0) {
            VRAM_ADDR = 0;
            VRAM_TEMP = 0;
            PPUCTRL = 0x10;
            PALETTES[0] = 0x3F;
            PALETTES[1] = 0x21;
            PALETTES[3] = 0x12;
        }*/

        //These two calls ensure the main thread is waiting on its condition variable before the PPU goes any further
        //std::cout << "Lock begin\n";
        if (SDL_LockMutex(mainThreadMutex) != 0)
            std::cout << "Lock failed\n";
        if (SDL_UnlockMutex(mainThreadMutex) != 0)
            std::cout << "Unlock failed\n";

        //std::cout << "Lock/Unlock complete\n";
        //std::cout << "Cyclecount: " << cycleCount << '\n';
        PPUcount = 0;
        PRE_RENDER();

        while (SLINE_NUM < 240) {
            SCANLINE(SLINE_NUM++);
        }
        ++SLINE_NUM;
        TICK = 0;
        SDL_CondSignal(mainPPUCond);
        //std::cout << "Entering post render\n";
        CYCLE(341); //Post-render, don't think any of the below happens as it does in every other scanline but will keep commented here for now
        /*Y_INCREMENT();
        CYCLE();
        if ((PPUMASK & 0x08) || (PPUMASK & 0x10)) {
            VRAM_ADDR &= 0xFBE0;
            VRAM_ADDR |= (VRAM_TEMP & 0x041F);
        }
        CYCLE(85);*/
        TICK = 0;
        ++SLINE_NUM;
        //std::cout << "Post render done\n";
        CYCLE(1);
    
        //Here, 82523 or 82522 PPU cycles have elapsed, CPU should be at 27507 cycles (with 2/3 through one more)
        while (cycleCount > 2)
            std::this_thread::yield();    

        PPUSTATUS |= 0x80; //VBLANK flag is actually only set in the 2nd tick of scanline 241 
        NMI_OCC = 1;

        if (NMI_OCC && NMI_OUT && !SuppressNmi)
            GEN_NMI = 1;      

        SuppressNmi = false; 
        CYCLE(340);
        ++SLINE_NUM;
        TICK = 0;
        //Start CPU cycle count over
        while (SLINE_NUM < 262) { //Every scanline does the 256 increment and 257 copy but as with pre-render, not sure if it happens in the vblank period, so commented here for now
            CYCLE(341);
            TICK = 0;       //The whole vblank section is a candidate for performance boost by putting it in idle mode and forgoing 
            ++SLINE_NUM;    //calls to CYCLE with just cyclecount increments
        }
            
        NMI_OCC = 0;
       
        //std::cout << "One frame rendered\n";
        //auto t2 = std::chrono::high_resolution_clock::now();
        //std::cout << "Frame time: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count() << '\n';
        //Get main to render the frame
        //if (Gamelog)
        //    nametable(VRAM);
        //SDL_CondSignal(mainPPUCond);
        /*for (uint8_t i : OAM_PRIMARY)
            P_LOG << int(i) << " ";*/
        
        while (cycleCount > 2) //29781 every other frame
            std::this_thread::yield();
        
        //P_LOG << CPUCycleCount << " " << PPUcount << '\n';
        
        CPUCycleCount = 0;
        SLINE_NUM = 0;
        ODD_FRAME = (ODD_FRAME) ? false : true;
    }
}


//increment fine y at dot 256 of pre-render and visible scanlines
//also, since pre-render should make the same memory accesses as a regular scanline, the vram address register changes
//Nesdev says vertical scroll bits are reloaded during pre-render if rendering is enabled?
void PPU::PRE_RENDER() {
    //auto t1 = std::chrono::high_resolution_clock::now();
    TICK = 0;
    uint8_t NTABLE_BYTE;

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

        //Although nesDev says OAMADDR is set to 0 during these ticks, enabling this causes tests to fail
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
    //std::cout << "Pre render done\n";
    //auto t2 = std::chrono::high_resolution_clock::now();
}


//Sprite evaluation for the next scanline occurs at the same time
void PPU::SCANLINE(uint16_t SLINE) {
    OAM_SECONDARY.assign(32, 255);
    TICK = 0;
    uint8_t NTABLE_BYTE, PTABLE_LOW, PTABLE_HIGH, ATTR_BYTE;
    uint8_t activeSprites = 0;
    uint16_t BGPIXEL, SPPIXEL;
    uint16_t fineXSelect = 0x8000 >> Fine_x;
    uint32_t COL;

    bool spriteZeroRenderedNext = false;
    bool spriteZeroHit = false; //true if zero hit has occurred
    bool spriteZeroLoaded = false;
    bool targ = false;
    bool spriteHasPriority = false;

    bool hideLeftBg = !(PPUMASK & 0x02);
    bool hideLeftSpr = !(PPUMASK & 0x04);

    //For sprite eval
    uint8_t n = 0;
    uint8_t m = 0;
    uint8_t step = 1;
    uint8_t data;
    uint8_t offset = 1;
    uint8_t foundSprites = 0;
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
    
    if (targ)
        P_LOG << SLINE << ":  \n";
    //Cycles 1-256: Fetch tile data starting at tile 3 of current scanline (first two fetched from previous scanline)
    //Remember the vram address updates
    while (TICK < 257) {
        switch (TICK % 8) {
            case 0:
                //Fetch PTable high
                PTABLE_HIGH = FETCH(((PPUCTRL & 0x10) << 8) | ((NTABLE_BYTE * 16) + 8 + ((VRAM_ADDR & 0x7000) >> 12)));
                //if (targ)
                //    P_LOG << "PTable high: " << std::hex << int(PTABLE_HIGH) << '\n';
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
                //ATTRSHIFT_ONE = ATTRSHIFT_TWO;
                //ATTRSHIFT_TWO = ATTR_BYTE;
                break;
            case 2:
                //Fetch nametable byte
                NTABLE_BYTE = FETCH(0x2000 | (VRAM_ADDR & 0x0FFF)); //Change based on base nametable select of ppuctrl register?
                //if (targ)
                //    P_LOG << "NTable byte: " << std::hex << int(NTABLE_BYTE) << '\n';
                break;
            case 4:
                //Fetch attribute byte - since one byte controls a 4*4 tile group, fetching becomes more complicated. Need to change this:
                //ATTR_BYTE = FETCH(0x23C0 | (VRAM_ADDR & 0x0C00) | ((VRAM_ADDR >> 4) & 0x38) | ((VRAM_ADDR >> 2) & 0x07));
                ATTR_BYTE = FETCH(0x23C0 | (VRAM_ADDR & 0x0C00) | ((VRAM_ADDR >> 4) & 0x38) | ((VRAM_ADDR >> 2) & 0x07));
                
                if (((VRAM_ADDR & 0x03E0) >> 5) & 0x02) 
                    ATTR_BYTE >>= 4;
                if ((VRAM_ADDR & 0x001F) & 0x02) 
                    ATTR_BYTE >>= 2;
                
                ATTR_NEXT |= ((ATTR_BYTE & 0x03) << 2);
            
                break;
            case 6:
                //Fetch ptable low
                PTABLE_LOW = FETCH(((PPUCTRL & 0x10) << 8) | ((NTABLE_BYTE * 16) + ((VRAM_ADDR & 0x7000) >> 12)));
                //if ((Gamelog) && (SLINE == 176))
                //    P_LOG << "Sline " << SLINE << " PTable low: " << std::hex << int(PTABLE_LOW) << " at "  << (((PPUCTRL & 0x10) << 8) | ((NTABLE_BYTE * 16) + ((VRAM_ADDR & 0x7000) >> 12))) << '\n';
                break;
            default:
                break;
        }
        


        BGPIXEL = 0x3F00;
        //Background pixel composition
        if ((PPUMASK & 0x08))  {
            BGPIXEL |= (((BGSHIFT_ONE & fineXSelect) >> (15 - Fine_x)) | ((BGSHIFT_TWO & fineXSelect) >> (14 - Fine_x))); //Apply fine x here?
            //Form the pixel color
            BGPIXEL |= ((ATTRSHIFT_ONE & (0x80 >> Fine_x)) >> (7 - Fine_x) << 2) | (((ATTRSHIFT_TWO & (0x80 >> Fine_x)) >> (7 - Fine_x)) << 3);

            /*if ((((SLINE)/16) % 2) == 0)
                BGPIXEL |= (((TICK-1)/16) % 2 == 0) ? ((ATTRSHIFT_ONE & 0x03) << 2) : (ATTRSHIFT_ONE & 0x0C);
            else
                BGPIXEL |= (((TICK-1)/16) % 2 == 0) ? ((ATTRSHIFT_ONE & 0x30) >> 2) : ((ATTRSHIFT_ONE & 0xC0) >> 4);
            */
        } 


        SPPIXEL = 0x3F10;
        //Sprite pixel composition
        if ((PPUMASK & 0x10) && SLINE != 1) {
            for (uint8_t i=0; i<SPR_XPOS.size(); i++) {
                //If sprite pattern is all zero, ignore it and move on
                if (!SPR_PAT[i*2] && !SPR_PAT[i*2 + 1])
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
                    SPR_PAT[i*2] <<= 1;
                    SPR_PAT[i*2 + 1] <<= 1;
                }
            }
        }

        SPPIXEL = (hideLeftSpr && (TICK < 9)) ? 0x3F10 : SPPIXEL;
        BGPIXEL = (hideLeftBg && (TICK < 9)) ? 0x3F00 : BGPIXEL;

        //if ((OAM_PRIMARY[0] == 112) && (OAM_PRIMARY[3] == 128) && (SPPIXEL > 0x3f10) && (BGPIXEL > 0x3f00))
            //P_LOG << spriteZeroHit << " " << spriteZeroRendered << " " << spriteZeroLoaded << '\n';

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



        //nesdev says there's a delay in rendering and the 1st pixel is output only at cycle 4?
        //COL = FETCH(BGPIXEL); 

        //Three sequential accesses to system palette at the same index is inefficient, should modify so a single access to palette gives full color
        framePixels[(SLINE * 256) + TICK - 1] = ((SYSTEM_PAL[COL].R << 24) | (SYSTEM_PAL[COL].G << 16) | (SYSTEM_PAL[COL].B << 8) | 0xFF);
      
        //Shift registers once
        //Pattern table data for one tile are stored in the lower 8 bytes of the two shift registers
        //Low entry in BGSHIFT_ONE, high entry in BGSHIFT_TWO
        //The attribute byte for the current tile will be in ATTRSHIFT_ONE, the next tile in ATTRSHIFT_TWO
        //Nesdev says shifters only shift starting at cycle 2? Does it even make a difference?
        BGSHIFT_ONE <<= 1;
        BGSHIFT_TWO <<= 1;
        ATTRSHIFT_ONE <<= 1;
        ATTRSHIFT_TWO <<= 1;
        ATTRSHIFT_ONE |= (ATTR_NEXT & 0x01);
        ATTRSHIFT_TWO |= ((ATTR_NEXT & 0x02) >> 1);

        //Sprite eval - happens if either background or sprite rendering is enabled
        //n = (TICK == 65) ? OAMADDR : n; -- Need to figure out how to get OAM[OAMADDR] to get treated as sprite 0 
         if (TICK >= 65 && ((PPUMASK & 0x10) || (PPUMASK & 0x08))) {
            switch (step) {
                case 1: //Reads OAM y-cord and determines if in range
                    if ((TICK % 2) == 1)
                        data = OAM_PRIMARY[n*4]; //possible to go out of bounds?
                    else if (foundSprites < 8) {
                        OAM_SECONDARY[foundSprites*4] = data;
                        if (In_range(data, SLINE+1, PPUCTRL & 0x20))
                            step = 2;
                        else
                            IncN();
                    }
                    break;
                case 2: //Copies next 3 bytes if y-cord in range
                    if ((TICK % 2) == 1) {
                        data = OAM_PRIMARY[n*4 + offset];
                    } else {
                        OAM_SECONDARY[foundSprites*4 + offset++] = data;
                        if (offset == 4) { //All bytes copied
                            offset = 1;
                            spriteZeroRenderedNext = (n == 0) ? true : spriteZeroRenderedNext;
                            foundSprites++;
                            IncN();
                            //foundSprites++;
                            break;
                        }     
                    }                                 
                    break;
                case 4: //Evaluating OAM contents
                    if (In_range(OAM_PRIMARY[n*4 + m], SLINE+1, PPUCTRL & 0x20)) {
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

        /*if (log) {
            P_LOG << "Sline: " << int(SLINE) << '\n';
            for (int i : OAM_SECONDARY)
                P_LOG << i << " ";
            P_LOG << "\n\n";
        }*/
        CYCLE();
    }
    
    spriteZeroRendered = spriteZeroRenderedNext;


    //Copy all horizontal bits from t to v at tick 257 if rendering enabled
    if ((PPUMASK & 0x08) || (PPUMASK & 0x10)) {
        VRAM_ADDR &= 0xFBE0;
        VRAM_ADDR |= (VRAM_TEMP & 0x041F);
    }

    //Call scanline function, only for MMC3 currently
    bool earlySlineClock = ((PPUCTRL & 0x18) == 0x08);
   
    //Cycles 257-320 - Fetch tile data for sprites on next scanline - OAMADDR register determines which is sprite 0
    SPR_ATTRS.clear();
    SPR_XPOS.clear();
    SPR_PAT.clear();
    uint16_t SPR_PAT_ADDR;
    for(int i=0; i < 8; i++) {
        //OAMADDR = 0; //Enabling causes failure despite nesdev saying it should happen
        if ((OAM_SECONDARY.size() < (i+1)*4) || (foundSprites == 0)) {
            CYCLE(8);
        } else {
            SPR_ATTRS.push_back(OAM_SECONDARY[i*4 + 2]);
            SPR_XPOS.push_back(OAM_SECONDARY[i*4 + 3]);
            CYCLE(4);

            //Pattern table data access different for 8*8 vs 8*16 sprites, see PPUCTRL
            //Also vertical flipping must be handled here, 8*16 sprites are flipped completely so last byte of bottom tile is fetched!
            if ((PPUCTRL & 0x20) == 0) { //8*8 sprites
                SPR_PAT_ADDR = (((PPUCTRL & 0x08) << 9) | (OAM_SECONDARY[i*4 + 1]*16));
                //Vertically flipped or not?
                SPR_PAT_ADDR += (SPR_ATTRS.back() & 0x80) ? (7 - (SLINE - OAM_SECONDARY[i*4])) : (SLINE - OAM_SECONDARY[i*4]); 
                
                SPR_PAT.push_back(FETCH(SPR_PAT_ADDR));
                CYCLE(2);
                SPR_PAT.push_back(FETCH(SPR_PAT_ADDR + 8));
                CYCLE(2);
            } else { //8*16 sprites
                SPR_PAT_ADDR = (((OAM_SECONDARY[i*4 + 1] & 0x01) << 12) | (((OAM_SECONDARY[i*4 + 1] & 0xFE)) * 16));

                //The SLINE - OAM_SECONDARY is recalculated a lot, consider assigning it to a variable at start of loop iteration
                if (SPR_ATTRS.back() & 0x80) {
                    SPR_PAT_ADDR += 23 - (SLINE - OAM_SECONDARY[i*4]) - ((SLINE - OAM_SECONDARY[i*4]) > 7)*8;
                } else {
                    //If current scanline less the y pos is > 7 then add 8 to the address
                    SPR_PAT_ADDR += ((SLINE - OAM_SECONDARY[i*4]) > 7)*8 + (SLINE - OAM_SECONDARY[i*4]);
                }

                //SPR_PAT_ADDR += (SPR_ATTRS.back() & 0x80) ? (23 - (SLINE - OAM_SECONDARY[i*4])) : (SLINE - OAM_SECONDARY[i*4]); 
                /*if (Gamelog) {
                    P_LOG << "Fetching Sprite at: " <<  std::hex << SPR_PAT_ADDR << " for sline " << int(SLINE) << '\n';
                }*/
                SPR_PAT.push_back(FETCH(SPR_PAT_ADDR));
                SPR_PAT.push_back(FETCH(SPR_PAT_ADDR + 8));
                CYCLE(4);
            }

            //Handle horizontal flipping here
            if (SPR_ATTRS.back() & 0x40) {
                auto flip_byte = [](uint8_t A) {
                    uint8_t B = 0;
                    for (uint8_t i=0; i<7; i++) {
                        B |= (A & 0x01);
                        B <<= 1;
                        A >>= 1;
                    }
                    return (B | A);
                };

                SPR_PAT.back() = flip_byte(SPR_PAT.back());
                SPR_PAT[SPR_PAT.size() - 2] = flip_byte(SPR_PAT[SPR_PAT.size() - 2]);
            }
            foundSprites--;
        }
        
        if ((TICK == 281) && (PPUMASK & 0x18))
            ROM->Scanline();
    }

    //Cycle 321-336
    PRE_SLINE_TILE_FETCH();

    //Cycle 337-340
    CYCLE(4); //Supposed to be nametable fetches identical to fetches at start of next scanline
    //auto t2 = std::chrono::high_resolution_clock::now();
    //std::cout << "Scanline time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(t2-t1).count() << '\n';
}


void PPU::Y_INCREMENT() {
    bool a12 = VRAM_ADDR & 0x1000;
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
    bool a12 = VRAM_ADDR & 0x1000;
    if ((PPUMASK & 0x08) || (PPUMASK & 0x10)) {
        if ((VRAM_ADDR & 0x001F) == 31) { 
            VRAM_ADDR &= ~0x001F;      
            VRAM_ADDR ^= 0x0400;          
        } else {
            VRAM_ADDR++;
        }
    }
}


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
    ATTR_NEXT = FETCH(0x23C0 | (VRAM_ADDR & 0x0C00) | ((VRAM_ADDR >> 4) & 0x38) | ((VRAM_ADDR >> 2) & 0x07));
    if (((VRAM_ADDR & 0x03E0) >> 5) & 0x02) 
        ATTR_NEXT >>= 4;
	if ((VRAM_ADDR & 0x001F) & 0x02) 
        ATTR_NEXT >>= 2;
	
    ATTR_NEXT &= 0x03;
    ATTRSHIFT_ONE = (ATTR_NEXT & 0x01) ? 0xFF : 0;
    //ATTRSHIFT_ONE = FETCH(0x23C0 | (VRAM_ADDR & 0x0C00) | ((VRAM_ADDR >> 4) & 0x38) | ((VRAM_ADDR >> 2) & 0x07));
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
    ATTRSHIFT_TWO = (ATTR_NEXT & 0x02) ? 0xFF : 0;
    ATTR_NEXT = FETCH(0x23C0 | (VRAM_ADDR & 0x0C00) | ((VRAM_ADDR >> 4) & 0x38) | ((VRAM_ADDR >> 2) & 0x07));

    if (((VRAM_ADDR & 0x03E0) >> 5) & 0x02) 
        ATTR_NEXT >>= 4;
	if ((VRAM_ADDR & 0x001F) & 0x02) 
        ATTR_NEXT >>= 2;
	
    ATTR_NEXT &= 0x03;
    //ATTRSHIFT_TWO = FETCH(0x23C0 | (VRAM_ADDR & 0x0C00) | ((VRAM_ADDR >> 4) & 0x38) | ((VRAM_ADDR >> 2) & 0x07));
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
void PPU::REG_WRITE(uint8_t DATA, uint8_t REG, long cycle) {
    //P_LOG << "Writing " << int(DATA) << " to register" << int(REG) << '\n';
    //P_LOG << "Toggle: " << WRITE_TOGGLE << '\n';
    uint16_t T;
    switch (REG) {
        case 0:
            //T = PPUCTRL & 0x10;
            if (!(PPUCTRL & 0x80) && (DATA & 0x80) && (PPUSTATUS & 0x80)) {
                GEN_NMI = 1;
                NmiDelay = true;
            }
            //if (visible) //To prevent toggling bit 4 while visible
                //PPUCTRL = (PPUCTRL & 0x10) | T;
            PPUCTRL = DATA;
            NMI_OUT = (DATA & 0x80);
            VRAM_TEMP &= 0xF3FF;
            VRAM_TEMP |= (((DATA & 0x03) << 10)); 
            break;
        case 1: //Possible mid-frame state change
            //T = PPUMASK & 0x08;
            PPUMASK = DATA;
            //if (visible) //To prevent toggling bit 3 while visible
                //PPUMASK = (PPUMASK & 0x08) | T;
            //P_LOG << "Render: " << bool(DATA & 0x08) << " cycle: " << cycle << '\n';
            break;
        case 2:
            break;
        case 3:
            OAMADDR = DATA;
            //P_LOG << "OAMADDR set to " << std::hex << int(OAMADDR) << '\n';
            break;
        case 4:
            OAMDATA = DATA;
            //P_LOG << "Wrote " << std::hex << int(DATA) << " OAMADDR: " << std::hex << int(OAMADDR) << '\n';
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
   // P_LOG << "PPUSCROLL: " << std::hex << int(PPUSCROLL) << '\n';
   // P_LOG << "PPUADDR: " << std::hex << int(PPUADDR) << '\n';
   // P_LOG << "PPUDATA: " << std::hex << int(PPUDATA) << '\n';
    //P_LOG << "VRAM: " << std::hex << int(VRAM_ADDR) << '\n';
    //P_LOG << "Toggle: " << WRITE_TOGGLE << '\n';
}


uint8_t PPU::REG_READ(uint8_t REG, long cycle) {
    static uint8_t data_buf = 0x00;
    //P_LOG << "PPU reg read of " << int(REG) << '\n';
    switch (REG) {
        case 0:
            break;
        case 1:
            break;
        case 2: {//Affects nmi generation, see http://wiki.nesdev.com/w/index.php/NMI
           // P_LOG << "2002 read, cycle: " << cycle << '\n';
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
            //P_LOG << "Read from " << std::hex << int(OAMADDR) << " Get: " << std::hex << int(OAM_PRIMARY[OAMADDR]) << '\n';
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


//Sprite eval does not happen on pre-render line, only on visible scanlines: 0-239
//Perhaps call this function after the first 256 cycles of each scanline and have it run without allocating a new thread of execution
//OAMADDR effectively selects sprite 0, see nesdev wiki
/*
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


}*/


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