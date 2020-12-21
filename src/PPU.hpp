#ifndef PPU_H
#define PPU_H


//Make a cartridge class that has access to the game pak PRG-ROM and CHR-ROM which PPU and CPU will 
//interface with to fetch data. Instantiate in NES.cpp and set the same object as the cartridge for 
//both CPU and PPU - add new data member method as needed


#include <iostream>
#include <chrono>
#include <thread>
#include <cstdint>
#include <vector>
#include <utility>
#include "Cartridge.hpp"
#include <mutex>
#include <condition_variable>


class PPU {
    friend class CPU;
    public:
        uint32_t* framePixels;
        long cycleCount;
        void GENERATE_SIGNAL();
        void REG_WRITE(uint8_t DATA, uint8_t REG, long cycle);
        uint8_t REG_READ(uint8_t REG, long cycle);
        PPU(Cartridge& NES): PPUCTRL(0), PPUMASK(0), PPUSTATUS(0), OAMADDR(0), OAMDATA(0), PPUSCROLL(0), PPUADDR(0), PPUDATA(0), OAMDMA(0),
        VRAM_ADDR(0), VRAM_TEMP(0), Fine_x(0), BGSHIFT_ONE(0), BGSHIFT_TWO(0), ATTRSHIFT_ONE(0), ATTRSHIFT_TWO(0), ATTR_NEXT(0), ODD_FRAME(false), 
        WRITE_TOGGLE(false), GEN_NMI(0), NMI_OCC(0), NMI_OUT(0), ROM(&NES), SuppressNmi(false), NmiDelay(false), spriteZeroRendered(false), 
        Reset(false) { framePixels = new uint32_t[256 * 240]; }

        ~PPU() { delete framePixels; }
        
    private:
        void RESET();
        void PRE_RENDER();
        void SCANLINE(uint16_t SLINE);
        void CYCLE(uint16_t N=1);
        void Y_INCREMENT();
        void X_INCREMENT();
        void PRE_SLINE_TILE_FETCH();
        uint8_t FETCH(uint16_t ADDR);
        void WRITE(uint16_t ADDR, uint8_t DATA);
        void nametable(std::array<uint8_t, 2048> N);

        uint8_t NMI_OCC;
        uint8_t NMI_OUT;
        uint8_t GEN_NMI;
        Cartridge *ROM;

        uint8_t PPUCTRL;
        uint8_t PPUMASK;
        uint8_t PPUSTATUS;
        uint8_t OAMADDR;
        uint8_t OAMDATA;
        uint8_t PPUSCROLL;
        uint8_t PPUADDR;
        uint8_t PPUDATA;
        uint8_t OAMDMA;

        std::array<uint8_t, 2048> VRAM; //May need less based on cartridge configuration
        std::vector<uint8_t> OAM_PRIMARY;
        std::vector<uint8_t> OAM_SECONDARY;
        std::array<uint8_t, 32> PALETTES;

        uint16_t SLINE_NUM;
        uint16_t TICK;
        uint16_t VRAM_ADDR;
        uint16_t VRAM_TEMP;
        uint8_t Fine_x;

        uint16_t BGSHIFT_ONE, BGSHIFT_TWO;
        uint8_t ATTRSHIFT_ONE, ATTRSHIFT_TWO, ATTR_NEXT;

        std::vector<uint8_t> SPR_PAT; //Supposed to be 8 pairs of 8-bit shift registers
        std::vector<uint8_t> SPR_ATTRS;
        std::vector<uint8_t> SPR_XPOS;

        bool ODD_FRAME;
        bool WRITE_TOGGLE;

        bool SuppressNmi;
        bool NmiDelay;
        bool spriteZeroRendered;
        bool Reset;

        //Scanline variables
        uint8_t NTABLE_BYTE, PTABLE_LOW, PTABLE_HIGH, ATTR_BYTE;
        uint8_t activeSprites;
        uint16_t BGPIXEL, SPPIXEL;
        uint16_t fineXSelect;
        uint32_t COL;

        //Sprite eval variables
        uint8_t n;
        uint8_t m;
        uint8_t step;
        uint8_t data;
        uint8_t offset;
        uint8_t foundSprites;

        //bools for sprite eval and rendering
        bool spriteZeroRenderedNext;
        bool spriteZeroHit;
        bool spriteZeroLoaded;
        bool spriteHasPriority;
        bool hideLeftBg;
        bool hideLeftSpr;

};


#endif